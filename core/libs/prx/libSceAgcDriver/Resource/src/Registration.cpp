#include "prx/libSceAgcDriver/Resource/include/Registration.hpp"

#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <mutex>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

// Resource registration feeds GPU debugging tools, none of which are attached; the game treats this code as benign.
static constexpr int SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE = static_cast<int>(0x8A6C9018);
static constexpr uint32_t RESOURCE_REGISTRATION_MAX_NAME_LENGTH = 0xfc;
static constexpr int SCE_AGC_DRIVER_ERROR_INVALID_VALUE = static_cast<int>(0x8A6C0033);
static constexpr int SCE_AGC_DRIVER_ERROR_INVALID_ARGUMENT = static_cast<int>(0x8A6C0035);
static constexpr int SCE_AGC_DRIVER_ERROR_NOT_REGISTERED = static_cast<int>(0x8A6C003A);

namespace {

constexpr std::uint32_t WorkloadStreamCount = 32;
constexpr std::size_t WorkloadStreamRecordBytes = 32;

struct WorkloadStreams {
    std::mutex mutex;
    std::uint32_t registered = 0;
    std::array<std::array<unsigned char, WorkloadStreamRecordBytes>, WorkloadStreamCount> records{};
};

WorkloadStreams& workloadStreams() {
    static WorkloadStreams streams;
    return streams;
}

struct alignas(16) WorkloadSlotTable {
    std::array<std::uint64_t, WorkloadStreamCount> slots;

    WorkloadSlotTable() {
        slots.fill(~std::uint64_t{0});
        slots[0] = 0;
        slots[1] = 0;
    }
};

std::uint64_t* workloadSlots() {
    static WorkloadSlotTable table;
    return table.slots.data();
}

bool validWorkloadStream(std::uint32_t streamId) {
    return streamId >= 1 && streamId < WorkloadStreamCount;
}

}

bool AgcDriver::WorkloadSlotTableCovers(std::uint64_t address, std::size_t bytes) {
    const auto begin = reinterpret_cast<std::uint64_t>(workloadSlots());
    constexpr std::uint64_t tableBytes = WorkloadStreamCount * sizeof(std::uint64_t);
    return address >= begin && bytes <= tableBytes && address - begin <= tableBytes - bytes;
}

extern "C" std::uint64_t AgcDriverWorkloadStreamSlot_nid_no_patch(std::uint32_t streamId) {
    if (!validWorkloadStream(streamId)) return 0;
    auto& streams = workloadStreams();
    std::lock_guard lock(streams.mutex);
    if ((streams.registered & (1u << streamId)) == 0) return 0;
    return reinterpret_cast<std::uint64_t>(workloadSlots() + streamId);
}

extern "C" {

bool AgcDriverWorkloadStreamRegistered_nid_postfix(uint32_t streamId) {
    if (!validWorkloadStream(streamId)) return false;
    auto& streams = workloadStreams();
    std::lock_guard lock(streams.mutex);
    return (streams.registered & (1u << streamId)) != 0;
}

int APS5_VABI sceAgcDriverRegisterOwner(uint32_t* owner_handle, const char* name) {
    (void)owner_handle;
    (void)name;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverRegisterResource(uint32_t* resource_handle, uint32_t owner_handle, const void* memory, size_t size, const char* name, uint32_t type, uint64_t user_data) {
    (void)resource_handle;
    (void)owner_handle;
    (void)memory;
    (void)size;
    (void)name;
    (void)type;
    (void)user_data;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverRegisterMultipleResources(uint32_t* resourceHandle, uint32_t ownerHandle, const void* resources, uint32_t resourceCount) {
    (void)resourceHandle;
    (void)ownerHandle;
    (void)resources;
    (void)resourceCount;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverRegisterWorkloadStream(uint32_t stream_id, const void* stream) {
    if (!validWorkloadStream(stream_id)) return SCE_AGC_DRIVER_ERROR_INVALID_VALUE;
    auto& streams = workloadStreams();
    std::lock_guard lock(streams.mutex);
    const auto bit = 1u << stream_id;
    if ((streams.registered & bit) != 0) return SCE_AGC_DRIVER_ERROR_INVALID_VALUE;
    if (stream == nullptr) return SCE_AGC_DRIVER_ERROR_INVALID_ARGUMENT;
    std::memcpy(streams.records[stream_id].data(), stream, WorkloadStreamRecordBytes);
    streams.registered |= bit;
    return 0;
}

int APS5_VABI sceAgcDriverUnregisterOwnerAndResources(uint32_t owner_handle) {
    (void)owner_handle;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverUnregisterResource(uint32_t resource_handle) {
    (void)resource_handle;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverRegisterGdsResource(uint32_t* resource_handle, uint32_t owner_handle, uint32_t gds_offset, uint32_t gds_size, const char* name, uint32_t type, uint64_t user_data) {
    (void)resource_handle;
    (void)owner_handle;
    (void)gds_offset;
    (void)gds_size;
    (void)name;
    (void)type;
    (void)user_data;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverUnregisterAllResourcesForOwner(uint32_t owner_handle) {
    (void)owner_handle;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverUnregisterWorkloadStream(uint32_t stream_id) {
    if (!validWorkloadStream(stream_id)) return SCE_AGC_DRIVER_ERROR_INVALID_VALUE;
    auto& streams = workloadStreams();
    std::lock_guard lock(streams.mutex);
    const auto bit = 1u << stream_id;
    if ((streams.registered & bit) == 0) return SCE_AGC_DRIVER_ERROR_NOT_REGISTERED;
    streams.records[stream_id].fill(0);
    streams.registered &= ~bit;
    return 0;
}

int APS5_VABI sceAgcDriverGetDefaultOwner(uint32_t* owner_handle) {
    (void)owner_handle;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverGetResourceRegistrationMaxNameLength(uint32_t* max_length) {
    if (max_length == nullptr) APS5_INVALID_ARG_EX;
    *max_length = RESOURCE_REGISTRATION_MAX_NAME_LENGTH;
    return 0;
}

int APS5_VABI sceAgcDriverGetOwnerName(uint32_t owner_handle, const char** name) {
    (void)owner_handle;
    (void)name;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverGetResourceName(uint32_t resource_handle, const char** name) {
    (void)resource_handle;
    (void)name;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverGetResourceType(uint32_t resource_handle, uint32_t* type) {
    (void)resource_handle;
    (void)type;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverGetResourceUserData(uint32_t resource_handle, uint64_t* user_data) {
    (void)resource_handle;
    (void)user_data;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverSetResourceUserData(uint32_t resource_handle, uint64_t user_data) {
    (void)resource_handle;
    (void)user_data;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverGetResourceBaseAddressAndSizeInBytes(uint32_t resource_handle, void** memory, size_t* size) {
    (void)resource_handle;
    (void)memory;
    (void)size;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverGetResourceShaderGuid(uint32_t resource_handle, void* guid) {
    (void)resource_handle;
    (void)guid;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverFindResourcesPublic(const void* callback, void* user_data) {
    (void)callback;
    (void)user_data;
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverRegisterOwnerForSystem() {
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverInitResourceRegistrationInterface() {
    return SCE_AGC_ERROR_RESOURCE_REGISTRATION_UNAVAILABLE;
}

int APS5_VABI sceAgcDriverSetResourceRegistrationInfo() {
    return 0;
}

}
