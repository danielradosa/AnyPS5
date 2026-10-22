#include "SceTypes.hpp"
#include "Sigevent.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/System/include/SignalQueue.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

struct GuestItimerspec {
    KernelTimespec interval;
    KernelTimespec value;
};
static_assert(sizeof(GuestItimerspec) == 32);

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI clock_gettime_nid_postfix(int clock, KernelTimespec* time);
bool GuestThreadIdIsLive_nid_no_patch(std::int64_t tid);
}

namespace {

constexpr int GuestEagain = 35;
constexpr int GuestEfault = 14;
constexpr int GuestEinval = 22;
constexpr int ClockRealtime = 0;
constexpr int ClockMonotonic = 4;
constexpr int TimerMax = 32;
constexpr int FirstTimer = 3;
constexpr int TimerAbstime = 1;
constexpr int Sigalrm = 14;
constexpr std::int64_t NanosPerSecond = 1000000000;
constexpr std::int64_t MinExactIntervalNanos = 1000000;
constexpr std::uint64_t TimerKeyTag = 0x6b74000000000000ull;

struct Timer {
    int clock = ClockRealtime;
    GuestSigevent event{};
    KernelTimespec value{};
    KernelTimespec interval{};
    int overrun = 0;
    int overrunLast = 0;
    std::uint64_t key = 0;
};

struct Timers {
    std::mutex lock;
    std::condition_variable changed;
    std::array<std::unique_ptr<Timer>, TimerMax> slots;
    std::uint64_t generation = 0;
    bool threadStarted = false;
};

Timers& State() {
    static auto* state = new Timers();
    return *state;
}

int Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

bool IsSet(const KernelTimespec& time) {
    return time.tv_sec != 0 || time.tv_nsec != 0;
}

bool Before(const KernelTimespec& first, const KernelTimespec& second) {
    return first.tv_sec != second.tv_sec ? first.tv_sec < second.tv_sec : first.tv_nsec < second.tv_nsec;
}

KernelTimespec Add(const KernelTimespec& first, const KernelTimespec& second) {
    KernelTimespec sum{};
    if (__builtin_add_overflow(first.tv_sec, second.tv_sec, &sum.tv_sec)) return {LLONG_MAX, NanosPerSecond - 1};
    sum.tv_nsec = first.tv_nsec + second.tv_nsec;
    if (sum.tv_nsec >= NanosPerSecond) {
        if (sum.tv_sec == LLONG_MAX) return {LLONG_MAX, NanosPerSecond - 1};
        ++sum.tv_sec;
        sum.tv_nsec -= NanosPerSecond;
    }
    return sum;
}

KernelTimespec Subtract(const KernelTimespec& first, const KernelTimespec& second) {
    KernelTimespec difference{first.tv_sec - second.tv_sec, first.tv_nsec - second.tv_nsec};
    if (difference.tv_nsec < 0) {
        --difference.tv_sec;
        difference.tv_nsec += NanosPerSecond;
    }
    return difference;
}

bool ValidTime(const KernelTimespec& time) {
    return time.tv_sec >= 0 && time.tv_nsec >= 0 && time.tv_nsec < NanosPerSecond;
}

KernelTimespec Now(int clock) {
    KernelTimespec now{};
    if (clock_gettime_nid_postfix(clock, &now) != 0) throw std::runtime_error("ktimer: clock_gettime failed for clock " + std::to_string(clock));
    return now;
}

bool Advance(Timer& timer, const KernelTimespec& now) {
    if (!IsSet(timer.value) || Before(now, timer.value)) return false;
    if (IsSet(timer.interval)) {
        timer.value = Add(timer.value, timer.interval);
        while (!Before(now, timer.value)) {
            if (timer.overrun < INT_MAX) ++timer.overrun;
            timer.value = Add(timer.value, timer.interval);
        }
    } else {
        timer.value = {};
    }
    return true;
}

Timer* Find(Timers& state, int id) {
    if (id < FirstTimer || id >= TimerMax) return nullptr;
    return state.slots[static_cast<std::size_t>(id)].get();
}

int APS5_VABI AcceptTimer(std::uint64_t key) {
    auto& state = State();
    const std::lock_guard lock(state.lock);
    for (auto& timer : state.slots) {
        if (!timer || timer->key != key) continue;
        timer->overrunLast = timer->overrun;
        timer->overrun = 0;
        return timer->overrunLast;
    }
    return 0;
}

void Fire(Timer& timer, int id) {
    if (!SigeventSignals(timer.event)) return;
    if (timer.event.notify == GuestSigevThreadId && !GuestThreadIdIsLive_nid_no_patch(timer.event.threadId)) {
        timer.value = {};
        timer.interval = {};
        return;
    }
    const GuestSignalRecord record{GuestSiTimer, 0, timer.event.value, {id, 0}, timer.key, AcceptTimer};
    if (GuestSignalPost_nid_no_patch(timer.event.signo, &record) == 1 && timer.overrun < INT_MAX) ++timer.overrun;
}

void RunTimers() {
    auto& state = State();
    std::unique_lock lock(state.lock);
    for (;;) {
        std::optional<KernelTimespec> wait;
        bool fired = false;
        for (int id = FirstTimer; id < TimerMax; ++id) {
            Timer* timer = Find(state, id);
            if (timer == nullptr || !IsSet(timer->value) || !SigeventSignals(timer->event)) continue;
            const KernelTimespec now = Now(timer->clock);
            if (Before(now, timer->value)) {
                const KernelTimespec remaining = Subtract(timer->value, now);
                if (!wait || Before(remaining, *wait)) wait = remaining;
                continue;
            }
            Advance(*timer, now);
            Fire(*timer, id);
            fired = true;
        }
        if (fired) continue;
        if (!wait) {
            state.changed.wait(lock);
            continue;
        }
        constexpr std::int64_t LongestWaitSeconds = 3600;
        const auto seconds = std::min<std::int64_t>(wait->tv_sec, LongestWaitSeconds);
        state.changed.wait_for(lock, std::chrono::seconds(seconds) + std::chrono::nanoseconds(seconds == wait->tv_sec ? wait->tv_nsec : 0));
    }
}

void Notify(Timers& state) {
    if (!state.threadStarted) {
        std::thread(RunTimers).detach();
        state.threadStarted = true;
    }
    state.changed.notify_all();
}

GuestItimerspec Remaining(Timer& timer) {
    const KernelTimespec now = Now(timer.clock);
    if (!SigeventSignals(timer.event)) Advance(timer, now);
    GuestItimerspec result{timer.interval, timer.value};
    if (IsSet(result.value)) {
        result.value = Subtract(result.value, now);
        if (result.value.tv_sec < 0 || !IsSet(result.value)) result.value = {0, 1};
    }
    return result;
}

}

extern "C" {

int APS5_VABI ktimer_create_nid_postfix(int clock, const GuestSigevent* event, int* timerid) {
    if (clock != ClockRealtime && clock != ClockMonotonic) return Fail(GuestEinval);
    if (event != nullptr && !SigeventValid(*event)) return Fail(GuestEinval);
    auto& state = State();
    const std::lock_guard lock(state.lock);
    int id = FirstTimer;
    while (id < TimerMax && state.slots[static_cast<std::size_t>(id)]) ++id;
    if (id == TimerMax) return Fail(GuestEagain);
    if (timerid == nullptr) return Fail(GuestEfault);
    auto timer = std::make_unique<Timer>();
    timer->clock = clock;
    if (event != nullptr) {
        timer->event = *event;
    } else {
        timer->event.notify = GuestSigevSignal;
        timer->event.signo = Sigalrm;
        timer->event.value = static_cast<std::uint32_t>(id);
    }
    timer->key = TimerKeyTag | (++state.generation << 8) | static_cast<std::uint64_t>(id);
    state.slots[static_cast<std::size_t>(id)] = std::move(timer);
    *timerid = id;
    return 0;
}

int APS5_VABI ktimer_delete_nid_postfix(int timerid) {
    auto& state = State();
    const std::lock_guard lock(state.lock);
    if (timerid < 0 || timerid >= TimerMax || !state.slots[static_cast<std::size_t>(timerid)]) return Fail(GuestEinval);
    GuestSignalCancel_nid_no_patch(state.slots[static_cast<std::size_t>(timerid)]->key);
    state.slots[static_cast<std::size_t>(timerid)].reset();
    state.changed.notify_all();
    return 0;
}

int APS5_VABI ktimer_settime_nid_postfix(int timerid, int flags, const GuestItimerspec* value, GuestItimerspec* previous) {
    if (value == nullptr) return Fail(GuestEfault);
    const GuestItimerspec requested = *value;
    auto& state = State();
    const std::lock_guard lock(state.lock);
    Timer* timer = Find(state, timerid);
    if (timer == nullptr) return Fail(GuestEinval);
    if (!ValidTime(requested.value)) return Fail(GuestEinval);
    KernelTimespec interval{};
    if (IsSet(requested.value)) {
        if (!ValidTime(requested.interval)) return Fail(GuestEinval);
        interval = requested.interval;
    }
    if (interval.tv_sec == 0 && interval.tv_nsec != 0 && interval.tv_nsec < MinExactIntervalNanos)
        throw std::runtime_error("ktimer_settime: it_interval of " + std::to_string(interval.tv_nsec) + " ns is below 1 ms, where the kernel rounds it up to a clock tick that is not known");
    const GuestItimerspec old = Remaining(*timer);
    timer->interval = interval;
    if (IsSet(requested.value)) timer->value = (flags & TimerAbstime) != 0 ? requested.value : Add(requested.value, Now(timer->clock));
    else timer->value = {};
    if (previous != nullptr) *previous = old;
    Notify(state);
    return 0;
}

int APS5_VABI ktimer_gettime_nid_postfix(int timerid, GuestItimerspec* value) {
    auto& state = State();
    const std::lock_guard lock(state.lock);
    Timer* timer = Find(state, timerid);
    if (timer == nullptr) return Fail(GuestEinval);
    if (value == nullptr) return Fail(GuestEfault);
    *value = Remaining(*timer);
    return 0;
}

int APS5_VABI ktimer_getoverrun_nid_postfix(int timerid) {
    auto& state = State();
    const std::lock_guard lock(state.lock);
    Timer* timer = Find(state, timerid);
    if (timer == nullptr) return Fail(GuestEinval);
    return timer->overrunLast;
}

}
