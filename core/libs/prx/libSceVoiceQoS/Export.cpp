#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceUserService/UserService.hpp"

namespace {

constexpr std::int32_t VOICE_QOS_APP_TYPE_GAME = 0x20000000;
constexpr std::int32_t VOICE_QOS_APP_TYPE_10000000 = 0x10000000;
constexpr std::uint32_t VOICE_QOS_MEMORY_SIZE = 0x40000;
constexpr int SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT = static_cast<int>(0x804E0901);
constexpr int SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID = static_cast<int>(0x804E0902);
constexpr int SCE_VOICE_ERROR_LIBVOICEQOS_INITIALIZED = static_cast<int>(0x804E0905);
constexpr int SCE_VOICE_ERROR_LIBVOICEQOS_RESOURCE_USED = static_cast<int>(0x804E0907);
constexpr std::int32_t VOICE_QOS_ATTR_MIC_MUTE = 1;
constexpr std::int32_t VOICE_QOS_ATTR_SPEAKER_MUTE = 3;
constexpr std::int32_t VOICE_QOS_ATTR_MIC_USABLE = 5;
constexpr std::int32_t VOICE_QOS_ATTR_SILENT_STATE = 6;
constexpr std::int32_t VOICE_QOS_ATTR_REMOTE_MUTE = 7;
constexpr std::int32_t VOICE_QOS_ATTR_SPEAKER_DESTINATION = 8;

using Attributes = std::map<std::int32_t, std::vector<std::uint8_t>>;

struct Connection {
    std::int32_t localId;
    std::int32_t remoteId;
    Attributes attributes;
};

std::mutex g_mutex;
bool g_initialized = false;
std::map<std::int32_t, Attributes> g_localEndpoints;
std::map<std::int32_t, Attributes> g_remoteEndpoints;
std::map<std::int32_t, Connection> g_connections;

[[noreturn]] void Fail(const char* function, const std::string& why) {
    throw std::logic_error(std::string(function) + ": " + why);
}

template <typename TValue>
std::int32_t FreeId(const std::map<std::int32_t, TValue>& table) {
    std::int32_t id = 0;
    for (const auto& entry : table) {
        if (entry.first != id) break;
        ++id;
    }
    return id;
}

bool IsFlag(std::int32_t attribute) {
    return attribute == VOICE_QOS_ATTR_MIC_MUTE || attribute == VOICE_QOS_ATTR_SPEAKER_MUTE || attribute == VOICE_QOS_ATTR_MIC_USABLE || attribute == VOICE_QOS_ATTR_REMOTE_MUTE;
}

int CheckAttribute(std::int32_t attribute, const void* value, std::int32_t size, const char* function) {
    if (attribute < 0 || attribute > VOICE_QOS_ATTR_SPEAKER_DESTINATION) Fail(function, "attribute " + std::to_string(attribute) + " not implemented");
    if (value == nullptr || size <= 0 || (IsFlag(attribute) && size != 1)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    return 0;
}

int SetAttribute(Attributes& attributes, std::int32_t attribute, const void* value, std::int32_t size, const char* function) {
    if (const int result = CheckAttribute(attribute, value, size, function); result != 0) return result;
    if (attribute == VOICE_QOS_ATTR_MIC_USABLE || attribute == VOICE_QOS_ATTR_SILENT_STATE) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    const auto* bytes = static_cast<const std::uint8_t*>(value);
    attributes[attribute].assign(bytes, bytes + size);
    return 0;
}

int GetAttribute(const Attributes& attributes, std::int32_t attribute, void* value, std::int32_t size, const char* function) {
    if (const int result = CheckAttribute(attribute, value, size, function); result != 0) return result;
    const auto stored = attributes.find(attribute);
    if (stored == attributes.end()) {
        if (!IsFlag(attribute)) Fail(function, "attribute " + std::to_string(attribute) + " was not set and its default is unknown");
        *static_cast<std::uint8_t*>(value) = 0;
        return 0;
    }
    if (stored->second.size() != static_cast<std::size_t>(size)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    std::memcpy(value, stored->second.data(), stored->second.size());
    return 0;
}

bool EndpointInUse(std::int32_t Connection::* side, std::int32_t id) {
    return std::any_of(g_connections.begin(), g_connections.end(), [side, id](const auto& entry) { return entry.second.*side == id; });
}

}

extern "C" {

int APS5_VABI sceVoiceQoSInit(void* mem_block, uint32_t mem_size, int32_t app_type) {
    std::lock_guard lock(g_mutex);
    if (g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_INITIALIZED;
    if (!mem_block || mem_size != VOICE_QOS_MEMORY_SIZE || (app_type != VOICE_QOS_APP_TYPE_GAME && app_type != VOICE_QOS_APP_TYPE_10000000)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    g_initialized = true;
    return 0;
}

int APS5_VABI sceVoiceQoSEnd() {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    g_connections.clear();
    g_localEndpoints.clear();
    g_remoteEndpoints.clear();
    g_initialized = false;
    return 0;
}

int APS5_VABI sceVoiceQoSCreateLocalEndpoint(int32_t* localId, int32_t userId, int32_t deviceInId, int32_t deviceOutId) {
    (void)deviceInId;
    (void)deviceOutId;
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    if (localId == nullptr || userId != USER_SERVICE_INITIAL_USER_ID) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    const auto id = FreeId(g_localEndpoints);
    g_localEndpoints.emplace(id, Attributes{});
    *localId = id;
    return 0;
}

int APS5_VABI sceVoiceQoSDeleteLocalEndpoint(int32_t localId) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    if (!g_localEndpoints.contains(localId)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    if (EndpointInUse(&Connection::localId, localId)) return SCE_VOICE_ERROR_LIBVOICEQOS_RESOURCE_USED;
    g_localEndpoints.erase(localId);
    return 0;
}

int APS5_VABI sceVoiceQoSCreateRemoteEndpoint(int32_t* remoteId) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    if (remoteId == nullptr) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    const auto id = FreeId(g_remoteEndpoints);
    g_remoteEndpoints.emplace(id, Attributes{});
    *remoteId = id;
    return 0;
}

int APS5_VABI sceVoiceQoSDeleteRemoteEndpoint(int32_t remoteId) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    if (!g_remoteEndpoints.contains(remoteId)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    if (EndpointInUse(&Connection::remoteId, remoteId)) return SCE_VOICE_ERROR_LIBVOICEQOS_RESOURCE_USED;
    g_remoteEndpoints.erase(remoteId);
    return 0;
}

int APS5_VABI sceVoiceQoSConnect(int32_t* connectionId, int32_t localId, int32_t remoteId) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    if (connectionId == nullptr || !g_localEndpoints.contains(localId) || !g_remoteEndpoints.contains(remoteId)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    const auto id = FreeId(g_connections);
    g_connections.emplace(id, Connection{localId, remoteId, {}});
    *connectionId = id;
    return 0;
}

int APS5_VABI sceVoiceQoSDisconnect(int32_t connectionId) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    if (g_connections.erase(connectionId) == 0) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    return 0;
}

int APS5_VABI sceVoiceQoSSetConnectionAttribute(int32_t connectionId, int32_t attributeId, const void* attributeValue, int32_t attributeSize) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    const auto connection = g_connections.find(connectionId);
    if (connection == g_connections.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    return SetAttribute(connection->second.attributes, attributeId, attributeValue, attributeSize, __func__);
}

int APS5_VABI sceVoiceQoSSetLocalEndpointAttribute(int32_t localId, int32_t attributeId, const void* attributeValue, int32_t attributeSize) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    const auto endpoint = g_localEndpoints.find(localId);
    if (endpoint == g_localEndpoints.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    return SetAttribute(endpoint->second, attributeId, attributeValue, attributeSize, __func__);
}

int APS5_VABI sceVoiceQoSSetRemoteEndpointAttribute(int32_t remoteId, int32_t attributeId, const void* attributeValue, int32_t attributeSize) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    const auto endpoint = g_remoteEndpoints.find(remoteId);
    if (endpoint == g_remoteEndpoints.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    return SetAttribute(endpoint->second, attributeId, attributeValue, attributeSize, __func__);
}

int APS5_VABI sceVoiceQoSReadPacket(int32_t connectionId, void* data, uint32_t* size) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    if (data == nullptr || size == nullptr || !g_connections.contains(connectionId)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    *size = 0;
    return 0;
}

int APS5_VABI sceVoiceQoSWritePacket(int32_t connectionId, const void* data, uint32_t* size) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    if (data == nullptr || size == nullptr || !g_connections.contains(connectionId)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    return 0;
}

int APS5_VABI sceVoiceQoSGetLocalEndpointAttribute(int32_t localId, int32_t attributeId, void* attributeValue, int32_t attributeSize) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    const auto endpoint = g_localEndpoints.find(localId);
    if (endpoint == g_localEndpoints.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    return GetAttribute(endpoint->second, attributeId, attributeValue, attributeSize, __func__);
}

int APS5_VABI sceVoiceQoSGetRemoteEndpointAttribute(int32_t remoteId, int32_t attributeId, void* attributeValue, int32_t attributeSize) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    const auto endpoint = g_remoteEndpoints.find(remoteId);
    if (endpoint == g_remoteEndpoints.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    return GetAttribute(endpoint->second, attributeId, attributeValue, attributeSize, __func__);
}

int APS5_VABI sceVoiceQoSGetConnectionAttribute(int32_t connectionId, int32_t attributeId, void* attributeValue, int32_t attributeSize) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    const auto connection = g_connections.find(connectionId);
    if (connection == g_connections.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    return GetAttribute(connection->second.attributes, attributeId, attributeValue, attributeSize, __func__);
}

int APS5_VABI sceVoiceQoSGetLocalEndpoint(int32_t connectionId, int32_t* localId) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    if (localId == nullptr) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    const auto connection = g_connections.find(connectionId);
    if (connection == g_connections.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    *localId = connection->second.localId;
    return 0;
}

int APS5_VABI sceVoiceQoSGetRemoteEndpoint(int32_t connectionId, int32_t* remoteId) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    if (remoteId == nullptr) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    const auto connection = g_connections.find(connectionId);
    if (connection == g_connections.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    *remoteId = connection->second.remoteId;
    return 0;
}

int APS5_VABI sceVoiceQoSEnableChat() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVoiceQoSDisableChat() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVoiceQoSGetStatus(int32_t connectionId, int32_t statusId, void* value, int32_t size) {
    (void)connectionId;
    (void)statusId;
    (void)value;
    (void)size;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVoiceQoSDebugGetStatus() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVoiceQoSInitHQ() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVoiceQoSSetMode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVoiceQoSTerminate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
