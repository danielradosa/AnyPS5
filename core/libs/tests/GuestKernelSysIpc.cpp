#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/System/include/UserContext.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <source_location>
#include <stdexcept>

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
    return 0;
}
