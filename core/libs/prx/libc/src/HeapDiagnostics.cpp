#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestBacktrace.hpp"
#include "prx/libc/include/HeapDiagnostics.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>

namespace {

std::uint64_t mspaceAtomicIdMask = 0;
constexpr std::size_t MspaceTableCapacity = 65;
std::array<std::uint64_t, MspaceTableCapacity> mstateTable{};

static_assert(MspaceTableCapacity == 65);
static_assert(mstateTable.size() == MspaceTableCapacity);

constexpr std::size_t HeapNameOffset = 0x3b0;
constexpr std::size_t HeapNameSize = 32;

std::mutex reportMutex;

static_assert(sizeof(LibcHeapInfo) == 32);
static_assert(offsetof(LibcHeapInfo, mspace_atomic_id_mask) == 16);
static_assert(offsetof(LibcHeapInfo, mstate_table) == 24);

std::string HeapName(const void* heap) {
    if (heap == nullptr) return "(null)";
    const auto* name = static_cast<const char*>(heap) + HeapNameOffset;
    const auto* end = static_cast<const char*>(std::memchr(name, 0, HeapNameSize));
    return std::string(name, end != nullptr ? end : name + HeapNameSize);
}

[[gnu::noinline]] void ReportBacktrace(const std::string& name, const void* frame) {
    const auto frames = GuestBacktrace::GuestBacktrace_nid_no_patch(frame);
    std::fprintf(stderr, "[%s] backtrace:\n", name.c_str());
    for (std::size_t index = 0; index < frames.size(); ++index)
        std::fprintf(stderr, "[%s]   #%llu %s\n", name.c_str(), static_cast<unsigned long long>(index), frames[index].c_str());
    std::fflush(stderr);
}

}

extern "C" {

void LibcHeapTraceInfo_nid_no_patch(LibcHeapInfo* info) {
    if (info == nullptr)
        throw std::invalid_argument("heap trace info: null output");
    if (info->size != sizeof(LibcHeapInfo))
        throw std::invalid_argument("heap trace info: unsupported structure size");
    info->unknown2 = 0;
    info->mspace_atomic_id_mask = &mspaceAtomicIdMask;
    info->mstate_table = mstateTable.data();
}

void APS5_VABI sceLibcInternalBacktraceForGame_nid_postfix(const char* heapName) {
    const std::lock_guard lock(reportMutex);
    ReportBacktrace(heapName != nullptr ? heapName : "(null)", __builtin_frame_address(0));
}

void APS5_VABI sceLibcInternalHeapErrorReportForGame_nid_postfix(void* heap, void* block, std::uint32_t error) {
    const std::lock_guard lock(reportMutex);
    const auto name = HeapName(heap);
    std::fprintf(stderr, "[%s] heap error %u on block 0x%llx\n", name.c_str(), error, static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(block)));
    ReportBacktrace(name, __builtin_frame_address(0));
}

}
