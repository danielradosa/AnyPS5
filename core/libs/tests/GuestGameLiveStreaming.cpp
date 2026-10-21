#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <stdexcept>

struct GameLiveStreamingStatus2 {
    std::int32_t userId;
    bool isOnAir;
    std::uint8_t align[3];
    std::uint32_t spectatorCounts;
    std::uint32_t textMessageCounts;
    std::uint32_t commandMessageCounts;
    std::uint32_t broadcastVideoResolution;
    std::uint8_t reserved[48];
};
static_assert(sizeof(GameLiveStreamingStatus2) == 72);

struct GameLiveStreamingStatus {
    bool isOnAir;
    std::uint8_t align[3];
    std::uint32_t spectatorCounts;
    std::int32_t userId;
    std::uint8_t reserved[60];
};
static_assert(sizeof(GameLiveStreamingStatus) == 72);

extern "C" {
int APS5_VABI sceGameLiveStreamingInitialize(std::size_t heap_size);
int APS5_VABI sceGameLiveStreamingTerminate(void);
int APS5_VABI sceGameLiveStreamingGetCurrentStatus2(GameLiveStreamingStatus2* status);
int APS5_VABI sceGameLiveStreamingGetProgramInfo(void);
int APS5_VABI sceGameLiveStreamingGetCurrentStatus(GameLiveStreamingStatus* status);
int APS5_VABI sceGameLiveStreamingGetSocialFeedbackMessagesCount(int type, std::uint32_t* count);
int APS5_VABI sceGameLiveStreamingGetSocialFeedbackMessages(int type, std::size_t feedbackCount, void* feedback, std::size_t* acquiredCount);
int APS5_VABI sceGameLiveStreamingClearSocialFeedbackMessages(int type);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr std::int32_t kInvalidUserId = -1;
constexpr int kInvalidParam = static_cast<int>(0x80A00002);

}

int main() {
    bool threwZeroHeap = false;
    try {
        sceGameLiveStreamingInitialize(0);
    } catch (const std::invalid_argument&) {
        threwZeroHeap = true;
    }
    Require(threwZeroHeap);

    Require(sceGameLiveStreamingInitialize(1024 * 1024) == 0);

    bool threwNullStatus = false;
    try {
        sceGameLiveStreamingGetCurrentStatus2(nullptr);
    } catch (const std::invalid_argument&) {
        threwNullStatus = true;
    }
    Require(threwNullStatus);

    GameLiveStreamingStatus2 status{};
    std::memset(&status, 0x7f, sizeof(status));
    Require(sceGameLiveStreamingGetCurrentStatus2(&status) == 0);
    Require(status.userId == kInvalidUserId);
    Require(!status.isOnAir);
    Require(status.spectatorCounts == 0);
    Require(status.textMessageCounts == 0);
    Require(status.commandMessageCounts == 0);
    Require(status.broadcastVideoResolution == 0);
    for (std::uint8_t byte : status.align) {
        Require(byte == 0);
    }
    for (std::uint8_t byte : status.reserved) {
        Require(byte == 0);
    }

    bool threwNullLegacyStatus = false;
    try {
        sceGameLiveStreamingGetCurrentStatus(nullptr);
    } catch (const std::invalid_argument&) {
        threwNullLegacyStatus = true;
    }
    Require(threwNullLegacyStatus);

    GameLiveStreamingStatus legacy{};
    std::memset(&legacy, 0x7f, sizeof(legacy));
    Require(sceGameLiveStreamingGetCurrentStatus(&legacy) == 0);
    Require(!legacy.isOnAir);
    Require(legacy.spectatorCounts == 0);
    Require(legacy.userId == kInvalidUserId);
    for (std::uint8_t byte : legacy.align) {
        Require(byte == 0);
    }
    for (std::uint8_t byte : legacy.reserved) {
        Require(byte == 0);
    }

    for (int type = 1; type <= 3; ++type) {
        std::uint32_t count = 7;
        Require(sceGameLiveStreamingGetSocialFeedbackMessagesCount(type, &count) == 0 && count == 0);
        unsigned char feedback[64];
        std::memset(feedback, 0x5a, sizeof(feedback));
        std::size_t acquired = 7;
        Require(sceGameLiveStreamingGetSocialFeedbackMessages(type, 1, feedback, &acquired) == 0 && acquired == 0);
        Require(feedback[0] == 0x5a && feedback[63] == 0x5a);
        acquired = 7;
        Require(sceGameLiveStreamingGetSocialFeedbackMessages(type, 0, nullptr, &acquired) == 0 && acquired == 0);
        Require(sceGameLiveStreamingClearSocialFeedbackMessages(type) == 0);
        Require(sceGameLiveStreamingGetSocialFeedbackMessagesCount(type, nullptr) == kInvalidParam);
        Require(sceGameLiveStreamingGetSocialFeedbackMessages(type, 1, feedback, nullptr) == kInvalidParam);
    }
    for (int type : {0, 4, -1}) {
        std::uint32_t count = 7;
        Require(sceGameLiveStreamingGetSocialFeedbackMessagesCount(type, &count) == kInvalidParam && count == 7);
        std::size_t acquired = 7;
        Require(sceGameLiveStreamingGetSocialFeedbackMessages(type, 0, nullptr, &acquired) == kInvalidParam && acquired == 7);
        Require(sceGameLiveStreamingClearSocialFeedbackMessages(type) == kInvalidParam);
    }

    Require(sceGameLiveStreamingGetProgramInfo() == 0);

    Require(sceGameLiveStreamingTerminate() == 0);
}
