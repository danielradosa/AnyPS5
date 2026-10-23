#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace {

constexpr std::size_t SignalSetWords = 4;
constexpr int SigSetmask = 3;

}

extern "C" {

int* APS5_VABI __error_nid_postfix();
int APS5_VABI select_nid_postfix(int nfds, void* readfds, void* writefds, void* exceptfds, const void* timeout);
int APS5_VABI sigprocmask_nid_postfix(int how, const void* set, void* previousSet);

int APS5_VABI pselect_nid_postfix(int nfds, void* readfds, void* writefds, void* exceptfds, const KernelTimespec* timeout, const void* mask) {
    KernelTimeval limit{};
    if (timeout != nullptr) {
        limit.tv_sec = timeout->tv_sec;
        limit.tv_usec = timeout->tv_nsec / 1000;
    }
    std::array<std::uint32_t, SignalSetWords> previous{};
    if (mask != nullptr && sigprocmask_nid_postfix(SigSetmask, mask, previous.data()) != 0) return -1;
    const int result = select_nid_postfix(nfds, readfds, writefds, exceptfds, timeout != nullptr ? &limit : nullptr);
    if (mask != nullptr) {
        const int saved = *__error_nid_postfix();
        if (sigprocmask_nid_postfix(SigSetmask, previous.data(), nullptr) != 0)
            throw std::runtime_error("pselect: restoring the signal mask failed");
        *__error_nid_postfix() = saved;
    }
    return result;
}

}
