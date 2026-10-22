#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <array>

extern "C" {
int APS5_VABI sceNpSessionSignalingInitialize(void* param);
int APS5_VABI sceNpSessionSignalingCreateContext(const void* param, std::uint32_t* contextId);
int APS5_VABI sceNpSessionSignalingCreateContext2(const void* param, std::uint32_t* contextId);
int APS5_VABI sceNpSessionSignalingDestroyContext(std::uint32_t contextId);
int APS5_VABI sceNpSessionSignalingDeactivate(std::uint32_t contextId, std::uint32_t connectionId);
std::int32_t APS5_VABI sceNpSessionSignalingGetLocalNetInfo(std::int32_t contextId, void* info);
int APS5_VABI sceNpSessionSignalingRequestPrepare(std::uint32_t contextId, std::uint32_t* requestId);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionStatus(std::int32_t contextId, std::int32_t connectionId, std::int32_t* status, void* peerAddress, std::uint16_t* peerPort);
int APS5_VABI sceNpSessionSignalingTerminate(void);
int APS5_VABI sceNpSessionSignalingGetMemoryInfo(void* memInfo);
int APS5_VABI sceNpSessionSignalingGetConnectionStatistics(void* stats);
int APS5_VABI sceNpSessionSignalingGetGroupFromSessionId(std::uint32_t contextId, const char* sessionId, std::uint32_t* groupId);
std::int32_t APS5_VABI sceNpSessionSignalingGetGroupInfo(std::int32_t contextId, std::int32_t groupId, void* info);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionFromNetAddress(std::int32_t contextId, std::uint32_t address, std::uint16_t port, std::int32_t* connectionId);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionFromNetAddress2(std::int32_t contextId, std::int32_t groupId, std::uint32_t address, std::uint16_t port, void* groupInfo);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionFromPeerAddress(std::int32_t contextId, const void* peerAddress, std::int32_t* connectionId);
std::int32_t APS5_VABI sceNpSessionSignalingGetConnectionFromPeerAddress2(std::int32_t contextId, std::int32_t groupId, const void* peerAddress, std::int32_t* connectionId);
}

namespace {

constexpr int InvalidArgument = static_cast<int>(0x80553303u);
constexpr int Unavailable = static_cast<int>(0x80552D06u);
constexpr int ConnectionNotFound = static_cast<int>(0x8055330Cu);
constexpr int NotInitialized = static_cast<int>(0x80553301u);
constexpr int AlreadyInitialized = static_cast<int>(0x80553302u);
constexpr int ContextLimit = static_cast<int>(0x80553307u);
constexpr int ContextNotFound = static_cast<int>(0x80553308u);
constexpr int GroupNotFound = static_cast<int>(0x8055330Au);

struct MemoryInfo {
    std::uint64_t poolSize;
    std::uint64_t usedSize;
    std::uint64_t peakUsedSize;
};
static_assert(sizeof(MemoryInfo) == 24);

struct ConnectionStatistics {
    std::uint32_t peakConnectionCount;
    std::uint32_t connectionCount;
    std::uint32_t connectingCount;
    std::uint32_t connectedCount;
};
static_assert(sizeof(ConnectionStatistics) == 16);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "NpSessionSignaling: %s\n", message);
        std::abort();
    }
}

}

int main() {
    alignas(8) unsigned char initializeParam[40] = {};
    Require(sceNpSessionSignalingTerminate() == NotInitialized, "terminate before initialize");
    Require(sceNpSessionSignalingCreateContext2(nullptr, nullptr) == NotInitialized, "initialization check precedes context arguments");
    Require(sceNpSessionSignalingGetConnectionStatus(0, 0, nullptr, nullptr, nullptr) == NotInitialized, "status before initialize");
    unsigned char memoryInfoBuffer[sizeof(MemoryInfo)];
    unsigned char statisticsBuffer[sizeof(ConnectionStatistics)];
    Require(sceNpSessionSignalingGetMemoryInfo(memoryInfoBuffer) == NotInitialized, "memory info before initialize");
    Require(sceNpSessionSignalingGetConnectionStatistics(statisticsBuffer) == NotInitialized, "statistics before initialize");
    std::uint32_t groupId = 0xa5a5a5a5u;
    Require(sceNpSessionSignalingGetGroupFromSessionId(1000, "00000000-0000-0000-0000-000000000000", &groupId) == NotInitialized,
        "group lookup before initialize");
    Require(sceNpSessionSignalingGetGroupInfo(1000, 1, nullptr) == NotInitialized, "group info before initialize");
    Require(sceNpSessionSignalingInitialize(nullptr) == InvalidArgument, "null initialize parameter");
    Require(sceNpSessionSignalingInitialize(initializeParam) == InvalidArgument, "zero pool size");
    const std::uint64_t poolSize = 4 * 1024 * 1024;
    std::memcpy(initializeParam + 8, &poolSize, sizeof(poolSize));
    Require(sceNpSessionSignalingInitialize(initializeParam) == 0, "initialize failed");
    Require(sceNpSessionSignalingInitialize(nullptr) == AlreadyInitialized, "initialization check precedes initialize arguments");
    alignas(8) unsigned char secondInitializeParam[40] = {};
    const std::uint64_t secondPoolSize = 8 * 1024 * 1024;
    std::memcpy(secondInitializeParam + 8, &secondPoolSize, sizeof(secondPoolSize));
    Require(sceNpSessionSignalingInitialize(secondInitializeParam) == AlreadyInitialized, "second initialize");

    Require(sceNpSessionSignalingGetMemoryInfo(nullptr) == InvalidArgument, "null memory info");
    std::memset(memoryInfoBuffer, 0xa5, sizeof(memoryInfoBuffer));
    Require(sceNpSessionSignalingGetMemoryInfo(memoryInfoBuffer) == 0, "memory info failed");
    MemoryInfo memoryInfo;
    std::memcpy(&memoryInfo, memoryInfoBuffer, sizeof(memoryInfo));
    Require(memoryInfo.poolSize == poolSize, "memory info pool size is not the first pool size");
    Require(memoryInfo.usedSize == 0 && memoryInfo.peakUsedSize == 0, "memory info usage");

    Require(sceNpSessionSignalingGetConnectionStatistics(nullptr) == InvalidArgument, "null statistics");
    std::memset(statisticsBuffer, 0xa5, sizeof(statisticsBuffer));
    Require(sceNpSessionSignalingGetConnectionStatistics(statisticsBuffer) == 0, "statistics failed");
    ConnectionStatistics statistics;
    std::memcpy(&statistics, statisticsBuffer, sizeof(statistics));
    Require(statistics.peakConnectionCount == 0 && statistics.connectionCount == 0 && statistics.connectingCount == 0 && statistics.connectedCount == 0,
        "statistics counts");
    alignas(8) unsigned char contextParam[56] = {};
    std::uint32_t contextId = 0;
    Require(sceNpSessionSignalingCreateContext(nullptr, &contextId) == InvalidArgument, "null context parameter");
    Require(contextId == 0, "invalid context parameter wrote an id");
    Require(sceNpSessionSignalingCreateContext2(nullptr, &contextId) == InvalidArgument, "null context2 parameter");
    Require(contextId == 0, "invalid context2 parameter wrote an id");
    Require(sceNpSessionSignalingCreateContext(contextParam, nullptr) == InvalidArgument, "null context id");
    Require(sceNpSessionSignalingCreateContext2(contextParam, nullptr) == InvalidArgument, "null context2 id");
    Require(sceNpSessionSignalingCreateContext2(contextParam, &contextId) == 0, "context creation failed");
    const auto firstId = contextId;
    Require(sceNpSessionSignalingCreateContext2(contextParam, &contextId) == ContextLimit, "duplicate user context");
    Require(contextId == firstId, "duplicate context modified output");
    std::array<std::uint32_t, 9> otherContexts{};
    for (std::int32_t userId = 1; userId <= 9; ++userId) {
        std::memcpy(contextParam, &userId, sizeof(userId));
        Require(sceNpSessionSignalingCreateContext2(contextParam, &otherContexts[userId - 1]) == 0, "create remaining contexts");
    }
    const std::int32_t overflowUser = 10;
    std::memcpy(contextParam, &overflowUser, sizeof(overflowUser));
    Require(sceNpSessionSignalingCreateContext2(contextParam, &contextId) == ContextLimit, "context limit");
    Require(contextId == firstId, "context limit modified output");
    Require(sceNpSessionSignalingDestroyContext(firstId) == 0, "destroy first context");
    Require(sceNpSessionSignalingDestroyContext(firstId) == ContextNotFound, "destroy stale context");
    Require(sceNpSessionSignalingCreateContext2(contextParam, &contextId) == 0, "reuse context slot");
    Require(contextId != firstId, "reused context kept stale id");

    std::uint32_t requestId = 0xa5a5a5a5u;
    Require(sceNpSessionSignalingRequestPrepare(firstId, &requestId) == ContextNotFound, "prepare stale context");
    Require(sceNpSessionSignalingRequestPrepare(firstId, nullptr) == InvalidArgument, "prepare argument precedence");
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

    std::int32_t status = 0x5a5a5a5a;
    std::uint32_t peerAddress = 0xa5a5a5a5u;
    std::uint16_t peerPort = 0xa5a5;
    Require(sceNpSessionSignalingGetConnectionStatus(firstId, 1, &status, &peerAddress, &peerPort) == ContextNotFound, "status stale context");
    Require(sceNpSessionSignalingGetConnectionStatus(netContextId, 1, nullptr, &peerAddress, &peerPort) == InvalidArgument, "null connection status");
    Require(sceNpSessionSignalingGetConnectionStatus(netContextId, 1, &status, &peerAddress, &peerPort) == ConnectionNotFound, "connection without network");
    Require(sceNpSessionSignalingGetConnectionStatus(netContextId, 1, &status, nullptr, nullptr) == ConnectionNotFound, "connection without peer outputs");
    Require(status == 0x5a5a5a5a && peerAddress == 0xa5a5a5a5u && peerPort == 0xa5a5, "failed status query wrote an output");
    Require(sceNpSessionSignalingDeactivate(firstId, 1) == ContextNotFound, "deactivate stale context");
    Require(sceNpSessionSignalingDeactivate(contextId, 1) == GroupNotFound, "deactivate nonexistent group");

    const char* sessionId = "8f2bba65-1f71-4cb2-9b3c-6d40b1a6e2f1";
    Require(sceNpSessionSignalingGetGroupFromSessionId(contextId, nullptr, &groupId) == InvalidArgument, "null session id");
    Require(sceNpSessionSignalingGetGroupFromSessionId(contextId, sessionId, nullptr) == InvalidArgument, "null group id");
    Require(sceNpSessionSignalingGetGroupFromSessionId(firstId, sessionId, &groupId) == ContextNotFound, "group lookup in stale context");
    Require(sceNpSessionSignalingGetGroupFromSessionId(contextId, sessionId, &groupId) == GroupNotFound, "group lookup without an activated session");
    Require(groupId == 0xa5a5a5a5u, "failed group lookup wrote a group id");

    const auto staleId = static_cast<std::int32_t>(firstId);
    unsigned char groupInfo[264];
    std::memset(groupInfo, 0xa5, sizeof(groupInfo));
    unsigned char untouchedGroupInfo[sizeof(groupInfo)];
    std::memcpy(untouchedGroupInfo, groupInfo, sizeof(groupInfo));
    Require(sceNpSessionSignalingGetGroupInfo(netContextId, 1, nullptr) == InvalidArgument, "null group info");
    Require(sceNpSessionSignalingGetGroupInfo(staleId, 1, groupInfo) == ContextNotFound, "group info in stale context");
    Require(sceNpSessionSignalingGetGroupInfo(netContextId, 1, groupInfo) == GroupNotFound, "group info without a session");
    Require(sceNpSessionSignalingGetConnectionFromNetAddress2(netContextId, 1, 0x0100007fu, 3658, nullptr) == InvalidArgument, "null output from group net address");
    Require(sceNpSessionSignalingGetConnectionFromNetAddress2(netContextId, 1, 0x0100007fu, 3658, groupInfo) == GroupNotFound, "connection from group net address without a session");
    Require(std::memcmp(groupInfo, untouchedGroupInfo, sizeof(groupInfo)) == 0, "failed group query modified the output");
    const unsigned char peer[16] = {};
    std::int32_t found = 0x5a5a5a5a;
    Require(sceNpSessionSignalingGetConnectionFromNetAddress(netContextId, 0x0100007fu, 3658, nullptr) == InvalidArgument, "null connection id from net address");
    Require(sceNpSessionSignalingGetConnectionFromNetAddress(staleId, 0x0100007fu, 3658, &found) == ContextNotFound, "connection from net address in stale context");
    Require(sceNpSessionSignalingGetConnectionFromNetAddress(netContextId, 0x0100007fu, 3658, &found) == ConnectionNotFound, "connection from net address without a session");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress(netContextId, nullptr, &found) == InvalidArgument, "null peer address");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress(netContextId, peer, nullptr) == InvalidArgument, "null connection id from peer address");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress(staleId, peer, &found) == ContextNotFound, "connection from peer address in stale context");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress(netContextId, peer, &found) == ConnectionNotFound, "connection from peer address without a session");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress2(netContextId, 1, nullptr, &found) == InvalidArgument, "null group peer address");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress2(netContextId, 1, peer, nullptr) == InvalidArgument, "null connection id from group peer address");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress2(staleId, 1, peer, &found) == ContextNotFound, "group peer lookup in stale context");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress2(netContextId, 1, peer, &found) == GroupNotFound, "connection from group peer address without a session");
    Require(found == 0x5a5a5a5a, "a failed lookup wrote an id");

    Require(sceNpSessionSignalingTerminate() == 0, "terminate failed");
    Require(sceNpSessionSignalingDestroyContext(contextId) == NotInitialized, "destroy after terminate");
    Require(sceNpSessionSignalingTerminate() == NotInitialized, "repeated terminate");
    Require(sceNpSessionSignalingGetMemoryInfo(memoryInfoBuffer) == NotInitialized, "memory info after terminate");
    Require(sceNpSessionSignalingInitialize(initializeParam) == 0, "reinitialize");
    Require(sceNpSessionSignalingRequestPrepare(contextId, &requestId) == ContextNotFound, "reinitialize retained a context");
    Require(sceNpSessionSignalingTerminate() == 0, "final terminate");
}
