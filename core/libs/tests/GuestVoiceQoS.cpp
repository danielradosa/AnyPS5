#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceUserService/UserService.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <vector>

extern "C" {
int APS5_VABI sceVoiceQoSInit(void*, std::uint32_t, std::int32_t);
int APS5_VABI sceVoiceQoSEnd();
int APS5_VABI sceVoiceQoSCreateLocalEndpoint(std::int32_t*, std::int32_t, std::int32_t, std::int32_t);
int APS5_VABI sceVoiceQoSDeleteLocalEndpoint(std::int32_t);
int APS5_VABI sceVoiceQoSCreateRemoteEndpoint(std::int32_t*);
int APS5_VABI sceVoiceQoSDeleteRemoteEndpoint(std::int32_t);
int APS5_VABI sceVoiceQoSConnect(std::int32_t*, std::int32_t, std::int32_t);
int APS5_VABI sceVoiceQoSDisconnect(std::int32_t);
int APS5_VABI sceVoiceQoSSetConnectionAttribute(std::int32_t, std::int32_t, const void*, std::int32_t);
int APS5_VABI sceVoiceQoSSetLocalEndpointAttribute(std::int32_t, std::int32_t, const void*, std::int32_t);
int APS5_VABI sceVoiceQoSSetRemoteEndpointAttribute(std::int32_t, std::int32_t, const void*, std::int32_t);
int APS5_VABI sceVoiceQoSGetLocalEndpointAttribute(std::int32_t, std::int32_t, void*, std::int32_t);
int APS5_VABI sceVoiceQoSGetRemoteEndpointAttribute(std::int32_t, std::int32_t, const void*, std::int32_t);
int APS5_VABI sceVoiceQoSGetConnectionAttribute(std::int32_t, std::int32_t, void*, std::int32_t);
int APS5_VABI sceVoiceQoSGetLocalEndpoint(std::int32_t, std::int32_t*);
int APS5_VABI sceVoiceQoSGetRemoteEndpoint(std::int32_t, std::int32_t*);
int APS5_VABI sceVoiceQoSEnableChat();
int APS5_VABI sceVoiceQoSDisableChat();
int APS5_VABI sceVoiceQoSGetStatus(std::int32_t, std::int32_t, void*, std::int32_t);
int APS5_VABI sceVoiceQoSDebugGetStatus();
int APS5_VABI sceVoiceQoSInitHQ();
int APS5_VABI sceVoiceQoSSetMode();
int APS5_VABI sceVoiceQoSTerminate();
int APS5_VABI sceVoiceQoSReadPacket(std::int32_t, void*, std::uint32_t*);
int APS5_VABI sceVoiceQoSWritePacket(std::int32_t, const void*, std::uint32_t*);
}

static void Require(bool value) { if (!value) std::abort(); }

template <typename TAction>
static void RequireThrows(TAction action) {
    try {
        action();
    } catch (const std::exception&) {
        return;
    }
    std::abort();
}

constexpr int argumentInvalid = static_cast<int>(0x804E0902);
constexpr int initialized = static_cast<int>(0x804E0905);
constexpr int notInitialized = static_cast<int>(0x804E0901);
constexpr int resourceUsed = static_cast<int>(0x804E0907);
constexpr std::int32_t micVolume = 0;
constexpr std::int32_t micMute = 1;
constexpr std::int32_t speakerVolume = 2;
constexpr std::int32_t speakerMute = 3;
constexpr std::int32_t desiredOutBitRate = 4;
constexpr std::int32_t micUsable = 5;
constexpr std::int32_t silentState = 6;
constexpr std::int32_t remoteMute = 7;

static std::uint8_t ReadFlag(std::int32_t localId, std::int32_t attribute) {
    std::uint8_t flag = 0xAA;
    Require(sceVoiceQoSGetLocalEndpointAttribute(localId, attribute, &flag, 1) == 0);
    return flag;
}

static void RequireNotInitialized() {
    std::int32_t id = -1;
    std::uint8_t flag = 0;
    std::uint8_t packet[16] = {};
    std::uint32_t size = sizeof(packet);
    Require(sceVoiceQoSCreateLocalEndpoint(&id, USER_SERVICE_INITIAL_USER_ID, 0, 0) == notInitialized);
    Require(sceVoiceQoSCreateRemoteEndpoint(&id) == notInitialized);
    Require(sceVoiceQoSGetLocalEndpoint(0, &id) == notInitialized);
    Require(sceVoiceQoSGetRemoteEndpoint(0, &id) == notInitialized);
    Require(id == -1);
    Require(sceVoiceQoSDeleteLocalEndpoint(0) == notInitialized);
    Require(sceVoiceQoSDeleteRemoteEndpoint(0) == notInitialized);
    Require(sceVoiceQoSConnect(&id, 0, 0) == notInitialized);
    Require(sceVoiceQoSDisconnect(0) == notInitialized);
    Require(sceVoiceQoSSetLocalEndpointAttribute(0, micMute, &flag, 1) == notInitialized);
    Require(sceVoiceQoSGetLocalEndpointAttribute(0, micMute, &flag, 1) == notInitialized);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(0, remoteMute, &flag, 1) == notInitialized);
    Require(sceVoiceQoSSetConnectionAttribute(0, desiredOutBitRate, &size, sizeof(size)) == notInitialized);
    Require(sceVoiceQoSReadPacket(0, packet, &size) == notInitialized);
    Require(sceVoiceQoSWritePacket(0, packet, &size) == notInitialized);
    Require(size == sizeof(packet));
}

static void RequireLocalAttributes(std::int32_t localId, std::int32_t otherLocalId) {
    const std::uint8_t on = 1;
    const std::uint8_t off = 0;
    std::uint32_t wide = 1;
    Require(ReadFlag(localId, micMute) == 0);
    Require(ReadFlag(localId, speakerMute) == 0);
    Require(ReadFlag(localId, micUsable) == 0);
    Require(sceVoiceQoSSetLocalEndpointAttribute(localId, micMute, &on, 1) == 0);
    Require(ReadFlag(localId, micMute) == 1);
    Require(ReadFlag(otherLocalId, micMute) == 0);
    Require(sceVoiceQoSSetLocalEndpointAttribute(localId, micMute, &off, 1) == 0);
    Require(ReadFlag(localId, micMute) == 0);
    Require(sceVoiceQoSSetLocalEndpointAttribute(localId, micMute, &on, 1) == 0);
    Require(sceVoiceQoSSetLocalEndpointAttribute(localId, micMute, &wide, sizeof(wide)) == argumentInvalid);
    Require(sceVoiceQoSSetLocalEndpointAttribute(localId, micMute, nullptr, 1) == argumentInvalid);
    Require(sceVoiceQoSSetLocalEndpointAttribute(localId, micMute, &off, 0) == argumentInvalid);
    Require(sceVoiceQoSSetLocalEndpointAttribute(localId, micMute, &off, -1) == argumentInvalid);
    Require(ReadFlag(localId, micMute) == 1);
    Require(sceVoiceQoSSetLocalEndpointAttribute(localId, micUsable, &on, 1) == argumentInvalid);
    Require(ReadFlag(localId, micUsable) == 0);
    Require(sceVoiceQoSSetLocalEndpointAttribute(localId, silentState, &wide, sizeof(wide)) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpointAttribute(localId, micMute, &wide, sizeof(wide)) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpointAttribute(localId, micMute, nullptr, 1) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpointAttribute(0x7FFF, micMute, &wide, 1) == argumentInvalid);
    Require(sceVoiceQoSSetLocalEndpointAttribute(0x7FFF, micMute, &on, 1) == argumentInvalid);

    const float volume = 0.25f;
    Require(sceVoiceQoSSetLocalEndpointAttribute(localId, micVolume, &volume, sizeof(volume)) == 0);
    float readVolume = 0.0f;
    Require(sceVoiceQoSGetLocalEndpointAttribute(localId, micVolume, &readVolume, sizeof(readVolume)) == 0);
    Require(std::memcmp(&readVolume, &volume, sizeof(volume)) == 0);
    std::uint16_t narrow = 0xAAAA;
    Require(sceVoiceQoSGetLocalEndpointAttribute(localId, micVolume, &narrow, sizeof(narrow)) == argumentInvalid);
    Require(narrow == 0xAAAA);
    RequireThrows([&] { sceVoiceQoSGetLocalEndpointAttribute(localId, speakerVolume, &readVolume, sizeof(readVolume)); });
    RequireThrows([&] { sceVoiceQoSGetLocalEndpointAttribute(localId, silentState, &wide, sizeof(wide)); });
    RequireThrows([&] { sceVoiceQoSGetLocalEndpointAttribute(otherLocalId, micVolume, &readVolume, sizeof(readVolume)); });
    RequireThrows([&] { sceVoiceQoSSetLocalEndpointAttribute(localId, 9, &wide, sizeof(wide)); });
    RequireThrows([&] { sceVoiceQoSSetLocalEndpointAttribute(localId, -1, &wide, sizeof(wide)); });
}

static void RequirePackets(std::int32_t connectionId) {
    std::vector<std::uint8_t> packet(0x2000, 0xAA);
    std::uint32_t size = static_cast<std::uint32_t>(packet.size());
    Require(sceVoiceQoSReadPacket(connectionId, packet.data(), &size) == 0);
    Require(size == 0);
    for (std::uint8_t byte : packet) Require(byte == 0xAA);
    size = 0x2000;
    Require(sceVoiceQoSReadPacket(connectionId, nullptr, &size) == argumentInvalid);
    Require(sceVoiceQoSReadPacket(connectionId, packet.data(), nullptr) == argumentInvalid);
    Require(sceVoiceQoSReadPacket(0x7FFF, packet.data(), &size) == argumentInvalid);
    Require(size == 0x2000);

    size = 0;
    Require(sceVoiceQoSWritePacket(connectionId, packet.data(), &size) == 0);
    Require(size == 0);
    size = 1;
    Require(sceVoiceQoSWritePacket(connectionId, nullptr, &size) == argumentInvalid);
    Require(sceVoiceQoSWritePacket(connectionId, packet.data(), nullptr) == argumentInvalid);
    Require(sceVoiceQoSWritePacket(0x7FFF, packet.data(), &size) == argumentInvalid);
    size = 100;
    Require(sceVoiceQoSWritePacket(connectionId, packet.data(), &size) == 0);
    Require(size == 100);
}

static void RequireEndpoints(std::int32_t& keptLocalId, std::int32_t& keptConnectionId) {
    std::int32_t localId = -1;
    Require(sceVoiceQoSCreateLocalEndpoint(nullptr, USER_SERVICE_INITIAL_USER_ID, 0, 0) == argumentInvalid);
    Require(sceVoiceQoSCreateLocalEndpoint(&localId, USER_SERVICE_USER_ID_INVALID, 0, 0) == argumentInvalid);
    Require(sceVoiceQoSCreateLocalEndpoint(&localId, USER_SERVICE_INITIAL_USER_ID + 1, 0, 0) == argumentInvalid);
    Require(localId == -1);
    Require(sceVoiceQoSCreateLocalEndpoint(&localId, USER_SERVICE_INITIAL_USER_ID, 0, 0) == 0);
    Require(localId >= 0);
    std::int32_t otherLocalId = -1;
    Require(sceVoiceQoSCreateLocalEndpoint(&otherLocalId, USER_SERVICE_INITIAL_USER_ID, 0, 0) == 0);
    Require(otherLocalId >= 0 && otherLocalId != localId);

    std::int32_t remoteId = -1;
    Require(sceVoiceQoSCreateRemoteEndpoint(nullptr) == argumentInvalid);
    Require(sceVoiceQoSCreateRemoteEndpoint(&remoteId) == 0);
    Require(remoteId >= 0);
    std::int32_t otherRemoteId = -1;
    Require(sceVoiceQoSCreateRemoteEndpoint(&otherRemoteId) == 0);
    Require(otherRemoteId >= 0 && otherRemoteId != remoteId);

    std::int32_t connectionId = -1;
    Require(sceVoiceQoSConnect(nullptr, localId, remoteId) == argumentInvalid);
    Require(sceVoiceQoSConnect(&connectionId, 0x7FFF, remoteId) == argumentInvalid);
    Require(sceVoiceQoSConnect(&connectionId, localId, 0x7FFF) == argumentInvalid);
    Require(connectionId == -1);
    Require(sceVoiceQoSConnect(&connectionId, localId, remoteId) == 0);
    Require(connectionId >= 0);
    std::int32_t otherConnectionId = -1;
    Require(sceVoiceQoSConnect(&otherConnectionId, otherLocalId, otherRemoteId) == 0);
    Require(otherConnectionId >= 0 && otherConnectionId != connectionId);
    std::int32_t queried = -1;
    Require(sceVoiceQoSGetLocalEndpoint(connectionId, nullptr) == argumentInvalid);
    Require(sceVoiceQoSGetRemoteEndpoint(connectionId, nullptr) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpoint(0x7FFF, &queried) == argumentInvalid);
    Require(sceVoiceQoSGetRemoteEndpoint(0x7FFF, &queried) == argumentInvalid);
    Require(queried == -1);
    Require(sceVoiceQoSGetLocalEndpoint(otherConnectionId, &queried) == 0 && queried == otherLocalId);
    Require(sceVoiceQoSGetRemoteEndpoint(otherConnectionId, &queried) == 0 && queried == otherRemoteId);

    RequireLocalAttributes(localId, otherLocalId);

    const std::uint8_t on = 1;
    std::uint32_t bitRate = 16000;
    Require(sceVoiceQoSSetRemoteEndpointAttribute(remoteId, remoteMute, &on, 1) == 0);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(remoteId, remoteMute, &bitRate, sizeof(bitRate)) == argumentInvalid);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(remoteId, micUsable, &on, 1) == argumentInvalid);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(remoteId, remoteMute, nullptr, 1) == argumentInvalid);
    Require(sceVoiceQoSSetRemoteEndpointAttribute(0x7FFF, remoteMute, &on, 1) == argumentInvalid);
    Require(sceVoiceQoSSetConnectionAttribute(connectionId, desiredOutBitRate, &bitRate, sizeof(bitRate)) == 0);
    Require(sceVoiceQoSSetConnectionAttribute(connectionId, desiredOutBitRate, &bitRate, 0) == argumentInvalid);
    Require(sceVoiceQoSSetConnectionAttribute(connectionId, silentState, &bitRate, sizeof(bitRate)) == argumentInvalid);
    Require(sceVoiceQoSSetConnectionAttribute(0x7FFF, desiredOutBitRate, &bitRate, sizeof(bitRate)) == argumentInvalid);
    RequireThrows([&] { sceVoiceQoSSetConnectionAttribute(connectionId, 9, &bitRate, sizeof(bitRate)); });

    RequirePackets(connectionId);

    Require(sceVoiceQoSDeleteLocalEndpoint(localId) == resourceUsed);
    Require(sceVoiceQoSDeleteRemoteEndpoint(remoteId) == resourceUsed);
    Require(sceVoiceQoSDisconnect(connectionId) == 0);
    Require(sceVoiceQoSDisconnect(connectionId) == argumentInvalid);
    Require(sceVoiceQoSGetLocalEndpoint(connectionId, &queried) == argumentInvalid);
    Require(sceVoiceQoSGetRemoteEndpoint(connectionId, &queried) == argumentInvalid);
    std::uint32_t size = 16;
    std::uint8_t packet[16] = {};
    Require(sceVoiceQoSReadPacket(connectionId, packet, &size) == argumentInvalid);
    Require(sceVoiceQoSDeleteLocalEndpoint(localId) == 0);
    Require(sceVoiceQoSDeleteLocalEndpoint(localId) == argumentInvalid);
    std::uint8_t flag = 0;
    Require(sceVoiceQoSGetLocalEndpointAttribute(localId, micMute, &flag, 1) == argumentInvalid);
    Require(sceVoiceQoSDeleteRemoteEndpoint(remoteId) == 0);
    Require(sceVoiceQoSDeleteRemoteEndpoint(remoteId) == argumentInvalid);
    Require(sceVoiceQoSDeleteRemoteEndpoint(0x7FFF) == argumentInvalid);
    Require(sceVoiceQoSDeleteLocalEndpoint(0x7FFF) == argumentInvalid);

    std::int32_t recreatedId = -1;
    Require(sceVoiceQoSCreateLocalEndpoint(&recreatedId, USER_SERVICE_INITIAL_USER_ID, 0, 0) == 0);
    Require(recreatedId != otherLocalId);
    Require(ReadFlag(recreatedId, micMute) == 0);
    Require(sceVoiceQoSDeleteLocalEndpoint(recreatedId) == 0);

    keptLocalId = otherLocalId;
    keptConnectionId = otherConnectionId;
}

int main(int argc, char** argv) {
    Require(argc == 2);
    const auto appType = static_cast<std::int32_t>(std::strtoul(argv[1], nullptr, 16));
    std::vector<std::uint8_t> memory(0x40000, 0xAA);
    Require(sceVoiceQoSEnd() == notInitialized);
    RequireNotInitialized();
    Require(sceVoiceQoSInit(nullptr, 0x40000, appType) == argumentInvalid);
    Require(sceVoiceQoSInit(nullptr, 0, appType) == argumentInvalid);
    for (std::uint32_t size : {0u, 1u, 0x100u, 0x3FFFFu, 0x40001u, 0x80000u, 0xFFFFFFFFu}) Require(sceVoiceQoSInit(memory.data(), size, appType) == argumentInvalid);
    for (std::int32_t type : {0, 1, 0x08000000, 0x20000001, 0x30000000, 0x40000000, static_cast<std::int32_t>(0x80000000), -1}) {
        Require(sceVoiceQoSInit(memory.data(), 0x40000, type) == argumentInvalid);
    }
    for (std::uint8_t byte : memory) Require(byte == 0xAA);
    Require(sceVoiceQoSInit(memory.data(), 0x40000, appType) == 0);
    Require(sceVoiceQoSInit(memory.data(), 0x40000, appType) == initialized);
    Require(sceVoiceQoSInit(nullptr, 0, 0) == initialized);
    std::int32_t keptLocalId = -1;
    std::int32_t keptConnectionId = -1;
    RequireEndpoints(keptLocalId, keptConnectionId);
    Require(sceVoiceQoSEnd() == 0);
    Require(sceVoiceQoSEnd() == notInitialized);
    RequireNotInitialized();
    Require(sceVoiceQoSInit(nullptr, 0x40000, appType) == argumentInvalid);
    Require(sceVoiceQoSInit(memory.data(), 0x40000, appType) == 0);
    Require(sceVoiceQoSInit(memory.data(), 0x40000, appType) == initialized);
    std::uint8_t flag = 0;
    Require(sceVoiceQoSGetLocalEndpointAttribute(keptLocalId, micMute, &flag, 1) == argumentInvalid);
    Require(sceVoiceQoSDisconnect(keptConnectionId) == argumentInvalid);
    Require(sceVoiceQoSDeleteLocalEndpoint(keptLocalId) == argumentInvalid);
    Require(sceVoiceQoSEnd() == 0);
    Require(sceVoiceQoSEnd() == notInitialized);

    RequireThrows([] { sceVoiceQoSGetRemoteEndpointAttribute(1, 1, nullptr, 0); });
    RequireThrows([] { sceVoiceQoSGetConnectionAttribute(1, 1, nullptr, 0); });
    RequireThrows([] { sceVoiceQoSEnableChat(); });
    RequireThrows([] { sceVoiceQoSDisableChat(); });
    RequireThrows([] { sceVoiceQoSGetStatus(1, 1, nullptr, 0); });
    RequireThrows([] { sceVoiceQoSDebugGetStatus(); });
    RequireThrows([] { sceVoiceQoSInitHQ(); });
    RequireThrows([] { sceVoiceQoSSetMode(); });
    RequireThrows([] { sceVoiceQoSTerminate(); });
}
