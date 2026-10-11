#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

constexpr std::int32_t SCE_USBD_ERROR_INVALID_ARG = static_cast<std::int32_t>(0x80240002);
constexpr std::int32_t SCE_USBD_ERROR_NOT_FOUND = static_cast<std::int32_t>(0x80240005);
constexpr std::uint8_t TRANSFER_FREE_BUFFER = 0x02;
constexpr std::uint8_t TRANSFER_TYPE_INTERRUPT = 3;

struct UsbdTimeval {
    std::int64_t seconds;
    std::int64_t microseconds;
};

struct UsbdIsoPacketDescriptor {
    std::uint32_t length;
    std::uint32_t actual_length;
    std::int32_t status;
};

struct UsbdTransfer;
using UsbdTransferCallback = void (APS5_VABI *)(UsbdTransfer* transfer);

struct UsbdTransfer {
    void* dev_handle;
    std::uint8_t flags;
    std::uint8_t endpoint;
    std::uint8_t type;
    std::uint32_t timeout;
    std::int32_t status;
    std::int32_t length;
    std::int32_t actual_length;
    UsbdTransferCallback callback;
    void* user_data;
    std::uint8_t* buffer;
    std::int32_t num_iso_packets;
    UsbdIsoPacketDescriptor iso_packet_desc[1];
};
static_assert(offsetof(UsbdTransfer, timeout) == 12 && offsetof(UsbdTransfer, callback) == 32 &&
    offsetof(UsbdTransfer, buffer) == 48 && offsetof(UsbdTransfer, num_iso_packets) == 56 && offsetof(UsbdTransfer, iso_packet_desc) == 60);

void* g_emptyDeviceList[1] = {nullptr};
std::mutex g_transfersMutex;
std::set<UsbdTransfer*> g_transfers;

[[noreturn]] void NoDevice(const char* function) {
    throw std::runtime_error(std::string(function) + ": sceUsbdGetDeviceList lists no device, so there is none to use");
}

[[noreturn]] void NoHandle(const char* function) {
    throw std::runtime_error(std::string(function) + ": no device can be opened, so there is no device handle to use");
}

}

extern "C" {

std::int32_t APS5_VABI sceUsbdInit() {
    return 0;
}

void APS5_VABI sceUsbdExit() {
}

std::int64_t APS5_VABI sceUsbdGetDeviceList(void*** list) {
    if (list == nullptr) return SCE_USBD_ERROR_INVALID_ARG;
    *list = g_emptyDeviceList;
    return 0;
}

void APS5_VABI sceUsbdFreeDeviceList(void** list, std::int32_t unrefDevices) {
    (void)unrefDevices;
    if (list != nullptr && list != g_emptyDeviceList) throw std::runtime_error(std::string(__func__) + ": list was not returned by sceUsbdGetDeviceList");
}

std::int32_t APS5_VABI sceUsbdHandleEventsTimeout(const UsbdTimeval* timeout) {
    if (timeout == nullptr || timeout->seconds < 0 || timeout->microseconds < 0 || timeout->microseconds >= 1000000) return SCE_USBD_ERROR_INVALID_ARG;
    std::this_thread::sleep_for(std::chrono::seconds(timeout->seconds) + std::chrono::microseconds(timeout->microseconds));
    return 0;
}

std::int32_t APS5_VABI sceUsbdEventHandlingOk() {
    return 1;
}

UsbdTransfer* APS5_VABI sceUsbdAllocTransfer(std::int32_t isoPackets) {
    if (isoPackets < 0) return nullptr;
    const std::size_t size = offsetof(UsbdTransfer, iso_packet_desc) + sizeof(UsbdIsoPacketDescriptor) * static_cast<std::size_t>(isoPackets);
    auto* transfer = static_cast<UsbdTransfer*>(std::calloc(1, std::max(size, sizeof(UsbdTransfer))));
    if (transfer == nullptr) return nullptr;
    transfer->num_iso_packets = isoPackets;
    std::lock_guard lock(g_transfersMutex);
    g_transfers.insert(transfer);
    return transfer;
}

void APS5_VABI sceUsbdFreeTransfer(UsbdTransfer* transfer) {
    if (transfer == nullptr) return;
    if ((transfer->flags & TRANSFER_FREE_BUFFER) != 0 && transfer->buffer != nullptr)
        throw std::runtime_error(std::string(__func__) + ": freeing the transfer buffer of a transfer is not supported");
    {
        std::lock_guard lock(g_transfersMutex);
        if (g_transfers.erase(transfer) == 0) throw std::runtime_error(std::string(__func__) + ": transfer was not allocated by sceUsbdAllocTransfer");
    }
    std::free(transfer);
}

void APS5_VABI sceUsbdFillInterruptTransfer(UsbdTransfer* transfer, void* devHandle, std::uint8_t endpoint, std::uint8_t* buffer, std::int32_t length,
    UsbdTransferCallback callback, void* userData, std::uint32_t timeout) {
    if (transfer == nullptr) throw std::invalid_argument(std::string(__func__) + ": transfer is null");
    transfer->dev_handle = devHandle;
    transfer->endpoint = endpoint;
    transfer->type = TRANSFER_TYPE_INTERRUPT;
    transfer->timeout = timeout;
    transfer->buffer = buffer;
    transfer->length = length;
    transfer->user_data = userData;
    transfer->callback = callback;
}

std::int32_t APS5_VABI sceUsbdSubmitTransfer(UsbdTransfer* transfer) {
    (void)transfer;
    NoHandle(__func__);
}

std::int32_t APS5_VABI sceUsbdCancelTransfer(UsbdTransfer* transfer) {
    if (transfer == nullptr) throw std::runtime_error(std::string(__func__) + ": null transfer");
    return SCE_USBD_ERROR_NOT_FOUND;
}

void* APS5_VABI sceUsbdRefDevice(void* device) {
    (void)device;
    NoDevice(__func__);
}

void APS5_VABI sceUsbdUnrefDevice(void* device) {
    (void)device;
    NoDevice(__func__);
}

std::uint8_t APS5_VABI sceUsbdGetBusNumber(void* device) {
    (void)device;
    NoDevice(__func__);
}

std::uint8_t APS5_VABI sceUsbdGetDeviceAddress(void* device) {
    (void)device;
    NoDevice(__func__);
}

std::int32_t APS5_VABI sceUsbdGetDeviceDescriptor(void* device, void* descriptor) {
    (void)device;
    (void)descriptor;
    NoDevice(__func__);
}

std::int32_t APS5_VABI sceUsbdGetActiveConfigDescriptor(void* device, void** config) {
    (void)device;
    (void)config;
    NoDevice(__func__);
}

std::int32_t APS5_VABI sceUsbdGetConfigDescriptor(void* device, std::uint8_t configIndex, void** config) {
    (void)device;
    (void)configIndex;
    (void)config;
    NoDevice(__func__);
}

void APS5_VABI sceUsbdFreeConfigDescriptor(void* config) {
    if (config != nullptr) NoDevice(__func__);
}

std::int32_t APS5_VABI sceUsbdOpen(void* device, void** devHandle) {
    (void)device;
    (void)devHandle;
    NoDevice(__func__);
}

void APS5_VABI sceUsbdClose(void* devHandle) {
    (void)devHandle;
    NoHandle(__func__);
}

std::int32_t APS5_VABI sceUsbdCheckConnected(void* devHandle) {
    (void)devHandle;
    NoHandle(__func__);
}

std::int32_t APS5_VABI sceUsbdSetConfiguration(void* devHandle, std::int32_t config) {
    (void)devHandle;
    (void)config;
    NoHandle(__func__);
}

std::int32_t APS5_VABI sceUsbdClaimInterface(void* devHandle, std::int32_t interfaceNumber) {
    (void)devHandle;
    (void)interfaceNumber;
    NoHandle(__func__);
}

std::int32_t APS5_VABI sceUsbdReleaseInterface(void* devHandle, std::int32_t interfaceNumber) {
    (void)devHandle;
    (void)interfaceNumber;
    NoHandle(__func__);
}

std::int32_t APS5_VABI sceUsbdResetDevice(void* devHandle) {
    (void)devHandle;
    NoHandle(__func__);
}

std::int32_t APS5_VABI sceUsbdKernelDriverActive(void* devHandle, std::int32_t interfaceNumber) {
    (void)devHandle;
    (void)interfaceNumber;
    NoHandle(__func__);
}

std::int32_t APS5_VABI sceUsbdAttachKernelDriver(void* devHandle, std::int32_t interfaceNumber) {
    (void)devHandle;
    (void)interfaceNumber;
    NoHandle(__func__);
}

std::int32_t APS5_VABI sceUsbdControlTransfer(void* devHandle, std::uint8_t requestType, std::uint8_t request, std::uint16_t value, std::uint16_t index,
    std::uint8_t* data, std::int32_t length, std::uint32_t timeout) {
    (void)devHandle;
    (void)requestType;
    (void)request;
    (void)value;
    (void)index;
    (void)data;
    (void)length;
    (void)timeout;
    NoHandle(__func__);
}

std::int32_t APS5_VABI sceUsbdGetStringDescriptor(void* devHandle, std::uint8_t descriptorIndex, std::uint16_t languageId, std::uint8_t* data, std::int32_t length) {
    (void)devHandle;
    (void)descriptorIndex;
    (void)languageId;
    (void)data;
    (void)length;
    NoHandle(__func__);
}

}
