#include "SceTypes.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>

extern "C" void APS5_VABI _sceVideoRecordingQueryParam_nid_postfix(void* param);
extern "C" int APS5_VABI sceVideoRecordingQueryMemSize(const void* param);
extern "C" int APS5_VABI sceVideoRecordingOpen(const char* path, const void* param, void* heap, int heapSize);
extern "C" int APS5_VABI sceVideoRecordingQueryMemSize2(const void* param);
extern "C" int APS5_VABI sceVideoRecordingOpen2(const char* path, const void* param, void* heap, int heapSize);
extern "C" int APS5_VABI sceVideoRecordingGetStatus(void);
extern "C" int APS5_VABI sceVideoRecordingStart(void);
extern "C" int APS5_VABI sceVideoRecordingStop(void);
extern "C" int APS5_VABI sceVideoRecordingClose(int discard);

namespace {

constexpr int32_t ERROR_INVALID_STATE = static_cast<int32_t>(0x80A80006);
constexpr int32_t ERROR_UNSUPPORTED = static_cast<int32_t>(0x80A80008);
constexpr int32_t STATUS_NONE = 0;
constexpr unsigned char Sentinel = 0xA5;

void Require(bool value) { if (!value) std::abort(); }

void RequireNoRecording() {
    Require(sceVideoRecordingStart() == ERROR_INVALID_STATE);
    Require(sceVideoRecordingStop() == ERROR_INVALID_STATE);
    Require(sceVideoRecordingClose(0) == ERROR_INVALID_STATE);
    Require(sceVideoRecordingClose(1) == ERROR_INVALID_STATE);
    Require(sceVideoRecordingGetStatus() == STATUS_NONE);
}

template <typename TAction>
bool Throws(TAction action) {
    try {
        action();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

}

int main() {
    std::array<unsigned char, 64> param{};
    param.fill(Sentinel);
    std::array<unsigned char, 4096> heap{};
    heap.fill(Sentinel);
    const char* path = "/video/clip.mp4";

    Require(sceVideoRecordingGetStatus() == STATUS_NONE);
    Require(sceVideoRecordingGetStatus() == STATUS_NONE);
    Require(Throws([&] { _sceVideoRecordingQueryParam_nid_postfix(param.data()); }));
    RequireNoRecording();
    Require(Throws([] { sceVideoRecordingQueryMemSize(nullptr); }));
    const int memSize = sceVideoRecordingQueryMemSize(param.data());
    Require(memSize == ERROR_UNSUPPORTED);

    Require(Throws([&] { sceVideoRecordingOpen(nullptr, param.data(), heap.data(), static_cast<int>(heap.size())); }));
    Require(Throws([&] { sceVideoRecordingOpen(path, nullptr, heap.data(), static_cast<int>(heap.size())); }));
    Require(sceVideoRecordingOpen(path, param.data(), heap.data(), static_cast<int>(heap.size())) == ERROR_UNSUPPORTED);
    Require(sceVideoRecordingOpen(path, param.data(), nullptr, memSize) == ERROR_UNSUPPORTED);
    Require(sceVideoRecordingGetStatus() == STATUS_NONE);
    RequireNoRecording();
    Require(Throws([] { sceVideoRecordingQueryMemSize2(nullptr); }));
    Require(sceVideoRecordingQueryMemSize2(param.data()) == ERROR_UNSUPPORTED);
    Require(Throws([&] { sceVideoRecordingOpen2(nullptr, param.data(), heap.data(), static_cast<int>(heap.size())); }));
    Require(Throws([&] { sceVideoRecordingOpen2(path, nullptr, heap.data(), static_cast<int>(heap.size())); }));
    Require(sceVideoRecordingOpen2(path, param.data(), heap.data(), static_cast<int>(heap.size())) == ERROR_UNSUPPORTED);
    Require(sceVideoRecordingGetStatus() == STATUS_NONE);
    RequireNoRecording();
    for (const unsigned char value : param) Require(value == Sentinel);
    for (const unsigned char value : heap) Require(value == Sentinel);

    std::puts("VideoRecording tests passed");
    return 0;
}
