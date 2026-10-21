#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/HostEntropy.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/Pthread/include/Pthread.hpp"
#include "prx/libSceLibcInternal/include/Statvfs.hpp"
#include "SceTypes.hpp"
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>

namespace {

constexpr int ThreadNameInvalid = 22;
constexpr int ThreadNameTooLong = 63;
constexpr std::size_t ThreadNameCapacity = 32;

constexpr int FnmNomatch = 1;
constexpr int FnmNoescape = 0x01;
constexpr int FnmPathname = 0x02;
constexpr int FnmPeriod = 0x04;
constexpr int FnmLeadingDir = 0x08;
constexpr int FnmCasefold = 0x10;

enum class RangeResult { Match, NoMatch, Error };

unsigned char FoldCase(unsigned char c, int flags) {
    return (flags & FnmCasefold) && c >= 'A' && c <= 'Z' ? static_cast<unsigned char>(c - 'A' + 'a') : c;
}

RangeResult MatchRange(const char* pattern, unsigned char test, int flags, const char** next) {
    const bool negate = *pattern == '!' || *pattern == '^';
    if (negate) ++pattern;
    test = FoldCase(test, flags);
    bool ok = false;
    const char* const origin = pattern;
    for (;;) {
        if (*pattern == ']' && pattern > origin) {
            ++pattern;
            break;
        }
        if (*pattern == '\0') return RangeResult::Error;
        if (*pattern == '/' && (flags & FnmPathname)) return RangeResult::NoMatch;
        if (*pattern == '\\' && !(flags & FnmNoescape)) ++pattern;
        const unsigned char low = FoldCase(static_cast<unsigned char>(*pattern), flags);
        if (*pattern != '\0') ++pattern;
        if (*pattern == '-' && pattern[1] != '\0' && pattern[1] != ']') {
            if (*++pattern == '\\' && !(flags & FnmNoescape)) ++pattern;
            const unsigned char high = static_cast<unsigned char>(*pattern);
            if (high == '\0') return RangeResult::Error;
            ++pattern;
            if (low <= test && test <= FoldCase(high, flags)) ok = true;
        } else if (low == test) {
            ok = true;
        }
    }
    *next = pattern;
    return ok == negate ? RangeResult::NoMatch : RangeResult::Match;
}

int MatchPattern(const char* pattern, const char* string, int flags) {
    const char* const start = string;
    const char* backtrackPattern = nullptr;
    const char* backtrackString = nullptr;
    const auto leadingPeriod = [&](const char* at) {
        return *at == '.' && (flags & FnmPeriod) && (at == start || ((flags & FnmPathname) && at[-1] == '/'));
    };
    for (;;) {
        const unsigned char pc = static_cast<unsigned char>(*pattern);
        if (pc != '\0') ++pattern;
        const unsigned char sc = static_cast<unsigned char>(*string);
        bool mismatch = false;
        bool literal = false;
        unsigned char expected = pc;
        switch (pc) {
            case '\0':
                if ((flags & FnmLeadingDir) && sc == '/') return 0;
                if (sc == '\0') return 0;
                mismatch = true;
                break;
            case '?':
                if (sc == '\0') return FnmNomatch;
                if ((sc == '/' && (flags & FnmPathname)) || leadingPeriod(string)) {
                    mismatch = true;
                    break;
                }
                ++string;
                break;
            case '*': {
                char c = *pattern;
                while (c == '*') c = *++pattern;
                if (leadingPeriod(string)) {
                    mismatch = true;
                    break;
                }
                if (c == '\0') {
                    if (flags & FnmPathname) return (flags & FnmLeadingDir) || std::strchr(string, '/') == nullptr ? 0 : FnmNomatch;
                    return 0;
                }
                if (c == '/' && (flags & FnmPathname)) {
                    string = std::strchr(string, '/');
                    if (string == nullptr) return FnmNomatch;
                    break;
                }
                backtrackPattern = pattern;
                backtrackString = string;
                break;
            }
            case '[': {
                if (sc == '\0') return FnmNomatch;
                if ((sc == '/' && (flags & FnmPathname)) || leadingPeriod(string)) {
                    mismatch = true;
                    break;
                }
                const char* next = nullptr;
                const RangeResult result = MatchRange(pattern, sc, flags, &next);
                if (result == RangeResult::Error) {
                    literal = true;
                } else if (result == RangeResult::Match) {
                    pattern = next;
                    ++string;
                } else {
                    mismatch = true;
                }
                break;
            }
            case '\\':
                if (!(flags & FnmNoescape)) {
                    expected = static_cast<unsigned char>(*pattern);
                    if (expected == '\0') return FnmNomatch;
                    ++pattern;
                }
                literal = true;
                break;
            default:
                literal = true;
                break;
        }
        if (literal) {
            if (sc != '\0') ++string;
            if (FoldCase(expected, flags) != FoldCase(sc, flags)) mismatch = true;
        }
        if (mismatch) {
            if (backtrackPattern == nullptr) return FnmNomatch;
            const unsigned char bc = static_cast<unsigned char>(*backtrackString);
            if (bc == '\0') return FnmNomatch;
            if (bc == '/' && (flags & FnmPathname)) return FnmNomatch;
            ++backtrackString;
            pattern = backtrackPattern;
            string = backtrackString;
        }
    }
}

}

extern "C" {

int APS5_VABI access_nid_postfix(const char* path, int mode);
int APS5_VABI clock_gettime_nid_postfix(int clockId, KernelTimespec* time);
int APS5_VABI fchown_nid_postfix(int descriptor, std::uint32_t owner, std::uint32_t group);
char* APS5_VABI getcwd_nid_postfix(char* buffer, std::size_t size);
char* APS5_VABI getenv_nid_postfix(const char* name);
int APS5_VABI link_nid_postfix(const char* existing, const char* created);
int APS5_VABI lstat_nid_postfix(const char* path, FileStat* status);
void* APS5_VABI mmap_nid_postfix(void* address, std::size_t length, int protection, int flags, int descriptor, std::int64_t offset) noexcept;
std::int64_t APS5_VABI readlink_nid_postfix(const char* path, char* buffer, std::size_t size);
int APS5_VABI statvfs_nid_postfix(const char* path, LibcStatvfs* status);
int APS5_VABI symlink_nid_postfix(const char* target, const char* created);

int APS5_VABI fnmatch_nid_postfix(const char* pattern, const char* string, int flags) {
    if (pattern == nullptr || string == nullptr) throw std::invalid_argument("fnmatch: null pattern or string");
    return MatchPattern(pattern, string, flags);
}

char* APS5_VABI strcasestr_nid_postfix(const char* text, const char* needle) {
    if (*needle == '\0') return const_cast<char*>(text);
    for (; *text != '\0'; ++text) {
        const char* candidate = text;
        const char* match = needle;
        while (*candidate != '\0' && *match != '\0' &&
               std::tolower(static_cast<unsigned char>(*candidate)) ==
               std::tolower(static_cast<unsigned char>(*match))) {
            ++candidate;
            ++match;
        }
        if (*match == '\0') return const_cast<char*>(text);
    }
    return nullptr;
}

void APS5_VABI arc4random_buf_nid_postfix(void* buffer, std::size_t size) {
    HostEntropy_nid_no_patch(buffer, size);
}

std::uint32_t APS5_VABI arc4random_nid_postfix() {
    std::uint32_t value;
    arc4random_buf_nid_postfix(&value, sizeof(value));
    return value;
}

int APS5_VABI pthread_setname_np_nid_postfix(Pthread thread, const char* name) {
    if (thread == nullptr || name == nullptr) return ThreadNameInvalid;
    if (std::strlen(name) >= ThreadNameCapacity) return ThreadNameTooLong;
    std::lock_guard lock(thread->nameLock);
    thread->name = name;
    return 0;
}

int APS5_VABI __wrap_access(const char* path, int mode) {
    return access_nid_postfix(path, mode);
}

int APS5_VABI __wrap_clock_gettime(int clockId, KernelTimespec* time) {
    return clock_gettime_nid_postfix(clockId, time);
}

int APS5_VABI __wrap_fchown(int descriptor, std::uint32_t owner, std::uint32_t group) {
    return fchown_nid_postfix(descriptor, owner, group);
}

char* APS5_VABI __wrap_getcwd(char* buffer, std::size_t size) {
    return getcwd_nid_postfix(buffer, size);
}

char* APS5_VABI __wrap_getenv(const char* name) {
    return getenv_nid_postfix(name);
}

int APS5_VABI __wrap_link(const char* existing, const char* created) {
    return link_nid_postfix(existing, created);
}

int APS5_VABI __wrap_lstat(const char* path, FileStat* status) {
    return lstat_nid_postfix(path, status);
}

void* APS5_VABI __wrap_mmap(void* address, std::size_t length, int protection, int flags, int descriptor, std::int64_t offset) {
    return mmap_nid_postfix(address, length, protection, flags, descriptor, offset);
}

std::int64_t APS5_VABI __wrap_readlink(const char* path, char* buffer, std::size_t size) {
    return readlink_nid_postfix(path, buffer, size);
}

int APS5_VABI __wrap_statvfs(const char* path, LibcStatvfs* status) {
    return statvfs_nid_postfix(path, status);
}

int APS5_VABI __wrap_symlink(const char* target, const char* created) {
    return symlink_nid_postfix(target, created);
}

int APS5_VABI clock_gettime_np() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI getenv_np() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI mmap_np() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI msetname() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_setschedparam_np() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI setenv_np() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

APS5_EXPORT("9Vqxt-qBPB4", scePosixForWebKit_sysinfo);
int APS5_VABI scePosixForWebKit_sysinfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
