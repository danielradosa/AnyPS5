#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/Pthread/Posix/Common.hpp"

extern "C" {

int APS5_VABI pthread_condattr_getclock_nid_postfix(const PthreadCondattr* attr, KernelClockid* clockId);

int APS5_VABI scePthreadCondattrGetclock_nid_postfix(const PthreadCondattr* attr, KernelClockid* clockId) {
    return PosixThread::ToSce(pthread_condattr_getclock_nid_postfix(attr, clockId));
}

}
