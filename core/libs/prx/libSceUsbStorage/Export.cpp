#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

struct UsbStorageEvent;
struct UsbStorageDeviceInfo;
using UsbStorageCallback = void (APS5_VABI *)(const UsbStorageEvent* event, void* userData);

namespace {

constexpr std::int32_t SCE_USB_STORAGE_ERROR_INVAL = static_cast<std::int32_t>(0x80F40002);
constexpr std::int32_t SCE_USB_STORAGE_ERROR_NOT_INITIALIZED = static_cast<std::int32_t>(0x80F40003);
constexpr std::int32_t SCE_USB_STORAGE_ERROR_NOENT = static_cast<std::int32_t>(0x80F40004);
constexpr std::int32_t SCE_USB_STORAGE_ERROR_NOTSUP = static_cast<std::int32_t>(0x80F40005);

struct SceUsbStorageDirent {
    std::uint32_t d_fileno;
    std::uint16_t d_reclen;
    std::uint8_t d_type;
    std::uint8_t d_namlen;
    char d_name[256];
};

struct UsbDirentItem {
    std::string name;
    bool is_directory;
};

struct UsbGetdentsState {
    std::vector<UsbDirentItem> items;
    std::size_t current_index = 0;
};

std::atomic<bool> g_initialized{false};
std::mutex g_getdentsMutex;
std::unordered_map<std::int32_t, UsbGetdentsState> g_getdentsTable;
std::int32_t g_nextFd = 1;

}

extern "C" {

int APS5_VABI sceUsbStorageInit(void* attr) {
    (void)attr;
    g_initialized.store(true);
    return 0;
}

int APS5_VABI sceUsbStorageTerm(void) {
    g_initialized.store(false);
    std::lock_guard lock(g_getdentsMutex);
    g_getdentsTable.clear();
    return 0;
}

int APS5_VABI sceUsbStorageGetDeviceList(uint32_t* devices, int32_t* count) {
    if (!devices || !count) return SCE_USB_STORAGE_ERROR_INVAL;
    if (!g_initialized.load()) return SCE_USB_STORAGE_ERROR_NOT_INITIALIZED;
    *count = 0;
    return 0;
}

int APS5_VABI sceUsbStorageGetDeviceInfo(uint32_t device, UsbStorageDeviceInfo* info) {
    (void)device;
    if (!info) return SCE_USB_STORAGE_ERROR_INVAL;
    if (!g_initialized.load()) return SCE_USB_STORAGE_ERROR_NOT_INITIALIZED;
    return SCE_USB_STORAGE_ERROR_NOENT;
}

bool APS5_VABI sceUsbStorageIsExist(uint32_t device, const char* directory, std::uint8_t* exists) {
    (void)device;
    (void)directory;
    (void)exists;
    return 0;
}

int APS5_VABI sceUsbStorageGetMountPointOfShellCore(uint32_t device, char* mountPoint) {
    if (!mountPoint) return SCE_USB_STORAGE_ERROR_INVAL;
    if (device != 0) return SCE_USB_STORAGE_ERROR_NOENT;
    const char default_mount[] = "/mnt/usb0";
    std::memcpy(mountPoint, default_mount, sizeof(default_mount));
    return 0;
}

int APS5_VABI sceUsbStorageGetdentsOpen(uint32_t device, int32_t* fd, uint64_t* bufferSize) {
    (void)device;
    if (!fd || !bufferSize) return SCE_USB_STORAGE_ERROR_INVAL;
    if (!g_initialized.load()) return SCE_USB_STORAGE_ERROR_NOT_INITIALIZED;
    return SCE_USB_STORAGE_ERROR_NOENT;
}

int APS5_VABI sceUsbStorageGetdentsRead(int32_t fd, void* buffer, uint64_t bufferSize, uint64_t* bytesRead) {
    if (!buffer || !bytesRead) return SCE_USB_STORAGE_ERROR_INVAL;
    if (!g_initialized.load()) return SCE_USB_STORAGE_ERROR_NOT_INITIALIZED;
    std::lock_guard lock(g_getdentsMutex);
    auto it = g_getdentsTable.find(fd);
    if (it == g_getdentsTable.end()) {
        *bytesRead = 0;
        return SCE_USB_STORAGE_ERROR_INVAL;
    }
    auto& state = it->second;
    uint64_t bytes_written = 0;
    while (state.current_index < state.items.size()) {
        const auto& item = state.items[state.current_index];
        const uint16_t namlen = static_cast<uint16_t>(std::min<size_t>(item.name.size(), 255));
        const uint16_t reclen = static_cast<uint16_t>((8 + namlen + 1 + 7) & ~7);
        if (bytes_written + reclen > bufferSize) break;
        auto* entry = reinterpret_cast<SceUsbStorageDirent*>(static_cast<uint8_t*>(buffer) + bytes_written);
        std::memset(entry, 0, reclen);
        entry->d_fileno = static_cast<uint32_t>(state.current_index + 1);
        entry->d_reclen = reclen;
        entry->d_type = item.is_directory ? 4 : 8;
        entry->d_namlen = static_cast<uint8_t>(namlen);
        std::memcpy(entry->d_name, item.name.data(), namlen);
        entry->d_name[namlen] = '\0';
        bytes_written += reclen;
        state.current_index++;
    }
    *bytesRead = bytes_written;
    return 0;
}

int APS5_VABI sceUsbStorageGetdentsClose(int32_t fd) {
    std::lock_guard lock(g_getdentsMutex);
    g_getdentsTable.erase(fd);
    return 0;
}

int APS5_VABI sceUsbStorageRegisterCallback(int32_t eventType, UsbStorageCallback callback, void* arg) {
    (void)arg;
    if (!callback) return SCE_USB_STORAGE_ERROR_INVAL;
    if (eventType != 1 && eventType != 2) return SCE_USB_STORAGE_ERROR_INVAL;
    return 0;
}

int APS5_VABI sceUsbStorageUnregisterCallback(int32_t eventType, UsbStorageCallback callback) {
    if (!callback) return SCE_USB_STORAGE_ERROR_INVAL;
    if (eventType != 1 && eventType != 2) return SCE_USB_STORAGE_ERROR_INVAL;
    return 0;
}

int APS5_VABI sceUsbStorageRegisterCallbackForMapAvailable(uint32_t device, const char* directory, UsbStorageCallback callback, void* arg) {
    (void)device;
    (void)arg;
    if (!directory || !callback) return SCE_USB_STORAGE_ERROR_INVAL;
    return 0;
}

int APS5_VABI sceUsbStorageUnregisterCallbackForMapAvailable(uint32_t device, const char* directory, UsbStorageCallback callback) {
    (void)device;
    if (!directory || !callback) return SCE_USB_STORAGE_ERROR_INVAL;
    return 0;
}

int APS5_VABI sceUsbStorageRegisterCallbackForMapUnavailable(uint32_t device, const char* directory, UsbStorageCallback callback, void* arg) {
    (void)device;
    (void)arg;
    if (!directory || !callback) return SCE_USB_STORAGE_ERROR_INVAL;
    return 0;
}

int APS5_VABI sceUsbStorageUnregisterCallbackForMapUnavailable(uint32_t device, const char* directory, UsbStorageCallback callback) {
    (void)device;
    if (!directory || !callback) return SCE_USB_STORAGE_ERROR_INVAL;
    return 0;
}

int APS5_VABI sceUsbStorageRequestMap(uint32_t device, const char* directory, int32_t flags, uint64_t volumeSize, char* mountPoint, uint64_t* mountId) {
    (void)flags;
    (void)volumeSize;
    if (device != 0) return SCE_USB_STORAGE_ERROR_NOENT;
    std::string mount_path = "/mnt/usb" + std::to_string(device);
    if (directory && directory[0] != '\0') {
        std::string_view sp(directory);
        if (sp.starts_with("/mnt/usb")) {
            mount_path = std::string(sp);
        } else if (sp.starts_with("/")) {
            mount_path += std::string(sp);
        } else {
            mount_path += "/" + std::string(sp);
        }
    }
    if (mountPoint) {
        std::strncpy(mountPoint, mount_path.c_str(), 127);
        mountPoint[127] = '\0';
    }
    if (mountId) *mountId = 0;
    return 0;
}

int APS5_VABI sceUsbStorageRequestMapWSB(uint32_t device, const char* directory, int32_t flags, uint64_t volumeSize, char* mountPoint, uint64_t* mountId) {
    return sceUsbStorageRequestMap(device, directory, flags, volumeSize, mountPoint, mountId);
}

int APS5_VABI sceUsbStorageRequestUnmap(uint32_t device, const char* directory) {
    (void)device;
    if (!directory) return SCE_USB_STORAGE_ERROR_INVAL;
    return 0;
}

int APS5_VABI sceUsbStorageClearFakeWriteProtection(uint32_t device_id) {
    (void)device_id;
    return 0;
}

int APS5_VABI sceUsbStorageSetFakeWriteProtection(uint32_t device_id, uint32_t enable) {
    (void)device_id;
    (void)enable;
    return 0;
}

int APS5_VABI sceUsbStorageSetFakeMapLockForDebug(uint32_t device_id, uint32_t lock) {
    (void)device_id;
    (void)lock;
    return 0;
}

}
