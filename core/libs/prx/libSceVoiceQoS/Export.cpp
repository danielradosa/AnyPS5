#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::int32_t VOICE_QOS_APP_TYPE_GAME = 0x20000000;
constexpr std::int32_t VOICE_QOS_APP_TYPE_10000000 = 0x10000000;
constexpr std::uint32_t VOICE_QOS_MEMORY_SIZE = 0x40000;
constexpr int SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT = static_cast<int>(0x804E0901);
constexpr int SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID = static_cast<int>(0x804E0902);
constexpr int SCE_VOICE_ERROR_LIBVOICEQOS_LIMIT = static_cast<int>(0x804E0907);
constexpr int SCE_VOICE_ERROR_LIBVOICEQOS_INITIALIZED = static_cast<int>(0x804E0905);
constexpr std::int32_t VOICE_QOS_ATTR_MIC_MUTE = 1;
constexpr std::int32_t VOICE_QOS_ATTR_SPEAKER_MUTE = 3;
constexpr std::int32_t VOICE_QOS_ATTR_MIC_USABLE = 5;
constexpr std::int32_t VOICE_QOS_ATTR_SILENT_STATE = 6;
constexpr std::int32_t VOICE_QOS_ATTR_REMOTE_MUTE = 7;
constexpr std::int32_t VOICE_QOS_ATTR_SPEAKER_DESTINATION = 8;

using Attributes = std::map<std::int32_t, std::vector<std::uint8_t>>;

struct Endpoint {
    Attributes attributes;
};

struct Connection {
    std::int32_t local;
    std::int32_t remote;
    Attributes attributes;
};

struct QoS {
    std::mutex mutex;
    bool initialized = false;
    std::int32_t endpointLimit = 0;
    std::map<std::int32_t, Endpoint> locals;
    std::map<std::int32_t, Endpoint> remotes;
    std::map<std::int32_t, Connection> connections;
};

QoS& State() {
    static QoS qos;
    return qos;
}

int CreateEndpoint(std::int32_t* id, std::map<std::int32_t, Endpoint>& endpoints, std::int32_t limit) {
    if (!id) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    for (std::int32_t candidate = 0; candidate < limit; ++candidate) {
        if (!endpoints.contains(candidate)) {
            endpoints.emplace(candidate, Endpoint{});
            *id = candidate;
            return 0;
        }
    }
    *id = -1;
    return SCE_VOICE_ERROR_LIBVOICEQOS_LIMIT;
}

int DeleteEndpoint(std::int32_t id, std::map<std::int32_t, Endpoint>& endpoints) {
    return endpoints.erase(id) != 0 ? 0 : SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
}

[[noreturn]] void Fail(const char* function, const std::string& why) {
    throw std::logic_error(std::string(function) + ": " + why);
}

bool IsFlag(std::int32_t attribute) {
    return attribute == VOICE_QOS_ATTR_MIC_MUTE || attribute == VOICE_QOS_ATTR_SPEAKER_MUTE || attribute == VOICE_QOS_ATTR_MIC_USABLE || attribute == VOICE_QOS_ATTR_REMOTE_MUTE;
}

int CheckAttribute(std::int32_t attribute, const void* value, std::int32_t size, const char* function) {
    if (attribute < 0 || attribute > VOICE_QOS_ATTR_SPEAKER_DESTINATION) Fail(function, "attribute " + std::to_string(attribute) + " not implemented");
    if (value == nullptr || size <= 0 || (IsFlag(attribute) && size != 1)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    return 0;
}

void StoreAttribute(Attributes& attributes, std::int32_t attribute, const void* value, std::int32_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(value);
    attributes[attribute].assign(bytes, bytes + size);
}

int SetAttribute(Attributes& attributes, std::int32_t attribute, const void* value, std::int32_t size, const char* function) {
    if (const int result = CheckAttribute(attribute, value, size, function); result != 0) return result;
    if (attribute == VOICE_QOS_ATTR_MIC_USABLE || attribute == VOICE_QOS_ATTR_SILENT_STATE) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    StoreAttribute(attributes, attribute, value, size);
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

template<typename TBody>
int Locked(TBody&& body) {
    auto& qos = State();
    std::lock_guard lock(qos.mutex);
    if (!qos.initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_NOT_INIT;
    return body(qos);
}

}

extern "C" {

int APS5_VABI sceVoiceQoSInit(void* memBlock, uint32_t memSize, int32_t appType) {
    auto& qos = State();
    std::lock_guard lock(qos.mutex);
    if (qos.initialized) return SCE_VOICE_ERROR_LIBVOICEQOS_INITIALIZED;
    if (!memBlock || memSize != VOICE_QOS_MEMORY_SIZE || (appType != VOICE_QOS_APP_TYPE_GAME && appType != VOICE_QOS_APP_TYPE_10000000)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
    qos.endpointLimit = appType == VOICE_QOS_APP_TYPE_10000000 ? 15 : 7;
    qos.initialized = true;
    return 0;
}

int APS5_VABI sceVoiceQoSEnd() {
    return Locked([](QoS& qos) {
        qos.connections.clear();
        qos.locals.clear();
        qos.remotes.clear();
        qos.initialized = false;
        return 0;
    });
}

int APS5_VABI sceVoiceQoSCreateLocalEndpoint(int32_t* localId, int32_t userId, int32_t deviceInId, int32_t deviceOutId) {
    (void)deviceInId;
    (void)deviceOutId;
    return Locked([&](QoS& qos) {
        if (localId && userId == -1) {
            *localId = -1;
            return SCE_VOICE_ERROR_LIBVOICEQOS_LIMIT;
        }
        return CreateEndpoint(localId, qos.locals, 1);
    });
}

int APS5_VABI sceVoiceQoSDeleteLocalEndpoint(int32_t localId) {
    return Locked([&](QoS& qos) { return DeleteEndpoint(localId, qos.locals); });
}

int APS5_VABI sceVoiceQoSCreateRemoteEndpoint(int32_t* remoteId) {
    return Locked([&](QoS& qos) { return CreateEndpoint(remoteId, qos.remotes, qos.endpointLimit); });
}

int APS5_VABI sceVoiceQoSDeleteRemoteEndpoint(int32_t remoteId) {
    return Locked([&](QoS& qos) {
        const int result = DeleteEndpoint(remoteId, qos.remotes);
        if (result != 0) return result;
        for (auto it = qos.connections.begin(); it != qos.connections.end(); ++it) {
            if (it->second.remote == remoteId) {
                qos.connections.erase(it);
                break;
            }
        }
        return 0;
    });
}

int APS5_VABI sceVoiceQoSConnect(int32_t* connectionId, int32_t localId, int32_t remoteId) {
    return Locked([&](QoS& qos) {
        if (!connectionId || localId != 0 || remoteId < 0 || remoteId >= qos.endpointLimit) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        if (!qos.locals.contains(localId) || !qos.remotes.contains(remoteId)) throw std::runtime_error("sceVoiceQoSConnect: missing voice port error behavior is not implemented");
        for (std::int32_t candidate = 0; candidate < qos.endpointLimit; ++candidate) {
            if (!qos.connections.contains(candidate)) {
                qos.connections.emplace(candidate, Connection{localId, remoteId, {}});
                *connectionId = candidate;
                return 0;
            }
        }
        *connectionId = -1;
        return SCE_VOICE_ERROR_LIBVOICEQOS_LIMIT;
    });
}

int APS5_VABI sceVoiceQoSDisconnect(int32_t connectionId) {
    return Locked([&](QoS& qos) {
        if (connectionId < 0 || connectionId >= qos.endpointLimit) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        qos.connections.erase(connectionId);
        return 0;
    });
}

int APS5_VABI sceVoiceQoSSetConnectionAttribute(std::int32_t connectionId, std::int32_t attributeId, const void* value, std::int32_t size) {
    return Locked([&](QoS& qos) {
        const auto connection = qos.connections.find(connectionId);
        if (connection == qos.connections.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        return SetAttribute(connection->second.attributes, attributeId, value, size, __func__);
    });
}

int APS5_VABI sceVoiceQoSSetLocalEndpointAttribute(int32_t localId, int32_t attributeId, const void* value, int32_t size) {
    return Locked([&](QoS& qos) -> int {
        if (localId != 0 || !value || attributeId < 0 || attributeId > 3 || size != (attributeId % 2 == 0 ? 4 : 1)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        const auto endpoint = qos.locals.find(localId);
        if (endpoint == qos.locals.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        StoreAttribute(endpoint->second.attributes, attributeId, value, size);
        return 0;
    });
}

int APS5_VABI sceVoiceQoSSetRemoteEndpointAttribute(int32_t remoteId, int32_t attributeId, const void* value, int32_t size) {
    return Locked([&](QoS& qos) -> int {
        if (remoteId < 0 || remoteId >= qos.endpointLimit || !value || attributeId != VOICE_QOS_ATTR_REMOTE_MUTE || size != 1) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        const auto endpoint = qos.remotes.find(remoteId);
        if (endpoint == qos.remotes.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        StoreAttribute(endpoint->second.attributes, attributeId, value, size);
        return 0;
    });
}

int APS5_VABI sceVoiceQoSReadPacket(int32_t connectionId, void* buffer, uint32_t* size) {
    return Locked([&](QoS& qos) {
        if (!buffer || !size || !qos.connections.contains(connectionId)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        *size = 0;
        return 0;
    });
}

int APS5_VABI sceVoiceQoSWritePacket(int32_t connectionId, const void* buffer, uint32_t* size) {
    return Locked([&](QoS& qos) {
        if (!buffer || !size || !qos.connections.contains(connectionId)) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        return 0;
    });
}

int APS5_VABI sceVoiceQoSGetLocalEndpointAttribute(int32_t localId, int32_t attributeId, void* value, int32_t size) {
    return Locked([&](QoS& qos) {
        const auto endpoint = qos.locals.find(localId);
        if (endpoint == qos.locals.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        return GetAttribute(endpoint->second.attributes, attributeId, value, size, __func__);
    });
}

int APS5_VABI sceVoiceQoSGetRemoteEndpointAttribute(int32_t remoteId, int32_t attributeId, void* value, int32_t size) {
    return Locked([&](QoS& qos) {
        const auto endpoint = qos.remotes.find(remoteId);
        if (endpoint == qos.remotes.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        return GetAttribute(endpoint->second.attributes, attributeId, value, size, __func__);
    });
}

int APS5_VABI sceVoiceQoSGetConnectionAttribute(int32_t connectionId, int32_t attributeId, void* value, int32_t size) {
    return Locked([&](QoS& qos) {
        const auto connection = qos.connections.find(connectionId);
        if (connection == qos.connections.end()) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        return GetAttribute(connection->second.attributes, attributeId, value, size, __func__);
    });
}

int APS5_VABI sceVoiceQoSGetLocalEndpoint(int32_t connectionId, int32_t* localId) {
    return Locked([&](QoS& qos) {
        if (!localId || connectionId < 0 || connectionId >= qos.endpointLimit) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        const auto it = qos.connections.find(connectionId);
        *localId = it == qos.connections.end() ? -1 : it->second.local;
        return 0;
    });
}

int APS5_VABI sceVoiceQoSGetRemoteEndpoint(int32_t connectionId, int32_t* remoteId) {
    return Locked([&](QoS& qos) {
        if (!remoteId || connectionId < 0 || connectionId >= qos.endpointLimit) return SCE_VOICE_ERROR_LIBVOICEQOS_ARGUMENT_INVALID;
        const auto it = qos.connections.find(connectionId);
        *remoteId = it == qos.connections.end() ? -1 : it->second.remote;
        return 0;
    });
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

int APS5_VABI sceVoiceQoSDebugGetStatus(std::int32_t connectionId, std::int32_t statusId, void* value, std::int32_t size) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVoiceQoSInitHQ(void* memory, std::uint32_t memorySize, std::int32_t appType) {
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
