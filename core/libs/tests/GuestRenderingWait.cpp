#include "prx/libSceAgcDriver/Execution/include/VideoOutput.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libSceAgcDriver/Resource/include/Registration.hpp"
#include "prx/libSceVideoOut/include/BufferReuseTracker.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
extern "C" std::uint32_t APS5_VABI sceAgcDriverGetWaitRenderingPacketSizeInDwords();
extern "C" std::uint32_t APS5_VABI sceAgcDriverGetSetFlipPacketSizeInDwords();
extern "C" std::uint32_t APS5_VABI sceAgcDriverGetSetWorkloadsActivePacketSize();
extern "C" std::uint32_t APS5_VABI sceAgcDriverGetSetWorkloadCompletePacketSize();
extern "C" std::uint32_t APS5_VABI sceAgcDriverGetSetWorkloadStreamInactivePacketSize();
extern "C" std::uint32_t APS5_VABI sceAgcDriverNopDataGetPacketSize(std::uint32_t);
extern "C" std::uint32_t APS5_VABI sceAgcDriverUserDataGetPacketSize(std::uint32_t);
extern "C" std::uint32_t APS5_VABI sceAgcDriverWaitUntilSafeForRendering(std::uint32_t**, std::uint32_t, std::uint32_t, std::uint32_t, int);
extern "C" std::uint32_t APS5_VABI sceAgcDriverSetFlip(std::uint32_t**, std::uint32_t, std::uint32_t, std::uint32_t, std::int32_t, std::uint32_t, std::int64_t);
extern "C" int APS5_VABI sceAgcDriverSetWorkloadsActive(std::uint32_t*, std::uint32_t, std::uint32_t, const std::uint32_t*, std::uint32_t);
extern "C" int APS5_VABI sceAgcDriverSetWorkloadComplete(std::uint32_t*, std::uint32_t, std::uint32_t, std::uint32_t);
extern "C" int APS5_VABI sceAgcDriverSetupAsyncGraphics(std::uint32_t);
extern "C" int APS5_VABI sceAgcDriverRegisterWorkloadStream(std::uint32_t, const void*);
extern "C" int APS5_VABI sceAgcDriverUnregisterWorkloadStream(std::uint32_t);
static void Require(bool value) { if (!value) std::abort(); }
template <typename TCall> static bool Rejects(TCall call) {
    try { call(); }
    catch (const std::invalid_argument&) { return true; }
    return false;
}
static std::uint32_t Low(std::uint64_t value) { return static_cast<std::uint32_t>(value); }
static std::uint32_t High(std::uint64_t value) { return static_cast<std::uint32_t>(value >> 32u); }
static void CheckWorkloadPackets() {
    const std::array<std::uint8_t, 32> name{'s', 't', 'r', 'e', 'a', 'm'};
    const std::uint32_t one[] = {1};
    const std::uint32_t pair[] = {1, 63};
    const std::uint32_t repeated[] = {3, 3};
    std::array<std::uint32_t, 18> active{};
    std::array<std::uint32_t, 12> complete{};
    const auto untouched = [&] { return std::all_of(active.begin(), active.end(), [](std::uint32_t word) { return word == 0; }) &&
        std::all_of(complete.begin(), complete.end(), [](std::uint32_t word) { return word == 0; }); };
    Require(Rejects([&] { sceAgcDriverSetWorkloadsActive(active.data(), 0, 1, one, 1); }));
    Require(Rejects([&] { sceAgcDriverSetWorkloadComplete(complete.data(), 0, 1, 1); }));
    Require(untouched());
    Require(sceAgcDriverRegisterWorkloadStream(1, name.data()) == 0);
    const auto slot = AgcDriverWorkloadStreamSlot_nid_no_patch(1);
    Require(slot != 0);
    Require(Rejects([&] { sceAgcDriverSetWorkloadsActive(active.data(), 2, 1, one, 1); }));
    Require(Rejects([&] { sceAgcDriverSetWorkloadsActive(active.data(), 0, 1, repeated, 2); }));
    Require(Rejects([&] { sceAgcDriverSetWorkloadsActive(active.data(), 0, 1, one, 0); }));
    Require(Rejects([&] { sceAgcDriverSetWorkloadsActive(active.data(), 0, 1, nullptr, 1); }));
    Require(Rejects([&] { sceAgcDriverSetWorkloadsActive(nullptr, 0, 1, one, 1); }));
    Require(Rejects([&] { sceAgcDriverSetWorkloadComplete(complete.data(), 0, 1, 64); }));
    Require(Rejects([&] { sceAgcDriverSetWorkloadComplete(complete.data(), 2, 1, 1); }));
    Require(untouched());
    Require(sceAgcDriverSetWorkloadsActive(active.data(), 0, 1, one, 1) == 0);
    Require(active == std::array<std::uint32_t, 18>{0xc0027904u, 0x342u, 0xcc000001u, 2u, 0xc0033704u, 0x06010000u, 0xc343u, 0u, 0u,
        0xc0071e00u, 0x40000267u, Low(slot), High(slot), 2u, 0u, 0u, 0u, 0u});
    Require(sceAgcDriverSetWorkloadsActive(active.data(), 1, 1, pair, 2) == 0);
    Require(active == std::array<std::uint32_t, 18>{0xc0027900u, 0x342u, 0xcc000001u, 2u, 0xc0033700u, 0x06010000u, 0xc343u, 0x80000000u, 0u,
        0xc0071e00u, 0x267u, Low(slot), High(slot), 2u, 0x80000000u, 0u, 0u, 0u});
    Require(sceAgcDriverSetWorkloadComplete(complete.data(), 0, 1, 1) == 0);
    Require(complete == std::array<std::uint32_t, 12>{0xc0017904u, 0x342u, 0xcd000021u, 0xc0071e00u, 0x40000275u, Low(slot), High(slot),
        0xfffffffdu, 0xffffffffu, 0u, 0u, 0u});
    Require(sceAgcDriverSetWorkloadComplete(complete.data(), 1, 1, 63) == 0);
    Require(complete == std::array<std::uint32_t, 12>{0xc0017900u, 0x342u, 0xcd0007e1u, 0xc0071e00u, 0x275u, Low(slot), High(slot),
        0xffffffffu, 0x7fffffffu, 0u, 0u, 0u});
    Require(sceAgcDriverUnregisterWorkloadStream(1) == 0);
    active.fill(0);
    Require(Rejects([&] { sceAgcDriverSetWorkloadsActive(active.data(), 0, 1, one, 1); }));
    Require(std::all_of(active.begin(), active.end(), [](std::uint32_t word) { return word == 0; }));
}
static void CheckFlipPacket() {
    std::array<std::uint32_t, 6> words{};
    auto* cursor = words.data();
    Require(sceAgcDriverSetFlip(&cursor, 6, 0, 0xfedcba98u, -2, 0x12345678u, -0x123456789abcdefLL) == 0);
    Require(cursor == words.data() + words.size());
    Require(words == std::array<std::uint32_t, 6>{AgcDriver::FlipPacketHeader, 0xfedcba98u, 0xfffffffeu, 0x12345678u, 0x76543211u, 0xfedcba98u});
}
class Output final : public AgcDriver::IVideoOutput, public AgcDriver::IRenderingWait,
    public AgcDriver::IFlipRequest, public std::enable_shared_from_this<Output> {
public:
    std::mutex mutex;
    std::condition_variable changed;
    BufferReuseTracker reuse;
    const std::uint64_t earlier = reuse.Reserve();
    std::uint64_t captured = 0, later = 0;
    bool entered = false, flipped = false;
    std::shared_ptr<AgcDriver::IRenderingWait> CaptureRenderingWait(std::uint32_t index) override {
        Require(index == 0);
        std::lock_guard lock(mutex);
        captured = reuse.Capture();
        return shared_from_this();
    }
    std::shared_ptr<AgcDriver::IFlipRequest> Reserve(const AgcDriver::FlipInfo& info) override {
        Require(info.index == 0);
        std::lock_guard lock(mutex);
        later = reuse.Reserve();
        return shared_from_this();
    }
    void Wait() override {
        std::unique_lock lock(mutex);
        entered = true;
        changed.notify_all();
        Require(changed.wait_for(lock, std::chrono::seconds(5), [&] { return reuse.IsComplete(captured); }));
    }
    void GpuReady(const std::shared_ptr<AgcDriver::FrameTiming>&) override {
        std::lock_guard lock(mutex);
        Require(reuse.IsComplete(captured));
        reuse.Complete(later);
        flipped = true;
    }
    void Fail(std::exception_ptr) noexcept override { std::abort(); }
};
int main() {
    auto output = std::make_shared<Output>();
    AgcDriverRegisterVideoOutput_nid_postfix(7, output);
    std::array<std::uint32_t, 11> words{};
    words.back() = 0x12345678;
    auto* cursor = words.data();
    const auto size = sceAgcDriverGetWaitRenderingPacketSizeInDwords();
    Require(size == AgcDriver::RenderingWaitPacketWords);
    const auto flipPacketSize = sceAgcDriverGetSetFlipPacketSizeInDwords();
    Require(flipPacketSize == AgcDriver::FlipPacketWords);
    Require(flipPacketSize == 6);
    Require(sceAgcDriverGetSetWorkloadsActivePacketSize() == 18);
    Require(sceAgcDriverGetSetWorkloadCompletePacketSize() == 12);
    Require(sceAgcDriverGetSetWorkloadStreamInactivePacketSize() == 9);
    Require(sceAgcDriverNopDataGetPacketSize(0) == 2 && sceAgcDriverNopDataGetPacketSize(1) == 3 && sceAgcDriverNopDataGetPacketSize(4) == 3);
    Require(sceAgcDriverNopDataGetPacketSize(5) == 4 && sceAgcDriverNopDataGetPacketSize(0xffffffffu) == 0x40000002u);
    Require(sceAgcDriverUserDataGetPacketSize(0) == 3 && sceAgcDriverUserDataGetPacketSize(1) == 4 && sceAgcDriverUserDataGetPacketSize(4) == 4);
    Require(sceAgcDriverUserDataGetPacketSize(5) == 9 && sceAgcDriverUserDataGetPacketSize(0xffffffffu) == 0x40000007u);
    bool rejected = false;
    try { sceAgcDriverWaitUntilSafeForRendering(&cursor, size - 1, 0, 7, 0); }
    catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected && cursor == words.data() && words[0] == 0);
    Require(sceAgcDriverWaitUntilSafeForRendering(&cursor, size, 0, 7, 0) == 0);
    Require(cursor == words.data() + size);
    Require(words[0] == AgcDriver::RenderingWaitPacketHeader && words[1] == 7 && words[2] == 0 && words[3] == 0);
    auto* const flipAt = cursor;
    Require(Rejects([&] { sceAgcDriverSetFlip(&cursor, flipPacketSize - 1, 0, 7, 0, 1, 0); }));
    Require(Rejects([&] { sceAgcDriverSetFlip(&cursor, flipPacketSize, 1, 7, 0, 1, 0); }));
    Require(Rejects([&] { sceAgcDriverSetFlip(nullptr, flipPacketSize, 0, 7, 0, 1, 0); }));
    Require(cursor == flipAt && flipAt[0] == 0);
    Require(sceAgcDriverSetFlip(&cursor, flipPacketSize, 0, 7, 0, 1, 0) == 0);
    Require(cursor == flipAt + flipPacketSize);
    const std::uint32_t flip[] = {AgcDriver::FlipPacketHeader, 7, 0, 1, 0, 0};
    Require(sizeof(flip) / sizeof(flip[0]) == flipPacketSize && std::equal(std::begin(flip), std::end(flip), flipAt));
    Require(words.back() == 0x12345678);
    CheckFlipPacket();
    CheckWorkloadPackets();
    Require(sceAgcDriverSetupAsyncGraphics(0) == 0 && sceAgcDriverSetupAsyncGraphics(1) == 0);
    Packet packet{words.data(), 10, 0, {}};
    sceAgcDriverSubmitDcb(&packet);
    {
        std::unique_lock lock(output->mutex);
        Require(output->changed.wait_for(lock, std::chrono::seconds(5), [&] { return output->entered; }));
        Require(!output->flipped && output->later > output->captured);
        output->reuse.Complete(output->earlier);
        output->changed.notify_all();
    }
    AgcDriverWaitIdle_nid_postfix();
    Require(output->flipped);
    AgcDriverUnregisterVideoOutput_nid_postfix(7, output);
    LibcRunShutdown_nid_postfix();
}
