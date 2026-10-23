#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "prx/libkernel/File/include/ReservedDescriptor.hpp"
#include "prx/libkernel/Semaphore/include/Ksem.hpp"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI close_nid_postfix(int descriptor);
int APS5_VABI sem_init_nid_postfix(void* sem, int pshared, unsigned int value);
int APS5_VABI sem_destroy_nid_postfix(void* sem);
int APS5_VABI sem_post_nid_postfix(void* sem);
int APS5_VABI sem_wait_nid_postfix(void* sem);
int APS5_VABI sem_trywait_nid_postfix(void* sem);
int APS5_VABI sem_timedwait_nid_postfix(void* sem, const KernelTimespec* abstime);
int APS5_VABI sem_getvalue_nid_postfix(void* sem, int* value);
}

namespace {

constexpr int GuestEagain = 35;
constexpr int GuestEbusy = 16;
constexpr int GuestEfault = 14;
constexpr int GuestEinval = 22;
constexpr int GuestEnoent = 2;
constexpr int GuestEexist = 17;
constexpr int GuestEnfile = 23;
constexpr int GuestEnospc = 28;
constexpr int GuestEnametoolong = 63;
constexpr unsigned int SemValueMax = 0x7fffffffu;
constexpr int SemMax = 30;
constexpr std::size_t GuestPathMax = 1024;
constexpr std::size_t GuestSemaphoreBytes = 16;

int Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

std::atomic<int> g_semaphores{0};

struct KernelSemaphore {
    alignas(8) unsigned char storage[GuestSemaphoreBytes] = {};
    bool anonymous = false;
    std::atomic<int> waiters{0};

    KernelSemaphore() = default;
    KernelSemaphore(const KernelSemaphore&) = delete;
    KernelSemaphore& operator=(const KernelSemaphore&) = delete;
    ~KernelSemaphore() {
        const int saved = *__error_nid_postfix();
        sem_destroy_nid_postfix(storage);
        *__error_nid_postfix() = saved;
        g_semaphores.fetch_sub(1);
    }
};

using SemaphoreRef = std::shared_ptr<KernelSemaphore>;

struct Registry {
    std::mutex lock;
    std::map<int, SemaphoreRef> descriptors;
    std::map<std::string, SemaphoreRef> names;
    bool watching = false;
};

Registry& State() {
    static auto* registry = new Registry();
    return *registry;
}

void APS5_VABI DescriptorClosed(int descriptor) {
    SemaphoreRef released;
    auto& registry = State();
    const std::lock_guard lock(registry.lock);
    const auto found = registry.descriptors.find(descriptor);
    if (found == registry.descriptors.end()) return;
    released = std::move(found->second);
    registry.descriptors.erase(found);
}

SemaphoreRef Allocate(unsigned int value, bool anonymous) {
    if (g_semaphores.load() >= SemMax) return nullptr;
    auto semaphore = std::make_shared<KernelSemaphore>();
    g_semaphores.fetch_add(1);
    semaphore->anonymous = anonymous;
    const int saved = *__error_nid_postfix();
    if (sem_init_nid_postfix(semaphore->storage, 1, value) != 0) return nullptr;
    *__error_nid_postfix() = saved;
    return semaphore;
}

SemaphoreRef Find(std::intptr_t id) {
    auto& registry = State();
    const std::lock_guard lock(registry.lock);
    const auto found = registry.descriptors.find(static_cast<int>(id));
    if (id < 0 || id > INT32_MAX || found == registry.descriptors.end()) return nullptr;
    return found->second;
}

int Create(std::intptr_t* idp, const char* name, int flags, unsigned int value) {
    if (value > SemValueMax) return Fail(GuestEinval);
    const int descriptor = GuestReserveDescriptor_nid_no_patch();
    if (descriptor < 0) return Fail(name == nullptr ? GuestEnospc : *__error_nid_postfix());
    const auto abandon = [descriptor](int error) {
        const int saved = *__error_nid_postfix();
        close_nid_postfix(descriptor);
        *__error_nid_postfix() = saved;
        return Fail(error);
    };
    if (idp == nullptr) return abandon(GuestEfault);
    *idp = descriptor;
    auto& registry = State();
    std::unique_lock lock(registry.lock);
    if (!registry.watching) {
        GuestWatchDescriptorClose_nid_no_patch(DescriptorClosed);
        registry.watching = true;
    }
    SemaphoreRef semaphore;
    if (name == nullptr) {
        semaphore = Allocate(value, true);
        if (!semaphore) {
            lock.unlock();
            return abandon(GuestEnospc);
        }
    } else {
        int error = 0;
        if (std::strlen(name) >= GuestPathMax) error = GuestEnametoolong;
        else if (name[0] != '/') error = GuestEinval;
        if (error == 0) {
            const auto found = registry.names.find(name);
            if (found == registry.names.end()) {
                if ((flags & SCE_KERNEL_O_CREAT) == 0) error = GuestEnoent;
                else if (!(semaphore = Allocate(value, false))) error = GuestEnfile;
                else registry.names.emplace(name, semaphore);
            } else if ((flags & (SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL)) == (SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL)) {
                error = GuestEexist;
            } else {
                semaphore = found->second;
            }
        }
        if (error != 0) {
            lock.unlock();
            return abandon(error);
        }
    }
    registry.descriptors[descriptor] = std::move(semaphore);
    return 0;
}

int Wait(std::intptr_t id, const KernelTimespec* abstime, bool forever) {
    const SemaphoreRef semaphore = Find(id);
    if (!semaphore) return Fail(GuestEinval);
    if (sem_trywait_nid_postfix(semaphore->storage) == 0) return 0;
    if (*__error_nid_postfix() != GuestEagain || (!forever && abstime == nullptr)) return -1;
    semaphore->waiters.fetch_add(1);
    const int result = abstime == nullptr ? sem_wait_nid_postfix(semaphore->storage) : sem_timedwait_nid_postfix(semaphore->storage, abstime);
    semaphore->waiters.fetch_sub(1);
    return result;
}

int Release(std::intptr_t id, bool destroy) {
    SemaphoreRef semaphore;
    {
        auto& registry = State();
        const std::lock_guard lock(registry.lock);
        const auto found = registry.descriptors.find(static_cast<int>(id));
        if (id < 0 || id > INT32_MAX || found == registry.descriptors.end()) return Fail(GuestEinval);
        if (found->second->anonymous != destroy) return Fail(GuestEinval);
        if (destroy && found->second->waiters.load() != 0) return Fail(GuestEbusy);
        semaphore = std::move(found->second);
        registry.descriptors.erase(found);
    }
    return close_nid_postfix(static_cast<int>(id));
}

}

bool IsKsemDescriptor(int descriptor) {
    auto& registry = State();
    const std::lock_guard lock(registry.lock);
    return registry.descriptors.contains(descriptor);
}

extern "C" {

int APS5_VABI ksem_init_nid_postfix(std::intptr_t* idp, unsigned int value) {
    return Create(idp, nullptr, 0, value);
}

int APS5_VABI ksem_open_nid_postfix(std::intptr_t* idp, const char* name, int flags, std::uint16_t mode, unsigned int value) {
    static_cast<void>(mode);
    if ((flags & ~(SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL)) != 0) return Fail(GuestEinval);
    return Create(idp, name, flags, value);
}

int APS5_VABI ksem_unlink_nid_postfix(const char* name) {
    if (name == nullptr) return Fail(GuestEfault);
    if (std::strlen(name) >= GuestPathMax) return Fail(GuestEnametoolong);
    SemaphoreRef released;
    auto& registry = State();
    const std::lock_guard lock(registry.lock);
    const auto found = registry.names.find(name);
    if (found == registry.names.end()) return Fail(GuestEnoent);
    released = std::move(found->second);
    registry.names.erase(found);
    return 0;
}

int APS5_VABI ksem_close_nid_postfix(std::intptr_t id) {
    return Release(id, false);
}

int APS5_VABI ksem_destroy_nid_postfix(std::intptr_t id) {
    return Release(id, true);
}

int APS5_VABI ksem_post_nid_postfix(std::intptr_t id) {
    const SemaphoreRef semaphore = Find(id);
    if (!semaphore) return Fail(GuestEinval);
    return sem_post_nid_postfix(semaphore->storage);
}

int APS5_VABI ksem_wait_nid_postfix(std::intptr_t id) {
    return Wait(id, nullptr, true);
}

int APS5_VABI ksem_trywait_nid_postfix(std::intptr_t id) {
    return Wait(id, nullptr, false);
}

int APS5_VABI ksem_timedwait_nid_postfix(std::intptr_t id, const KernelTimespec* abstime) {
    constexpr std::int64_t NanosPerSecond = 1000000000;
    if (abstime != nullptr && (abstime->tv_nsec < 0 || abstime->tv_nsec >= NanosPerSecond)) return Fail(GuestEinval);
    return Wait(id, abstime, true);
}

int APS5_VABI ksem_getvalue_nid_postfix(std::intptr_t id, int* value) {
    const SemaphoreRef semaphore = Find(id);
    if (!semaphore) return Fail(GuestEinval);
    int current = 0;
    if (sem_getvalue_nid_postfix(semaphore->storage, &current) != 0) return -1;
    if (value == nullptr) return Fail(GuestEfault);
    *value = current;
    return 0;
}

}
