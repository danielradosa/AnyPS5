#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "prx/libkernel/System/include/UserContext.hpp"
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <source_location>
#include <string>
#include <vector>
#include <stdexcept>
#include <thread>

struct TestSiginfo {
    std::int32_t signo;
    std::int32_t error;
    std::int32_t code;
    std::int32_t pid;
    std::uint32_t uid;
    std::int32_t status;
    std::uint64_t address;
    std::uint64_t value;
    std::int32_t reason[2];
    std::int32_t spare[8];
};
static_assert(sizeof(TestSiginfo) == 80 && offsetof(TestSiginfo, value) == 32 && offsetof(TestSiginfo, reason) == 40);

struct TestSigevent {
    std::int32_t notify;
    std::int32_t signo;
    std::uint64_t value;
    std::int32_t threadId;
    std::int32_t threadIdTail;
    std::uint64_t spare[7];
};
static_assert(sizeof(TestSigevent) == 80);

struct TestItimerspec {
    KernelTimespec interval;
    KernelTimespec value;
};

using SignalSet = std::array<std::uint32_t, 4>;
using SignalHandler = void (APS5_VABI*)(int);

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI getpid_nid_postfix();
int APS5_VABI kill_nid_postfix(int, int);
int APS5_VABI sigprocmask_nid_postfix(int, const void*, void*);
int APS5_VABI sigpending_nid_postfix(void*);
int APS5_VABI sigtimedwait_nid_postfix(const void*, void*, const KernelTimespec*);
SignalHandler APS5_VABI signal_nid_postfix(int, SignalHandler);
int APS5_VABI sigqueue_nid_postfix(int, int, void*);
int APS5_VABI clock_gettime_nid_postfix(int, KernelTimespec*);
int APS5_VABI pthread_getthreadid_np_nid_postfix();
int APS5_VABI ktimer_create_nid_postfix(int, const TestSigevent*, int*);
int APS5_VABI ktimer_delete_nid_postfix(int);
int APS5_VABI ktimer_settime_nid_postfix(int, int, const TestItimerspec*, TestItimerspec*);
int APS5_VABI ktimer_gettime_nid_postfix(int, TestItimerspec*);
int APS5_VABI ktimer_getoverrun_nid_postfix(int);
int APS5_VABI pipe_nid_postfix(int*);
int APS5_VABI pthread_create_nid_postfix(Pthread*, const PthreadAttr*, PthreadEntry, void*);
int APS5_VABI pthread_join_nid_postfix(Pthread, void**);
int APS5_VABI close_nid_postfix(int);
int APS5_VABI ksem_init_nid_postfix(std::intptr_t*, unsigned int);
int APS5_VABI ksem_open_nid_postfix(std::intptr_t*, const char*, int, std::uint16_t, unsigned int);
int APS5_VABI ksem_unlink_nid_postfix(const char*);
int APS5_VABI ksem_close_nid_postfix(std::intptr_t);
int APS5_VABI ksem_destroy_nid_postfix(std::intptr_t);
int APS5_VABI ksem_post_nid_postfix(std::intptr_t);
int APS5_VABI ksem_wait_nid_postfix(std::intptr_t);
int APS5_VABI ksem_trywait_nid_postfix(std::intptr_t);
int APS5_VABI ksem_timedwait_nid_postfix(std::intptr_t, const KernelTimespec*);
int APS5_VABI ksem_getvalue_nid_postfix(std::intptr_t, int*);
[[gnu::returns_twice]] int APS5_VABI getcontext_nid_postfix(UserUcontext*);
int APS5_VABI setcontext_nid_postfix(const UserUcontext*);
[[gnu::returns_twice]] int APS5_VABI swapcontext_nid_postfix(UserUcontext*, const UserUcontext*);
}

namespace {

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (!condition) {
        std::fprintf(stderr, "Kernel sys IPC check failed at line %u (guest errno %d)\n", location.line(), *__error_nid_postfix());
        std::abort();
    }
}

int Error() {
    return *__error_nid_postfix();
}

constexpr int SigBlock = 1;
constexpr int SigSetmask = 3;
constexpr int Sigusr1 = 30;
constexpr int Sigusr2 = 31;
constexpr std::int32_t SiQueue = 0x10002;
const KernelTimespec Poll{0, 0};

SignalSet Only(int signal) {
    SignalSet set{};
    set[static_cast<std::size_t>(signal - 1) / 32] = 1u << ((signal - 1) % 32);
    return set;
}

SignalSet BlockOnly(int signal) {
    SignalSet previous{};
    const SignalSet set = Only(signal);
    Require(sigprocmask_nid_postfix(SigBlock, set.data(), previous.data()) == 0);
    return previous;
}

void RestoreMask(const SignalSet& previous) {
    Require(sigprocmask_nid_postfix(SigSetmask, previous.data(), nullptr) == 0);
}

int TakeSignal(int signal, TestSiginfo* info) {
    const SignalSet set = Only(signal);
    return sigtimedwait_nid_postfix(set.data(), info, &Poll);
}

volatile int g_handled = 0;
void APS5_VABI CountSignal(int signal) {
    g_handled = signal;
}

void CheckSigqueue() {
    const int self = getpid_nid_postfix();
    Require(sigqueue_nid_postfix(self, 129, nullptr) == -1 && Error() == 22);
    Require(sigqueue_nid_postfix(self, -1, nullptr) == -1 && Error() == 22);
    Require(sigqueue_nid_postfix(0, Sigusr1, nullptr) == -1 && Error() == 22);
    Require(sigqueue_nid_postfix(-self, Sigusr1, nullptr) == -1 && Error() == 22);
    Require(sigqueue_nid_postfix(self + 1, Sigusr1, nullptr) == -1 && Error() == 3);
    Require(sigqueue_nid_postfix(self + 1, 0, nullptr) == -1 && Error() == 3);
    Require(sigqueue_nid_postfix(self, 0, nullptr) == 0);
    const SignalSet previous = BlockOnly(Sigusr1);
    Require(sigqueue_nid_postfix(self, Sigusr1, reinterpret_cast<void*>(0x1234)) == 0);
    Require(sigqueue_nid_postfix(self, Sigusr1, reinterpret_cast<void*>(0x5678)) == 0);
    SignalSet pending{};
    Require(sigpending_nid_postfix(pending.data()) == 0 && pending == Only(Sigusr1));
    TestSiginfo info{};
    Require(TakeSignal(Sigusr1, &info) == Sigusr1);
    Require(info.signo == Sigusr1 && info.code == SiQueue && info.pid == self && info.value == 0x1234 && info.reason[0] == 0);
    Require(TakeSignal(Sigusr1, &info) == Sigusr1 && info.code == SiQueue && info.value == 0x5678);
    Require(TakeSignal(Sigusr1, &info) == -1 && Error() == 35);
    Require(kill_nid_postfix(self, Sigusr1) == 0 && sigqueue_nid_postfix(self, Sigusr1, reinterpret_cast<void*>(7)) == 0);
    Require(TakeSignal(Sigusr1, &info) == Sigusr1 && info.code == 0 && info.value == 0);
    Require(TakeSignal(Sigusr1, &info) == Sigusr1 && info.code == SiQueue && info.value == 7);
    Require(TakeSignal(Sigusr1, nullptr) == -1 && Error() == 35);
    for (std::uintptr_t index = 0; index < 128; ++index)
        Require(sigqueue_nid_postfix(self, Sigusr1, reinterpret_cast<void*>(index)) == 0);
    Require(sigqueue_nid_postfix(self, Sigusr1, nullptr) == -1 && Error() == 35);
    for (std::uint64_t index = 0; index < 128; ++index)
        Require(TakeSignal(Sigusr1, &info) == Sigusr1 && info.value == index);
    Require(TakeSignal(Sigusr1, nullptr) == -1 && Error() == 35);
    RestoreMask(previous);
    Require(signal_nid_postfix(Sigusr2, CountSignal) == nullptr);
    g_handled = 0;
    Require(sigqueue_nid_postfix(self, Sigusr2, reinterpret_cast<void*>(9)) == 0 && g_handled == Sigusr2);
    Require(signal_nid_postfix(Sigusr2, nullptr) == CountSignal);
    Require(signal_nid_postfix(Sigusr1, reinterpret_cast<SignalHandler>(1)) == nullptr);
    Require(sigqueue_nid_postfix(self, Sigusr1, nullptr) == 0);
    Require(signal_nid_postfix(Sigusr1, nullptr) == reinterpret_cast<SignalHandler>(1));
    Require(sigpending_nid_postfix(pending.data()) == 0 && pending == SignalSet{});
}

template <typename F>
bool Throws(F call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

constexpr int Sigalrm = 14;
constexpr int ClockRealtime = 0;
constexpr int ClockMonotonic = 4;
constexpr int TimerAbstime = 1;
constexpr std::int32_t SiTimer = 0x10003;
const KernelTimespec Patience{2, 0};

bool Zero(const KernelTimespec& time) {
    return time.tv_sec == 0 && time.tv_nsec == 0;
}

bool Within(const KernelTimespec& time, std::int64_t lowNanos, std::int64_t highNanos) {
    const std::int64_t nanos = time.tv_sec * 1000000000 + time.tv_nsec;
    return nanos > lowNanos && nanos <= highNanos;
}

int WaitSignal(int signal, TestSiginfo* info) {
    const SignalSet set = Only(signal);
    return sigtimedwait_nid_postfix(set.data(), info, &Patience);
}

void Pause(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void CheckTimerArguments() {
    int id = -1;
    for (const int clock : {-1, 1, 2, 3, 5, 7})
        Require(ktimer_create_nid_postfix(clock, nullptr, &id) == -1 && Error() == 22);
    for (const TestSigevent& event : {TestSigevent{2, Sigusr1}, TestSigevent{3, Sigusr1}, TestSigevent{5, Sigusr1}, TestSigevent{1, 0},
                                      TestSigevent{1, 129}, TestSigevent{4, -1}})
        Require(ktimer_create_nid_postfix(ClockMonotonic, &event, &id) == -1 && Error() == 22);
    Require(ktimer_create_nid_postfix(ClockMonotonic, nullptr, nullptr) == -1 && Error() == 14);
    const TestSigevent unusedSignal{0, 999};
    Require(ktimer_create_nid_postfix(ClockRealtime, &unusedSignal, &id) == 0 && id >= 3 && id < 32);
    Require(ktimer_delete_nid_postfix(id) == 0);
    Require(ktimer_create_nid_postfix(ClockMonotonic, nullptr, &id) == 0 && id >= 3 && id < 32);
    TestItimerspec spec{{9, 9}, {9, 9}};
    Require(ktimer_gettime_nid_postfix(id, &spec) == 0 && Zero(spec.interval) && Zero(spec.value));
    Require(ktimer_gettime_nid_postfix(id, nullptr) == -1 && Error() == 14);
    Require(ktimer_getoverrun_nid_postfix(id) == 0);
    Require(ktimer_settime_nid_postfix(id, 0, nullptr, nullptr) == -1 && Error() == 14);
    for (const TestItimerspec& bad : {TestItimerspec{{0, 0}, {0, 1000000000}}, TestItimerspec{{0, 0}, {-1, 0}}, TestItimerspec{{0, 0}, {0, -1}},
                                      TestItimerspec{{0, -1}, {1, 0}}, TestItimerspec{{-1, 0}, {1, 0}}, TestItimerspec{{0, 1000000000}, {1, 0}}})
        Require(ktimer_settime_nid_postfix(id, 0, &bad, nullptr) == -1 && Error() == 22);
    const TestItimerspec ignoredInterval{{0, -1}, {0, 0}};
    Require(ktimer_settime_nid_postfix(id, 0, &ignoredInterval, &spec) == 0 && Zero(spec.value) && Zero(spec.interval));
    Require(ktimer_gettime_nid_postfix(id, &spec) == 0 && Zero(spec.interval) && Zero(spec.value));
    const TestItimerspec tiny{{0, 500000}, {1, 0}};
    Require(Throws([&] { ktimer_settime_nid_postfix(id, 0, &tiny, nullptr); }));
    Require(ktimer_gettime_nid_postfix(id, &spec) == 0 && Zero(spec.value));
    for (const int bogus : {-1, 0, 1, 2, 32, 1000}) {
        Require(ktimer_gettime_nid_postfix(bogus, &spec) == -1 && Error() == 22);
        Require(ktimer_settime_nid_postfix(bogus, 0, &ignoredInterval, nullptr) == -1 && Error() == 22);
        Require(ktimer_getoverrun_nid_postfix(bogus) == -1 && Error() == 22);
        Require(ktimer_delete_nid_postfix(bogus) == -1 && Error() == 22);
    }
    Require(ktimer_settime_nid_postfix(1, 0, nullptr, nullptr) == -1 && Error() == 14);
    Require(ktimer_delete_nid_postfix(id) == 0 && ktimer_delete_nid_postfix(id) == -1 && Error() == 22);
    Require(ktimer_gettime_nid_postfix(id, &spec) == -1 && Error() == 22);
    int ids[32] = {};
    int created = 0;
    while (created < 32 && ktimer_create_nid_postfix(ClockRealtime, &unusedSignal, &ids[created]) == 0) ++created;
    Require(created == 29 && Error() == 35);
    for (int index = 0; index < created; ++index) Require(ids[index] >= 3 && ktimer_delete_nid_postfix(ids[index]) == 0);
}

void CheckQuietTimers() {
    const TestSigevent none{0};
    int id = -1;
    Require(ktimer_create_nid_postfix(ClockMonotonic, &none, &id) == 0);
    const TestItimerspec ten{{2, 0}, {10, 0}};
    TestItimerspec old{{9, 9}, {9, 9}};
    Require(ktimer_settime_nid_postfix(id, 0, &ten, &old) == 0 && Zero(old.interval) && Zero(old.value));
    TestItimerspec spec{};
    Require(ktimer_gettime_nid_postfix(id, &spec) == 0 && spec.interval.tv_sec == 2 && spec.interval.tv_nsec == 0);
    Require(Within(spec.value, 9000000000, 10000000000));
    KernelTimespec now{};
    Require(clock_gettime_nid_postfix(ClockMonotonic, &now) == 0);
    const TestItimerspec absolute{{0, 0}, {now.tv_sec + 5, now.tv_nsec}};
    Require(ktimer_settime_nid_postfix(id, TimerAbstime, &absolute, &old) == 0);
    Require(old.interval.tv_sec == 2 && Within(old.value, 9000000000, 10000000000));
    Require(ktimer_gettime_nid_postfix(id, &spec) == 0 && Zero(spec.interval) && Within(spec.value, 4000000000, 5000000000));
    const TestItimerspec past{{0, 0}, {1, 0}};
    Require(ktimer_settime_nid_postfix(id, TimerAbstime, &past, nullptr) == 0);
    Require(ktimer_gettime_nid_postfix(id, &spec) == 0 && Zero(spec.value) && Zero(spec.interval));
    const TestItimerspec brief{{0, 0}, {0, 2000000}};
    Require(ktimer_settime_nid_postfix(id, 0, &brief, nullptr) == 0);
    Pause(30);
    Require(ktimer_gettime_nid_postfix(id, &spec) == 0 && Zero(spec.value));
    const TestItimerspec periodic{{0, 10000000}, {0, 10000000}};
    Require(ktimer_settime_nid_postfix(id, 0, &periodic, nullptr) == 0);
    Pause(35);
    Require(ktimer_gettime_nid_postfix(id, &spec) == 0 && spec.interval.tv_nsec == 10000000 && Within(spec.value, 0, 10000000));
    Require(ktimer_getoverrun_nid_postfix(id) == 0);
    Require(ktimer_delete_nid_postfix(id) == 0);
}

void CheckSignalTimers() {
    SignalSet blocked = Only(Sigusr1);
    blocked[0] |= Only(Sigalrm)[0];
    SignalSet previous{};
    Require(sigprocmask_nid_postfix(SigBlock, blocked.data(), previous.data()) == 0);
    const TestSigevent signal{1, Sigusr1, 0xabcdef};
    int id = -1;
    Require(ktimer_create_nid_postfix(ClockMonotonic, &signal, &id) == 0);
    const TestItimerspec once{{0, 0}, {0, 20000000}};
    Require(ktimer_settime_nid_postfix(id, 0, &once, nullptr) == 0);
    TestSiginfo info{};
    Require(WaitSignal(Sigusr1, &info) == Sigusr1);
    Require(info.signo == Sigusr1 && info.code == SiTimer && info.pid == 0 && info.value == 0xabcdef && info.reason[0] == id && info.reason[1] == 0);
    TestItimerspec spec{};
    Require(ktimer_gettime_nid_postfix(id, &spec) == 0 && Zero(spec.value) && ktimer_getoverrun_nid_postfix(id) == 0);
    const TestItimerspec fast{{0, 5000000}, {0, 5000000}};
    Require(ktimer_settime_nid_postfix(id, 0, &fast, nullptr) == 0);
    Pause(120);
    const TestItimerspec stop{};
    Require(ktimer_settime_nid_postfix(id, 0, &stop, &spec) == 0 && spec.interval.tv_nsec == 5000000);
    Require(TakeSignal(Sigusr1, &info) == Sigusr1 && info.code == SiTimer && info.reason[0] == id && info.reason[1] > 5);
    Require(ktimer_getoverrun_nid_postfix(id) == info.reason[1]);
    Require(TakeSignal(Sigusr1, nullptr) == -1 && Error() == 35);
    Require(ktimer_settime_nid_postfix(id, 0, &once, nullptr) == 0);
    Pause(80);
    SignalSet pending{};
    Require(sigpending_nid_postfix(pending.data()) == 0 && pending == Only(Sigusr1));
    Require(ktimer_delete_nid_postfix(id) == 0);
    Require(sigpending_nid_postfix(pending.data()) == 0 && pending == SignalSet{});
    Require(TakeSignal(Sigusr1, nullptr) == -1 && Error() == 35);
    Require(ktimer_create_nid_postfix(ClockRealtime, nullptr, &id) == 0);
    Require(ktimer_settime_nid_postfix(id, 0, &once, nullptr) == 0);
    Require(WaitSignal(Sigalrm, &info) == Sigalrm && info.code == SiTimer && info.value == static_cast<std::uint64_t>(id) && info.reason[0] == id);
    Require(ktimer_delete_nid_postfix(id) == 0);
    const TestSigevent self{4, Sigusr1, 5, pthread_getthreadid_np_nid_postfix()};
    Require(ktimer_create_nid_postfix(ClockMonotonic, &self, &id) == 0);
    Require(ktimer_settime_nid_postfix(id, 0, &once, nullptr) == 0);
    Require(WaitSignal(Sigusr1, &info) == Sigusr1 && info.code == SiTimer && info.value == 5 && info.reason[0] == id);
    Require(ktimer_delete_nid_postfix(id) == 0);
    const TestSigevent gone{4, Sigusr1, 6, 0x7ffffff0};
    Require(ktimer_create_nid_postfix(ClockMonotonic, &gone, &id) == 0);
    Require(ktimer_settime_nid_postfix(id, 0, &fast, nullptr) == 0);
    Pause(60);
    Require(ktimer_gettime_nid_postfix(id, &spec) == 0 && Zero(spec.value) && Zero(spec.interval));
    Require(TakeSignal(Sigusr1, nullptr) == -1 && Error() == 35);
    Require(ktimer_delete_nid_postfix(id) == 0);
    Require(signal_nid_postfix(Sigusr1, reinterpret_cast<SignalHandler>(1)) == nullptr);
    Require(ktimer_create_nid_postfix(ClockMonotonic, &signal, &id) == 0);
    Require(ktimer_settime_nid_postfix(id, 0, &once, nullptr) == 0);
    RestoreMask(previous);
    Pause(60);
    Require(ktimer_delete_nid_postfix(id) == 0);
    Require(signal_nid_postfix(Sigusr1, nullptr) == reinterpret_cast<SignalHandler>(1));
}

int KernelSemaphoreValue(std::intptr_t id) {
    int value = -1;
    Require(ksem_getvalue_nid_postfix(id, &value) == 0);
    return value;
}

struct KernelSemaphoreWaiter {
    std::intptr_t id;
    int result;
};

void* APS5_VABI WaitKernelSemaphore(void* argument) {
    auto* waiter = static_cast<KernelSemaphoreWaiter*>(argument);
    waiter->result = ksem_wait_nid_postfix(waiter->id);
    return nullptr;
}

void CheckAnonymousKernelSemaphores() {
    std::intptr_t id = -1;
    Require(ksem_init_nid_postfix(&id, 0x80000000u) == -1 && Error() == 22);
    Require(ksem_init_nid_postfix(nullptr, 1) == -1 && Error() == 14);
    Require(ksem_init_nid_postfix(&id, 2) == 0 && id >= 0 && KernelSemaphoreValue(id) == 2);
    Require(ksem_trywait_nid_postfix(id) == 0 && ksem_trywait_nid_postfix(id) == 0);
    Require(ksem_trywait_nid_postfix(id) == -1 && Error() == 35);
    Require(ksem_post_nid_postfix(id) == 0 && KernelSemaphoreValue(id) == 1 && ksem_wait_nid_postfix(id) == 0);
    Require(ksem_getvalue_nid_postfix(id, nullptr) == -1 && Error() == 14);
    const KernelTimespec invalid{0, 1000000000};
    Require(ksem_timedwait_nid_postfix(id, &invalid) == -1 && Error() == 22);
    Require(ksem_timedwait_nid_postfix(-5, &invalid) == -1 && Error() == 22);
    const KernelTimespec past{1, 0};
    Require(ksem_timedwait_nid_postfix(id, &past) == -1 && Error() == 60);
    KernelTimespec soon{};
    Require(clock_gettime_nid_postfix(ClockRealtime, &soon) == 0);
    soon.tv_nsec += 30000000;
    if (soon.tv_nsec >= 1000000000) {
        ++soon.tv_sec;
        soon.tv_nsec -= 1000000000;
    }
    Require(ksem_timedwait_nid_postfix(id, &soon) == -1 && Error() == 60);
    Require(ksem_post_nid_postfix(id) == 0 && ksem_timedwait_nid_postfix(id, &past) == 0);
    Require(ksem_post_nid_postfix(id) == 0 && ksem_timedwait_nid_postfix(id, nullptr) == 0);
    Require(ksem_close_nid_postfix(id) == -1 && Error() == 22);
    KernelSemaphoreWaiter waiter{id, -1};
    Pthread thread = nullptr;
    Require(pthread_create_nid_postfix(&thread, nullptr, WaitKernelSemaphore, &waiter) == 0);
    Pause(50);
    Require(ksem_destroy_nid_postfix(id) == -1 && Error() == 16);
    Require(ksem_post_nid_postfix(id) == 0);
    Require(pthread_join_nid_postfix(thread, nullptr) == 0);
    Require(waiter.result == 0 && KernelSemaphoreValue(id) == 0);
    Require(ksem_destroy_nid_postfix(id) == 0 && ksem_destroy_nid_postfix(id) == -1 && Error() == 22);
    int value = 0;
    Require(ksem_getvalue_nid_postfix(id, &value) == -1 && Error() == 22);
    Require(ksem_post_nid_postfix(id) == -1 && Error() == 22 && ksem_wait_nid_postfix(id) == -1 && Error() == 22);
    Require(ksem_init_nid_postfix(&id, 0x7fffffff) == 0 && ksem_post_nid_postfix(id) == -1 && Error() == 84);
    Require(ksem_destroy_nid_postfix(id) == 0);
    Require(ksem_open_nid_postfix(&id, nullptr, 0, 0, 1) == 0 && KernelSemaphoreValue(id) == 1);
    Require(ksem_close_nid_postfix(id) == -1 && Error() == 22 && ksem_destroy_nid_postfix(id) == 0);
    int streams[2];
    Require(pipe_nid_postfix(streams) == 0);
    Require(ksem_post_nid_postfix(streams[0]) == -1 && Error() == 22 && ksem_destroy_nid_postfix(streams[1]) == -1 && Error() == 22);
    Require(close_nid_postfix(streams[0]) == 0 && close_nid_postfix(streams[1]) == 0);
}

void CheckNamedKernelSemaphores() {
    constexpr int Create = SCE_KERNEL_O_CREAT;
    constexpr int Exclusive = SCE_KERNEL_O_EXCL;
    const char* name = "/anyps5-ksem";
    std::intptr_t first = -1;
    std::intptr_t second = -1;
    std::intptr_t third = -1;
    Require(ksem_open_nid_postfix(&first, name, 0, 0, 0) == -1 && Error() == 2);
    Require(ksem_open_nid_postfix(&first, name, SCE_KERNEL_O_RDWR, 0, 0) == -1 && Error() == 22);
    Require(ksem_open_nid_postfix(&first, name, Create, 0644, 0x80000000u) == -1 && Error() == 22);
    Require(ksem_open_nid_postfix(&first, "anyps5-ksem", Create, 0644, 1) == -1 && Error() == 22);
    const std::string longName = "/" + std::string(1023, 'k');
    Require(ksem_open_nid_postfix(&first, longName.c_str(), Create, 0644, 1) == -1 && Error() == 63);
    Require(ksem_unlink_nid_postfix(longName.c_str()) == -1 && Error() == 63);
    Require(ksem_unlink_nid_postfix(nullptr) == -1 && Error() == 14);
    Require(ksem_open_nid_postfix(&first, name, Create, 0644, 1) == 0);
    Require(ksem_open_nid_postfix(&second, name, Create, 0644, 7) == 0 && second != first && KernelSemaphoreValue(second) == 1);
    Require(ksem_post_nid_postfix(second) == 0 && KernelSemaphoreValue(first) == 2);
    Require(ksem_open_nid_postfix(&third, name, Create | Exclusive, 0644, 1) == -1 && Error() == 17);
    Require(ksem_destroy_nid_postfix(first) == -1 && Error() == 22);
    Require(ksem_close_nid_postfix(first) == 0 && ksem_close_nid_postfix(first) == -1 && Error() == 22);
    Require(ksem_unlink_nid_postfix(name) == 0 && ksem_unlink_nid_postfix(name) == -1 && Error() == 2);
    Require(ksem_unlink_nid_postfix("anyps5-ksem") == -1 && Error() == 2);
    Require(KernelSemaphoreValue(second) == 2 && ksem_open_nid_postfix(&third, name, 0, 0, 0) == -1 && Error() == 2);
    Require(ksem_open_nid_postfix(&third, name, Create | Exclusive, 0600, 0) == 0 && KernelSemaphoreValue(third) == 0);
    Require(close_nid_postfix(static_cast<int>(third)) == 0);
    Require(ksem_post_nid_postfix(third) == -1 && Error() == 22);
    Require(ksem_open_nid_postfix(&third, name, 0, 0, 0) == 0 && KernelSemaphoreValue(third) == 0);
    Require(ksem_close_nid_postfix(third) == 0 && ksem_close_nid_postfix(second) == 0 && ksem_unlink_nid_postfix(name) == 0);
    std::vector<std::intptr_t> held;
    std::intptr_t id = -1;
    while (held.size() < 40 && ksem_init_nid_postfix(&id, 0) == 0) held.push_back(id);
    Require(held.size() == 30 && Error() == 28);
    Require(ksem_open_nid_postfix(&id, name, Create, 0644, 0) == -1 && Error() == 23);
    Require(ksem_destroy_nid_postfix(held.back()) == 0);
    held.pop_back();
    Require(ksem_open_nid_postfix(&id, name, Create, 0644, 0) == 0 && ksem_unlink_nid_postfix(name) == 0);
    Require(ksem_init_nid_postfix(&first, 0) == -1 && Error() == 28 && ksem_close_nid_postfix(id) == 0);
    for (const std::intptr_t semaphore : held) Require(ksem_destroy_nid_postfix(semaphore) == 0);
}

UserUcontext g_swapMain;
UserUcontext g_swapOther;
volatile int g_swapPhase = 0;

[[gnu::noinline]] void CheckSwapcontext() {
    UserUcontext scratch{};
    Require(swapcontext_nid_postfix(nullptr, &scratch) == -1 && Error() == 22);
    Require(swapcontext_nid_postfix(&scratch, nullptr) == -1 && Error() == 22);
    const UserUcontext invalid{};
    Require(swapcontext_nid_postfix(&scratch, &invalid) == -1 && Error() == 22);
    Require(scratch.mcontext.len == sizeof(UserMcontext) && scratch.mcontext.rip != 0 && scratch.mcontext.rax == 0);
    g_swapPhase = 0;
    Require(getcontext_nid_postfix(&g_swapOther) == 0);
    if (g_swapPhase == 1) {
        g_swapPhase = 2;
        setcontext_nid_postfix(&g_swapMain);
        std::abort();
    }
    g_swapPhase = 1;
    Require(swapcontext_nid_postfix(&g_swapMain, &g_swapOther) == 0);
    Require(g_swapPhase == 2);
    Require(g_swapMain.mcontext.len == sizeof(UserMcontext) && g_swapMain.mcontext.rax == 0 && g_swapMain.mcontext.rdx == 0);
}

}

int main() {
    CheckSwapcontext();
    CheckSigqueue();
    CheckTimerArguments();
    CheckQuietTimers();
    CheckSignalTimers();
    CheckAnonymousKernelSemaphores();
    CheckNamedKernelSemaphores();
    return 0;
}
