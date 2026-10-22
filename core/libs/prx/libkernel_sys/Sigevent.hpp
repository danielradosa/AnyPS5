#ifndef CORE_LIBS_PRX_LIBKERNEL_SYS_SIGEVENT_HPP
#define CORE_LIBS_PRX_LIBKERNEL_SYS_SIGEVENT_HPP

#include <cstddef>
#include <cstdint>

struct GuestSigevent {
    std::int32_t notify;
    std::int32_t signo;
    std::uint64_t value;
    std::int32_t threadId;
    std::int32_t threadIdTail;
    std::uint64_t spare[7];
};
static_assert(sizeof(GuestSigevent) == 80 && offsetof(GuestSigevent, value) == 8 && offsetof(GuestSigevent, threadId) == 16);

inline constexpr int GuestSigevNone = 0;
inline constexpr int GuestSigevSignal = 1;
inline constexpr int GuestSigevThreadId = 4;

inline bool SigeventSignals(const GuestSigevent& event) {
    return event.notify == GuestSigevSignal || event.notify == GuestSigevThreadId;
}

inline bool SigeventValid(const GuestSigevent& event) {
    constexpr int MaxSignal = 128;
    if (event.notify != GuestSigevNone && !SigeventSignals(event)) return false;
    return !SigeventSignals(event) || (event.signo > 0 && event.signo <= MaxSignal);
}

#endif
