#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/RemoveTree.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "SceTypes.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <source_location>
#include <stdexcept>
#include <set>
#include <string>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#endif
#endif
#ifndef _WIN32
#include <cerrno>
#include <unistd.h>
#endif

struct TestAiocb {
    int fildes;
    std::int64_t offset;
    void* buf;
    std::uint64_t nbytes;
    int spare[2];
    void* spare2;
    int lioOpcode;
    int reqprio;
    std::int64_t status;
    std::int64_t error;
    void* kernelinfo;
    int notify;
    int signo;
    std::uint64_t value;
    std::uint64_t data[8];
};
static_assert(sizeof(TestAiocb) == 160);

extern "C" {
int APS5_VABI fstatfs_nid_postfix(int, void*);
int APS5_VABI _fstatfs_nid_postfix(int, void*);
int APS5_VABI open_nid_postfix(const char*, int, int);
int APS5_VABI close_nid_postfix(int);
std::int64_t APS5_VABI readlink_nid_postfix(const char*, char*, std::size_t);
int APS5_VABI lstat_nid_postfix(const char*, FileStat*);
std::int64_t APS5_VABI pathconf_nid_postfix(const char*, int);
std::int64_t APS5_VABI fpathconf_nid_postfix(int, int);
std::int64_t APS5_VABI _fpathconf_nid_postfix(int, int);
int APS5_VABI pipe_nid_postfix(int*);
int APS5_VABI socket_nid_postfix(int, int, int);
int APS5_VABI fchdir_nid_postfix(int);
int APS5_VABI chdir_nid_postfix(const char*);
int APS5_VABI chroot_nid_postfix(const char*);
char* APS5_VABI getcwd_nid_postfix(char*, std::size_t);
int APS5_VABI getpid_nid_postfix();
int APS5_VABI sceKernelTitleWorkaroundIsEnabled(const void*, std::int32_t, std::int32_t*);
int APS5_VABI sceKernelGetProcessType(int);
int APS5_VABI sceKernelOpen(const char*, int, std::uint16_t);
int APS5_VABI sceKernelClose(int);
int APS5_VABI sceKernelFlock(int, int);
int APS5_VABI sceKernelRename(const char*, const char*);
int APS5_VABI sceKernelFstat(int, FileStat*);
int APS5_VABI sceKernelGetdents(int, char*, int);
std::int64_t APS5_VABI sceKernelLseek(int, std::int64_t, int);
std::int64_t APS5_VABI sceKernelPread(int, void*, std::size_t, std::int64_t);
int APS5_VABI dup_nid_postfix(int);
int APS5_VABI dup2_nid_postfix(int, int);
int APS5_VABI setsid_nid_postfix(void);
int APS5_VABI fork_nid_postfix(void);
int APS5_VABI link_nid_postfix(const char*, const char*);
int APS5_VABI symlink_nid_postfix(const char*, const char*);
int APS5_VABI lchflags_nid_postfix(const char*, std::uint64_t);
int APS5_VABI openat_nid_postfix(int, const char*, int, ...);
int APS5_VABI unlinkat_nid_postfix(int, const char*, int);
std::int64_t APS5_VABI readlinkat_nid_postfix(int, const char*, char*, std::size_t);
int APS5_VABI mkdirat_nid_postfix(int, const char*, std::uint16_t);
int APS5_VABI renameat_nid_postfix(int, const char*, int, const char*);
int APS5_VABI fchmodat_nid_postfix(int, const char*, int, int);
int APS5_VABI fchown_nid_postfix(int, std::uint32_t, std::uint32_t);
int APS5_VABI fstatat_nid_postfix(int, const char*, FileStat*, int);
std::int64_t APS5_VABI sceKernelRead(int, void*, std::size_t);
std::uint16_t APS5_VABI umask_nid_postfix(std::uint16_t);
int* APS5_VABI __error_nid_postfix();
int APS5_VABI fstat_nid_postfix(int, FileStat*);
std::int64_t APS5_VABI _fstat_nid_postfix(int, FileStat*);
int APS5_VABI sceKernelGetCompiledSdkVersion(std::int32_t*);
int APS5_VABI sceKernelGetCompiledSdkVersion_nid_postfix(std::int32_t*);
int APS5_VABI scePthreadCondattrInit(PthreadCondattr*);
int APS5_VABI scePthreadCondattrDestroy(PthreadCondattr*);
int APS5_VABI scePthreadCondattrSetclock(PthreadCondattr*, KernelClockid);
int APS5_VABI scePthreadCondattrGetclock_nid_postfix(const PthreadCondattr*, KernelClockid*);
int APS5_VABI scePthreadAttrInit(PthreadAttr*);
int APS5_VABI scePthreadAttrDestroy(PthreadAttr*);
int APS5_VABI scePthreadMutexInitForInternalLibc(PthreadMutex*, const PthreadMutexattr*);
int APS5_VABI scePthreadMutexattrInitForInternalLibc(PthreadMutexattr*);
int APS5_VABI scePthreadMutexattrGettype(const PthreadMutexattr*, int*);
int APS5_VABI scePthreadMutexattrDestroy(PthreadMutexattr*);
int APS5_VABI scePthreadMutexLock(PthreadMutex*);
int APS5_VABI scePthreadMutexUnlock(PthreadMutex*);
int APS5_VABI scePthreadMutexDestroy(PthreadMutex*);
Pthread APS5_VABI scePthreadSelf(void);
int APS5_VABI pthread_timedjoin_np_nid_postfix(Pthread, void**, const KernelTimespec*);
int APS5_VABI pthread_create_nid_postfix(Pthread*, const PthreadAttr*, PthreadEntry, void*);
int APS5_VABI clock_gettime_nid_postfix(int, KernelTimespec*);
int APS5_VABI pthread_rwlock_reltimedrdlock_np_nid_postfix(PthreadRwlock*, KernelUseconds);
int APS5_VABI pthread_rwlock_reltimedwrlock_np_nid_postfix(PthreadRwlock*, KernelUseconds);
int APS5_VABI pthread_rwlock_wrlock_nid_postfix(PthreadRwlock*);
int APS5_VABI pthread_rwlock_unlock_nid_postfix(PthreadRwlock*);
int APS5_VABI pthread_rwlock_destroy_nid_postfix(PthreadRwlock*);
int APS5_VABI scePthreadRwlockattrInit(PthreadRwlockattr*);
int APS5_VABI scePthreadRwlockattrDestroy(PthreadRwlockattr*);
int APS5_VABI pthread_rwlockattr_settype_np_nid_postfix(PthreadRwlockattr*, int);
int APS5_VABI pthread_rwlockattr_gettype_np_nid_postfix(const PthreadRwlockattr*, int*);
int APS5_VABI scePthreadMutexInit(PthreadMutex*, const PthreadMutexattr*, const char*);
int APS5_VABI pthread_mutex_setname_np_nid_postfix(PthreadMutex*, const char*);
int APS5_VABI scePthreadCondInit(PthreadCond*, const PthreadCondattr*, const char*);
int APS5_VABI scePthreadCondDestroy(PthreadCond*);
int APS5_VABI pthread_cond_setname_np_nid_postfix(PthreadCond*, const char*);
int APS5_VABI pthread_rwlock_setname_np_nid_postfix(PthreadRwlock*, const char*);
int APS5_VABI sem_init_nid_postfix(void*, int, unsigned);
int APS5_VABI sem_destroy_nid_postfix(void*);
int APS5_VABI sem_setname_nid_postfix(void*, const char*);
int APS5_VABI chown_nid_postfix(const char*, std::uint32_t, std::uint32_t);
int APS5_VABI lchown_nid_postfix(const char*, std::uint32_t, std::uint32_t);
int APS5_VABI fchownat_nid_postfix(int, const char*, std::uint32_t, std::uint32_t, int);
int APS5_VABI lchmod_nid_postfix(const char*, std::uint16_t);
int APS5_VABI linkat_nid_postfix(int, const char*, int, const char*, int);
char* APS5_VABI getlogin_nid_postfix(void);
int APS5_VABI getlogin_r_nid_postfix(char*, int);
int APS5_VABI vfork_nid_postfix(void);
int APS5_VABI aio_read_nid_postfix(TestAiocb*);
int APS5_VABI aio_write_nid_postfix(TestAiocb*);
int APS5_VABI aio_fsync_nid_postfix(int, TestAiocb*);
int APS5_VABI aio_error_nid_postfix(const TestAiocb*);
std::int64_t APS5_VABI aio_return_nid_postfix(TestAiocb*);
int APS5_VABI aio_suspend_nid_postfix(const TestAiocb* const*, int, const KernelTimespec*);
int APS5_VABI aio_cancel_nid_postfix(int, TestAiocb*);
std::int64_t APS5_VABI aio_waitcomplete_nid_postfix(TestAiocb**, const KernelTimespec*);
void* APS5_VABI sem_open_nid_postfix(const char*, int, ...);
int APS5_VABI sem_close_nid_postfix(void*);
int APS5_VABI sem_unlink_nid_postfix(const char*);
int APS5_VABI sem_trywait_nid_postfix(void*);
int APS5_VABI sem_post_nid_postfix(void*);
int APS5_VABI sem_getvalue_nid_postfix(void*, int*);
int APS5_VABI pselect_nid_postfix(int, void*, void*, void*, const KernelTimespec*, const void*);
int APS5_VABI sigprocmask_nid_postfix(int, const void*, void*);
int APS5_VABI settimeofday_nid_postfix(const KernelTimeval*, const KernelTimezone*);
int APS5_VABI sceKernelSettimeofday(const KernelTimeval*, const KernelTimezone*);
int APS5_VABI sceKernelMlockall(int);
int APS5_VABI sceKernelGetModuleInfoInternal(KernelModule, ModuleInfoEx*);
int APS5_VABI sceKernelGetModuleInfo(KernelModule, ModuleInfo*);
int APS5_VABI sceKernelGetModuleList(KernelModule*, std::size_t, std::size_t*);
int APS5_VABI sceKernelIsInSandbox(void);
int APS5_VABI tcgetsid_nid_postfix(int);
int APS5_VABI tcsetsid_nid_postfix(int, int);
int APS5_VABI getsid_nid_postfix(int);
void AddPathAlias_nid_no_patch(const char*, const char*);
void RemovePathAlias_nid_no_patch(const char*);
}

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (!condition) {
        std::fprintf(stderr, "Kernel sys check failed at line %u (guest errno %d)\n", location.line(), *__error_nid_postfix());
        std::abort();
    }
}

int Error() {
    return *__error_nid_postfix();
}

bool PathconfThrows(const char* path, int name) {
    try {
        pathconf_nid_postfix(path, name);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

bool ReadsLinkOutsideGuest(const std::string& path) {
    char buffer[64];
    try {
        readlink_nid_postfix(path.c_str(), buffer, sizeof(buffer));
    } catch (const std::runtime_error& error) {
        return std::strstr(error.what(), "is outside the guest filesystem") != nullptr;
    }
    return false;
}

bool ReadsLink(const std::string& path, const std::string& expected) {
    char buffer[1024];
    std::memset(buffer, '#', sizeof(buffer));
    const auto count = readlink_nid_postfix(path.c_str(), buffer, sizeof(buffer));
    return count == static_cast<std::int64_t>(expected.size()) && std::memcmp(buffer, expected.data(), expected.size()) == 0 &&
        buffer[expected.size()] == '#';
}

void CheckAbsoluteTargets(const std::filesystem::path& root) {
    const auto guestRoot = "/" + root.filename().string();
    std::filesystem::create_directory(root / "absolute-dir");
    std::ofstream(root / "absolute-dir" / "inner.txt") << "xyz";
    const auto fileLink = (root / "absolute-file").string();
    Require(symlink_nid_postfix((guestRoot + "/absolute-dir/inner.txt").c_str(), fileLink.c_str()) == 0);
    Require(ReadsLink(fileLink, guestRoot + "/absolute-dir/inner.txt"));
    const int opened = sceKernelOpen(fileLink.c_str(), SCE_KERNEL_O_RDONLY, 0);
    char byte = 0;
    Require(opened >= 0 && sceKernelRead(opened, &byte, 1) == 1 && byte == 'x' && sceKernelClose(opened) == 0);
    Require(symlink_nid_postfix((guestRoot + "/absolute-dir/inner.txt").c_str(), fileLink.c_str()) == -1 && Error() == 17);
    const auto directoryLink = (root / "absolute-directory").string();
    Require(symlink_nid_postfix((guestRoot + "/absolute-dir").c_str(), directoryLink.c_str()) == 0);
    Require(ReadsLink(directoryLink, guestRoot + "/absolute-dir"));
    const int through = sceKernelOpen((directoryLink + "/inner.txt").c_str(), SCE_KERNEL_O_RDONLY, 0);
    byte = 0;
    Require(through >= 0 && sceKernelRead(through, &byte, 1) == 1 && byte == 'x' && sceKernelClose(through) == 0);
    const auto danglingLink = (root / "absolute-dangling").string();
    Require(symlink_nid_postfix((guestRoot + "/absolute-dir/missing.txt").c_str(), danglingLink.c_str()) == 0);
    Require(ReadsLink(danglingLink, guestRoot + "/absolute-dir/missing.txt"));
    AddPathAlias_nid_no_patch("/aliasmount", std::filesystem::canonical(root / "absolute-dir").string().c_str());
    const auto aliasLink = (root / "absolute-alias").string();
    Require(symlink_nid_postfix("/aliasmount/inner.txt", aliasLink.c_str()) == 0);
    Require(ReadsLink(aliasLink, "/aliasmount/inner.txt"));
    Require(ReadsLink(fileLink, "/aliasmount/inner.txt"));
    const auto mountLink = (root / "absolute-mount").string();
    Require(symlink_nid_postfix("/aliasmount", mountLink.c_str()) == 0 && ReadsLink(mountLink, "/aliasmount"));
    RemovePathAlias_nid_no_patch("/aliasmount");
    Require(ReadsLink(aliasLink, guestRoot + "/absolute-dir/inner.txt"));
    Require(ReadsLink(mountLink, guestRoot + "/absolute-dir"));
    const auto outside = std::filesystem::canonical(std::filesystem::current_path()).parent_path() / ("missing-" + root.filename().string());
    Require(!std::filesystem::exists(outside));
    const auto outsideLink = root / "absolute-outside";
#ifdef _WIN32
    Require(CreateSymbolicLinkW(outsideLink.wstring().c_str(), outside.wstring().c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) != 0);
#else
    std::filesystem::create_symlink(outside, outsideLink);
#endif
    Require(ReadsLinkOutsideGuest(outsideLink.string()));
}

bool ChrootThrows(const std::string& path) {
    try {
        chroot_nid_postfix(path.c_str());
    } catch (const std::runtime_error& error) {
        return std::strstr(error.what(), path.c_str()) != nullptr;
    }
    return false;
}

bool SymlinkTargetThrows(const char* target, const std::string& created) {
    try {
        symlink_nid_postfix(target, created.c_str());
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

bool RemoveSymlink(const std::string& link) {
#ifdef _WIN32
    // std::filesystem::remove follows the link, reports a dangling one as
    // missing and clears the error, so remove the reparse point itself. A
    // directory reparse point, such as the one the "..." target creates, is
    // refused by DeleteFileW and needs RemoveDirectoryW instead.
    const auto wide = std::filesystem::path(link).wstring();
    return DeleteFileW(wide.c_str()) != FALSE || RemoveDirectoryW(wide.c_str()) != FALSE;
#else
    return std::filesystem::remove(link);
#endif
}

void CheckSymlinkTargetRestrictions(const std::filesystem::path& root) {
    const auto created = (root / "rejected").string();
    for (const auto& target : {std::string(".."), std::string("../x"), std::string("a/../b"), std::string("a/../../x")})
        Require(SymlinkTargetThrows(target.c_str(), created));
#ifdef _WIN32
    for (const auto& target : {std::string("C:/"), std::string("\\\\server\\share")})
        Require(SymlinkTargetThrows(target.c_str(), created));
#endif
    const auto links = [](const std::string& created, const char* target) {
        char buffer[16];
        const auto length = std::strlen(target);
        return readlink_nid_postfix(created.c_str(), buffer, sizeof(buffer)) == static_cast<std::int64_t>(length) &&
            std::memcmp(buffer, target, length) == 0;
    };
    const auto nested = (root / "accepted-nested").string();
    const int link = symlink_nid_postfix("sub/dir", nested.c_str());
    if (link == 0) {
        Require(links(nested, "sub/dir"));
        const auto dots = (root / "accepted-dots").string();
        Require(symlink_nid_postfix("a..b", dots.c_str()) == 0 && links(dots, "a..b"));
        const auto ellipsis = (root / "accepted-ellipsis").string();
        Require(symlink_nid_postfix("...", ellipsis.c_str()) == 0 && links(ellipsis, "..."));
        Require(RemoveSymlink(nested) && RemoveSymlink(dots) && RemoveSymlink(ellipsis));
    } else {
        Require(link == -1 && Error() == 1);
        std::fprintf(stderr, "symbolic links unavailable: the symlink target success cases not tested\n");
    }
}

void CheckPathconf(const std::filesystem::path& root, const std::string& file) {
    const auto directory = root.generic_string();
    for (const auto& path : {file, directory}) {
        Require(pathconf_nid_postfix(path.c_str(), 1) == 32767);
        Require(pathconf_nid_postfix(path.c_str(), 4) == 255);
        Require(pathconf_nid_postfix(path.c_str(), 5) == 1024);
        Require(pathconf_nid_postfix(path.c_str(), 6) == 512);
        Require(pathconf_nid_postfix(path.c_str(), 7) == 1);
        Require(pathconf_nid_postfix(path.c_str(), 8) == 1);
        Require(pathconf_nid_postfix(path.c_str(), 12) == 64);
        *__error_nid_postfix() = 0;
        Require(pathconf_nid_postfix(path.c_str(), 15) == -1 && Error() == 0);
        Require(pathconf_nid_postfix(path.c_str(), 18) == 1024);
        Require(pathconf_nid_postfix(path.c_str(), 54) == 0);
        Require(pathconf_nid_postfix(path.c_str(), 55) == 0);
    }
    for (const int name : {0, 11, 61, 62, 65, -1})
        Require(pathconf_nid_postfix(file.c_str(), name) == -1 && Error() == 22);
    for (const int name : {2, 3, 9, 10, 14, 16, 17, 21, 53, 59, 60, 63, 64})
        Require(PathconfThrows(file.c_str(), name));
    const auto missing = (root / "missing").string();
    Require(pathconf_nid_postfix(missing.c_str(), 4) == -1 && Error() == 2);
    Require(pathconf_nid_postfix(missing.c_str(), 0) == -1 && Error() == 2);
    Require(pathconf_nid_postfix((root / "missing" / "file").string().c_str(), 4) == -1 && Error() == 2);
    Require(pathconf_nid_postfix("", 4) == -1 && Error() == 2);
    Require(pathconf_nid_postfix(nullptr, 4) == -1 && Error() == 14);
    const auto longName = directory + "/" + std::string(256, 'n');
    Require(pathconf_nid_postfix(longName.c_str(), 4) == -1 && Error() == 63);
    Require(pathconf_nid_postfix((directory + "/missing/" + std::string(256, 'n')).c_str(), 4) == -1 && Error() == 2);
    const auto longPath = directory + "/" + std::string(1024, 'p');
    Require(pathconf_nid_postfix(longPath.c_str(), 4) == -1 && Error() == 63);
}

void DirectoryRenameTests(const std::filesystem::path& parent) {
    const auto fixture = parent / "directory-identity";
    const auto original = fixture / "original";
    const auto moved = fixture / "moved";
    const auto twice = fixture / "twice";
    Require(std::filesystem::create_directory(fixture));
    Require(std::filesystem::create_directory(original));
    { std::ofstream(original / "marker").put('o'); }
    { std::ofstream(original / "old-only").put('a'); }
    const auto readMarker = [](int directory) {
        const int file = openat_nid_postfix(directory, "marker", SCE_KERNEL_O_RDONLY);
        Require(file >= 0);
        char marker = 0;
        Require(sceKernelRead(file, &marker, 1) == 1);
        Require(sceKernelClose(file) == 0);
        return marker;
    };
    const auto names = [](int directory) {
        std::set<std::string> result;
        std::array<char, 4096> buffer{};
        for (;;) {
            const int bytes = sceKernelGetdents(directory, buffer.data(), buffer.size());
            Require(bytes >= 0);
            if (bytes == 0) return result;
            for (int offset = 0; offset < bytes;) {
                std::uint16_t length = 0;
                std::memcpy(&length, buffer.data() + offset + 4, sizeof(length));
                Require(length >= 12 && offset + length <= bytes);
                result.emplace(buffer.data() + offset + 8);
                offset += length;
            }
        }
    };
    const int first = sceKernelOpen(original.string().c_str(), SCE_KERNEL_O_RDONLY | SCE_KERNEL_O_DIRECTORY, 0);
    Require(first >= 0 && readMarker(first) == 'o');
    FileStat before{};
    Require(sceKernelFstat(first, &before) == 0 && (before.st_mode & 0170000) == 0040000);
    const int duplicate = dup_nid_postfix(first);
    const int target = sceKernelOpen((original / "marker").string().c_str(), SCE_KERNEL_O_RDONLY, 0);
    Require(duplicate >= 0 && target >= 0);
    Require(dup2_nid_postfix(first, target) == target);
    Require(sceKernelClose(first) == 0);
    Require(sceKernelRename(original.string().c_str(), moved.string().c_str()) == 0);
    Require(std::filesystem::create_directory(original));
    { std::ofstream(original / "marker").put('r'); }
    { std::ofstream(original / "new-only").put('b'); }
    const char marker = readMarker(duplicate);
    std::fprintf(stderr, "directory marker after rename/replacement: %c, expected o\n", marker);
    Require(marker == 'o' && readMarker(target) == 'o');
    const std::set<std::string> expected{".", "..", "marker", "old-only"};
    Require(names(duplicate) == expected);
    Require(names(target).empty());
    FileStat after{};
    Require(sceKernelFstat(target, &after) == 0 && after.st_dev == before.st_dev && after.st_ino == before.st_ino);
    Require(sceKernelLseek(target, 0, 0) == 0);
    char directoryByte = 0;
    Require(sceKernelRead(target, &directoryByte, 1) == static_cast<int>(0x80020015u));
    Require(sceKernelPread(target, &directoryByte, 1, 0) == static_cast<int>(0x80020015u));
    char previous[4096];
    Require(getcwd_nid_postfix(previous, sizeof(previous)) == previous);
    Require(fchdir_nid_postfix(target) == 0 && readMarker(-100) == 'o');
    Require(chdir_nid_postfix(previous) == 0);
    Require(mkdirat_nid_postfix(target, "created", 0755) == 0);
    Require(std::filesystem::is_directory(moved / "created") && !std::filesystem::exists(original / "created"));
    Require(unlinkat_nid_postfix(duplicate, "created", 0x800) == 0);
    Require(sceKernelRename(moved.string().c_str(), twice.string().c_str()) == 0);
    Require(std::filesystem::create_directory(moved));
    { std::ofstream(moved / "different").put('c'); }
    Require(sceKernelLseek(target, 0, 0) == 0 && names(target) == expected);
    Require(sceKernelClose(duplicate) == 0);
    Require(readMarker(target) == 'o');
    Require(unlinkat_nid_postfix(target, "marker", 0) == 0);
    Require(unlinkat_nid_postfix(target, "old-only", 0) == 0);
    Require(std::filesystem::remove(twice));
    Require(sceKernelFstat(target, &after) == 0 && after.st_dev == before.st_dev && after.st_ino == before.st_ino && (after.st_mode & 0170000) == 0040000);
    Require(sceKernelClose(target) == 0);
    Require(std::filesystem::remove(original / "marker") && std::filesystem::remove(original / "new-only"));
    Require(std::filesystem::remove(moved / "different"));
    Require(std::filesystem::remove(original) && std::filesystem::remove(moved) && std::filesystem::remove(fixture));
}

template <typename F>
bool DescriptorPathconfThrows(F fn, int descriptor, int name) {
    try {
        fn(descriptor, name);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

static bool LchflagsThrows(const std::string& path, std::uint64_t flags, const char* names) {
    try {
        lchflags_nid_postfix(path.c_str(), flags);
    } catch (const std::runtime_error& error) {
        return std::strstr(error.what(), names) != nullptr;
    }
    return false;
}

template <typename F>
void CheckStreamPathconf(F fn, int descriptor) {
    Require(fn(descriptor, 6) == 512);
    Require(fn(descriptor, 53) == 200112);
    for (const int name : {1, 4, 5, 9, 54, 0, -1})
        Require(fn(descriptor, name) == -1 && Error() == 22);
}

template <typename F>
void CheckDescriptorPathconf(F fn, int descriptor) {
    Require(fn(descriptor, 1) == 32767);
    Require(fn(descriptor, 4) == 255);
    Require(fn(descriptor, 5) == 1024);
    Require(fn(descriptor, 6) == 512);
    Require(fn(descriptor, 7) == 1);
    Require(fn(descriptor, 8) == 1);
    Require(fn(descriptor, 12) == 64);
    *__error_nid_postfix() = 0;
    Require(fn(descriptor, 15) == -1 && Error() == 0);
    Require(fn(descriptor, 18) == 1024);
    Require(fn(descriptor, 54) == 0);
    Require(fn(descriptor, 55) == 0);
    for (const int name : {0, 11, 61, 62, 65, -1})
        Require(fn(descriptor, name) == -1 && Error() == 22);
    Require(fn(descriptor, 53) == 200112);
    for (const int name : {2, 3, 9, 10, 14, 16, 17, 21, 59, 60, 63, 64})
        Require(DescriptorPathconfThrows(fn, descriptor, name));
    Require(fn(-1, 4) == -1 && Error() == 9);
    Require(fn(descriptor + 1000, 4) == -1 && Error() == 9);
}

static void CheckProcessQueries() {
    struct {
        std::int32_t version;
        std::int32_t align;
        std::uint64_t ids[2];
    } workaround{1, 0, {(1ull << 3) | (1ull << 0x39), ~0ull}};
    std::int32_t enabled = -1;
    Require(sceKernelTitleWorkaroundIsEnabled(nullptr, 3, &enabled) == static_cast<int>(0x8002000E));
    Require(sceKernelTitleWorkaroundIsEnabled(&workaround, 3, nullptr) == static_cast<int>(0x8002000E));
    Require(sceKernelTitleWorkaroundIsEnabled(&workaround, 0x3A, &enabled) == static_cast<int>(0x80020016) && enabled == -1);
    Require(sceKernelTitleWorkaroundIsEnabled(&workaround, -1, &enabled) == static_cast<int>(0x80020016) && enabled == -1);
    Require(sceKernelTitleWorkaroundIsEnabled(&workaround, 3, &enabled) == 0 && enabled == 1);
    Require(sceKernelTitleWorkaroundIsEnabled(&workaround, 4, &enabled) == 0 && enabled == 0);
    Require(sceKernelTitleWorkaroundIsEnabled(&workaround, 63, &enabled) == static_cast<int>(0x80020016));
    Require(sceKernelTitleWorkaroundIsEnabled(&workaround, 0x39, &enabled) == 0 && enabled == 1);
    Require(sceKernelTitleWorkaroundIsEnabled(&workaround, 0x38, &enabled) == 0 && enabled == 0);
    Require(sceKernelGetProcessType(getpid_nid_postfix()) == 0);
    Require(sceKernelGetProcessType(-1) == 0 && sceKernelGetProcessType(0) == 0);
    Require(sceKernelGetProcessType(getpid_nid_postfix() + 1) == static_cast<int>(0x80020003));
}

static void SceKernelFlock() {
    const auto path = (std::filesystem::temp_directory_path() / "anyps5-sce-flock.bin").string();
    const int first = open_nid_postfix(path.c_str(), 0x602, 0644);
    const int second = open_nid_postfix(path.c_str(), 2, 0);
    Require(first >= 0 && second >= 0);
    Require(sceKernelFlock(first, 2 | 4) == 0);
    Require(sceKernelFlock(second, 2 | 4) == static_cast<int>(0x80020023u));
    Require(sceKernelFlock(first, 8) == 0);
    Require(sceKernelFlock(second, 2 | 4) == 0);
    Require(sceKernelFlock(second, 8) == 0);
    Require(sceKernelFlock(-1, 2) == static_cast<int>(0x80020009u));
    Require(close_nid_postfix(second) == 0 && close_nid_postfix(first) == 0);
    std::filesystem::remove(path);
}

template<typename F>
bool Throws(F call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

void CheckForwards(const std::filesystem::path& root) {
    const auto path = (root / "forward.bin").string();
    std::ofstream(path) << "0123456789";
    const int descriptor = sceKernelOpen(path.c_str(), SCE_KERNEL_O_RDWR, 0);
    Require(descriptor >= 0);
    FileStat viaSys{};
    FileStat viaKernel{};
    Require(fstat_nid_postfix(descriptor, &viaSys) == 0 && _fstat_nid_postfix(descriptor, &viaKernel) == 0);
    Require(viaSys.st_size == 10 && viaSys.st_mode == viaKernel.st_mode && viaSys.st_ino == viaKernel.st_ino);
    Require(fstat_nid_postfix(-1, &viaSys) == -1 && Error() == 9);
    Require(fstat_nid_postfix(descriptor, nullptr) == -1 && Error() == 14);
    Require(sceKernelClose(descriptor) == 0);
    Require(sceKernelGetCompiledSdkVersion(nullptr) == static_cast<int>(0x80020016u));
    std::int32_t viaSysVersion = 0;
    std::int32_t viaKernelVersion = 0;
    const bool sysThrows = Throws([&] { sceKernelGetCompiledSdkVersion(&viaSysVersion); });
    const bool kernelThrows = Throws([&] { sceKernelGetCompiledSdkVersion_nid_postfix(&viaKernelVersion); });
    Require(sysThrows == kernelThrows && viaSysVersion == viaKernelVersion);
    PthreadCondattr attr = nullptr;
    Require(scePthreadCondattrInit(&attr) == 0 && scePthreadCondattrSetclock(&attr, 4) == 0);
    KernelClockid clock = -1;
    Require(scePthreadCondattrGetclock_nid_postfix(&attr, &clock) == 0 && clock == 4);
    Require(scePthreadCondattrDestroy(&attr) == 0);
    PthreadCondattr missing = nullptr;
    Require(scePthreadCondattrGetclock_nid_postfix(&missing, &clock) == static_cast<int>(0x80020016u));
    Require(scePthreadCondattrGetclock_nid_postfix(nullptr, &clock) == static_cast<int>(0x80020016u));
}

void CheckScePthreadWrappers() {
    PthreadMutexattr mutexAttr = nullptr;
    int type = 0;
    Require(scePthreadMutexattrInitForInternalLibc(&mutexAttr) == 0);
    Require(scePthreadMutexattrGettype(&mutexAttr, &type) == 0 && type == 1);
    PthreadMutex mutex = nullptr;
    Require(scePthreadMutexInitForInternalLibc(&mutex, &mutexAttr) == 0);
    Require(scePthreadMutexLock(&mutex) == 0 && scePthreadMutexLock(&mutex) == static_cast<int>(0x8002000Bu));
    Require(scePthreadMutexUnlock(&mutex) == 0 && scePthreadMutexDestroy(&mutex) == 0);
    Require(scePthreadMutexInitForInternalLibc(&mutex, nullptr) == 0);
    Require(scePthreadMutexLock(&mutex) == 0 && scePthreadMutexLock(&mutex) == static_cast<int>(0x8002000Bu));
    Require(scePthreadMutexUnlock(&mutex) == 0 && scePthreadMutexDestroy(&mutex) == 0);
    Require(scePthreadMutexattrDestroy(&mutexAttr) == 0);
}

std::atomic<bool> joinRelease{false};

void* APS5_VABI JoinTarget(void* argument) {
    while (!joinRelease.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return argument;
}

KernelTimespec RealtimeAfter(std::int64_t milliseconds) {
    KernelTimespec now{};
    Require(clock_gettime_nid_postfix(0, &now) == 0);
    const std::int64_t nanos = now.tv_nsec + milliseconds * 1000000;
    now.tv_sec += nanos / 1000000000;
    now.tv_nsec = nanos % 1000000000;
    if (now.tv_nsec < 0) {
        now.tv_nsec += 1000000000;
        --now.tv_sec;
    }
    return now;
}

void CheckPosixThreadExtensions() {
    Pthread thread = nullptr;
    joinRelease = false;
    Require(pthread_create_nid_postfix(&thread, nullptr, JoinTarget, reinterpret_cast<void*>(0x1234)) == 0);
    void* value = nullptr;
    const auto soon = RealtimeAfter(30);
    Require(pthread_timedjoin_np_nid_postfix(thread, &value, &soon) == 60 && value == nullptr);
    const auto past = RealtimeAfter(-1000);
    Require(pthread_timedjoin_np_nid_postfix(thread, &value, &past) == 60);
    auto invalid = RealtimeAfter(1000);
    invalid.tv_nsec = 1000000000;
    Require(pthread_timedjoin_np_nid_postfix(thread, &value, &invalid) == 22 && pthread_timedjoin_np_nid_postfix(thread, &value, nullptr) == 22);
    const auto self = RealtimeAfter(1000);
    Require(pthread_timedjoin_np_nid_postfix(scePthreadSelf(), &value, &self) == 11);
    joinRelease = true;
    const auto later = RealtimeAfter(10000);
    Require(pthread_timedjoin_np_nid_postfix(thread, &value, &later) == 0 && value == reinterpret_cast<void*>(0x1234));
    PthreadRwlock rwlock = nullptr;
    Require(pthread_rwlock_wrlock_nid_postfix(&rwlock) == 0);
    Require(pthread_rwlock_reltimedrdlock_np_nid_postfix(&rwlock, 1000) == 11);
    int readTimeout = 0;
    int writeTimeout = 0;
    std::thread contender([&] {
        readTimeout = pthread_rwlock_reltimedrdlock_np_nid_postfix(&rwlock, 2000);
        writeTimeout = pthread_rwlock_reltimedwrlock_np_nid_postfix(&rwlock, 2000);
    });
    contender.join();
    Require(readTimeout == 60 && writeTimeout == 60);
    Require(pthread_rwlock_unlock_nid_postfix(&rwlock) == 0);
    Require(pthread_rwlock_reltimedrdlock_np_nid_postfix(&rwlock, 1000) == 0 && pthread_rwlock_unlock_nid_postfix(&rwlock) == 0);
    Require(pthread_rwlock_reltimedwrlock_np_nid_postfix(&rwlock, 1000) == 0 && pthread_rwlock_unlock_nid_postfix(&rwlock) == 0);
    Require(pthread_rwlock_setname_np_nid_postfix(&rwlock, "rw") == 0 && pthread_rwlock_setname_np_nid_postfix(&rwlock, nullptr) == 22);
    Require(pthread_rwlock_destroy_nid_postfix(&rwlock) == 0);
    PthreadRwlockattr rwlockAttr = nullptr;
    int type = -1;
    Require(scePthreadRwlockattrInit(&rwlockAttr) == 0);
    Require(pthread_rwlockattr_settype_np_nid_postfix(&rwlockAttr, 1) == 0);
    Require(pthread_rwlockattr_gettype_np_nid_postfix(&rwlockAttr, &type) == 0 && type == 1);
    Require(scePthreadRwlockattrDestroy(&rwlockAttr) == 0);
    PthreadRwlockattr noAttr = nullptr;
    Require(pthread_rwlockattr_settype_np_nid_postfix(&noAttr, 1) == 22 && pthread_rwlockattr_settype_np_nid_postfix(nullptr, 1) == 22);
    PthreadMutex mutex = nullptr;
    Require(scePthreadMutexInit(&mutex, nullptr, "named") == 0);
    Require(pthread_mutex_setname_np_nid_postfix(&mutex, "renamed") == 0 && pthread_mutex_setname_np_nid_postfix(&mutex, nullptr) == 0);
    Require(scePthreadMutexDestroy(&mutex) == 0);
    PthreadCond cond = nullptr;
    Require(scePthreadCondInit(&cond, nullptr, "cond") == 0);
    Require(pthread_cond_setname_np_nid_postfix(&cond, "renamed") == 0 && pthread_cond_setname_np_nid_postfix(&cond, nullptr) == 22);
    Require(scePthreadCondDestroy(&cond) == 0);
    void* semaphore = nullptr;
    Require(sem_init_nid_postfix(&semaphore, 0, 1) == 0);
    *__error_nid_postfix() = 0;
    Require(sem_setname_nid_postfix(&semaphore, "sem") == 0 && Error() == 0);
    Require(sem_setname_nid_postfix(&semaphore, nullptr) == -1 && Error() == 22);
    Require(sem_destroy_nid_postfix(&semaphore) == 0);
}

void CheckOwnershipAndLinks(const std::filesystem::path& root) {
    const auto directory = root / "owners";
    Require(std::filesystem::create_directory(directory));
    const auto file = (directory / "owned.txt").string();
    std::ofstream(file) << "owned";
    const auto missing = (directory / "missing.txt").string();
    Require(chown_nid_postfix(file.c_str(), 0, 0) == 0 && chown_nid_postfix(file.c_str(), 0xFFFFFFFFu, 0xFFFFFFFFu) == 0);
    Require(chown_nid_postfix(missing.c_str(), 0, 0) == -1 && Error() == 2);
    Require(chown_nid_postfix(nullptr, 0, 0) == -1 && Error() == 14);
    Require(chown_nid_postfix("", 0, 0) == -1 && Error() == 2);
    Require(chown_nid_postfix((file + "/inner").c_str(), 0, 0) == -1 && Error() == 20);
    Require(chown_nid_postfix((directory / std::string(256, 'n')).string().c_str(), 0, 0) == -1 && Error() == 63);
    Require(Throws([&] { chown_nid_postfix(file.c_str(), 1, 0); }) && Throws([&] { chown_nid_postfix(file.c_str(), 0, 1); }));
    Require(chown_nid_postfix(missing.c_str(), 1, 1) == -1 && Error() == 2);
    Require(lchown_nid_postfix(file.c_str(), 0, 0) == 0 && lchown_nid_postfix(missing.c_str(), 0, 0) == -1 && Error() == 2);
    Require(Throws([&] { lchown_nid_postfix(file.c_str(), 7, 7); }));
    const auto dangling = (directory / "dangling").string();
    const bool symbolic = symlink_nid_postfix("absent.txt", dangling.c_str()) == 0;
    if (!symbolic) Require(Error() == 1);
    if (symbolic) Require(lchown_nid_postfix(dangling.c_str(), 0, 0) == 0 && chown_nid_postfix(dangling.c_str(), 0, 0) == -1 && Error() == 2);
    const int base = sceKernelOpen(directory.string().c_str(), SCE_KERNEL_O_RDONLY | SCE_KERNEL_O_DIRECTORY, 0);
    Require(base >= 0);
    Require(fchownat_nid_postfix(base, "owned.txt", 0, 0, 0) == 0);
    if (symbolic) Require(fchownat_nid_postfix(base, "dangling", 0, 0, 0x0200) == 0 && fchownat_nid_postfix(base, "dangling", 0, 0, 0) == -1 && Error() == 2);
    Require(fchownat_nid_postfix(base, "owned.txt", 0, 0, 0x0400) == -1 && Error() == 22);
    Require(fchownat_nid_postfix(-100, file.c_str(), 0, 0, 0) == 0);
    Require(fchownat_nid_postfix(-1, "owned.txt", 0, 0, 0) == -1 && Error() == 9);
    Require(fchownat_nid_postfix(base, nullptr, 0, 0, 0) == -1 && Error() == 14);
    Require(Throws([&] { fchownat_nid_postfix(base, "owned.txt", 3, 0, 0); }));
    Require(lchmod_nid_postfix(file.c_str(), 0644) == 0);
    Require(lchmod_nid_postfix(missing.c_str(), 0644) == -1 && Error() == 2);
    Require(lchmod_nid_postfix(nullptr, 0644) == -1 && Error() == 14);
    const auto linked = (directory / "linked.txt").string();
    Require(linkat_nid_postfix(-100, file.c_str(), -100, linked.c_str(), 0) == 0);
    Require(std::filesystem::equivalent(file, linked) && std::filesystem::hard_link_count(file) == 2);
    Require(linkat_nid_postfix(base, "owned.txt", base, "relative.txt", 0x0400) == 0);
    Require(std::filesystem::equivalent(file, directory / "relative.txt") && std::filesystem::hard_link_count(file) == 3);
    Require(linkat_nid_postfix(base, "owned.txt", base, "linked.txt", 0) == -1 && Error() == 17);
    Require(linkat_nid_postfix(base, "missing.txt", base, "other.txt", 0) == -1 && Error() == 2);
    Require(linkat_nid_postfix(-100, directory.string().c_str(), base, "folder", 0) == -1 && Error() == 1);
    Require(linkat_nid_postfix(base, "owned.txt", base, "other.txt", 0x0200) == -1 && Error() == 22);
    Require(linkat_nid_postfix(-1, "owned.txt", base, "other.txt", 0) == -1 && Error() == 9);
    Require(linkat_nid_postfix(base, "owned.txt", -1, "other.txt", 0) == -1 && Error() == 9);
    Require(linkat_nid_postfix(base, nullptr, base, "other.txt", 0) == -1 && Error() == 14);
    Require(linkat_nid_postfix(base, "", base, "other.txt", 0) == -1 && Error() == 2);
    if (symbolic) Require(Throws([&] { linkat_nid_postfix(base, "dangling", base, "other.txt", 0); }));
    Require(!std::filesystem::exists(std::filesystem::symlink_status(directory / "other.txt")));
    Require(sceKernelClose(base) == 0);
    char expected[64] = {};
    Require(getlogin_r_nid_postfix(expected, sizeof(expected)) == 0);
    char* login = getlogin_nid_postfix();
    if (std::strlen(expected) < 33) Require(login != nullptr && std::strcmp(login, expected) == 0 && getlogin_nid_postfix() == login);
    else Require(login == nullptr && Error() == 34);
    *__error_nid_postfix() = 0;
    Require(vfork_nid_postfix() == -1 && Error() == 78);
}

TestAiocb Request(int fd, std::int64_t offset, void* buffer, std::uint64_t bytes) {
    TestAiocb block{};
    block.fildes = fd;
    block.offset = offset;
    block.buf = buffer;
    block.nbytes = bytes;
    block.status = 7;
    block.error = 7;
    return block;
}

void CheckPosixAio(const std::filesystem::path& root) {
    const auto path = (root / "aio.bin").string();
    std::ofstream(path) << "0123456789";
    const int fd = sceKernelOpen(path.c_str(), SCE_KERNEL_O_RDWR, 0);
    Require(fd >= 0);
    const KernelTimespec zero{};
    const KernelTimespec brief{0, 20000000};
    Require(aio_suspend_nid_postfix(nullptr, 0, nullptr) == -1 && Error() == 35);
    Require(aio_cancel_nid_postfix(fd, nullptr) == 3);
    char buffer[4] = {};
    auto block = Request(fd, 2, buffer, sizeof(buffer));
    Require(aio_error_nid_postfix(&block) == 22);
    Require(aio_return_nid_postfix(&block) == -1 && Error() == 22);
    Require(aio_read_nid_postfix(&block) == 0 && block.error == 36 && block.status == -1);
    Require(block.kernelinfo != nullptr && block.kernelinfo != reinterpret_cast<void*>(-1));
    Require(aio_error_nid_postfix(&block) == 0);
    const TestAiocb* listed[] = {nullptr, &block};
    Require(aio_suspend_nid_postfix(listed, 2, nullptr) == 0 && aio_suspend_nid_postfix(listed, 2, &zero) == 0);
    Require(aio_return_nid_postfix(&block) == 4 && std::memcmp(buffer, "2345", 4) == 0 && block.status == 4 && block.error == 0);
    Require(aio_return_nid_postfix(&block) == -1 && Error() == 22 && aio_error_nid_postfix(&block) == 22);
    Require(aio_suspend_nid_postfix(listed, 2, nullptr) == 0);
    auto write = Request(fd, 8, const_cast<char*>("ab"), 2);
    Require(aio_write_nid_postfix(&write) == 0);
    TestAiocb* completed = nullptr;
    Require(aio_waitcomplete_nid_postfix(&completed, &zero) == 2 && completed == &write && write.status == 2 && write.error == 0);
    char contents[10] = {};
    Require(sceKernelPread(fd, contents, sizeof(contents), 0) == 10 && std::memcmp(contents, "01234567ab", 10) == 0);
    Require(aio_waitcomplete_nid_postfix(&completed, &zero) == -1 && Error() == 35 && completed == nullptr);
    Require(aio_waitcomplete_nid_postfix(&completed, &brief) == -1 && Error() == 35);
    const KernelTimespec invalid{0, 1000000000};
    Require(aio_waitcomplete_nid_postfix(&completed, &invalid) == -1 && Error() == 22);
    auto empty = Request(fd, 0, buffer, 0);
    Require(aio_read_nid_postfix(&empty) == 0 && aio_return_nid_postfix(&empty) == 0);
    auto sync = Request(fd, 0, nullptr, 0);
    Require(aio_fsync_nid_postfix(0, &sync) == -1 && Error() == 22 && sync.error == 7);
    Require(aio_fsync_nid_postfix(0x80, &sync) == 0 && aio_error_nid_postfix(&sync) == 0 && aio_return_nid_postfix(&sync) == 0);
    auto bad = Request(-1, 0, buffer, 1);
    Require(aio_read_nid_postfix(&bad) == -1 && Error() == 9 && bad.error == 9 && bad.status == -1);
    Require(aio_error_nid_postfix(&bad) == 9 && aio_return_nid_postfix(&bad) == -1 && Error() == 22);
    auto negative = Request(fd, -1, buffer, 1);
    Require(aio_read_nid_postfix(&negative) == -1 && Error() == 22 && negative.error == 22);
    auto huge = Request(fd, 0, buffer, 0x8000000000000000ull);
    Require(aio_read_nid_postfix(&huge) == -1 && Error() == 22 && huge.error == 0);
    auto thread = Request(fd, 0, buffer, 1);
    thread.notify = 2;
    Require(aio_read_nid_postfix(&thread) == -1 && Error() == 22 && thread.error == 22);
    auto signal = Request(fd, 0, buffer, 1);
    signal.notify = 1;
    Require(aio_read_nid_postfix(&signal) == -1 && Error() == 22);
    signal.signo = 30;
    Require(Throws([&] { aio_read_nid_postfix(&signal); }));
    Require(aio_read_nid_postfix(nullptr) == -1 && Error() == 14);
    const int readOnly = sceKernelOpen(path.c_str(), SCE_KERNEL_O_RDONLY, 0);
    const int writeOnly = sceKernelOpen(path.c_str(), SCE_KERNEL_O_WRONLY, 0);
    Require(readOnly >= 0 && writeOnly >= 0);
    auto readFromWriter = Request(writeOnly, 0, buffer, 1);
    Require(aio_read_nid_postfix(&readFromWriter) == -1 && Error() == 9);
    auto writeToReader = Request(readOnly, 0, buffer, 1);
    Require(aio_write_nid_postfix(&writeToReader) == -1 && Error() == 9);
    Require(sceKernelClose(readOnly) == 0 && sceKernelClose(writeOnly) == 0);
    int streams[2];
    Require(pipe_nid_postfix(streams) == 0);
    auto piped = Request(streams[0], 0, buffer, 1);
    Require(aio_read_nid_postfix(&piped) == -1 && Error() == 45 && piped.error == 45);
    auto pipeSync = Request(streams[1], 0, nullptr, 0);
    Require(aio_fsync_nid_postfix(0x80, &pipeSync) == -1 && Error() == 22);
    Require(close_nid_postfix(streams[0]) == 0 && close_nid_postfix(streams[1]) == 0);
    Require(aio_suspend_nid_postfix(listed, -1, nullptr) == -1 && Error() == 22);
    Require(aio_suspend_nid_postfix(nullptr, 1, nullptr) == -1 && Error() == 14);
    Require(aio_suspend_nid_postfix(listed, 2, &invalid) == -1 && Error() == 22);
    Require(aio_cancel_nid_postfix(-1, nullptr) == -1 && Error() == 9);
    Require(aio_cancel_nid_postfix(fd, &block) == 3);
    std::vector<TestAiocb> many(257);
    std::vector<char> bytes(many.size());
    for (std::size_t index = 0; index < 256; ++index) {
        many[index] = Request(fd, static_cast<std::int64_t>(index % 10), &bytes[index], 1);
        Require(aio_read_nid_postfix(&many[index]) == 0);
    }
    many[256] = Request(fd, 0, &bytes[256], 1);
    Require(aio_read_nid_postfix(&many[256]) == -1 && Error() == 35 && many[256].error == 35);
    for (std::size_t index = 0; index < 256; ++index) {
        Require(aio_waitcomplete_nid_postfix(&completed, nullptr) == 1 && completed == &many[index]);
        Require(bytes[index] == contents[index % 10]);
    }
    TestAiocb* woken = nullptr;
    std::int64_t wokenStatus = 0;
    std::thread waiter([&] { wokenStatus = aio_waitcomplete_nid_postfix(&woken, nullptr); });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    auto late = Request(fd, 9, buffer, 1);
    Require(aio_read_nid_postfix(&late) == 0);
    waiter.join();
    Require(wokenStatus == 1 && woken == &late && buffer[0] == 'b');
    Require(sceKernelClose(fd) == 0);
}

int SemaphoreValue(void* semaphore) {
    int value = -1;
    Require(sem_getvalue_nid_postfix(semaphore, &value) == 0);
    return value;
}

void CheckNamedSemaphores() {
    constexpr int Create = SCE_KERNEL_O_CREAT;
    constexpr int Exclusive = SCE_KERNEL_O_EXCL;
    const char* name = "/anyps5-kernel-sys-semaphore";
    Require(sem_open_nid_postfix("anyps5", 0) == nullptr && Error() == 22);
    Require(sem_open_nid_postfix(name, 0) == nullptr && Error() == 2);
    Require(sem_open_nid_postfix(name, SCE_KERNEL_O_RDWR) == nullptr && Error() == 22);
    void* first = sem_open_nid_postfix(name, Create, 0644, 2);
    Require(first != nullptr && SemaphoreValue(first) == 2);
    Require(sem_trywait_nid_postfix(first) == 0 && sem_trywait_nid_postfix(first) == 0);
    Require(sem_trywait_nid_postfix(first) == -1 && Error() == 35);
    Require(sem_post_nid_postfix(first) == 0);
    Require(sem_open_nid_postfix(name, 0) == first && sem_open_nid_postfix(name, Create, 0644, 9) == first && SemaphoreValue(first) == 1);
    Require(sem_open_nid_postfix(name, Create | Exclusive, 0644, 1) == nullptr && Error() == 17);
    Require(sem_close_nid_postfix(first) == 0 && sem_close_nid_postfix(first) == 0 && sem_close_nid_postfix(first) == 0);
    Require(sem_close_nid_postfix(first) == -1 && Error() == 22);
    void* reopened = sem_open_nid_postfix(name, 0);
    Require(reopened != nullptr && SemaphoreValue(reopened) == 1);
    Require(sem_unlink_nid_postfix(name) == 0);
    Require(sem_unlink_nid_postfix(name) == -1 && Error() == 2);
    Require(sem_unlink_nid_postfix("anyps5") == -1 && Error() == 2);
    Require(sem_post_nid_postfix(reopened) == 0 && SemaphoreValue(reopened) == 2);
    Require(sem_open_nid_postfix(name, 0) == nullptr && Error() == 2);
    void* replaced = sem_open_nid_postfix(name, Create | Exclusive, 0600, 0);
    Require(replaced != nullptr && replaced != reopened && SemaphoreValue(replaced) == 0 && SemaphoreValue(reopened) == 2);
    Require(sem_open_nid_postfix(name, 0) == replaced);
    Require(sem_close_nid_postfix(reopened) == 0 && sem_close_nid_postfix(replaced) == 0 && sem_close_nid_postfix(replaced) == 0);
    Require(sem_unlink_nid_postfix(name) == 0);
    const auto longPath = "/" + std::string(1015, 'p');
    Require(sem_open_nid_postfix(longPath.c_str(), Create, 0644, 1) == nullptr && Error() == 63);
    Require(sem_unlink_nid_postfix(longPath.c_str()) == -1 && Error() == 63);
    const auto longName = "/" + std::string(252, 'c');
    Require(sem_open_nid_postfix(longName.c_str(), Create, 0644, 1) == nullptr && Error() == 63);
    Require(sem_open_nid_postfix(longName.c_str(), SCE_KERNEL_O_RDWR, 0644, 1) == nullptr && Error() == 22);
    const auto fittingName = "/" + std::string(251, 'c');
    void* fitting = sem_open_nid_postfix(fittingName.c_str(), Create, 0644, 1);
    Require(fitting != nullptr && sem_close_nid_postfix(fitting) == 0 && sem_unlink_nid_postfix(fittingName.c_str()) == 0);
    Require(sem_open_nid_postfix("/anyps5-kernel-sys-big", Create, 0644, -1) == nullptr && Error() == 22);
    Require(sem_open_nid_postfix("/anyps5-kernel-sys-big", 0) == nullptr && Error() == 2);
    Require(Throws([] { sem_open_nid_postfix("/anyps5/nested", Create, 0644, 1); }));
    void* unnamed = nullptr;
    Require(sem_init_nid_postfix(&unnamed, 0, 1) == 0);
    Require(sem_close_nid_postfix(&unnamed) == -1 && Error() == 22);
    Require(sem_destroy_nid_postfix(&unnamed) == 0);
}

std::array<std::uint32_t, 4> SignalMask() {
    std::array<std::uint32_t, 4> mask{};
    Require(sigprocmask_nid_postfix(1, nullptr, mask.data()) == 0);
    return mask;
}

void CheckTimeAndSelect() {
    const auto before = SignalMask();
    std::array<std::uint32_t, 4> blocked = before;
    blocked[0] |= 1u << (30 - 1);
    const KernelTimespec brief{0, 1000000};
    Require(pselect_nid_postfix(0, nullptr, nullptr, nullptr, &brief, blocked.data()) == 0);
    Require(SignalMask() == before);
    const KernelTimespec invalid{0, 1000000000};
    Require(pselect_nid_postfix(0, nullptr, nullptr, nullptr, &invalid, blocked.data()) == -1 && Error() == 22 && SignalMask() == before);
    const KernelTimespec negative{0, -1000};
    Require(pselect_nid_postfix(0, nullptr, nullptr, nullptr, &negative, nullptr) == -1 && Error() == 22);
    Require(pselect_nid_postfix(-1, nullptr, nullptr, nullptr, &brief, nullptr) == -1 && Error() == 22);
    int streams[2];
    Require(pipe_nid_postfix(streams) == 0);
    std::array<std::uint64_t, 16> writable{};
    writable[static_cast<std::size_t>(streams[1]) / 64] |= std::uint64_t{1} << (streams[1] % 64);
    const KernelTimespec second{1, 0};
    Require(pselect_nid_postfix(streams[1] + 1, nullptr, writable.data(), nullptr, &second, blocked.data()) == 1);
    Require((writable[static_cast<std::size_t>(streams[1]) / 64] >> (streams[1] % 64)) & 1);
    Require(SignalMask() == before);
    Require(close_nid_postfix(streams[0]) == 0 && close_nid_postfix(streams[1]) == 0);
    Require(settimeofday_nid_postfix(nullptr, nullptr) == 0);
    const KernelTimezone utc{0, 0};
    Require(settimeofday_nid_postfix(nullptr, &utc) == 0 && sceKernelSettimeofday(nullptr, &utc) == 0);
    const KernelTimeval badTime{1, 1000000};
    Require(settimeofday_nid_postfix(&badTime, nullptr) == -1 && Error() == 22);
    Require(sceKernelSettimeofday(&badTime, nullptr) == static_cast<int>(0x80020016u));
    const KernelTimeval earlier{-1, 0};
    Require(settimeofday_nid_postfix(&earlier, &utc) == -1 && Error() == 22);
    const KernelTimeval valid{1700000000, 0};
    Require(Throws([&] { settimeofday_nid_postfix(&valid, nullptr); }) && Throws([&] { sceKernelSettimeofday(&valid, &utc); }));
    const KernelTimezone east{-60, 0};
    Require(Throws([&] { settimeofday_nid_postfix(nullptr, &east); }));
    Require(sceKernelMlockall(0) == static_cast<int>(0x80020016u) && sceKernelMlockall(4) == static_cast<int>(0x80020016u));
    Require(Throws([] { sceKernelMlockall(1); }));
}

void CheckModuleAndTerminalQueries(const std::filesystem::path& root) {
    std::array<KernelModule, 512> modules{};
    std::size_t count = 0;
    Require(sceKernelGetModuleList(modules.data(), modules.size(), &count) == 0);
    for (std::size_t index = 0; index < count; ++index) {
        ModuleInfoEx extended{};
        extended.st_size = sizeof(extended);
        ModuleInfo plain{};
        plain.st_size = sizeof(plain);
        const int internal = sceKernelGetModuleInfoInternal(modules[index], &extended);
        Require(internal == sceKernelGetModuleInfo(modules[index], &plain));
        if (internal != 0) continue;
        Require(extended.st_size == sizeof(extended) && extended.id == modules[index] && std::strcmp(extended.name, plain.name) == 0);
        Require(extended.segment_count == plain.segment_count && extended.ref_count == 1);
        for (std::uint32_t segment = 0; segment < plain.segment_count; ++segment)
            Require(extended.segments[segment].address == plain.segments[segment].address && extended.segments[segment].size == plain.segments[segment].size);
    }
    ModuleInfoEx probe{};
    probe.st_size = sizeof(probe) - 8;
    if (count != 0) Require(sceKernelGetModuleInfoInternal(modules[0], &probe) == static_cast<int>(0x80020016u));
    Require(sceKernelGetModuleInfoInternal(0, nullptr) == static_cast<int>(0x8002000Eu));
    probe.st_size = sizeof(probe);
    Require(sceKernelGetModuleInfoInternal(-0x5a5a, &probe) == static_cast<int>(0x80020003u));
    Require(sceKernelIsInSandbox() == 1);
    const auto path = (root / "terminal.txt").string();
    std::ofstream(path) << "t";
    const int fd = sceKernelOpen(path.c_str(), SCE_KERNEL_O_RDONLY, 0);
    Require(fd >= 0);
    Require(tcgetsid_nid_postfix(fd) == -1 && Error() == 25);
    Require(tcgetsid_nid_postfix(-1) == -1 && Error() == 9);
    const int session = getsid_nid_postfix(0);
    Require(tcsetsid_nid_postfix(fd, session + 1) == -1 && Error() == 22);
    Require(tcsetsid_nid_postfix(fd, session) == -1 && Error() == 25);
    Require(tcsetsid_nid_postfix(-1, session) == -1 && Error() == 9);
    Require(sceKernelClose(fd) == 0);
}

int main() {
#ifdef _WIN32
    auto hostDirectorySpelling = std::filesystem::current_path().native();
    CharUpperBuffW(hostDirectorySpelling.data(), static_cast<DWORD>(hostDirectorySpelling.size()));
    Require(SetCurrentDirectoryW(hostDirectorySpelling.c_str()) != 0);
#endif
    CheckProcessQueries();
    Require(setsid_nid_postfix() == -1 && Error() == 1);
    *__error_nid_postfix() = 0;
    Require(fork_nid_postfix() == -1 && Error() == 78);
    Require(fork_nid_postfix() == -1 && Error() == 78);
    SceKernelFlock();
    Require(umask_nid_postfix(0) == 022);
    Require(umask_nid_postfix(07777) == 0);
    Require(umask_nid_postfix(022) == 07777);
    const auto root = std::filesystem::path("anyps5-kernel-sys-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Require(std::filesystem::create_directory(root));
    DirectoryRenameTests(root);
    const auto file = (root / "file.txt").string();
    std::ofstream(file).put('x');
    CheckPathconf(root, file);
    const int descriptor = open_nid_postfix(file.c_str(), 0, 0);
    Require(descriptor >= 0);
    unsigned char viaPublic[472] = {};
    unsigned char viaSyscall[472] = {};
    Require(fstatfs_nid_postfix(descriptor, viaPublic) == 0 && _fstatfs_nid_postfix(descriptor, viaSyscall) == 0);
    Require(std::memcmp(viaPublic, viaSyscall, 4) == 0 && std::memcmp(viaPublic + 16, viaSyscall + 16, 8) == 0);
    Require(std::memcmp(viaPublic + 280, viaSyscall + 280, 16) == 0);
    Require(fstatfs_nid_postfix(descriptor, nullptr) == -1 && Error() == 14);
    CheckDescriptorPathconf(fpathconf_nid_postfix, descriptor);
    CheckDescriptorPathconf(_fpathconf_nid_postfix, descriptor);
    Require(close_nid_postfix(descriptor) == 0);
    Require(fstatfs_nid_postfix(-1, viaPublic) == -1 && Error() == 9);
    Require(fpathconf_nid_postfix(descriptor, 4) == -1 && Error() == 9);
    Require(_fpathconf_nid_postfix(descriptor, 4) == -1 && Error() == 9);
    const int dirDescriptor = open_nid_postfix(root.generic_string().c_str(), 0, 0);
    Require(dirDescriptor >= 0);
    CheckDescriptorPathconf(fpathconf_nid_postfix, dirDescriptor);
    CheckDescriptorPathconf(_fpathconf_nid_postfix, dirDescriptor);
    int streams[2];
    Require(pipe_nid_postfix(streams) == 0);
    CheckStreamPathconf(fpathconf_nid_postfix, streams[0]);
    CheckStreamPathconf(_fpathconf_nid_postfix, streams[1]);
    Require(close_nid_postfix(streams[0]) == 0 && close_nid_postfix(streams[1]) == 0);
    const int datagram = socket_nid_postfix(2, 2, 0);
    Require(datagram >= 0);
    CheckStreamPathconf(fpathconf_nid_postfix, datagram);
    CheckStreamPathconf(_fpathconf_nid_postfix, datagram);
    Require(close_nid_postfix(datagram) == 0);
    Require(close_nid_postfix(dirDescriptor) == 0);
    char buffer[64];
    for (const auto& component : {std::string("missing"), std::string("file.txt")}) {
        const auto prefix = root.generic_string() + "/" + component + "/../";
        const auto invalid = prefix + "file.txt";
        const auto created = prefix + "new.txt";
        const int expected = component == "missing" ? 2 : 20;
        Require(readlink_nid_postfix(invalid.c_str(), buffer, sizeof(buffer)) == -1 && Error() == expected);
        Require(link_nid_postfix(invalid.c_str(), created.c_str()) == -1 && Error() == expected);
        Require(link_nid_postfix(file.c_str(), created.c_str()) == -1 && Error() == expected);
        Require(symlink_nid_postfix("file.txt", created.c_str()) == -1 && Error() == expected);
        Require(std::filesystem::hard_link_count(file) == 1 && !std::filesystem::exists(root / "new.txt"));
    }
    Require(readlink_nid_postfix(file.c_str(), buffer, sizeof(buffer)) == -1 && Error() == 22);
    Require(readlink_nid_postfix(root.string().c_str(), buffer, sizeof(buffer)) == -1 && Error() == 22);
    Require(readlink_nid_postfix((root / "missing").string().c_str(), buffer, sizeof(buffer)) == -1 && Error() == 2);
    Require(readlink_nid_postfix((root / "file.txt" / "child").string().c_str(), buffer, sizeof(buffer)) == -1 && Error() == 20);
    Require(readlink_nid_postfix("", buffer, sizeof(buffer)) == -1 && Error() == 2);
    Require(readlink_nid_postfix(nullptr, buffer, sizeof(buffer)) == -1 && Error() == 14);
    constexpr auto hugeSize = static_cast<std::size_t>(-1);
    Require(readlink_nid_postfix(nullptr, buffer, hugeSize) == -1 && Error() == 22);
    Require(readlink_nid_postfix("", buffer, hugeSize) == -1 && Error() == 22);
    Require(readlink_nid_postfix((root / "missing").string().c_str(), buffer, hugeSize) == -1 && Error() == 22);
    Require(readlinkat_nid_postfix(-100, nullptr, buffer, hugeSize) == -1 && Error() == 22);
    Require(readlinkat_nid_postfix(-100, "", buffer, hugeSize) == -1 && Error() == 22);
    Require(readlinkat_nid_postfix(-100, nullptr, buffer, sizeof(buffer)) == -1 && Error() == 14);
    Require(readlink_nid_postfix((root / std::string(300, 'n')).string().c_str(), buffer, sizeof(buffer)) == -1 && Error() == 63);
    Require(readlink_nid_postfix(("/" + std::string(1024, 'p')).c_str(), buffer, sizeof(buffer)) == -1 && Error() == 63);
    Require(readlink_nid_postfix((root.generic_string() + "/missing/" + std::string(256, 'n')).c_str(), buffer, sizeof(buffer)) == -1 && Error() == 2);
#ifdef _WIN32
    const auto setReparseTag = [](const std::filesystem::path& path, DWORD tag) {
        const HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        Require(handle != INVALID_HANDLE_VALUE);
        std::array<std::byte, REPARSE_GUID_DATA_BUFFER_HEADER_SIZE + 4> reparseData{};
        REPARSE_GUID_DATA_BUFFER reparseHeader{};
        reparseHeader.ReparseTag = tag;
        reparseHeader.ReparseDataLength = 4;
        reparseHeader.ReparseGuid = {0x6a1e3d0b, 0x1f6c, 0x4c1e, {0x9b, 0x52, 0x3d, 0x8e, 0x41, 0x07, 0x5c, 0x2a}};
        std::memcpy(reparseData.data(), &reparseHeader, REPARSE_GUID_DATA_BUFFER_HEADER_SIZE);
        DWORD returned = 0;
        const bool set = DeviceIoControl(handle, FSCTL_SET_REPARSE_POINT, reparseData.data(), static_cast<DWORD>(reparseData.size()),
            nullptr, 0, &returned, nullptr) != 0;
        if (!set) Require(GetLastError() == ERROR_PRIVILEGE_NOT_HELD || GetLastError() == ERROR_ACCESS_DENIED);
        Require(CloseHandle(handle) != 0);
        if (!set) {
            Require(std::filesystem::remove(path));
            std::fprintf(stderr, "Windows reparse point checks skipped: setting a reparse point on a file needs rights this user lacks\n");
            return false;
        }
        Require((GetFileAttributesW(path.c_str()) & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
        return true;
    };
    const auto clearReparseTag = [](const std::filesystem::path& path, DWORD tag) {
        const HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        Require(handle != INVALID_HANDLE_VALUE);
        REPARSE_GUID_DATA_BUFFER reparseHeader{};
        reparseHeader.ReparseTag = tag;
        reparseHeader.ReparseGuid = {0x6a1e3d0b, 0x1f6c, 0x4c1e, {0x9b, 0x52, 0x3d, 0x8e, 0x41, 0x07, 0x5c, 0x2a}};
        DWORD returned = 0;
        Require(DeviceIoControl(handle, FSCTL_DELETE_REPARSE_POINT, &reparseHeader, REPARSE_GUID_DATA_BUFFER_HEADER_SIZE,
            nullptr, 0, &returned, nullptr) != 0);
        Require(CloseHandle(handle) != 0 && std::filesystem::remove(path));
    };
    const auto tagged = root / "tagged.bin";
    std::ofstream(tagged, std::ios::binary) << "abc";
    if (setReparseTag(tagged, 0x00000ABC)) {
        Require(readlink_nid_postfix(tagged.string().c_str(), buffer, sizeof(buffer)) == -1 && Error() == 22);
        clearReparseTag(tagged, 0x00000ABC);
    }
    const auto surrogate = root / "surrogate.bin";
    std::ofstream(surrogate, std::ios::binary) << "abc";
    if (setReparseTag(surrogate, 0x20000ABC)) {
        Require(readlink_nid_postfix(surrogate.string().c_str(), buffer, sizeof(buffer)) == -1 && Error() == 22);
        FileStat surrogateStatus{};
        Require(lstat_nid_postfix(surrogate.string().c_str(), &surrogateStatus) == 0);
        Require(surrogateStatus.st_mode == 0100666 && surrogateStatus.st_nlink == 1);
        clearReparseTag(surrogate, 0x20000ABC);
    }
#endif
    std::error_code error;
    const auto link = (root / "link").string();
    std::filesystem::create_symlink("sub/target.txt", link, error);
    if (!error) {
        std::memset(buffer, '#', sizeof(buffer));
        Require(readlink_nid_postfix(link.c_str(), buffer, sizeof(buffer)) == 14);
        Require(std::memcmp(buffer, "sub/target.txt", 14) == 0 && buffer[14] == '#');
        std::memset(buffer, '#', sizeof(buffer));
        Require(readlink_nid_postfix(link.c_str(), buffer, 3) == 3);
        Require(std::memcmp(buffer, "sub", 3) == 0 && buffer[3] == '#');
        Require(readlink_nid_postfix(link.c_str(), nullptr, 0) == 0);
        Require(readlink_nid_postfix(link.c_str(), nullptr, sizeof(buffer)) == -1 && Error() == 14);
        Require(readlink_nid_postfix((root / "LINK").string().c_str(), buffer, sizeof(buffer)) == 14);
        const int linkDir = sceKernelOpen(root.string().c_str(), SCE_KERNEL_O_RDONLY | SCE_KERNEL_O_DIRECTORY, 0);
        Require(linkDir >= 0);
        std::memset(buffer, '#', sizeof(buffer));
        Require(readlinkat_nid_postfix(linkDir, "link", buffer, sizeof(buffer)) == 14);
        Require(std::memcmp(buffer, "sub/target.txt", 14) == 0 && buffer[14] == '#');
        Require(readlinkat_nid_postfix(linkDir, "link", buffer, 3) == 3 && std::memcmp(buffer, "sub", 3) == 0);
        Require(readlinkat_nid_postfix(linkDir, "link", nullptr, 0) == 0);
        Require(readlinkat_nid_postfix(linkDir, "link", nullptr, sizeof(buffer)) == -1 && Error() == 14);
        const auto guestLink = "/" + root.filename().string() + "/link";
        Require(readlinkat_nid_postfix(-1, guestLink.c_str(), buffer, sizeof(buffer)) == 14 && std::memcmp(buffer, "sub/target.txt", 14) == 0);
        Require(readlinkat_nid_postfix(-100, link.c_str(), buffer, sizeof(buffer)) == 14 && std::memcmp(buffer, "sub/target.txt", 14) == 0);
        Require(sceKernelClose(linkDir) == 0);
#ifndef _WIN32
        std::filesystem::create_symlink("loopB", root / "loopA");
        std::filesystem::create_symlink("loopA", root / "loopB");
        Require(readlink_nid_postfix((root / "loopA" / "inner").string().c_str(), buffer, sizeof(buffer)) == -1 && Error() == 62);
#endif
    } else {
        std::fprintf(stderr, "symbolic links unavailable: the link cases not tested\n");
    }
    const auto original = (root / "original.bin").string();
    const auto linked = (root / "linked.bin").string();
    std::ofstream(original) << "abc";
    Require(link_nid_postfix(original.c_str(), linked.c_str()) == 0);
    Require(std::filesystem::hard_link_count(original) == 2 && std::filesystem::file_size(linked) == 3);
    std::ofstream(linked, std::ios::trunc).close();
    Require(std::filesystem::file_size(original) == 0);
    Require(link_nid_postfix(original.c_str(), linked.c_str()) == -1 && Error() == 17);
    Require(link_nid_postfix((root / "missing").string().c_str(), (root / "other").string().c_str()) == -1 && Error() == 2);
    Require(link_nid_postfix(original.c_str(), (root / "missing" / "linked.bin").string().c_str()) == -1 && Error() == 2);
    Require(link_nid_postfix(root.string().c_str(), (root / "directory").string().c_str()) == -1 && Error() == 1);
    Require(link_nid_postfix(nullptr, linked.c_str()) == -1 && Error() == 14);
    Require(link_nid_postfix(original.c_str(), "") == -1 && Error() == 2);
    Require(std::filesystem::remove(linked) && std::filesystem::hard_link_count(original) == 1);
    const int directory = sceKernelOpen(root.string().c_str(), SCE_KERNEL_O_RDONLY | SCE_KERNEL_O_DIRECTORY, 0);
    Require(directory >= 0);
    const int fileDescriptor = sceKernelOpen(file.c_str(), SCE_KERNEL_O_RDONLY, 0);
    Require(fileDescriptor >= 0);
    Require(fchdir_nid_postfix(-1) == -1 && Error() == 9);
    Require(fchdir_nid_postfix(fileDescriptor) == -1 && Error() == 20);
    Require(fchdir_nid_postfix(directory) == 0);
    char directoryPath[1024];
    Require(getcwd_nid_postfix(directoryPath, sizeof(directoryPath)) == directoryPath);
    Require(std::strcmp(directoryPath, ("/" + root.filename().string()).c_str()) == 0);
#ifndef _WIN32
    Require(chdir_nid_postfix("/") == 0);
    const auto originalPermissions = std::filesystem::status(root).permissions();
    std::filesystem::permissions(root, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
    const bool searchDenied = ::access(root.c_str(), X_OK) == -1 && errno == EACCES;
    const int deniedResult = searchDenied ? fchdir_nid_postfix(directory) : 0;
    const int deniedError = Error();
    const bool cwdUnchanged = getcwd_nid_postfix(directoryPath, sizeof(directoryPath)) == directoryPath && std::strcmp(directoryPath, "/") == 0;
    std::filesystem::permissions(root, std::filesystem::perms::owner_exec);
    const int searchOnlyResult = fchdir_nid_postfix(directory);
    std::filesystem::permissions(root, originalPermissions);
    if (searchDenied) Require(deniedResult == -1 && deniedError == EACCES && cwdUnchanged);
    Require(searchOnlyResult == 0);
#endif
    Require(sceKernelClose(fileDescriptor) == 0);
    Require(sceKernelClose(directory) == 0);
    Require(fchdir_nid_postfix(directory) == -1 && Error() == 9);
    Require(chdir_nid_postfix("/") == 0);
    Require(chroot_nid_postfix(nullptr) == -1 && Error() == 14);
    Require(chroot_nid_postfix("") == -1 && Error() == 2);
    Require(chroot_nid_postfix((root / "missing").string().c_str()) == -1 && Error() == 2);
    Require(chroot_nid_postfix(file.c_str()) == -1 && Error() == 20);
    Require(chroot_nid_postfix((root / "file.txt" / "child").string().c_str()) == -1 && Error() == 20);
    Require(chroot_nid_postfix((file + "/").c_str()) == -1 && Error() == 20);
    Require(chroot_nid_postfix((root / "file.txt" / "..").string().c_str()) == -1 && Error() == 20);
    Require(chroot_nid_postfix((root / std::string(256, 'n')).string().c_str()) == -1 && Error() == 63);
    Require(chroot_nid_postfix(("/" + std::string(1024, 'p')).c_str()) == -1 && Error() == 63);
    const std::string throughFile = (root / "file.txt" / "child").string();
    Require(pathconf_nid_postfix(throughFile.c_str(), 4) == -1 && Error() == 20);
    Require(fchmodat_nid_postfix(-100, throughFile.c_str(), 0600, 0) == -1 && Error() == 20);
    Require(symlink_nid_postfix("target", throughFile.c_str()) == -1 && Error() == 20);
    Require(link_nid_postfix(file.c_str(), throughFile.c_str()) == -1 && Error() == 20);
    Require(link_nid_postfix(throughFile.c_str(), (root / "linked").string().c_str()) == -1 && Error() == 20);
    Require(ChrootThrows(root.string()));
    Require(ChrootThrows("/" + root.filename().string() + "/."));
    const auto guestRoot = "/" + root.filename().string();
    const auto chrootFile = guestRoot + "/file.txt";
    const int top = sceKernelOpen("/", SCE_KERNEL_O_RDONLY | SCE_KERNEL_O_DIRECTORY, 0);
    Require(top >= 0);
    Require(chdir_nid_postfix(guestRoot.c_str()) == 0);
    Require(fchdir_nid_postfix(top) == 0 && chroot_nid_postfix(".") == 0);
    Require(getcwd_nid_postfix(directoryPath, sizeof(directoryPath)) == directoryPath && std::strcmp(directoryPath, "/") == 0);
    Require(sceKernelClose(top) == 0);
    Require(chroot_nid_postfix("/") == 0 && chroot_nid_postfix("/..") == 0 && chroot_nid_postfix("//./") == 0);
    Require(chdir_nid_postfix(guestRoot.c_str()) == 0);
    Require(chroot_nid_postfix("..") == 0 && chroot_nid_postfix("../..") == 0);
    Require(getcwd_nid_postfix(directoryPath, sizeof(directoryPath)) == directoryPath && guestRoot == directoryPath);
    Require(ChrootThrows("."));
    for (const auto& path : {std::string("file.txt"), chrootFile, "/../../.." + chrootFile, "../.." + chrootFile}) {
        const int opened = sceKernelOpen(path.c_str(), SCE_KERNEL_O_RDONLY, 0);
        char byte = 0;
        Require(opened >= 0 && sceKernelRead(opened, &byte, 1) == 1 && byte == 'x');
        Require(sceKernelClose(opened) == 0);
    }
    Require(chdir_nid_postfix("/") == 0);
    Require(getcwd_nid_postfix(directoryPath, sizeof(directoryPath)) == directoryPath && std::strcmp(directoryPath, "/") == 0);
    Require(readlink_nid_postfix(file.c_str(), buffer, sizeof(buffer)) == -1 && Error() == 22);
    Require(lchflags_nid_postfix(file.c_str(), 0) == 0);
    Require(lchflags_nid_postfix(nullptr, 0) == -1 && Error() == 14);
    Require(lchflags_nid_postfix("", 0) == -1 && Error() == 2);
    Require(lchflags_nid_postfix((root / "missing.bin").string().c_str(), 0) == -1 && Error() == 2);
    Require(lchflags_nid_postfix((file + "/child").c_str(), 0) == -1 && Error() == 20);
    Require(lchflags_nid_postfix(file.c_str(), 0x20) == -1 && Error() == 45);
    Require(lchflags_nid_postfix(file.c_str(), 0x200000) == -1 && Error() == 1);
    const auto symbolic = (root / "symbolic").string();
    const int created = symlink_nid_postfix("original.bin", symbolic.c_str());
    if (created == 0) {
        const auto dangling = (root / "dangling").string();
#ifdef _WIN32
        Require((GetFileAttributesW(std::filesystem::path(symbolic).wstring().c_str()) & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
        Require(symlink_nid_postfix("original.bin", symbolic.c_str()) == -1 && Error() == 17);
        Require(symlink_nid_postfix("missing.bin", dangling.c_str()) == 0);
        Require((GetFileAttributesW(std::filesystem::path(dangling).wstring().c_str()) & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
        Require(symlink_nid_postfix("missing.bin", dangling.c_str()) == -1 && Error() == 17);
        Require(readlink_nid_postfix(symbolic.c_str(), buffer, sizeof(buffer)) == 12 && std::memcmp(buffer, "original.bin", 12) == 0);
        Require(readlink_nid_postfix(dangling.c_str(), buffer, sizeof(buffer)) == 11 && std::memcmp(buffer, "missing.bin", 11) == 0);
        const auto nested = (root / "nested").string();
        Require(symlink_nid_postfix("sub/target.txt", nested.c_str()) == 0);
        std::memset(buffer, '#', sizeof(buffer));
        Require(readlink_nid_postfix(nested.c_str(), buffer, sizeof(buffer)) == 14);
        Require(std::memcmp(buffer, "sub/target.txt", 14) == 0 && buffer[14] == '#');
        Require(readlink_nid_postfix(nested.c_str(), buffer, 3) == 3 && std::memcmp(buffer, "sub", 3) == 0);
        Require(readlink_nid_postfix(nested.c_str(), nullptr, 0) == 0);
        Require(readlink_nid_postfix(nested.c_str(), nullptr, sizeof(buffer)) == -1 && Error() == 14);
        const auto accented = (root / "accented").string();
        Require(symlink_nid_postfix("a\xc3\xa7\xc3\xa3o.bin", accented.c_str()) == 0);
        Require(readlink_nid_postfix(accented.c_str(), buffer, sizeof(buffer)) == 10 && std::memcmp(buffer, "a\xc3\xa7\xc3\xa3o.bin", 10) == 0);
        Require(std::filesystem::create_directory(root / "sub"));
        const auto directoryLink = (root / "directory-link").string();
        Require(symlink_nid_postfix("sub", directoryLink.c_str()) == 0);
        Require(readlink_nid_postfix(directoryLink.c_str(), buffer, sizeof(buffer)) == 3 && std::memcmp(buffer, "sub", 3) == 0);
        const auto absolute = std::filesystem::absolute(root / "absolute");
        Require(CreateSymbolicLinkW(absolute.wstring().c_str(), std::filesystem::canonical(original).wstring().c_str(),
            SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) != 0);
        Require(ReadsLink((root / "absolute").string(), "/" + root.filename().string() + "/original.bin"));
#else
        Require(std::filesystem::is_symlink(std::filesystem::symlink_status(symbolic)));
        Require(readlink_nid_postfix(symbolic.c_str(), buffer, sizeof(buffer)) == 12 && std::memcmp(buffer, "original.bin", 12) == 0);
        Require(std::filesystem::file_size(symbolic) == std::filesystem::file_size(original));
        Require(symlink_nid_postfix("original.bin", symbolic.c_str()) == -1 && Error() == 17);
        Require(symlink_nid_postfix("missing.bin", dangling.c_str()) == 0);
        Require(readlink_nid_postfix(dangling.c_str(), buffer, sizeof(buffer)) == 11 && !std::filesystem::exists(dangling));
        Require(symlink_nid_postfix("missing.bin", dangling.c_str()) == -1 && Error() == 17);
#endif
        Require(lchflags_nid_postfix(dangling.c_str(), 0) == 0);
        Require(lchflags_nid_postfix(symbolic.c_str(), 0) == 0);
        Require(lchflags_nid_postfix(dangling.c_str(), 0x20) == -1 && Error() == 45);
        Require(lchflags_nid_postfix(dangling.c_str(), 0x200000) == -1 && Error() == 1);
        Require(LchflagsThrows(dangling, 0x2, "UF_IMMUTABLE"));
        CheckAbsoluteTargets(root);
    } else {
        Require(created == -1 && Error() == 1);
        std::fprintf(stderr, "symbolic links unavailable: the symlink success cases not tested\n");
    }
    Require(symlink_nid_postfix("original.bin", (root / "missing" / "symbolic").string().c_str()) == -1 && Error() == 2);
    Require(symlink_nid_postfix("original.bin", (root / "file.txt" / "symbolic").string().c_str()) == -1 && Error() == 20);
    Require(symlink_nid_postfix("original.bin", (root / "file.txt" / "deeper" / "symbolic").string().c_str()) == -1 && Error() == 20);
    Require(symlink_nid_postfix(std::string(1024, 't').c_str(), (root / "long-target").string().c_str()) == -1 && Error() == 63);
    Require(symlink_nid_postfix("original.bin", (root.generic_string() + "/" + std::string(256, 'n')).c_str()) == -1 && Error() == 63);
    Require(symlink_nid_postfix("original.bin", ("/" + std::string(1024, 'p')).c_str()) == -1 && Error() == 63);
    Require(!std::filesystem::exists(std::filesystem::symlink_status(root / "long-target")));
    Require(symlink_nid_postfix(nullptr, symbolic.c_str()) == -1 && Error() == 14);
    Require(symlink_nid_postfix("original.bin", "") == -1 && Error() == 2);
    CheckSymlinkTargetRestrictions(root);
    const int base = sceKernelOpen(root.string().c_str(), SCE_KERNEL_O_RDONLY | SCE_KERNEL_O_DIRECTORY, 0);
    Require(base >= 0);
    const int relative = openat_nid_postfix(base, "file.txt", SCE_KERNEL_O_RDONLY);
    char byte = 0;
    Require(relative >= 0 && sceKernelRead(relative, &byte, 1) == 1 && byte == 'x');
    Require(sceKernelClose(relative) == 0);
    const int made = openat_nid_postfix(base, "made.bin", SCE_KERNEL_O_WRONLY | SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL, 0644);
    Require(made >= 0 && std::filesystem::is_regular_file(root / "made.bin"));
    Require(sceKernelClose(made) == 0);
    Require(openat_nid_postfix(base, "made.bin", SCE_KERNEL_O_WRONLY | SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL, 0644) == -1 && Error() == 17);
    Require(openat_nid_postfix(base, "missing.bin", SCE_KERNEL_O_RDONLY) == -1 && Error() == 2);
    Require(openat_nid_postfix(base, "file.txt", SCE_KERNEL_O_ACCMODE) == -1 && Error() == 22);
    Require(openat_nid_postfix(base, "absent.bin", SCE_KERNEL_O_WRONLY | SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL | SCE_KERNEL_O_DIRECTORY, 0644) == -1 && Error() == 22);
    Require(openat_nid_postfix(base, nullptr, SCE_KERNEL_O_ACCMODE) == -1 && Error() == 22);
    Require(openat_nid_postfix(base, "", SCE_KERNEL_O_ACCMODE) == -1 && Error() == 22);
    Require(openat_nid_postfix(9999, "", SCE_KERNEL_O_WRONLY | SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL | SCE_KERNEL_O_DIRECTORY, 0644) == -1 && Error() == 22);
    Require(openat_nid_postfix(-100, nullptr, SCE_KERNEL_O_ACCMODE) == -1 && Error() == 22);
    const auto guestFile = "/" + root.filename().string() + "/file.txt";
    const int absolute = openat_nid_postfix(-1, guestFile.c_str(), SCE_KERNEL_O_RDONLY);
    Require(absolute >= 0 && sceKernelClose(absolute) == 0);
    const int current = openat_nid_postfix(-100, file.c_str(), SCE_KERNEL_O_RDONLY);
    Require(current >= 0 && sceKernelClose(current) == 0);
    const int plain = sceKernelOpen(file.c_str(), SCE_KERNEL_O_RDONLY, 0);
    Require(plain >= 0);
    Require(openat_nid_postfix(plain, "file.txt", SCE_KERNEL_O_RDONLY) == -1 && Error() == 20);
    Require(readlinkat_nid_postfix(base, "file.txt", buffer, sizeof(buffer)) == -1 && Error() == 22);
    Require(readlinkat_nid_postfix(base, "missing.bin", buffer, sizeof(buffer)) == -1 && Error() == 2);
    Require(readlinkat_nid_postfix(base, "file.txt/child", buffer, sizeof(buffer)) == -1 && Error() == 20);
    Require(readlinkat_nid_postfix(plain, "file.txt", buffer, sizeof(buffer)) == -1 && Error() == 20);
    Require(readlinkat_nid_postfix(-1, "file.txt", buffer, sizeof(buffer)) == -1 && Error() == 9);
    Require(readlinkat_nid_postfix(base, nullptr, buffer, sizeof(buffer)) == -1 && Error() == 14);
    Require(readlinkat_nid_postfix(base, "", buffer, sizeof(buffer)) == -1 && Error() == 2);
    Require(readlinkat_nid_postfix(base, std::string(256, 'r').c_str(), buffer, sizeof(buffer)) == -1 && Error() == 63);
    Require(readlinkat_nid_postfix(-1, guestFile.c_str(), buffer, sizeof(buffer)) == -1 && Error() == 22);
    Require(readlinkat_nid_postfix(-100, file.c_str(), buffer, sizeof(buffer)) == -1 && Error() == 22);
    Require(mkdirat_nid_postfix(base, "dir", 0700) == 0 && std::filesystem::is_directory(root / "dir"));
    Require(mkdirat_nid_postfix(base, "dir", 0700) == -1 && Error() == 17);
    Require(mkdirat_nid_postfix(base, "file.txt", 0700) == -1 && Error() == 17);
    Require(mkdirat_nid_postfix(base, "missing/child", 0700) == -1 && Error() == 2);
    Require(mkdirat_nid_postfix(base, "file.txt/child", 0700) == -1 && Error() == 20);
    Require(mkdirat_nid_postfix(base, "slash_dir/", 0700) == 0 && std::filesystem::is_directory(root / "slash_dir"));
    const auto guestSubdir = "/" + root.filename().string() + "/abs_dir";
    Require(mkdirat_nid_postfix(-1, guestSubdir.c_str(), 0700) == 0 && std::filesystem::is_directory(root / "abs_dir"));
    const auto cwdDir = (root / "cwd_dir").string();
    Require(mkdirat_nid_postfix(-100, cwdDir.c_str(), 0700) == 0 && std::filesystem::is_directory(root / "cwd_dir"));
    Require(mkdirat_nid_postfix(plain, "child", 0700) == -1 && Error() == 20);
    Require(mkdirat_nid_postfix(-1, "child", 0700) == -1 && Error() == 9);
    Require(mkdirat_nid_postfix(base, nullptr, 0700) == -1 && Error() == 14);
    Require(mkdirat_nid_postfix(base, "", 0700) == -1 && Error() == 2);
    const std::string longName(256, 'a');
    Require(mkdirat_nid_postfix(base, longName.c_str(), 0700) == -1 && Error() == 63);
    Require(mkdirat_nid_postfix(base, "dir/../dotdot_dir", 0700) == 0 && std::filesystem::is_directory(root / "dotdot_dir"));
    Require(unlinkat_nid_postfix(plain, "file.txt", 0) == -1 && Error() == 20);
    Require(unlinkat_nid_postfix(-1, "file.txt", 0) == -1 && Error() == 9);
    Require(sceKernelClose(plain) == 0);
    Require(unlinkat_nid_postfix(base, "file.txt", 1) == -1 && Error() == 22);
    Require(unlinkat_nid_postfix(base, nullptr, 1) == -1 && Error() == 22);
    Require(unlinkat_nid_postfix(base, nullptr, 0) == -1 && Error() == 14);
    Require(unlinkat_nid_postfix(base, "", 0) == -1 && Error() == 2);
    Require(unlinkat_nid_postfix(base, "missing.bin", 0) == -1 && Error() == 2);
    Require(unlinkat_nid_postfix(base, "missing.bin", 0x800) == -1 && Error() == 2);
    Require(unlinkat_nid_postfix(base, "missing/child", 0) == -1 && Error() == 2);
    Require(unlinkat_nid_postfix(base, "file.txt/child", 0) == -1 && Error() == 20);
    Require(unlinkat_nid_postfix(base, "file.txt/child", 0x800) == -1 && Error() == 20);
    Require(unlinkat_nid_postfix(base, "file.txt/", 0) == -1 && Error() == 20);
    Require(unlinkat_nid_postfix(base, longName.c_str(), 0) == -1 && Error() == 63);
    Require(unlinkat_nid_postfix(base, "file.txt", 0x800) == -1 && Error() == 20);
    Require(unlinkat_nid_postfix(base, ".", 0x800) == -1 && Error() == 22);
    Require(unlinkat_nid_postfix(base, "..", 0x800) == -1 && Error() == 22);
    Require(unlinkat_nid_postfix(base, ".", 0) == -1 && Error() == 1);
    std::filesystem::create_directory(root / "usub");
    Require(unlinkat_nid_postfix(base, "usub", 0) == -1 && Error() == 1);
    std::filesystem::create_directory(root / "nonempty");
    { std::ofstream out(root / "nonempty" / "f.bin"); out << 'x'; }
    Require(unlinkat_nid_postfix(base, "nonempty", 0x800) == -1 && Error() == 66);
    Require(unlinkat_nid_postfix(base, "nonempty/f.bin", 0) == 0 && !std::filesystem::exists(root / "nonempty" / "f.bin"));
    Require(unlinkat_nid_postfix(base, "nonempty", 0x800) == 0 && !std::filesystem::exists(root / "nonempty"));
    { std::ofstream out(root / "to_unlink.bin"); out << 'x'; }
    Require(unlinkat_nid_postfix(base, "to_unlink.bin", 0) == 0 && !std::filesystem::exists(root / "to_unlink.bin"));
    { std::ofstream out(root / "dotdot.bin"); out << 'x'; }
    Require(unlinkat_nid_postfix(base, "usub/../dotdot.bin", 0) == 0 && !std::filesystem::exists(root / "dotdot.bin"));
    std::filesystem::create_directory(root / "uslash_sub");
    Require(unlinkat_nid_postfix(base, "uslash_sub/", 0x800) == 0 && !std::filesystem::exists(root / "uslash_sub"));
    Require(unlinkat_nid_postfix(base, "usub", 0x800) == 0 && !std::filesystem::exists(root / "usub"));
    { std::ofstream out(root / "abs.bin"); out << 'x'; }
    const auto guestAbsFile = "/" + root.filename().string() + "/abs.bin";
    Require(unlinkat_nid_postfix(-1, guestAbsFile.c_str(), 0) == 0 && !std::filesystem::exists(root / "abs.bin"));
    std::filesystem::create_directory(root / "unlink_abs_dir");
    const auto guestAbsDir = "/" + root.filename().string() + "/unlink_abs_dir";
    Require(unlinkat_nid_postfix(-1, guestAbsDir.c_str(), 0x800) == 0 && !std::filesystem::exists(root / "unlink_abs_dir"));
    { std::ofstream out(root / "cwd.bin"); out << 'x'; }
    Require(unlinkat_nid_postfix(-100, (root / "cwd.bin").string().c_str(), 0) == 0 && !std::filesystem::exists(root / "cwd.bin"));
    std::filesystem::create_directory(root / "unlink_cwd_dir");
    Require(unlinkat_nid_postfix(-100, (root / "unlink_cwd_dir").string().c_str(), 0x800) == 0 && !std::filesystem::exists(root / "unlink_cwd_dir"));
    Require(openat_nid_postfix(-1, "file.txt", SCE_KERNEL_O_RDONLY) == -1 && Error() == 9);
    Require(openat_nid_postfix(base, nullptr, SCE_KERNEL_O_RDONLY) == -1 && Error() == 14);
    Require(openat_nid_postfix(base, "", SCE_KERNEL_O_RDONLY) == -1 && Error() == 2);
    Require(renameat_nid_postfix(base, nullptr, base, "renamed.txt") == -1 && Error() == 14);
    Require(renameat_nid_postfix(base, "made.bin", base, nullptr) == -1 && Error() == 14);
    Require(renameat_nid_postfix(base, "", base, "renamed.txt") == -1 && Error() == 2);
    Require(renameat_nid_postfix(base, "made.bin", base, "") == -1 && Error() == 2);
    const int bad = sceKernelOpen(file.c_str(), SCE_KERNEL_O_RDONLY, 0);
    Require(bad >= 0);
    Require(renameat_nid_postfix(-1, "made.bin", base, "renamed.txt") == -1 && Error() == 9);
    Require(renameat_nid_postfix(base, "made.bin", -1, "renamed.txt") == -1 && Error() == 9);
    Require(renameat_nid_postfix(bad, "made.bin", base, "renamed.txt") == -1 && Error() == 20);
    Require(renameat_nid_postfix(base, "made.bin", bad, "renamed.txt") == -1 && Error() == 20);
    Require(sceKernelClose(bad) == 0);
    const auto longPart = std::string(256, 'y');
    Require(renameat_nid_postfix(base, longPart.c_str(), base, "renamed.txt") == -1 && Error() == 63);
    Require(renameat_nid_postfix(base, "made.bin", base, longPart.c_str()) == -1 && Error() == 63);
    Require(renameat_nid_postfix(base, ".", base, "renamed.txt") == -1 && Error() == 22);
    Require(renameat_nid_postfix(base, "..", base, "renamed.txt") == -1 && Error() == 22);
    Require(renameat_nid_postfix(base, "made.bin", base, ".") == -1 && Error() == 22);
    Require(renameat_nid_postfix(base, "made.bin", base, "..") == -1 && Error() == 22);
    Require(renameat_nid_postfix(base, "missing.bin", base, "renamed.txt") == -1 && Error() == 2);
    Require(renameat_nid_postfix(base, "made.bin", base, "renamed.bin") == 0);
    Require(!std::filesystem::exists(root / "made.bin"));
    Require(std::filesystem::is_regular_file(root / "renamed.bin"));
    Require(renameat_nid_postfix(base, "renamed.bin", base, "renamed.bin") == 0);
    const auto subPath = root / "sub_dir";
    std::filesystem::create_directory(subPath);
    Require(renameat_nid_postfix(base, "renamed.bin", base, "sub_dir") == -1 && Error() == 21);
    Require(renameat_nid_postfix(base, "sub_dir", base, "renamed.bin") == -1 && Error() == 20);
    Require(renameat_nid_postfix(base, "sub_dir", base, "sub_renamed") == 0);
    Require(!std::filesystem::exists(subPath));
    Require(std::filesystem::is_directory(root / "sub_renamed"));
    const int subFd = sceKernelOpen((root / "sub_renamed").string().c_str(), SCE_KERNEL_O_RDONLY | SCE_KERNEL_O_DIRECTORY, 0);
    Require(subFd >= 0);
    Require(renameat_nid_postfix(base, "renamed.bin", subFd, "nested.bin") == 0);
    Require(!std::filesystem::exists(root / "renamed.bin"));
    Require(std::filesystem::is_regular_file(root / "sub_renamed" / "nested.bin"));
    Require(renameat_nid_postfix(base, "sub_renamed", subFd, "child") == -1 && Error() == 22);
    const auto otherDir = root / "other_dir";
    std::filesystem::create_directory(otherDir);
    { std::ofstream stream(otherDir / "item.txt"); }
    Require(renameat_nid_postfix(base, "sub_renamed", base, "other_dir") == -1 && Error() == 66);
    Require(renameat_nid_postfix(subFd, "nested.bin/", base, "back.bin") == -1 && Error() == 20);
    Require(renameat_nid_postfix(subFd, "nested.bin", base, "nonexistent/") == -1 && Error() == 20);
    Require(renameat_nid_postfix(subFd, "nested.bin", base, "made.bin") == 0);
    Require(std::filesystem::is_regular_file(root / "made.bin"));
    Require(sceKernelClose(subFd) == 0);
    Require(renameat_nid_postfix(-100, (root / "made.bin").string().c_str(), -100, (root / "final.bin").string().c_str()) == 0);
    Require(std::filesystem::is_regular_file(root / "final.bin"));
    Require(fchmodat_nid_postfix(base, "file.txt", 0600, 0x0200) == 0);
    Require(fchmodat_nid_postfix(-100, (root / "file.txt").string().c_str(), 0644, 0) == 0);
#ifndef _WIN32
    Require((std::filesystem::status(root / "file.txt").permissions() & std::filesystem::perms::all) ==
        (std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::group_read | std::filesystem::perms::others_read));
#endif
    Require(fchmodat_nid_postfix(base, "missing.bin", 0600, 0) == -1 && Error() == 2);
    Require(fchmodat_nid_postfix(base, nullptr, 0600, 0) == -1 && Error() == 14);
    Require(fchmodat_nid_postfix(base, "", 0600, 0) == -1 && Error() == 2);
    Require(fchmodat_nid_postfix(base, "file.txt", 0600, 0x4000) == -1 && Error() == 22);
    Require(fchmodat_nid_postfix(-1, "file.txt", 0600, 0) == -1 && Error() == 9);
#ifndef _WIN32
    Require(symlink_nid_postfix("loop", (root / "loop").string().c_str()) == 0);
    Require(fchmodat_nid_postfix(base, "loop/x", 0600, 0) == -1 && Error() == 62);
    Require(fchmodat_nid_postfix(base, longName.c_str(), 0600, 0) == -1 && Error() == 63);
#endif
    Require(fchown_nid_postfix(base, 0, 0) == 0);
    Require(fchown_nid_postfix(base, 0xFFFFFFFFu, 0xFFFFFFFFu) == 0);
    Require(fchown_nid_postfix(-1, 0, 0) == -1 && Error() == 9);
    Require(fchown_nid_postfix(-1, 1, 1) == -1 && Error() == 9);
    bool fchownThrew = false;
    try { fchown_nid_postfix(base, 1, 0); } catch (const std::exception&) { fchownThrew = true; }
    Require(fchownThrew);
    fchownThrew = false;
    try { fchown_nid_postfix(base, 0, 1); } catch (const std::exception&) { fchownThrew = true; }
    Require(fchownThrew);
    FileStat stat{};
    Require(fstatat_nid_postfix(base, "file.txt", &stat, 0) == 0 && (stat.st_mode & 0170000) == 0100000 && stat.st_size == 1);
    Require(fstatat_nid_postfix(base, "other_dir", &stat, 0) == 0 && (stat.st_mode & 0170000) == 0040000);
    Require(fstatat_nid_postfix(base, "other_dir/", &stat, 0) == 0 && (stat.st_mode & 0170000) == 0040000);
    Require(fstatat_nid_postfix(base, "file.txt/", &stat, 0) == -1 && Error() == 20);
    Require(fstatat_nid_postfix(base, "file.txt", &stat, 0x0200) == 0 && (stat.st_mode & 0170000) == 0100000);
    Require(fstatat_nid_postfix(-100, file.c_str(), &stat, 0) == 0 && stat.st_size == 1);
    Require(fstatat_nid_postfix(-1, guestFile.c_str(), &stat, 0) == 0 && stat.st_size == 1);
    Require(fstatat_nid_postfix(base, "missing.bin", &stat, 0) == -1 && Error() == 2);
    Require(fstatat_nid_postfix(base, "missing.bin", &stat, 0x0200) == -1 && Error() == 2);
    Require(fstatat_nid_postfix(base, nullptr, &stat, 0) == -1 && Error() == 14);
    Require(fstatat_nid_postfix(base, "", &stat, 0) == -1 && Error() == 2);
    Require(fstatat_nid_postfix(base, "file.txt", &stat, 0x4000) == -1 && Error() == 22);
    Require(fstatat_nid_postfix(-1, "file.txt", &stat, 0) == -1 && Error() == 9);
    const int notDirectory = sceKernelOpen(file.c_str(), SCE_KERNEL_O_RDONLY, 0);
    Require(notDirectory >= 0);
    Require(fstatat_nid_postfix(notDirectory, "file.txt", &stat, 0) == -1 && Error() == 20);
    Require(sceKernelClose(notDirectory) == 0);
#ifndef _WIN32
    Require(symlink_nid_postfix("file.txt", (root / "link.txt").string().c_str()) == 0);
    Require(fstatat_nid_postfix(base, "link.txt", &stat, 0) == 0 && (stat.st_mode & 0170000) == 0100000 && stat.st_size == 1);
    Require(fstatat_nid_postfix(base, "link.txt", &stat, 0x0200) == 0 && (stat.st_mode & 0170000) == 0120000);
#endif
    Require(sceKernelClose(base) == 0);
#ifdef _WIN32
    const auto junctionTarget = root / "junction-target";
    const auto junction = root / "junction";
    Require(std::filesystem::create_directory(junctionTarget));
    const auto junctionCommand = L"cmd /c mklink /J \"" + junction.wstring() + L"\" \"" + std::filesystem::absolute(junctionTarget).wstring() + L"\"";
    Require(_wsystem(junctionCommand.c_str()) == 0);
    char junctionBuffer[512];
    std::memset(junctionBuffer, '#', sizeof(junctionBuffer));
    const auto junctionLength = readlink_nid_postfix(junction.string().c_str(), junctionBuffer, sizeof(junctionBuffer));
    Require(junctionLength > 0 && static_cast<std::size_t>(junctionLength) < sizeof(junctionBuffer) && junctionBuffer[junctionLength] == '#');
    const std::string junctionText(junctionBuffer, static_cast<std::size_t>(junctionLength));
    Require(junctionText == "/" + root.filename().string() + "/junction-target");
    Require(std::filesystem::remove(junction) && std::filesystem::remove(junctionTarget));
#endif
#ifndef _WIN32
    if (created == 0) {
        const auto throughLink = (root / "through-link.bin").string();
        Require(link_nid_postfix(symbolic.c_str(), throughLink.c_str()) == 0);
        Require(!std::filesystem::is_symlink(std::filesystem::symlink_status(throughLink)));
        Require(std::filesystem::equivalent(throughLink, original) && std::filesystem::hard_link_count(original) == 2);
        Require(std::filesystem::remove(throughLink) && std::filesystem::hard_link_count(original) == 1);
        Require(link_nid_postfix((root / "dangling").string().c_str(), throughLink.c_str()) == -1 && Error() == 2);
        const auto directoryLink = (root / "directory-link").string();
        Require(symlink_nid_postfix(".", directoryLink.c_str()) == 0);
        Require(link_nid_postfix(directoryLink.c_str(), throughLink.c_str()) == -1 && Error() == 1);
        Require(!std::filesystem::exists(std::filesystem::symlink_status(throughLink)));
    }
#endif
    CheckForwards(root);
    CheckPosixAio(root);
    CheckModuleAndTerminalQueries(root);
    CheckOwnershipAndLinks(root);
    CheckScePthreadWrappers();
    CheckPosixThreadExtensions();
    CheckNamedSemaphores();
    CheckTimeAndSelect();
    RemoveTree(root);
    return 0;
}
