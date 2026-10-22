#include <cstdint>
#include <stdexcept>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/Pthread/Posix/Common.hpp"
#include "prx/libkernel/Pthread/include/Pthread.hpp"
#include "prx/libkernel/Pthread/include/Rwlock.hpp"

extern "C" int* APS5_VABI __error_nid_postfix();

namespace {

constexpr int GuestEinval = 22;

}

extern "C" {

int APS5_VABI pthread_condattr_getclock_nid_postfix(const PthreadCondattr* attr, KernelClockid* clockId);
int APS5_VABI pthread_mutex_init_nid_postfix(PthreadMutex* mutex, const PthreadMutexattr* attr);
int APS5_VABI pthread_mutexattr_init_nid_postfix(PthreadMutexattr* attr);

int APS5_VABI scePthreadCondattrGetclock_nid_postfix(const PthreadCondattr* attr, KernelClockid* clockId) {
    return PosixThread::ToSce(pthread_condattr_getclock_nid_postfix(attr, clockId));
}

int APS5_VABI scePthreadMutexInitForInternalLibc(PthreadMutex* mutex, const PthreadMutexattr* attr) {
    return PosixThread::ToSce(pthread_mutex_init_nid_postfix(mutex, attr));
}

int APS5_VABI scePthreadMutexattrInitForInternalLibc(PthreadMutexattr* attr) {
    return PosixThread::ToSce(pthread_mutexattr_init_nid_postfix(attr));
}

int APS5_VABI pthread_timedjoin_np_nid_postfix(Pthread thread, void** value, const KernelTimespec* abstime) {
    if (!abstime || abstime->tv_sec < 0 || abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000) return GuestEinval;
    return PthreadTimedJoin_nid_no_patch(thread, value, abstime);
}

int APS5_VABI pthread_rwlock_reltimedrdlock_np_nid_postfix(PthreadRwlock* rwlock, KernelUseconds usec) {
    return PosixThread::ToErrno(scePthreadRwlockTimedrdlock(rwlock, usec));
}

int APS5_VABI pthread_rwlock_reltimedwrlock_np_nid_postfix(PthreadRwlock* rwlock, KernelUseconds usec) {
    return PosixThread::ToErrno(scePthreadRwlockTimedwrlock(rwlock, usec));
}

int APS5_VABI pthread_rwlockattr_settype_np_nid_postfix(PthreadRwlockattr* attr, int type) {
    if (!attr || !*attr) return GuestEinval;
    return PosixThread::ToErrno(scePthreadRwlockattrSettype(attr, type));
}

int APS5_VABI pthread_mutex_setname_np_nid_postfix(PthreadMutex* mutex, const char*) {
    if (!mutex) throw std::invalid_argument("pthread_mutex_setname_np: null mutex");
    return 0;
}

int APS5_VABI pthread_cond_setname_np_nid_postfix(PthreadCond* cond, const char* name) {
    if (!cond) throw std::invalid_argument("pthread_cond_setname_np: null condition variable");
    return name ? 0 : GuestEinval;
}

int APS5_VABI pthread_rwlock_setname_np_nid_postfix(PthreadRwlock* rwlock, const char* name) {
    if (!rwlock) throw std::invalid_argument("pthread_rwlock_setname_np: null rwlock");
    return name ? 0 : GuestEinval;
}

int APS5_VABI sem_setname_nid_postfix(PthreadSem* sem, const char* name) {
    if (!sem) throw std::invalid_argument("sem_setname: null semaphore");
    if (name) return 0;
    *__error_nid_postfix() = GuestEinval;
    return -1;
}

}
