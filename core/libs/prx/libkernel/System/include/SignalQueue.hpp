#ifndef CORE_LIBS_PRX_LIBKERNEL_SYSTEM_INCLUDE_SIGNALQUEUE_HPP
#define CORE_LIBS_PRX_LIBKERNEL_SYSTEM_INCLUDE_SIGNALQUEUE_HPP

#include <cstdint>
#include "prx/libc/include/general/VabiMacros.hpp"

using GuestSignalAccept = int (APS5_VABI*)(std::uint64_t key);

struct GuestSignalRecord {
    std::int32_t code;
    std::int32_t pid;
    std::uint64_t value;
    std::int32_t reason[2];
    std::uint64_t key;
    GuestSignalAccept accept;
};

inline constexpr std::int32_t GuestSiQueue = 0x10002;
inline constexpr std::int32_t GuestSiTimer = 0x10003;
inline constexpr std::int32_t GuestSiMesgq = 0x10005;

extern "C" {
int APS5_VABI GuestSignalQueue_nid_no_patch(int guest, const GuestSignalRecord* record);
int APS5_VABI GuestSignalPost_nid_no_patch(int guest, const GuestSignalRecord* record);
void APS5_VABI GuestSignalCancel_nid_no_patch(std::uint64_t key);
}

#endif
