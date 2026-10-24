#include "prx/libc/include/general/VabiMacros.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
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
std::int32_t APS5_VABI sceUsbdOpen(void* device, void** deviceHandle);
void APS5_VABI sceUsbdClose(void* deviceHandle);
void* APS5_VABI sceUsbdRefDevice(void* device);
void APS5_VABI sceUsbdUnrefDevice(void* device);
std::uint8_t APS5_VABI sceUsbdGetBusNumber(void* device);
std::uint8_t APS5_VABI sceUsbdGetDeviceAddress(void* device);
std::int32_t APS5_VABI sceUsbdGetDeviceDescriptor(void* device, void* descriptor);
std::int32_t APS5_VABI sceUsbdGetConfigDescriptor(void* device, std::uint8_t configIndex, void** config);
std::int32_t APS5_VABI sceUsbdGetActiveConfigDescriptor(void* device, void** config);
std::int32_t APS5_VABI sceUsbdClaimInterface(void* deviceHandle, std::int32_t interfaceNumber);
std::int32_t APS5_VABI sceUsbdReleaseInterface(void* deviceHandle, std::int32_t interfaceNumber);
std::int32_t APS5_VABI sceUsbdSetConfiguration(void* deviceHandle, std::int32_t configuration);
std::int32_t APS5_VABI sceUsbdResetDevice(void* deviceHandle);
std::int32_t APS5_VABI sceUsbdCheckConnected(void* deviceHandle);
std::int32_t APS5_VABI sceUsbdKernelDriverActive(void* deviceHandle, std::int32_t interfaceNumber);
std::int32_t APS5_VABI sceUsbdAttachKernelDriver(void* deviceHandle, std::int32_t interfaceNumber);
std::int32_t APS5_VABI sceUsbdControlTransfer(void* deviceHandle, std::uint8_t requestType, std::uint8_t request, std::uint16_t value, std::uint16_t index, std::uint8_t* data, std::uint16_t length, std::uint32_t timeout);
std::int32_t APS5_VABI sceUsbdGetStringDescriptor(void* deviceHandle, std::uint8_t descriptorIndex, std::uint16_t languageId, std::uint8_t* data, std::int32_t length);
std::int32_t APS5_VABI sceUsbdSubmitTransfer(void* transfer);
void* APS5_VABI sceUsbdAllocTransfer(std::int32_t isoPackets);
void APS5_VABI sceUsbdFreeTransfer(void* transfer);
void APS5_VABI sceUsbdFillInterruptTransfer(void* transfer, void* deviceHandle, std::uint8_t endpoint, std::uint8_t* buffer, std::int32_t length, void* callback, void* userData, std::uint32_t timeout);
std::int32_t APS5_VABI sceUsbdCancelTransfer(void* transfer);
std::int32_t APS5_VABI sceUsbdEventHandlingOk();
void APS5_VABI sceUsbdFreeConfigDescriptor(void* config);
std::int32_t APS5_VABI sceUsbdClearHalt(void* deviceHandle, std::uint8_t endpoint);
std::int32_t APS5_VABI sceUsbdDetachKernelDriver(void* deviceHandle, int interfaceNumber);
std::int32_t APS5_VABI sceUsbdEventHandlerActive();
std::int32_t APS5_VABI sceUsbdGetConfigDescriptorByValue(void* device, std::uint8_t configurationValue, void** config);
std::int32_t APS5_VABI sceUsbdGetConfiguration(void* deviceHandle, std::int32_t* configuration);
std::int32_t APS5_VABI sceUsbdGetDescriptor(void* deviceHandle, std::uint8_t descriptorType, std::uint8_t descriptorIndex, std::uint8_t* data, std::int32_t length);
void* APS5_VABI sceUsbdGetDevice(void* deviceHandle);
int APS5_VABI sceUsbdGetDeviceSpeed(void* device);
std::int32_t APS5_VABI sceUsbdGetMaxIsoPacketSize(void* device, std::uint8_t endpoint);
std::int32_t APS5_VABI sceUsbdGetMaxPacketSize(void* device, std::uint8_t endpoint);
std::int32_t APS5_VABI sceUsbdGetStringDescriptorAscii(void* deviceHandle, std::uint8_t descriptorIndex, std::uint8_t* data, std::int32_t length);
std::int32_t APS5_VABI sceUsbdHandleEvents();
std::int32_t APS5_VABI sceUsbdHandleEventsLocked(const UsbdTimeval* timeout);
void APS5_VABI sceUsbdLockEventWaiters();
void APS5_VABI sceUsbdLockEvents();
void* APS5_VABI sceUsbdOpenDeviceWithVidPid(std::uint16_t vendorId, std::uint16_t productId);
std::int32_t APS5_VABI sceUsbdSetInterfaceAltSetting(void* deviceHandle, int interfaceNumber, int alternateSetting);
std::int32_t APS5_VABI sceUsbdTryLockEvents();
void APS5_VABI sceUsbdUnlockEventWaiters();
void APS5_VABI sceUsbdUnlockEvents();
std::int32_t APS5_VABI sceUsbdWaitForEvent(const UsbdTimeval* timeout);
void APS5_VABI sceUsbdFillControlSetup(std::uint8_t* buffer, std::uint8_t requestType, std::uint8_t request, std::uint16_t value, std::uint16_t index, std::uint16_t length);
}

namespace {

constexpr std::int32_t invalidArgument = static_cast<std::int32_t>(0x80240002);
constexpr std::int32_t noDevice = static_cast<std::int32_t>(0x80240004);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "USBD: %s\n", message);
        std::abort();
    }
}

template<typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

struct HostTransfer {
    void* deviceHandle;
    std::uint8_t flags;
    std::uint8_t endpoint;
    std::uint8_t type;
    std::uint32_t timeout;
    std::int32_t status;
    std::int32_t length;
    std::int32_t actualLength;
    void (APS5_VABI* callback)(HostTransfer*);
    void* userData;
    std::uint8_t* buffer;
    std::int32_t numIsoPackets;
};

void APS5_VABI MarkDone(HostTransfer* transfer) {
    *static_cast<int*>(transfer->userData) += 1;
}

void APS5_VABI MarkDoneThenSignal(HostTransfer* transfer) {
    auto* notified = static_cast<std::atomic<bool>*>(transfer->userData);
    notified->store(true, std::memory_order_release);
}

void Pump(const int& done) {
    const UsbdTimeval slice{0, 100000};
    for (int round = 0; round < 50 && done == 0; ++round) Require(sceUsbdHandleEventsTimeout(&slice) == 0, "event handling failed while a transfer was in flight");
}

void Device() {
    const char* wanted = std::getenv("APS5_USBD_DEVICE");
    Require(wanted != nullptr, "APS5_USBD_DEVICE is not set to vendor:product in hex");
    const unsigned long vendor = std::strtoul(wanted, nullptr, 16);
    const char* separator = std::strchr(wanted, ':');
    Require(separator != nullptr, "APS5_USBD_DEVICE has no ':'");
    const unsigned long product = std::strtoul(separator + 1, nullptr, 16);
    void** list = nullptr;
    const std::int64_t count = sceUsbdGetDeviceList(&list);
    void* device = nullptr;
    std::uint8_t descriptor[18] = {};
    for (std::int64_t index = 0; index < count && device == nullptr; ++index) {
        Require(sceUsbdGetDeviceDescriptor(list[index], descriptor) == 0, "device descriptor failed");
        if ((descriptor[8] | descriptor[9] << 8) == vendor && (descriptor[10] | descriptor[11] << 8) == product) device = sceUsbdRefDevice(list[index]);
    }
    sceUsbdFreeDeviceList(list, 1);
    Require(device != nullptr, "the APS5_USBD_DEVICE device was not listed");
    std::printf("device bus %u address %u\n", sceUsbdGetBusNumber(device), sceUsbdGetDeviceAddress(device));
    void* config = nullptr;
    Require(sceUsbdGetActiveConfigDescriptor(device, &config) == 0 && config != nullptr, "active config descriptor failed");
    const auto* raw = static_cast<const std::uint8_t*>(config);
    Require(raw[1] == 2 && raw[4] > 0, "active config descriptor is not a configuration with interfaces");
    const std::uint8_t interfaces = raw[4];
    const std::uint8_t configurationValue = raw[5];
    sceUsbdFreeConfigDescriptor(config);
    Require(Throws([&] { sceUsbdFreeConfigDescriptor(config); }), "a config descriptor was freed twice");
    Require(sceUsbdGetConfigDescriptor(device, 0, &config) == 0, "config descriptor 0 failed");
    sceUsbdFreeConfigDescriptor(config);
    void* handle = nullptr;
    Require(sceUsbdOpen(device, &handle) == 0 && handle != nullptr, "opening the device failed");
    Require(sceUsbdCheckConnected(handle) == 0, "an open device was reported disconnected");
    std::uint8_t text[256] = {};
    if (descriptor[15] != 0) {
        const std::int32_t length = sceUsbdGetStringDescriptor(handle, descriptor[15], 0x409, text, sizeof(text));
        Require(length >= 2 && text[1] == 3, "product string descriptor failed");
        std::string product_name;
        for (std::int32_t index = 2; index + 1 < length; index += 2) product_name += static_cast<char>(text[index]);
        std::printf("product \"%s\"\n", product_name.c_str());
    }
    Require(sceUsbdGetDevice(handle) == device, "the handle does not lead back to its device");
    std::int32_t configuration = -1;
    Require(sceUsbdGetConfiguration(handle, &configuration) == 0 && configuration >= 0, "reading the configuration failed");
    std::printf("configuration %d speed %d\n", configuration, sceUsbdGetDeviceSpeed(device));
    std::uint8_t wire[18] = {};
    Require(sceUsbdGetDescriptor(handle, 1, 0, wire, sizeof(wire)) == 18 && std::memcmp(wire, descriptor, sizeof(wire)) == 0, "the device descriptor read with GetDescriptor differs");
    if (descriptor[15] != 0) {
        char ascii[128] = {};
        const std::int32_t length = sceUsbdGetStringDescriptorAscii(handle, descriptor[15], reinterpret_cast<std::uint8_t*>(ascii), sizeof(ascii));
        Require(length > 0 && ascii[length] == 0, "the ASCII product string failed");
        std::printf("ascii product \"%s\"\n", ascii);
    }
    void* byValue = nullptr;
    Require(sceUsbdGetConfigDescriptorByValue(device, configurationValue, &byValue) == 0 && byValue != nullptr, "the active configuration was not found by its value");
    sceUsbdFreeConfigDescriptor(byValue);
    std::uint8_t again[18] = {};
    Require(sceUsbdControlTransfer(handle, 0x80, 6, 0x0100, 0, again, sizeof(again), 1000) == 18, "synchronous device descriptor request failed");
    Require(std::memcmp(again, descriptor, sizeof(again)) == 0, "device descriptor read over the wire differs from the cached one");
    for (std::int32_t interfaceNumber = 0; interfaceNumber < interfaces; ++interfaceNumber) {
        Require(sceUsbdClaimInterface(handle, interfaceNumber) == 0, "claiming an interface failed");
        Require(sceUsbdSetInterfaceAltSetting(handle, interfaceNumber, 0) == 0, "selecting alternate setting 0 failed");
        Require(sceUsbdReleaseInterface(handle, interfaceNumber) == 0, "releasing an interface failed");
    }
    Require(sceUsbdReleaseInterface(handle, 0) == static_cast<std::int32_t>(0x80240005), "releasing an unclaimed interface did not report not found");
    Require(sceUsbdGetConfigDescriptor(device, 0, &config) == 0, "config descriptor 0 for the endpoint scan failed");
    const std::uint8_t* config0 = static_cast<const std::uint8_t*>(config);
    const std::uint8_t* interfaceArray = *reinterpret_cast<const std::uint8_t* const*>(config0 + 16);
    std::uint8_t interruptAddress = 0;
    std::uint16_t interruptMax = 0;
    if (interfaceArray == nullptr) {
        Require(false, "the active config descriptor has no interface array");
    }
    for (std::uint8_t index = 0; index < interfaces && interruptAddress == 0; ++index) {
        const std::uint8_t* alternate = *reinterpret_cast<const std::uint8_t* const*>(interfaceArray + index * 16);
        if (alternate == nullptr) continue;
        const std::uint8_t* endpoints = *reinterpret_cast<const std::uint8_t* const*>(alternate + 16);
        if (endpoints == nullptr) continue;
        for (std::uint8_t endpoint = 0; endpoint < alternate[4]; ++endpoint) {
            const std::uint8_t* item = endpoints + endpoint * 32;
            if ((item[2] & 0x80) != 0 && (item[3] & 3) == 3) {
                interruptAddress = item[2];
                interruptMax = static_cast<std::uint16_t>(item[4] | item[5] << 8);
            }
        }
    }
    Require(interruptAddress != 0 && interruptMax != 0, "the device has no interrupt IN endpoint");
    Require(sceUsbdGetMaxPacketSize(device, interruptAddress) > 0 && sceUsbdGetMaxIsoPacketSize(device, interruptAddress) > 0, "the interrupt endpoint has no packet size");
    std::printf("clear halt %d, detach kernel driver %d\n", sceUsbdClearHalt(handle, interruptAddress), sceUsbdDetachKernelDriver(handle, 0));
    sceUsbdFreeConfigDescriptor(config);

    auto* transfer = static_cast<HostTransfer*>(sceUsbdAllocTransfer(0));
    std::uint8_t setup[8 + 18] = {0x80, 6, 0x00, 0x01, 0x00, 0x00, 18, 0};
    int done = 0;
    transfer->deviceHandle = handle;
    transfer->type = 0;
    transfer->timeout = 1000;
    transfer->buffer = setup;
    transfer->length = sizeof(setup);
    transfer->callback = MarkDone;
    transfer->userData = &done;
    Require(sceUsbdSubmitTransfer(transfer) == 0, "submitting an asynchronous control transfer failed");
    Require(Throws([&] { sceUsbdFreeTransfer(transfer); }), "an in-flight transfer was freed");
    Pump(done);
    Require(done == 1 && transfer->status == 0 && transfer->actualLength == 18, "asynchronous control transfer did not complete");
    Require(std::memcmp(setup + 8, descriptor, 18) == 0, "asynchronous device descriptor differs from the cached one");
    done = 0;
    std::memset(setup + 8, 0, 18);
    Require(sceUsbdSubmitTransfer(transfer) == 0, "resubmitting the asynchronous control transfer failed");
    for (int round = 0; round < 10 && done == 0; ++round) Require(sceUsbdHandleEvents() == 0, "blocking event handling failed");
    Require(done == 1 && transfer->status == 0 && std::memcmp(setup + 8, descriptor, 18) == 0, "the transfer did not complete under sceUsbdHandleEvents");
    bool broken = false;
    {
        const UsbdTimeval slice{0, 200};
        for (int round = 0; round < 300 && !broken; ++round) {
            std::atomic<bool> notified{false};
            std::uint8_t blink[64] = {};
            sceUsbdFillInterruptTransfer(transfer, handle, interruptAddress, blink, interruptMax, reinterpret_cast<void*>(&MarkDoneThenSignal), &notified, 0);
            const int submitResult = sceUsbdSubmitTransfer(transfer);
            if (submitResult != 0) {
                broken = true;
                break;
            }
            std::thread canceller([&] {
                const int outcome = sceUsbdCancelTransfer(transfer);
                if (outcome != 0 && outcome != static_cast<std::int32_t>(0x80240005)) broken = true;
            });
            while (!notified.load(std::memory_order_acquire)) {
                if (sceUsbdHandleEventsTimeout(&slice) != 0) broken = true;
            }
            canceller.join();
            if (transfer->status != 0 && transfer->status != 2 && transfer->status != 3) broken = true;
        }
    }
    Require(!broken, "the cancel race failed");

    Require(sceUsbdCancelTransfer(transfer) == static_cast<std::int32_t>(0x80240005), "cancelling a completed transfer did not report not found");
    sceUsbdFreeTransfer(transfer);
    sceUsbdClose(handle);
    Require(Throws([&] { sceUsbdClose(handle); }), "a handle was closed twice");
    void* second = sceUsbdOpenDeviceWithVidPid(static_cast<std::uint16_t>(vendor), static_cast<std::uint16_t>(product));
    Require(second != nullptr && sceUsbdGetDevice(second) != nullptr, "opening the device by vendor and product failed");
    sceUsbdClose(second);
    sceUsbdUnrefDevice(device);
    std::puts("USBD device tests passed");
}

}

int main(int argc, char** argv) {
    Require(sceUsbdInit() == 0, "initialization failed");
    void** list = nullptr;
    const std::int64_t count = sceUsbdGetDeviceList(&list);
    Require(count >= 0 && list != nullptr && list[count] == nullptr, "device list is not a null-terminated array of the reported length");
    for (std::int64_t index = 0; index < count; ++index) {
        std::uint8_t deviceDescriptor[18] = {};
        Require(sceUsbdGetDeviceDescriptor(list[index], deviceDescriptor) == 0 && deviceDescriptor[0] == 18 && deviceDescriptor[1] == 1, "listed device has no device descriptor");
        Require(sceUsbdRefDevice(list[index]) == list[index], "listed device was not referenced");
        sceUsbdUnrefDevice(list[index]);
    }
    sceUsbdFreeDeviceList(list, 1);
    Require(Throws([&] { sceUsbdFreeDeviceList(list, 1); }), "a device list was freed twice");
    Require(sceUsbdGetDeviceList(nullptr) == invalidArgument, "null list accepted");
    Require(sceUsbdHandleEventsTimeout(nullptr) == invalidArgument, "null timeout accepted");
    const UsbdTimeval invalid{0, 1000000};
    Require(sceUsbdHandleEventsTimeout(&invalid) == invalidArgument, "out-of-range microseconds accepted");
    const UsbdTimeval timeout{0, 50000};
    const auto start = std::chrono::steady_clock::now();
    Require(sceUsbdHandleEventsTimeout(&timeout) == 0, "event handling failed");
    Require(std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(45), "event handling returned before the timeout");
    int foreign = 0;
    void* handle = &foreign;
    Require(sceUsbdOpen(&foreign, nullptr) == invalidArgument, "null handle output accepted");
    Require(Throws([&] { sceUsbdOpen(nullptr, &handle); }), "opening a null device did not throw");
    Require(Throws([&] { sceUsbdOpen(&foreign, &handle); }), "opening a device that was never listed did not throw");
    sceUsbdClose(nullptr);
    Require(Throws([&] { sceUsbdClose(&foreign); }), "closing a handle that was never opened did not throw");
    Require(sceUsbdRefDevice(nullptr) == nullptr, "null device was referenced");
    sceUsbdUnrefDevice(nullptr);
    Require(Throws([&] { sceUsbdRefDevice(&foreign); }), "referencing a device that was never listed did not throw");
    Require(Throws([&] { sceUsbdUnrefDevice(&foreign); }), "unreferencing a device that was never listed did not throw");
    Require(sceUsbdGetBusNumber(nullptr) == 0 && sceUsbdGetDeviceAddress(nullptr) == 0, "null device reported a bus or address");
    Require(Throws([&] { sceUsbdGetBusNumber(&foreign); }), "bus number of a device that was never listed did not throw");
    std::uint8_t descriptor[18] = {};
    void* config = &foreign;
    Require(sceUsbdGetDeviceDescriptor(nullptr, descriptor) == invalidArgument, "null device descriptor accepted");
    Require(sceUsbdGetDeviceDescriptor(&foreign, nullptr) == invalidArgument, "null descriptor output accepted");
    Require(sceUsbdGetConfigDescriptor(nullptr, 0, &config) == invalidArgument, "null config device accepted");
    Require(sceUsbdGetConfigDescriptor(&foreign, 0, nullptr) == invalidArgument, "null config output accepted");
    Require(Throws([&] { sceUsbdGetConfigDescriptor(&foreign, 0, &config); }), "config of a device that was never listed did not throw");
    Require(Throws([&] { sceUsbdGetActiveConfigDescriptor(nullptr, &config); }), "active config of a null device did not throw");
    Require(sceUsbdClaimInterface(nullptr, 0) == invalidArgument && sceUsbdReleaseInterface(nullptr, 0) == invalidArgument, "null handle interface call accepted");
    Require(sceUsbdSetConfiguration(nullptr, 1) == invalidArgument && sceUsbdResetDevice(nullptr) == invalidArgument, "null handle configuration or reset accepted");
    Require(sceUsbdCheckConnected(nullptr) == invalidArgument, "null handle connection check accepted");
    Require(sceUsbdKernelDriverActive(nullptr, 0) == invalidArgument && sceUsbdAttachKernelDriver(nullptr, 0) == invalidArgument, "null handle kernel driver call accepted");
    Require(sceUsbdControlTransfer(nullptr, 0x80, 6, 0x100, 0, descriptor, 18, 1000) == invalidArgument, "null handle control transfer accepted");
    Require(sceUsbdControlTransfer(&foreign, 0x80, 6, 0x100, 0, nullptr, 18, 1000) == invalidArgument, "control transfer without data accepted");
    Require(Throws([&] { sceUsbdControlTransfer(&foreign, 0x80, 6, 0x100, 0, descriptor, 18, 1000); }), "control transfer on a handle that was never opened did not throw");
    Require(sceUsbdGetStringDescriptor(nullptr, 1, 0x409, descriptor, 18) == invalidArgument, "null handle string descriptor accepted");
    Require(sceUsbdGetStringDescriptor(&foreign, 1, 0x409, descriptor, 0) == invalidArgument, "empty string descriptor buffer accepted");
    Require(Throws([&] { sceUsbdClaimInterface(&foreign, 0); }), "claiming on a handle that was never opened did not throw");
    auto* transfer = static_cast<std::uint8_t*>(sceUsbdAllocTransfer(2));
    Require(transfer != nullptr, "transfer allocation failed");
    Require(*reinterpret_cast<std::int32_t*>(transfer + 56) == 2, "iso packet count not stored");
    Require(sceUsbdAllocTransfer(-1) == nullptr, "negative iso packet count accepted");
    std::uint8_t buffer[8] = {};
    int user = 0;
    sceUsbdFillInterruptTransfer(transfer, nullptr, 0x81, buffer, 8, reinterpret_cast<void*>(&Require), &user, 250);
    Require(transfer[9] == 0x81 && transfer[10] == 3, "endpoint or interrupt type not filled");
    Require(*reinterpret_cast<std::uint32_t*>(transfer + 12) == 250 && *reinterpret_cast<std::int32_t*>(transfer + 20) == 8, "timeout or length not filled");
    Require(*reinterpret_cast<void**>(transfer + 40) == &user && *reinterpret_cast<std::uint8_t**>(transfer + 48) == buffer, "user data or buffer not filled");
    Require(sceUsbdCancelTransfer(transfer) == static_cast<std::int32_t>(0x80240005), "cancelling an unsubmitted transfer did not report not found");
    Require(sceUsbdSubmitTransfer(nullptr) == invalidArgument, "null transfer submitted");
    Require(sceUsbdSubmitTransfer(transfer) == invalidArgument, "transfer without a handle submitted");
    sceUsbdFillInterruptTransfer(transfer, &foreign, 0x81, buffer, 8, reinterpret_cast<void*>(&Require), &user, 250);
    Require(Throws([&] { sceUsbdSubmitTransfer(transfer); }), "submitting on a handle that was never opened did not throw");
    Require(Throws([&] { sceUsbdFreeTransfer(&foreign); }), "freeing a transfer that was never allocated did not throw");
    transfer[8] = 0x02;
    Require(Throws([&] { sceUsbdFreeTransfer(transfer); }), "freeing a transfer flagged to free its buffer did not throw");
    transfer[8] = 0;
    sceUsbdFreeTransfer(transfer);
    sceUsbdFreeTransfer(nullptr);
    Require(sceUsbdEventHandlingOk() == 1, "event handling not allowed");
    Require(sceUsbdClearHalt(nullptr, 0x81) == invalidArgument && sceUsbdDetachKernelDriver(nullptr, 0) == invalidArgument, "null handle halt or detach accepted");
    Require(sceUsbdSetInterfaceAltSetting(nullptr, 0, 0) == invalidArgument, "null handle alternate setting accepted");
    std::int32_t configuration = -1;
    Require(sceUsbdGetConfiguration(nullptr, &configuration) == invalidArgument && sceUsbdGetConfiguration(&foreign, nullptr) == invalidArgument, "null configuration argument accepted");
    Require(configuration == -1, "a rejected configuration read wrote its output");
    Require(sceUsbdGetDescriptor(nullptr, 1, 0, descriptor, 18) == invalidArgument && sceUsbdGetDescriptor(&foreign, 1, 0, nullptr, 18) == invalidArgument && sceUsbdGetDescriptor(&foreign, 1, 0, descriptor, 0) == invalidArgument, "bad descriptor request accepted");
    Require(sceUsbdGetStringDescriptorAscii(nullptr, 1, descriptor, 18) == invalidArgument && sceUsbdGetStringDescriptorAscii(&foreign, 1, nullptr, 18) == invalidArgument && sceUsbdGetStringDescriptorAscii(&foreign, 1, descriptor, 0) == invalidArgument, "bad ASCII string request accepted");
    Require(sceUsbdGetConfigDescriptorByValue(nullptr, 1, &config) == invalidArgument && sceUsbdGetConfigDescriptorByValue(&foreign, 1, nullptr) == invalidArgument, "null config-by-value argument accepted");
    Require(sceUsbdGetDevice(nullptr) == nullptr && sceUsbdGetDeviceSpeed(nullptr) == 0, "a null handle or device reported a device or speed");
    Require(sceUsbdGetMaxPacketSize(nullptr, 0x81) == noDevice, "a null device packet size did not report no device");
    Require(Throws([&] { sceUsbdGetMaxIsoPacketSize(nullptr, 0x81); }), "iso packet size of a null device did not throw");
    Require(Throws([&] { sceUsbdClearHalt(&foreign, 0x81); }) && Throws([&] { sceUsbdSetInterfaceAltSetting(&foreign, 0, 0); }), "halt or alternate setting on a handle that was never opened did not throw");
    Require(Throws([&] { sceUsbdGetDevice(&foreign); }) && Throws([&] { sceUsbdGetConfiguration(&foreign, &configuration); }), "a handle that was never opened was used");
    Require(Throws([&] { sceUsbdGetDeviceSpeed(&foreign); }) && Throws([&] { sceUsbdGetMaxPacketSize(&foreign, 0x81); }), "a device that was never listed was used");
    Require(Throws([&] { sceUsbdGetConfigDescriptorByValue(&foreign, 1, &config); }), "config by value of a device that was never listed did not throw");
    Require(sceUsbdOpenDeviceWithVidPid(0, 0) == nullptr, "a device with vendor and product 0 was opened");

    Require(sceUsbdEventHandlerActive() == 0, "an event handler is active before anyone locked events");
    Require(sceUsbdTryLockEvents() == 0, "the free event lock was not taken");
    Require(sceUsbdEventHandlerActive() == 1, "the event lock holder is not the active handler");
    std::int32_t contended = -1;
    std::thread([&] { contended = sceUsbdTryLockEvents(); }).join();
    Require(contended == 1, "a second thread took the held event lock");
    Require(sceUsbdHandleEventsLocked(nullptr) == invalidArgument && sceUsbdHandleEventsLocked(&invalid) == invalidArgument, "invalid locked event timeout accepted");
    const UsbdTimeval shortWait{0, 20000};
    Require(sceUsbdHandleEventsLocked(&shortWait) == 0, "locked event handling failed");
    sceUsbdUnlockEvents();
    Require(sceUsbdEventHandlerActive() == 0, "the handler stayed active after unlocking");
    sceUsbdLockEventWaiters();
    Require(sceUsbdWaitForEvent(&invalid) == invalidArgument, "invalid wait timeout accepted");
    Require(sceUsbdWaitForEvent(&shortWait) == 1, "a wait without events did not time out");
    sceUsbdUnlockEventWaiters();
    sceUsbdLockEvents();
    Require(sceUsbdEventHandlerActive() == 1, "locking events did not make an active handler");
    std::atomic<bool> waiting{false};
    std::int32_t woken = -1;
    std::thread waiter([&] {
        sceUsbdLockEventWaiters();
        waiting.store(true, std::memory_order_release);
        woken = sceUsbdWaitForEvent(nullptr);
        sceUsbdUnlockEventWaiters();
    });
    while (!waiting.load(std::memory_order_acquire)) std::this_thread::yield();
    sceUsbdUnlockEvents();
    waiter.join();
    Require(woken == 0, "unlocking events did not wake the waiter");
    sceUsbdLockEvents();
    std::int32_t handled = -1;
    std::thread handler([&] { handled = sceUsbdHandleEvents(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    sceUsbdUnlockEvents();
    handler.join();
    Require(handled == 0, "blocking event handling behind another handler failed");
    sceUsbdFreeConfigDescriptor(nullptr);
    Require(Throws([] { sceUsbdFillControlSetup(nullptr, 0x80, 6, 0x0100, 0, 18); }), "null control setup buffer accepted");
    std::uint8_t setupBuffer[8] = {};
    sceUsbdFillControlSetup(setupBuffer, 0x80, 6, 0x0100, 0x0409, 18);
    Require(setupBuffer[0] == 0x80 && setupBuffer[1] == 6, "request type or request not filled");
    Require(setupBuffer[2] == 0x00 && setupBuffer[3] == 0x01, "value not filled little-endian");
    Require(setupBuffer[4] == 0x09 && setupBuffer[5] == 0x04, "index not filled little-endian");
    Require(setupBuffer[6] == 18 && setupBuffer[7] == 0, "length not filled little-endian");
    if (argc == 2 && std::strcmp(argv[1], "device") == 0) Device();
    sceUsbdExit();
    std::puts("USBD tests passed");
    return 0;
}
