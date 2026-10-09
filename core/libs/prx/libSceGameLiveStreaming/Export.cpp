#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int GAME_LIVE_STREAMING_ERROR_INVALID_PARAM = static_cast<int>(0x80A00002u);

}

struct GameLiveStreamingStatus2 {
    std::int32_t user_id;
    std::uint8_t is_on_air;
    std::uint8_t align[3];
    std::uint32_t spectator_counts;
    std::uint32_t text_message_counts;
    std::uint32_t command_message_counts;
    std::int32_t broadcast_video_resolution;
    std::uint8_t reserved[48];
};
static_assert(sizeof(GameLiveStreamingStatus2) == 72);

extern "C" {

int APS5_VABI sceGameLiveStreamingInitialize(size_t heap_size) {
    if (heap_size == 0) APS5_INVALID_ARG_EX;
    return 0;
}

int APS5_VABI sceGameLiveStreamingTerminate(void) {
    return 0;
}

int APS5_VABI sceGameLiveStreamingGetCurrentStatus2(GameLiveStreamingStatus2* status) {
    if (status == nullptr) return GAME_LIVE_STREAMING_ERROR_INVALID_PARAM;
    *status = {};
    return 0;
}

int APS5_VABI sceGameLiveStreamingGetProgramInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
