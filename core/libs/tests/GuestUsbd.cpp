#include "prx/libc/include/general/VabiMacros.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

struct UsbdTimeval {
    std::int64_t seconds;
    std::int64_t microseconds;
};

}

extern "C" {
std::int32_t APS5_VABI sceUsbdInit();
void APS5_VABI sceUsbdExit();
std::int64_t APS5_VABI sceUsbdGetDeviceList(void*** list);
void APS5_VABI sceUsbdFreeDeviceList(void** list, std::int32_t unrefDevices);
std::int32_t APS5_VABI sceUsbdHandleEventsTimeout(const UsbdTimeval* timeout);
std::int32_t APS5_VABI sceUsbdOpen(void* device, void** devHandle);
std::int32_t APS5_VABI sceUsbdEventHandlingOk();
void* APS5_VABI sceUsbdAllocTransfer(std::int32_t isoPackets);
void APS5_VABI sceUsbdFreeTransfer(void* transfer);
void APS5_VABI sceUsbdFillInterruptTransfer(void* transfer, void* devHandle, std::uint8_t endpoint, std::uint8_t* buffer, std::int32_t length,
    void (APS5_VABI *callback)(void*), void* userData, std::uint32_t timeout);
std::int32_t APS5_VABI sceUsbdSubmitTransfer(void* transfer);
std::int32_t APS5_VABI sceUsbdCancelTransfer(void* transfer);
void APS5_VABI sceUsbdFreeConfigDescriptor(void* config);
}

static void APS5_VABI TransferDone(void*) {}

namespace {

constexpr std::int32_t invalidArgument = static_cast<std::int32_t>(0x80240002);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "USBD: %s\n", message);
        std::abort();
    }
}

}

int main() {
    Require(sceUsbdInit() == 0, "initialization failed");
    void** list = nullptr;
    Require(sceUsbdGetDeviceList(&list) == 0, "a device was listed");
    Require(list != nullptr && list[0] == nullptr, "device list is not an empty null-terminated array");
    sceUsbdFreeDeviceList(list, 1);
    Require(sceUsbdGetDeviceList(nullptr) == invalidArgument, "null list accepted");
    Require(sceUsbdHandleEventsTimeout(nullptr) == invalidArgument, "null timeout accepted");
    const UsbdTimeval invalid{0, 1000000};
    Require(sceUsbdHandleEventsTimeout(&invalid) == invalidArgument, "out-of-range microseconds accepted");
    const UsbdTimeval timeout{0, 50000};
    const auto start = std::chrono::steady_clock::now();
    Require(sceUsbdHandleEventsTimeout(&timeout) == 0, "event handling failed");
    Require(std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(45), "event handling returned before the timeout");
    bool threw = false;
    try {
        void* handle = nullptr;
        sceUsbdOpen(nullptr, &handle);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "opening a device did not throw");
    Require(sceUsbdEventHandlingOk() == 1, "event handling not reported as ok");
    Require(sceUsbdAllocTransfer(-1) == nullptr, "negative iso packet count accepted");
    auto* transfer = static_cast<unsigned char*>(sceUsbdAllocTransfer(2));
    Require(transfer != nullptr, "transfer allocation failed");
    std::int32_t isoPackets = 0;
    std::memcpy(&isoPackets, transfer + 56, sizeof(isoPackets));
    Require(isoPackets == 2, "iso packet count not stored");
    std::uint8_t buffer[8]{};
    int userData = 0;
    sceUsbdFillInterruptTransfer(transfer, nullptr, 0x81, buffer, sizeof(buffer), TransferDone, &userData, 500);
    std::uint32_t timeoutMs = 0;
    std::int32_t length = 0;
    void* storedBuffer = nullptr;
    void* storedUserData = nullptr;
    std::memcpy(&timeoutMs, transfer + 12, sizeof(timeoutMs));
    std::memcpy(&length, transfer + 20, sizeof(length));
    std::memcpy(&storedUserData, transfer + 40, sizeof(storedUserData));
    std::memcpy(&storedBuffer, transfer + 48, sizeof(storedBuffer));
    Require(transfer[9] == 0x81 && transfer[10] == 3 && timeoutMs == 500 && length == 8, "interrupt transfer fields not filled");
    Require(storedBuffer == buffer && storedUserData == &userData, "interrupt transfer pointers not filled");
    threw = false;
    try {
        sceUsbdSubmitTransfer(transfer);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "submitting a transfer without a device did not throw");
    Require(sceUsbdCancelTransfer(transfer) == static_cast<std::int32_t>(0x80240005), "cancelling an unsubmitted transfer did not report not found");
    sceUsbdFreeTransfer(transfer);
    sceUsbdFreeTransfer(nullptr);
    sceUsbdFreeConfigDescriptor(nullptr);
    sceUsbdExit();
    std::puts("USBD tests passed");
    return 0;
}
