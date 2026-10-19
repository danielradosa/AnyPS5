#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <initializer_list>

extern "C" {
int APS5_VABI sceVoiceInit(VoiceInitParam*, std::int32_t);
int APS5_VABI sceVoiceEnd_nid_postfix(void);
int APS5_VABI sceVoiceCreatePort(std::uint32_t*, const VoicePortParam*);
int APS5_VABI sceVoiceDeletePort(std::uint32_t);
int APS5_VABI sceVoiceSetMuteFlag(std::uint32_t, std::uint32_t);
int APS5_VABI sceVoiceResetPort(std::uint32_t);
int APS5_VABI sceVoiceGetResourceInfo(VoiceResourceInfo*);
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

static VoicePortParam PcmInput(std::uint16_t mute) {
    VoicePortParam param{};
    param.port_type = 1;
    param.mute = mute;
    param.volume = 1.0f;
    param.pcmaudio = {640, 0, 16000};
    return param;
}

struct ResourceProbe {
    VoiceResourceInfo info;
    std::uint16_t after;
};

static void RequireResourceInfo() {
    ResourceProbe probe{{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}, 0xA5A5};
    Require(sceVoiceGetResourceInfo(&probe.info) == 0);
    Require(probe.info.max_voice_input_ports == 7);
    Require(probe.info.max_voice_output_ports == 7);
    Require(probe.info.max_device_input_ports == 1);
    Require(probe.info.max_device_output_ports == 1);
    Require(probe.info.max_ports == 64);
    Require(probe.after == 0xA5A5);
}

int main() {
    RequireThrows([] { sceVoiceSetMuteFlag(0, 1); });
    RequireThrows([] { sceVoiceResetPort(0); });
    RequireThrows([] {
        VoiceResourceInfo info{};
        sceVoiceGetResourceInfo(&info);
    });
    VoiceInitParam init{};
    Require(sceVoiceInit(&init, 0) == 0);
    RequireThrows([] { sceVoiceGetResourceInfo(nullptr); });
    RequireResourceInfo();

    std::uint32_t first = 0;
    const auto unmutedParam = PcmInput(0);
    Require(sceVoiceCreatePort(&first, &unmutedParam) == 0);
    std::uint32_t second = 0;
    const auto mutedParam = PcmInput(0xFFFF);
    Require(sceVoiceCreatePort(&second, &mutedParam) == 0);
    Require(first != second);

    for (std::uint32_t value : {1u, 0u, 2u, 0xFFFFu, 0x10000u}) {
        Require(sceVoiceSetMuteFlag(first, value) == 0);
    }
    Require(sceVoiceResetPort(first) == 0);
    Require(sceVoiceResetPort(second) == 0);

    RequireThrows([] { sceVoiceSetMuteFlag(0xFFFF, 0); });
    RequireThrows([] { sceVoiceResetPort(0xFFFF); });

    Require(sceVoiceDeletePort(second) == 0);
    RequireThrows([&] { sceVoiceSetMuteFlag(second, 1); });
    RequireThrows([&] { sceVoiceResetPort(second); });

    RequireResourceInfo();

    Require(sceVoiceEnd_nid_postfix() == 0);
    RequireThrows([&] { sceVoiceSetMuteFlag(first, 0); });
    RequireThrows([&] { sceVoiceResetPort(first); });
    RequireThrows([] {
        VoiceResourceInfo info{};
        sceVoiceGetResourceInfo(&info);
    });
}
