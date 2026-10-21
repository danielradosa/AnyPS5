#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/System/include/UserContext.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <source_location>
#include <stdexcept>

extern "C" {
int* APS5_VABI __error_nid_postfix();
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
    return 0;
}
