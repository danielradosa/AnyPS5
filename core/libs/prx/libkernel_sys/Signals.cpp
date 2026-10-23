#include "prx/libc/include/general/VabiMacros.hpp"

extern "C" {

int APS5_VABI GuestSigqueue_nid_no_patch(int pid, int signo, void* value);

int APS5_VABI sigqueue_nid_postfix(int pid, int signo, void* value) {
    return GuestSigqueue_nid_no_patch(pid, signo, value);
}

}
