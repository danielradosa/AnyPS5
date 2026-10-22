#include "SceTypes.hpp"
#include <cstdlib>
#include <initializer_list>
#include <stdexcept>

extern "C" {
int APS5_VABI pthread_condattr_init_nid_postfix(PthreadCondattr* attr);
int APS5_VABI pthread_condattr_destroy_nid_postfix(PthreadCondattr* attr);
int APS5_VABI pthread_condattr_setclock_nid_postfix(PthreadCondattr* attr, KernelClockid clock_id);
int APS5_VABI pthread_condattr_getclock_nid_postfix(const PthreadCondattr* attr, KernelClockid* clock_id);
int APS5_VABI pthread_condattr_setpshared_nid_postfix(PthreadCondattr* attr, int pshared);
int APS5_VABI pthread_condattr_getpshared_nid_postfix(const PthreadCondattr* attr, int* pshared);

int APS5_VABI scePthreadCondattrInit(PthreadCondattr* attr);
int APS5_VABI scePthreadCondattrDestroy(PthreadCondattr* attr);
int APS5_VABI scePthreadCondattrSetclock(PthreadCondattr* attr, KernelClockid clockId);
int APS5_VABI scePthreadCondattrGetclock(const PthreadCondattr* attr, KernelClockid* clockId);
int APS5_VABI scePthreadCondattrSetpshared(PthreadCondattr* attr, int pshared);
int APS5_VABI scePthreadCondattrGetpshared(const PthreadCondattr* attr, int* pshared);
}

namespace {

constexpr int GUEST_EINVAL = 22;
constexpr int PROCESS_PRIVATE = 0;
constexpr int PROCESS_SHARED = 1;
constexpr KernelClockid MONOTONIC = 4;
constexpr int SCE_OK = 0;
constexpr int SCE_KERNEL_ERROR_EINVAL = static_cast<int>(0x80020016u);

void Require(bool value) {
    if (!value) std::abort();
}

template <typename Call>
bool Throws(Call call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

void CheckPosixClock() {
    PthreadCondattr attr = nullptr;
    Require(pthread_condattr_init_nid_postfix(&attr) == 0);
    KernelClockid clockId = -1;
    Require(pthread_condattr_getclock_nid_postfix(&attr, &clockId) == 0 && clockId == 0);
    for (const KernelClockid set : {1, 2, 0, MONOTONIC, 5, 12}) {
        Require(pthread_condattr_setclock_nid_postfix(&attr, set) == 0);
        Require(pthread_condattr_getclock_nid_postfix(&attr, &clockId) == 0 && clockId == set);
    }
    Require(Throws([&] { pthread_condattr_getclock_nid_postfix(&attr, nullptr); }));
    Require(pthread_condattr_destroy_nid_postfix(&attr) == 0);
    clockId = -1;
    Require(pthread_condattr_getclock_nid_postfix(&attr, &clockId) == GUEST_EINVAL && clockId == -1);
    Require(pthread_condattr_getclock_nid_postfix(nullptr, &clockId) == GUEST_EINVAL && clockId == -1);
    Require(pthread_condattr_setclock_nid_postfix(&attr, 0) == GUEST_EINVAL);
    Require(pthread_condattr_setclock_nid_postfix(nullptr, 0) == GUEST_EINVAL);
}

void CheckPosixPshared() {
    PthreadCondattr attr = nullptr;
    Require(pthread_condattr_init_nid_postfix(&attr) == 0);
    int pshared = -1;
    Require(pthread_condattr_getpshared_nid_postfix(&attr, &pshared) == 0 && pshared == PROCESS_PRIVATE);
    for (const int invalid : {PROCESS_SHARED, 2, -1}) {
        Require(pthread_condattr_setpshared_nid_postfix(&attr, invalid) == GUEST_EINVAL);
        pshared = -1;
        Require(pthread_condattr_getpshared_nid_postfix(&attr, &pshared) == 0 && pshared == PROCESS_PRIVATE);
    }
    Require(pthread_condattr_setpshared_nid_postfix(&attr, PROCESS_PRIVATE) == 0);
    Require(pthread_condattr_getpshared_nid_postfix(&attr, &pshared) == 0 && pshared == PROCESS_PRIVATE);

    Require(Throws([&] { pthread_condattr_getpshared_nid_postfix(&attr, nullptr); }));
    Require(pthread_condattr_destroy_nid_postfix(&attr) == 0);
    pshared = -1;
    Require(pthread_condattr_getpshared_nid_postfix(&attr, &pshared) == GUEST_EINVAL && pshared == -1);
    Require(pthread_condattr_getpshared_nid_postfix(nullptr, &pshared) == GUEST_EINVAL && pshared == -1);
    Require(pthread_condattr_setpshared_nid_postfix(&attr, PROCESS_PRIVATE) == GUEST_EINVAL);
    Require(pthread_condattr_setpshared_nid_postfix(nullptr, PROCESS_PRIVATE) == GUEST_EINVAL);
}

void CheckSceCondattr() {
    PthreadCondattr attr = nullptr;
    Require(Throws([&] { scePthreadCondattrInit(nullptr); }));
    Require(scePthreadCondattrInit(&attr) == SCE_OK);

    KernelClockid clockId = -1;
    Require(scePthreadCondattrGetclock(&attr, &clockId) == SCE_OK && clockId == 0);
    for (const KernelClockid set : {0, 2, MONOTONIC, 5, 12, 1}) Require(scePthreadCondattrSetclock(&attr, set) == SCE_OK);
    Require(scePthreadCondattrGetclock(&attr, &clockId) == SCE_OK && clockId == 1);
    Require(Throws([&] { scePthreadCondattrGetclock(&attr, nullptr); }));

    int pshared = -1;
    Require(scePthreadCondattrGetpshared(&attr, &pshared) == SCE_OK && pshared == PROCESS_PRIVATE);
    for (const int invalid : {PROCESS_SHARED, 2, -1}) Require(scePthreadCondattrSetpshared(&attr, invalid) == SCE_KERNEL_ERROR_EINVAL);
    Require(scePthreadCondattrSetpshared(&attr, PROCESS_PRIVATE) == SCE_OK);
    Require(scePthreadCondattrGetpshared(&attr, &pshared) == SCE_OK && pshared == PROCESS_PRIVATE);
    Require(Throws([&] { scePthreadCondattrGetpshared(&attr, nullptr); }));

    Require(scePthreadCondattrDestroy(&attr) == SCE_OK);
    Require(attr == nullptr);
    Require(scePthreadCondattrDestroy(&attr) == SCE_KERNEL_ERROR_EINVAL);
    Require(scePthreadCondattrDestroy(nullptr) == SCE_KERNEL_ERROR_EINVAL);
    Require(Throws([&] { scePthreadCondattrGetclock(&attr, &clockId); }));
    Require(scePthreadCondattrSetclock(&attr, MONOTONIC) == SCE_KERNEL_ERROR_EINVAL);
    Require(scePthreadCondattrSetclock(nullptr, MONOTONIC) == SCE_KERNEL_ERROR_EINVAL);
    pshared = -1;
    Require(scePthreadCondattrGetpshared(&attr, &pshared) == SCE_KERNEL_ERROR_EINVAL && pshared == -1);
    Require(scePthreadCondattrGetpshared(nullptr, &pshared) == SCE_KERNEL_ERROR_EINVAL && pshared == -1);
    Require(scePthreadCondattrSetpshared(&attr, PROCESS_PRIVATE) == SCE_KERNEL_ERROR_EINVAL);
    Require(scePthreadCondattrSetpshared(nullptr, PROCESS_PRIVATE) == SCE_KERNEL_ERROR_EINVAL);
}

}

int main() {
    CheckPosixClock();
    CheckPosixPshared();
    CheckSceCondattr();
    return 0;
}
