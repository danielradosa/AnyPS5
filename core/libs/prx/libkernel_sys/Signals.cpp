#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/System/include/SignalQueue.hpp"
#include <cstdint>

extern "C" {

int* APS5_VABI __error_nid_postfix();
int APS5_VABI getpid_nid_postfix();

int APS5_VABI sigqueue_nid_postfix(int pid, int signo, void* value) {
    constexpr int GuestEsrch = 3;
    constexpr int GuestEinval = 22;
    constexpr unsigned MaxSignal = 128;
    if (static_cast<unsigned>(signo) > MaxSignal || pid <= 0) {
        *__error_nid_postfix() = GuestEinval;
        return -1;
    }
    const int self = getpid_nid_postfix();
    if (pid != self) {
        *__error_nid_postfix() = GuestEsrch;
        return -1;
    }
    if (signo == 0) return 0;
    const GuestSignalRecord record{GuestSiQueue, self, reinterpret_cast<std::uintptr_t>(value), {0, 0}, 0, nullptr};
    return GuestSignalQueue_nid_no_patch(signo, &record);
}

}
