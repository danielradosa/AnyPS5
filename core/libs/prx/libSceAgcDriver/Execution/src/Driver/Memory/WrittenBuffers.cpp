#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Memory/DrawWriteRanges.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Graphics/include/ColorTargetLayout.hpp"
#include "prx/libSceAgcDriver/Graphics/include/DepthSurface.hpp"
#include "prx/libSceAgcDriver/Graphics/include/GuestTextureResource.hpp"
#include "prx/libSceAgcDriver/Graphics/include/TextureTiling.hpp"
#include "prx/libSceAgcDriver/Graphics/include/VertexInput.hpp"
#include "Optimization/ShaderStageInputInfo.hpp"
#include "ThreadOwned.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace AgcDriver::DriverDetail {

namespace {

template<typename TVisit>
void visitWrittenBuffers(const ShaderRecompiler::RecompileResult& compiled, TVisit&& visit) {
    for (const auto& binding : compiled.bindings) {
        if (binding.role != ShaderRecompiler::DescriptorRole::GuestBuffers) continue;
        for (std::uint32_t element = 0; element < binding.count; ++element) {
            const bool written = element >= binding.bufferWritten.size() || binding.bufferWritten[element];
            if (!written || binding.guestDescriptor.size() < (static_cast<std::size_t>(element) + 1) * 4) continue;
            const auto words = std::span<const std::uint32_t>(binding.guestDescriptor).subspan(static_cast<std::size_t>(element) * 4, 4);
            const ShaderRecompiler::ShaderBufferResource descriptor{{words[0], words[1], words[2], words[3]}};
            const auto base = descriptor.Base48();
            const auto size = descriptor.GetSize();
            if (base == 0 || size == 0) continue;
            visit(element, base, base + size, element < binding.bufferAtomic.size() && binding.bufferAtomic[element]);
        }
    }
}

}

template<typename TVisit>
void Driver::forEachWrittenBuffer(const ShaderRecompiler::RecompileResult& compiled, TVisit&& visit) {
    visitWrittenBuffers(compiled, std::forward<TVisit>(visit));
}

void Driver::appendWrittenRanges(const ShaderRecompiler::RecompileResult& compiled, std::vector<std::pair<std::uint64_t, std::uint64_t>>& ranges) {
    forEachWrittenBuffer(compiled, [&](std::uint32_t, std::uint64_t begin, std::uint64_t end, bool) { ranges.emplace_back(begin, end); });
}

void Driver::noteWrittenBuffers(std::uint64_t program, std::uint32_t queue, const ShaderRecompiler::RecompileResult& compiled) {
    std::lock_guard lock(writtenBuffersMutex);
    const auto serial = ++writtenBufferSerial;
    forEachWrittenBuffer(compiled, [&](std::uint32_t, std::uint64_t begin, std::uint64_t end, bool atomic) {
        writtenBuffers.push_back({program, begin, end, serial, queue, atomic});
    });
    while (writtenBuffers.size() > WrittenBufferRing) writtenBuffers.pop_front();
}

std::vector<std::pair<std::uint64_t, std::uint64_t>> DispatchWriteRanges(const ShaderRecompiler::RecompileResult& compiled, bool& bounded) {
    bounded = compiled.bdaAbiVersion == 0;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> ranges;
    constexpr std::uint64_t UnknownImageBytes = 64ull * 1024 * 1024;
    for (const auto& binding : compiled.bindings) {
        if (binding.role == ShaderRecompiler::DescriptorRole::Gds || binding.role == ShaderRecompiler::DescriptorRole::BdaPagetable || binding.role == ShaderRecompiler::DescriptorRole::FaultBuffer) bounded = false;
        if (binding.role != ShaderRecompiler::DescriptorRole::GuestImages || binding.kind != ShaderRecompiler::DescriptorKind::StorageImage) continue;
        for (std::uint32_t element = 0; element < binding.count; ++element) {
            const bool written = element >= binding.imageWritten.size() || binding.imageWritten[element];
            if (!written) continue;
            if (binding.guestDescriptor.size() < (static_cast<std::size_t>(element) + 1) * 8) {
                bounded = false;
                continue;
            }
            const auto words = std::span<const std::uint32_t>(binding.guestDescriptor).subspan(static_cast<std::size_t>(element) * 8, 8);
            if (Graphics::IsNullTextureResource(words)) continue;
            const auto resource = Graphics::DecodeTextureResource(words);
            const auto surfaceBytes = Graphics::DescribeSurface(resource).guestBytes;
            ranges.emplace_back(resource.baseAddress, resource.baseAddress + (surfaceBytes != 0 ? surfaceBytes : UnknownImageBytes));
            if (resource.dccAddress != 0) ranges.emplace_back(resource.dccAddress, resource.dccAddress + (surfaceBytes != 0 ? surfaceBytes / 256 + 1 : UnknownImageBytes));
        }
    }
    visitWrittenBuffers(compiled, [&](std::uint32_t, std::uint64_t begin, std::uint64_t end, bool) { ranges.emplace_back(begin, end); });
    return ranges;
}

void Driver::noteForeignWriter(std::uint64_t begin, std::uint64_t end, std::uint32_t queue) {
    if (!foreignWriters() || !(writeEvidenceEnabled() || traceCapSync()) || end <= begin) return;
    std::lock_guard lock(writtenBuffersMutex);
    writtenBuffers.push_back({0, begin, end, ++writtenBufferSerial, queue, false});
    while (writtenBuffers.size() > WrittenBufferRing) writtenBuffers.pop_front();
}

void Driver::noteDrawWriters(std::span<const Graphics::CompiledShader> stages, std::uint32_t queue) {
    // A queued commit notes the list its worker collected for these stages (Graphics::PreparedWriters).
    if (const auto* writers = Graphics::PreparedWriters(Graphics::PreparedDrawScope::Active(), stages)) {
        for (const auto& [begin, end] : *writers) noteForeignWriter(begin, end, queue);
        return;
    }
    for (const auto& stage : stages) {
        forEachWrittenBuffer(*stage.program, [&](std::uint32_t, std::uint64_t begin, std::uint64_t end, bool) { noteForeignWriter(begin, end, queue); });
    }
}

bool ExactDrawWrites() {
    static const bool exact = [] { const char* text = std::getenv("APS5_EXACT_DRAW_WRITES"); return text == nullptr || std::strcmp(text, "0") != 0; }();
    return exact;
}

std::vector<std::pair<std::uint64_t, std::uint64_t>> DrawWriteRanges(const Graphics::State& graphics, std::span<const Graphics::CompiledShader> stages, bool exact, std::vector<std::pair<std::uint64_t, std::uint64_t>>* writtenBuffers) {
    constexpr std::uint64_t Slack = 64 * 1024;
    constexpr std::uint64_t UnknownBytes = 64ull * 1024 * 1024;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> ranges;
    const auto add = [&](std::uint64_t begin, std::uint64_t bytes) {
        if (begin != 0) ranges.emplace_back(begin, begin + bytes);
    };
    for (const auto& stage : stages) {
        if (stage.program == nullptr) continue;
        visitWrittenBuffers(*stage.program, [&](std::uint32_t, std::uint64_t begin, std::uint64_t end, bool) {
            ranges.emplace_back(begin, end);
            if (writtenBuffers != nullptr) writtenBuffers->emplace_back(begin, end);
        });
        for (const auto& binding : stage.program->bindings) {
            if (binding.role != ShaderRecompiler::DescriptorRole::GuestImages || binding.kind != ShaderRecompiler::DescriptorKind::StorageImage) continue;
            for (std::uint32_t element = 0; element < binding.count; ++element) {
                const bool written = element >= binding.imageWritten.size() || binding.imageWritten[element];
                if (!written || binding.guestDescriptor.size() < (static_cast<std::size_t>(element) + 1) * 8) continue;
                const auto words = std::span<const std::uint32_t>(binding.guestDescriptor).subspan(static_cast<std::size_t>(element) * 8, 8);
                if (words[0] == 0u && (words[1] & 0xffu) == 0u) continue;
                const auto resource = Graphics::DecodeTextureResource(words);
                if (resource.baseAddress == 0) continue;
                const auto surfaceBytes = Graphics::DescribeSurface(resource).guestBytes;
                add(resource.baseAddress, surfaceBytes != 0 ? surfaceBytes : UnknownBytes);
                add(resource.dccAddress, surfaceBytes != 0 ? surfaceBytes / 256 + 1 : UnknownBytes);
            }
        }
    }
    const auto addColor = [&](const Graphics::ColorTarget& color) {
        if (exact && color.bytes != 0) {
            const auto slices = static_cast<std::uint64_t>(std::max<std::uint32_t>(color.depth, 1));
            const auto bytes = static_cast<std::uint64_t>(color.bytes) * slices;
            add(color.address, bytes);
            add(color.dccAddress, (bytes + 255) / 256);
            add(color.cmaskAddress, static_cast<std::uint64_t>(color.cmaskBytes) * slices);
            return;
        }
        const std::uint64_t bytes = color.bytes != 0 ? static_cast<std::uint64_t>(color.bytes) : UnknownBytes;
        const auto base = color.surfaceAddress != 0 ? std::min(color.surfaceAddress, color.address) : color.address;
        if (base != 0) ranges.emplace_back(base, color.address + 2 * bytes + Slack);
        add(color.dccAddress, bytes / 16 + Slack);
        add(color.cmaskAddress, static_cast<std::uint64_t>(color.cmaskBytes) * std::max<std::uint32_t>(color.depth, 1));
    };
    if (graphics.hasColorTarget) addColor(graphics.color);
    for (const auto& color : graphics.colors) addColor(color);
    if (graphics.depth) {
        const auto& depth = *graphics.depth;
        if (exact && depth.extent.width != 0 && depth.extent.height != 0) {
            const bool d16 = depth.format == VK_FORMAT_D16_UNORM || depth.format == VK_FORMAT_D16_UNORM_S8_UINT;
            add(depth.address, Graphics::DepthSliceBytes(depth.extent, d16 ? 2u : 4u));
            add(depth.stencilAddress, Graphics::DepthSliceBytes(depth.extent, 1u));
        } else {
            const auto align = [](std::uint64_t value) { return (value + 255) & ~255ull; };
            const auto pixels = align(depth.extent.width) * align(depth.extent.height);
            const auto bytes = pixels != 0 ? pixels * 8 + Slack : UnknownBytes;
            add(depth.address, bytes);
            add(depth.stencilAddress, bytes);
        }
    }
    return ranges;
}

std::vector<std::pair<std::uint64_t, std::uint64_t>> Driver::drawWriteRanges(const Graphics::State& graphics, std::span<const Graphics::CompiledShader> stages, Graphics::PreparedDraw* prepared) {
    // A queued draw's written buffers for noteDrawWriters at commit come from this walk on the
    // queue-0 worker (APS5_NO_PREPARED_DRAW_WRITERS=1 walks the bindings at commit).
    auto* writers = prepared != nullptr && Graphics::PreparedDrawWriters() ? &prepared->writers : nullptr;
    auto ranges = DrawWriteRanges(graphics, stages, ExactDrawWrites(), writers);
    if (writers != nullptr) prepared->writersPrepared = true;
    return ranges;
}

std::vector<std::pair<std::uint64_t, std::uint64_t>> QueuedDrawReadRanges(const Pm4::DrawParameters& draw, std::span<const Graphics::CompiledShader> stages) {
    std::vector<std::pair<std::uint64_t, std::uint64_t>> ranges;
    const auto add = [&](std::uint64_t begin, std::uint64_t bytes) {
        if (begin == 0 || bytes == 0 || bytes > std::numeric_limits<std::uint64_t>::max() - begin) return;
        ranges.emplace_back(begin, begin + bytes);
    };
    if (draw.indexed && draw.indexSize != 0 && draw.indexCount != 0) add(draw.indexAddress, static_cast<std::uint64_t>(draw.indexCount) * draw.indexSize);
    if (stages.empty()) return ranges;
    for (const auto& stage : stages) {
        if (stage.program == nullptr) continue;
        for (const auto& binding : stage.program->bindings) {
            if (binding.role != ShaderRecompiler::DescriptorRole::GuestBuffers || binding.guestDescriptor.size() < static_cast<std::size_t>(binding.count) * 4u) continue;
            for (std::uint32_t element = 0; element < binding.count; ++element) {
                const auto* words = binding.guestDescriptor.data() + static_cast<std::size_t>(element) * 4u;
                const ShaderRecompiler::ShaderBufferResource descriptor{{words[0], words[1], words[2], words[3]}};
                add(descriptor.Base48(), descriptor.GetSize());
            }
        }
    }
    if (stages.empty() || stages.front().program == nullptr) return ranges;
    for (const auto& attribute : stages.front().program->vertexAttributes) {
        if (Graphics::NullVertexDescriptor(attribute)) continue;
        const auto address = attribute.resource.fields[0] | (static_cast<std::uint64_t>(attribute.resource.fields[1] & 0xffffu) << 32u);
        std::uint64_t bytes = 0;
        try {
            if (Graphics::VertexFetchOutOfRange(attribute)) continue;
            bytes = Graphics::VertexBufferExtent(attribute);
        } catch (const std::runtime_error&) {
            continue;
        }
        add(address, bytes);
    }
    return ranges;
}

// What the commit collects (GuestMemory::CollectWrites, memoized per epoch), as a superset where the
// exact range needs commit-time state: `reads` (the index range, every guest buffer descriptor, the
// vertex descriptors' whole extents, where CopyDrawInput collects the part up to the highest index)
// and `writes` (DrawWriteRanges: the color targets with DCC keys and CMASK, depth and stencil, the
// written buffers and storage images), then what neither holds: every image element's surface and
// DCC keys (fastRevalidate, the stage lookups, ProvedClearKeys), the HTILE (the depth view), and a
// mip-chain color target's whole surface (refreshProved walks the resident image, not the mip).
// Commit-time ranges that differ (an extended mip chain, a 2D array target) only walk their gaps.
// A descriptor that does not decode is skipped: the commit reports it as before.
void PrewalkDrawCollects(const Graphics::State& graphics, std::span<const Graphics::CompiledShader> stages, std::span<const std::pair<std::uint64_t, std::uint64_t>> writes, std::span<const std::pair<std::uint64_t, std::uint64_t>> reads) {
    if (GuestMemory::HandOffCollectEpoch() == 0) return;
    thread_local std::vector<std::pair<std::uint64_t, std::uint64_t>>* slot = nullptr;
    auto& ranges = ShaderRecompiler::ThreadOwned(slot);
    ranges.assign(reads.begin(), reads.end());
    ranges.insert(ranges.end(), writes.begin(), writes.end());
    const auto addSurface = [&](const Graphics::GuestTextureResource& resource) {
        const auto bytes = Graphics::DescribeSurface(resource).guestBytes;
        if (resource.baseAddress == 0 || bytes == 0) return;
        ranges.emplace_back(resource.baseAddress, resource.baseAddress + bytes);
        if (resource.dccAddress != 0) ranges.emplace_back(resource.dccAddress, resource.dccAddress + bytes / 256 + 1);
    };
    for (const auto& stage : stages) {
        if (stage.program == nullptr) continue;
        for (const auto& binding : stage.program->bindings) {
            if (binding.role != ShaderRecompiler::DescriptorRole::GuestImages || (binding.kind != ShaderRecompiler::DescriptorKind::SampledImage && binding.kind != ShaderRecompiler::DescriptorKind::StorageImage)) continue;
            std::span<const std::uint32_t> previous;
            for (std::uint32_t element = 0; element < binding.count && binding.guestDescriptor.size() >= (static_cast<std::size_t>(element) + 1) * 8; ++element) {
                const auto words = std::span<const std::uint32_t>(binding.guestDescriptor).subspan(static_cast<std::size_t>(element) * 8, 8);
                // Consecutive equal descriptors (the mips of one storage image) are one surface.
                if (!previous.empty() && std::equal(words.begin(), words.end(), previous.begin())) continue;
                previous = words;
                try {
                    if (!Graphics::IsNullTextureResource(words)) addSurface(Graphics::DecodeTextureResource(words));
                } catch (const std::exception&) {
                }
            }
        }
    }
    if (graphics.depth && graphics.depth->htileAddress != 0 && graphics.depth->samples == 1) ranges.emplace_back(graphics.depth->htileAddress, graphics.depth->htileAddress + Graphics::HtileMetadataBytes(graphics.depth->extent));
    for (const auto& color : graphics.colors) {
        if (color.mipCount <= 1 || color.samples > 1 || color.arrayLastSlice != 0 || color.tileMode == Graphics::ColorTileMode::Linear) continue;
        try {
            addSurface(Graphics::SurfaceForTarget(color));
        } catch (const std::exception&) {
        }
    }
    GuestMemory::PrewalkCollects(ranges);
}

std::optional<WrittenBuffer> Driver::newestWriterLocked(std::uint64_t begin, std::uint64_t end) const {
    for (auto it = writtenBuffers.rbegin(); it != writtenBuffers.rend(); ++it) {
        if (begin < it->end && it->begin < end) return *it;
    }
    return std::nullopt;
}

std::optional<WrittenBuffer> Driver::newestWriter(std::uint64_t begin, std::uint64_t end) {
    std::lock_guard lock(writtenBuffersMutex);
    return newestWriterLocked(begin, end);
}

std::string Driver::describeWriters(std::uint64_t begin, std::uint64_t end) {
    std::string text;
    std::lock_guard lock(writtenBuffersMutex);
    int shown = 0;
    for (auto it = writtenBuffers.rbegin(); it != writtenBuffers.rend() && shown < 3; ++it) {
        if (begin >= it->end || it->begin >= end) continue;
        char item[128];
        if (it->program == 0) std::snprintf(item, sizeof(item), " [foreign q0x%x 0x%llx+0x%llx age %llu]", it->queue, static_cast<unsigned long long>(it->begin), static_cast<unsigned long long>(it->end - it->begin), static_cast<unsigned long long>(writtenBufferSerial - it->serial));
        else std::snprintf(item, sizeof(item), " [0x%llx q0x%x 0x%llx+0x%llx%s%s age %llu]", static_cast<unsigned long long>(it->program), it->queue, static_cast<unsigned long long>(it->begin), static_cast<unsigned long long>(it->end - it->begin), it->atomic ? " atomic" : "", it->value ? " known" : "", static_cast<unsigned long long>(writtenBufferSerial - it->serial));
        text += item;
        ++shown;
    }
    if (shown == 0) return " none";
    std::uint32_t streakMin = 0xffffffffu, streakMax = 0, changed = 0, unseen = 0;
    for (std::uint64_t dword = begin & ~3ull; dword < end; dword += 4) {
        const auto found = dwordEvidence.find(dword);
        if (found == dwordEvidence.end()) {
            ++unseen;
            continue;
        }
        streakMin = std::min(streakMin, found->second.streak);
        streakMax = std::max(streakMax, found->second.streak);
        changed += found->second.changed;
    }
    char item[96];
    std::snprintf(item, sizeof(item), " dwords: streak %u..%u changed %u unseen %u", streakMin == 0xffffffffu ? 0u : streakMin, streakMax, changed, unseen);
    return text + item;
}

std::string Driver::describeSelf(const ShaderRecompiler::RecompileResult& compiled, std::uint64_t begin, std::uint64_t end) {
    std::string text;
    forEachWrittenBuffer(compiled, [&](std::uint32_t element, std::uint64_t base, std::uint64_t limit, bool atomic) {
        if (begin >= limit || base >= end) return;
        char item[96];
        std::snprintf(item, sizeof(item), " own element %u 0x%llx+0x%llx%s", element, static_cast<unsigned long long>(base), static_cast<unsigned long long>(limit - base), atomic ? " atomic" : "");
        text += item;
    });
    return text;
}

}
