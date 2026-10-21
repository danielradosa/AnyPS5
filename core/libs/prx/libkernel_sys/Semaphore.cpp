#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI sem_init_nid_postfix(void* sem, int pshared, unsigned int value);
int APS5_VABI sem_destroy_nid_postfix(void* sem);
}

namespace {

constexpr int GuestEnoent = 2;
constexpr int GuestEexist = 17;
constexpr int GuestEinval = 22;
constexpr int GuestEnametoolong = 63;
constexpr std::size_t GuestPathMax = 1024;
constexpr std::size_t GuestNameMax = 255;
constexpr std::size_t PrefixLength = sizeof("/tmp/SEMD") - 1;
constexpr std::size_t FilePrefixLength = sizeof("SEMD") - 1;
constexpr std::size_t GuestSemaphoreBytes = 16;

struct Backing {
    alignas(8) unsigned char storage[GuestSemaphoreBytes] = {};

    Backing() = default;
    Backing(const Backing&) = delete;
    Backing& operator=(const Backing&) = delete;
    ~Backing() {
        const int saved = *__error_nid_postfix();
        sem_destroy_nid_postfix(storage);
        *__error_nid_postfix() = saved;
    }
};

struct Handle {
    std::string name;
    bool named = true;
    std::shared_ptr<Backing> backing;
    int openCount = 1;
};

struct Names {
    std::mutex lock;
    std::map<std::string, std::shared_ptr<Backing>> files;
    std::list<Handle> handles;
};

Names& State() {
    static Names names;
    return names;
}

void* Fail(int error) {
    *__error_nid_postfix() = error;
    return nullptr;
}

int NameError(const char* name, int missingSlash) {
    if (name == nullptr) throw std::invalid_argument("named semaphore: null name");
    if (name[0] != '/') return missingSlash;
    if (PrefixLength + std::strlen(name + 1) >= GuestPathMax) return GuestEnametoolong;
    return 0;
}

int FileNameError(const char* name) {
    if (std::strchr(name + 1, '/') != nullptr)
        throw std::runtime_error(std::string("named semaphore: ") + name + " names a file below a /tmp/SEMD directory, which is not modelled");
    if (FilePrefixLength + std::strlen(name + 1) > GuestNameMax) return GuestEnametoolong;
    return 0;
}

void* OpenSemaphore(const char* name, int flags, unsigned int value) {
    if (const int error = NameError(name, GuestEinval)) return Fail(error);
    if (flags & ~(SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL)) return Fail(GuestEinval);
    if (const int error = FileNameError(name)) return Fail(error);
    const std::string key(name + 1);
    const bool create = (flags & SCE_KERNEL_O_CREAT) != 0;
    const bool exclusive = create && (flags & SCE_KERNEL_O_EXCL) != 0;
    auto& names = State();
    const std::lock_guard lock(names.lock);
    const auto file = names.files.find(key);
    if (file != names.files.end() && exclusive) return Fail(GuestEexist);
    if (file == names.files.end() && !create) return Fail(GuestEnoent);
    std::shared_ptr<Backing> backing;
    if (file != names.files.end()) {
        backing = file->second;
    } else {
        backing = std::make_shared<Backing>();
        const int saved = *__error_nid_postfix();
        if (sem_init_nid_postfix(backing->storage, 1, value) != 0) return nullptr;
        *__error_nid_postfix() = saved;
        names.files.emplace(key, backing);
    }
    for (auto& handle : names.handles) {
        if (!handle.named || handle.name != key) continue;
        if (handle.backing != backing) {
            handle.named = false;
            break;
        }
        ++handle.openCount;
        return handle.backing->storage;
    }
    names.handles.push_back(Handle{key, true, backing});
    return backing->storage;
}

}

extern "C" {

void* APS5_VABI sem_open_nid_postfix(const char* name, int flags, ...) {
    unsigned int value = 0;
    if (flags & SCE_KERNEL_O_CREAT) {
#ifdef _WIN32
        __builtin_sysv_va_list arguments;
        __builtin_sysv_va_start(arguments, flags);
        static_cast<void>(__builtin_va_arg(arguments, int));
        value = static_cast<unsigned int>(__builtin_va_arg(arguments, int));
        __builtin_sysv_va_end(arguments);
#else
        std::va_list arguments;
        va_start(arguments, flags);
        static_cast<void>(va_arg(arguments, int));
        value = static_cast<unsigned int>(va_arg(arguments, int));
        va_end(arguments);
#endif
    }
    return OpenSemaphore(name, flags, value);
}

int APS5_VABI sem_close_nid_postfix(void* sem) {
    if (sem == nullptr) throw std::invalid_argument("sem_close: null semaphore");
    auto& names = State();
    std::shared_ptr<Backing> released;
    {
        const std::lock_guard lock(names.lock);
        for (auto handle = names.handles.begin(); handle != names.handles.end(); ++handle) {
            if (handle->backing->storage != sem) continue;
            if (--handle->openCount == 0) {
                released = std::move(handle->backing);
                names.handles.erase(handle);
            }
            return 0;
        }
    }
    *__error_nid_postfix() = GuestEinval;
    return -1;
}

int APS5_VABI sem_unlink_nid_postfix(const char* name) {
    int error = NameError(name, GuestEnoent);
    if (error == 0) error = FileNameError(name);
    if (error != 0) {
        *__error_nid_postfix() = error;
        return -1;
    }
    std::shared_ptr<Backing> released;
    auto& names = State();
    const std::lock_guard lock(names.lock);
    const auto file = names.files.find(name + 1);
    if (file == names.files.end()) {
        *__error_nid_postfix() = GuestEnoent;
        return -1;
    }
    released = std::move(file->second);
    names.files.erase(file);
    return 0;
}

}
