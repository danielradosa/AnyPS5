#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
int APS5_VABI sceNpSessionSignalingInitialize(void* param);
int APS5_VABI sceNpSessionSignalingCreateContext2(const void* param, std::uint32_t* contextId);
int APS5_VABI sceNpSessionSignalingDestroyContext(std::uint32_t contextId);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionStatus(std::uint32_t contextId, std::uint32_t connectionId, std::int32_t* status, void* peerAddress, std::uint16_t* peerPort);
std::int32_t APS5_VABI sceNpSessionSignalingGetLocalNetInfo(std::int32_t contextId, void* info);
int APS5_VABI sceNpSessionSignalingRequestPrepare(std::uint32_t contextId, std::uint32_t* requestId);
int APS5_VABI sceNpSessionSignalingGetMemoryInfo(void* memInfo);
int APS5_VABI sceNpSessionSignalingGetConnectionStatistics(void* stats);
int APS5_VABI sceNpSessionSignalingTerminate(void);
int APS5_VABI sceNpSessionSignalingGetGroupFromSessionId(std::uint32_t contextId, const char* sessionId, std::uint32_t* groupId);
std::int32_t APS5_VABI sceNpSessionSignalingGetGroupInfo(std::int32_t contextId, std::int32_t groupId, void* info);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionFromNetAddress(std::int32_t contextId, std::uint32_t address, std::uint16_t port, std::int32_t* connectionId);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionFromNetAddress2(std::int32_t contextId, std::int32_t groupId, std::uint32_t address, std::uint16_t port, std::int32_t* connectionId);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionFromPeerAddress(std::int32_t contextId, const void* peerAddress, std::int32_t* connectionId);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionFromPeerAddress2(std::int32_t contextId, std::int32_t groupId, const void* peerAddress, std::int32_t* connectionId);
}

namespace {

constexpr int NotInitialized = static_cast<int>(0x80553301u);
constexpr int AlreadyInitialized = static_cast<int>(0x80553302u);
constexpr int InvalidArgument = static_cast<int>(0x80553303u);
constexpr int ContextNotFound = static_cast<int>(0x80553308u);
constexpr int GroupNotFound = static_cast<int>(0x8055330Au);
constexpr int ConnectionNotFound = static_cast<int>(0x8055330Cu);
constexpr int Unavailable = static_cast<int>(0x80552D06u);

constexpr std::size_t InitializeParamSize = 0x28;
constexpr std::size_t PoolSizeOffset = 8;

struct MemoryInfo {
    std::size_t totalMemSize;
    std::size_t curMemUsage;
    std::size_t maxMemUsage;
    unsigned char reserved[12];
};
static_assert(sizeof(MemoryInfo) == 0x28);

struct ConnectionStatistics {
    std::uint32_t maxConnection;
    std::uint32_t totalConnection;
    std::uint32_t connecting;
    std::uint32_t connected;
};
static_assert(sizeof(ConnectionStatistics) == 0x10);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "NpSessionSignaling: %s\n", message);
        std::abort();
    }
}

}

int main() {
    unsigned char memoryInfoBuffer[sizeof(MemoryInfo)];
    unsigned char statisticsBuffer[sizeof(ConnectionStatistics)];
    std::int32_t status = 0x5a5a5a5a;
    Require(sceNpSessionSignalingGetMemoryInfo(memoryInfoBuffer) == NotInitialized, "memory info before initialize");
    Require(sceNpSessionSignalingGetConnectionStatistics(statisticsBuffer) == NotInitialized, "statistics before initialize");
    Require(sceNpSessionSignalingGetConnectionStatus(1, 1, &status, nullptr, nullptr) == NotInitialized, "status before initialize");
    Require(sceNpSessionSignalingTerminate() == NotInitialized, "terminate before initialize");
    std::uint32_t groupId = 0xa5a5a5a5u;
    Require(sceNpSessionSignalingGetGroupFromSessionId(1, "00000000-0000-0000-0000-000000000000", &groupId) == NotInitialized,
        "group lookup before initialize");
    Require(sceNpSessionSignalingGetGroupInfo(1, 1, nullptr) == NotInitialized, "group info before initialize");

    Require(sceNpSessionSignalingInitialize(nullptr) == InvalidArgument, "null initialize param");
    const std::size_t poolSize = 256 * 1024;
    unsigned char initializeParam[InitializeParamSize] = {};
    std::memcpy(initializeParam + PoolSizeOffset, &poolSize, sizeof(poolSize));
    Require(sceNpSessionSignalingInitialize(initializeParam) == 0, "initialize failed");
    const std::size_t secondPoolSize = 512 * 1024;
    unsigned char secondInitializeParam[InitializeParamSize] = {};
    std::memcpy(secondInitializeParam + PoolSizeOffset, &secondPoolSize, sizeof(secondPoolSize));
    Require(sceNpSessionSignalingInitialize(secondInitializeParam) == AlreadyInitialized, "second initialize");

    Require(sceNpSessionSignalingGetMemoryInfo(nullptr) == InvalidArgument, "null memory info");
    std::memset(memoryInfoBuffer, 0xa5, sizeof(memoryInfoBuffer));
    Require(sceNpSessionSignalingGetMemoryInfo(memoryInfoBuffer) == 0, "memory info failed");
    MemoryInfo memoryInfo;
    std::memcpy(&memoryInfo, memoryInfoBuffer, sizeof(memoryInfo));
    Require(memoryInfo.totalMemSize == poolSize, "memory info total is not the first pool size");
    Require(memoryInfo.curMemUsage == 0, "memory info current usage");
    Require(memoryInfo.maxMemUsage == 0, "memory info maximum usage");

    Require(sceNpSessionSignalingGetConnectionStatistics(nullptr) == InvalidArgument, "null statistics");
    std::memset(statisticsBuffer, 0xa5, sizeof(statisticsBuffer));
    Require(sceNpSessionSignalingGetConnectionStatistics(statisticsBuffer) == 0, "statistics failed");
    ConnectionStatistics statistics;
    std::memcpy(&statistics, statisticsBuffer, sizeof(statistics));
    Require(statistics.maxConnection == 0, "statistics maximum connections");
    Require(statistics.totalConnection == 0, "statistics total connections");
    Require(statistics.connecting == 0, "statistics connecting");
    Require(statistics.connected == 0, "statistics connected");

    unsigned char contextParam[64] = {};
    std::uint32_t contextId = 0;
    Require(sceNpSessionSignalingCreateContext2(contextParam, &contextId) == 0, "context creation failed");

    std::uint32_t peerAddress = 0xa5a5a5a5u;
    std::uint16_t peerPort = 0xa5a5;
    Require(sceNpSessionSignalingGetConnectionStatus(contextId, 1, nullptr, nullptr, nullptr) == InvalidArgument, "null status");
    Require(sceNpSessionSignalingGetConnectionStatus(contextId + 1, 1, &status, &peerAddress, &peerPort) == ContextNotFound, "status of unknown context");
    Require(sceNpSessionSignalingGetConnectionStatus(contextId, 1, &status, &peerAddress, &peerPort) == ConnectionNotFound, "status of unknown connection");
    Require(status == 0x5a5a5a5a && peerAddress == 0xa5a5a5a5u && peerPort == 0xa5a5, "failed query wrote the status");

    std::uint32_t requestId = 0xa5a5a5a5u;
    Require(sceNpSessionSignalingRequestPrepare(contextId, nullptr) == InvalidArgument, "null request id");
    Require(sceNpSessionSignalingRequestPrepare(contextId, &requestId) == Unavailable, "prepare without network");
    Require(requestId == 0xa5a5a5a5u, "failed prepare wrote a request id");

    const auto netContextId = static_cast<std::int32_t>(contextId);
    unsigned char info[16];
    std::memset(info, 0xa5, sizeof(info));
    unsigned char untouched[sizeof(info)];
    std::memcpy(untouched, info, sizeof(info));
    Require(sceNpSessionSignalingGetLocalNetInfo(netContextId, nullptr) == InvalidArgument, "null info");
    Require(sceNpSessionSignalingGetLocalNetInfo(netContextId, info) == Unavailable, "local net info without network");
    Require(std::memcmp(info, untouched, sizeof(info)) == 0, "failed query modified the info");

    Require(sceNpSessionSignalingGetConnectionStatus(contextId, 1, &status, nullptr, nullptr) == ConnectionNotFound, "connection without peer outputs");
    Require(status == 0x5a5a5a5a && peerAddress == 0xa5a5a5a5u && peerPort == 0xa5a5, "failed status query wrote an output");
    const char* sessionId = "8f2bba65-1f71-4cb2-9b3c-6d40b1a6e2f1";
    Require(sceNpSessionSignalingGetGroupFromSessionId(contextId, nullptr, &groupId) == InvalidArgument, "null session id");
    Require(sceNpSessionSignalingGetGroupFromSessionId(contextId, sessionId, nullptr) == InvalidArgument, "null group id");
    Require(sceNpSessionSignalingGetGroupFromSessionId(contextId + 1, sessionId, &groupId) == ContextNotFound, "group lookup in unknown context");
    Require(sceNpSessionSignalingGetGroupFromSessionId(contextId, sessionId, &groupId) == GroupNotFound, "group lookup without an activated session");
    Require(groupId == 0xa5a5a5a5u, "failed group lookup wrote a group id");

    Require(sceNpSessionSignalingGetGroupInfo(netContextId, 1, nullptr) == InvalidArgument, "null group info");
    Require(sceNpSessionSignalingGetGroupInfo(netContextId, 1, info) == GroupNotFound, "group info without a session");
    Require(std::memcmp(info, untouched, sizeof(info)) == 0, "failed group query modified the info");
    const unsigned char peer[40] = {};
    std::int32_t found = 0x5a5a5a5a;
    Require(sceNpSessionSignalingGetConnectionFromNetAddress(netContextId, 0x0100007fu, 3658, nullptr) == InvalidArgument, "null connection id from net address");
    Require(sceNpSessionSignalingGetConnectionFromNetAddress(netContextId, 0x0100007fu, 3658, &found) == ConnectionNotFound, "connection from net address without a session");
    Require(sceNpSessionSignalingGetConnectionFromNetAddress2(netContextId, 1, 0x0100007fu, 3658, nullptr) == InvalidArgument, "null connection id from group net address");
    Require(sceNpSessionSignalingGetConnectionFromNetAddress2(netContextId, 1, 0x0100007fu, 3658, &found) == GroupNotFound, "connection from group net address without a session");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress(netContextId, nullptr, &found) == InvalidArgument, "null peer address");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress(netContextId, peer, nullptr) == InvalidArgument, "null connection id from peer address");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress(netContextId, peer, &found) == ConnectionNotFound, "connection from peer address without a session");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress2(netContextId, 1, nullptr, &found) == InvalidArgument, "null group peer address");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress2(netContextId, 1, peer, nullptr) == InvalidArgument, "null connection id from group peer address");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress2(netContextId, 1, peer, &found) == GroupNotFound, "connection from group peer address without a session");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress2(netContextId + 1, 1, peer, nullptr) == ContextNotFound, "a peer lookup in an unknown context");
    Require(found == 0x5a5a5a5a, "a failed lookup wrote an id");
    Require(sceNpSessionSignalingDestroyContext(contextId) == 0, "context destruction failed");
    Require(sceNpSessionSignalingGetGroupFromSessionId(contextId, sessionId, &groupId) == ContextNotFound, "group lookup in destroyed context");
    Require(sceNpSessionSignalingGetConnectionStatus(contextId, 1, &status, nullptr, nullptr) == ContextNotFound, "status of destroyed context");

    Require(sceNpSessionSignalingTerminate() == 0, "terminate failed");
    Require(sceNpSessionSignalingTerminate() == NotInitialized, "second terminate");
    Require(sceNpSessionSignalingGetMemoryInfo(memoryInfoBuffer) == NotInitialized, "memory info after terminate");
}
