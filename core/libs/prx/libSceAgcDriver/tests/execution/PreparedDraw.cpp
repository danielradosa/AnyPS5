#include "prx/libSceAgcDriver/Execution/include/Driver/Memory/DrawWriteRanges.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "Recompiler.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// The commit-time checks of a prepared draw (Graphics::PreparedDraw): a piece the queue-0 worker
// computed before the enqueue may serve only the device, stages and state it was made for, and the
// committer must compute the piece itself for any other. The pieces are pure functions of those
// inputs, so no GPU is needed: the device handles are made up. Run with the APS5_NO_PREPARED_DRAW_*
// switches set, nothing may be prepared or taken (the old path).

namespace {

namespace Graphics = AgcDriver::Graphics;
using Stage = ShaderRecompiler::ShaderStage;

void Require(bool condition, const std::string& what) {
    if (!condition) throw std::runtime_error("prepared draw: " + what);
}

VkDevice MadeUpDevice(std::uintptr_t value) {
    return reinterpret_cast<VkDevice>(value);
}

// One guest buffer binding of two V#s, the first written, the second only read.
ShaderRecompiler::RecompileResult Program(std::uint64_t variant, std::uint64_t written, std::uint64_t readOnly) {
    ShaderRecompiler::RecompileResult program;
    program.variantId = variant;
    ShaderRecompiler::DescriptorBinding buffers;
    buffers.kind = ShaderRecompiler::DescriptorKind::StorageBuffer;
    buffers.role = ShaderRecompiler::DescriptorRole::GuestBuffers;
    buffers.count = 2;
    for (const auto base : {written, readOnly}) {
        buffers.guestDescriptor.insert(buffers.guestDescriptor.end(), {static_cast<std::uint32_t>(base), static_cast<std::uint32_t>(base >> 32u) & 0xffffu, 0x1000u, 0u});
    }
    buffers.bufferWritten = {true, false};
    program.bindings.push_back(buffers);
    return program;
}

void CheckKeys(const std::vector<Graphics::CompiledShader>& stages, const ShaderRecompiler::RecompileResult& twin) {
    Graphics::Context deviceA{};
    deviceA.device = MadeUpDevice(0x1000);
    Graphics::Context deviceB{};
    deviceB.device = MadeUpDevice(0x2000);
    Graphics::State one{};
    one.colors.resize(1);
    Graphics::State two{};
    two.colors.resize(2);
    auto prepared = Graphics::PrepareDraw(deviceA, one, stages, true);
    if (!Graphics::PreparedDrawKeys()) {
        Require(prepared == nullptr || prepared->contentKey.empty(), "APS5_NO_PREPARED_DRAW_KEY=1 still prepared a key");
        return;
    }
    Require(prepared != nullptr && !prepared->contentKey.empty(), "a cacheable draw's key was not prepared");
    const auto keyA = prepared->contentKey;
    // What the checks guard: the same stages key differently per device and per attachment count.
    Require(Graphics::PrepareDraw(deviceB, one, stages, true)->contentKey != keyA, "the key does not name the device");
    Require(Graphics::PrepareDraw(deviceA, two, stages, true)->contentKey != keyA, "the key does not name the color attachment count");
    const auto recipeDraw = Graphics::PrepareDraw(deviceA, one, stages, false);
    Require(recipeDraw == nullptr || recipeDraw->contentKey.empty(), "a draw with a recipe prepared a key");

    Graphics::ResourceCache::Key key;
    Require(!Graphics::TakePreparedKey(nullptr, deviceA, one, stages, key) && key.empty(), "a key was taken from no prepared draw");
    Require(!Graphics::TakePreparedKey(prepared.get(), deviceB, one, stages, key) && key.empty(), "a key prepared for one device served another (a replaced device)");
    Require(!Graphics::TakePreparedKey(prepared.get(), deviceA, two, stages, key) && key.empty(), "a key prepared for one color attachment served two");
    auto other = stages;
    other[0].program = &twin;
    Require(!Graphics::TakePreparedKey(prepared.get(), deviceA, one, other, key) && key.empty(), "a key prepared for one program object served an equal copy");
    other = stages;
    other[1].stage = Stage::Geometry;
    Require(!Graphics::TakePreparedKey(prepared.get(), deviceA, one, other, key) && key.empty(), "a key prepared for a pixel stage served a geometry stage");
    other = stages;
    other[1].pushConstantOffset += 16;
    Require(!Graphics::TakePreparedKey(prepared.get(), deviceA, one, other, key) && key.empty(), "a key prepared for one push constant offset served another");
    Require(!Graphics::TakePreparedKey(prepared.get(), deviceA, one, std::span(stages).first(1), key) && key.empty(), "a key prepared for two stages served one");
    prepared->trimmedKey = false;
    Require(!Graphics::TakePreparedKey(prepared.get(), deviceA, one, stages, key) && key.empty(), "a key prepared under the other trim mode was taken");
    prepared->trimmedKey = true;
    Require(prepared->contentKey == keyA, "a rejected check consumed the prepared key");

    // Another state object with the same attachment count keys the same: the key is taken, once.
    Graphics::State another{};
    another.colors.resize(1);
    Require(Graphics::TakePreparedKey(prepared.get(), deviceA, another, stages, key) && key == keyA, "a key prepared for this device, these stages and one attachment was not taken");
    Require(prepared->contentKey.empty(), "a taken key stayed in the prepared draw");
    Graphics::ResourceCache::Key again;
    Require(!Graphics::TakePreparedKey(prepared.get(), deviceA, another, stages, again) && again.empty(), "a prepared key was taken twice");
}

void CheckWriters(const std::vector<Graphics::CompiledShader>& stages, const ShaderRecompiler::RecompileResult& twin) {
    using Range = std::pair<std::uint64_t, std::uint64_t>;
    Graphics::Context device{};
    device.device = MadeUpDevice(0x1000);
    Graphics::State state{};
    state.colors.resize(1);
    // A draw with a recipe prepares no key, but its commit still notes its writers.
    auto prepared = Graphics::PrepareDraw(device, state, stages, false);
    if (!Graphics::PreparedDrawWriters()) {
        Require(prepared == nullptr || !prepared->writersPrepared, "APS5_NO_PREPARED_DRAW_WRITERS=1 still prepared the writers");
        Graphics::PreparedDraw collected;
        collected.shaders = stages;
        collected.writersPrepared = true;
        Require(Graphics::PreparedWriters(&collected, stages) == nullptr, "APS5_NO_PREPARED_DRAW_WRITERS=1 still noted a prepared list");
        return;
    }
    Require(prepared != nullptr && !prepared->writersPrepared, "a draw with a recipe has no prepared draw for its writers");
    // Driver::drawWriteRanges on the worker: the pipeline item's write ranges and the list, one walk.
    const auto ranges = AgcDriver::DriverDetail::DrawWriteRanges(state, stages, true, &prepared->writers);
    prepared->writersPrepared = true;
    const std::vector<Range> expected{{0x100000000ull, 0x100001000ull}, {0x300000000ull, 0x300001000ull}};
    Require(prepared->writers == expected, "the prepared list is not the stages' written buffers in stage order (read-only ones left out)");
    for (const auto& range : expected) Require(std::find(ranges.begin(), ranges.end(), range) != ranges.end(), "a written buffer left the pipeline item's write ranges");
    Require(Graphics::PreparedWriters(nullptr, stages) == nullptr, "a list was noted from no prepared draw");
    auto other = stages;
    other[0].program = &twin;
    Require(Graphics::PreparedWriters(prepared.get(), other) == nullptr, "a list collected for one program object served an equal copy");
    Require(Graphics::PreparedWriters(prepared.get(), std::span(stages).first(1)) == nullptr, "a list collected for two stages served one");
    Require(Graphics::PreparedWriters(prepared.get(), stages) == &prepared->writers, "a list collected for these stages was not noted");
    prepared->writersPrepared = false;
    Require(Graphics::PreparedWriters(prepared.get(), stages) == nullptr, "a list that was never collected was noted");
}

void CheckScope() {
    Graphics::PreparedDraw outer;
    Require(Graphics::PreparedDrawScope::Active() == nullptr, "a prepared draw is current outside any commit");
    {
        Graphics::PreparedDrawScope scope(&outer);
        Require(Graphics::PreparedDrawScope::Active() == &outer, "the commit's prepared draw is not current");
        {
            Graphics::PreparedDrawScope inner(nullptr);
            Require(Graphics::PreparedDrawScope::Active() == nullptr, "a nested scope did not replace the prepared draw");
        }
        Require(Graphics::PreparedDrawScope::Active() == &outer, "a nested scope did not restore the outer prepared draw");
    }
    Require(Graphics::PreparedDrawScope::Active() == nullptr, "the prepared draw outlived its commit");
}

}

int main() {
    try {
        const auto vertex = Program(0x1111, 0x100000000ull, 0x200000000ull);
        const auto pixel = Program(0x2222, 0x300000000ull, 0x400000000ull);
        const auto twin = vertex;
        const std::vector<Graphics::CompiledShader> stages{{Stage::Vertex, &vertex, 0}, {Stage::Fragment, &pixel, 16}};
        CheckKeys(stages, twin);
        CheckWriters(stages, twin);
        CheckScope();
        std::puts("prepared draw tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
