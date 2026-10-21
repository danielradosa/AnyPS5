#include "prx/libc/include/General.hpp"
#include "prx/libSceLibcInternal/include/Statvfs.hpp"
#include "SceTypes.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <random>
#include <source_location>
#include <string>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

extern "C" {
int APS5_VABI mkstemp_nid_postfix(char*);
int APS5_VABI isatty_nid_postfix(int);
int* APS5_VABI __error_nid_postfix();
int APS5_VABI close_nid_postfix(int);
int APS5_VABI pipe_nid_postfix(int*);
int APS5_VABI socket_nid_postfix(int, int, int);
std::int64_t APS5_VABI read_nid_postfix(int, void*, std::size_t);
std::int64_t APS5_VABI write_nid_postfix(int, const void*, std::size_t);
std::int64_t APS5_VABI lseek_nid_postfix(int, std::int64_t, int);
int APS5_VABI munmap_nid_postfix(void*, std::size_t) noexcept;
char* APS5_VABI getcwd_nid_postfix(char*, std::size_t);
int APS5_VABI setenv_nid_postfix(const char*, const char*, int);
int APS5_VABI unsetenv_nid_postfix(const char*);
int APS5_VABI __wrap_access(const char*, int);
int APS5_VABI __wrap_clock_gettime(int, KernelTimespec*);
int APS5_VABI __wrap_fchown(int, std::uint32_t, std::uint32_t);
char* APS5_VABI __wrap_getcwd(char*, std::size_t);
char* APS5_VABI __wrap_getenv(const char*);
int APS5_VABI __wrap_link(const char*, const char*);
int APS5_VABI __wrap_lstat(const char*, FileStat*);
void* APS5_VABI __wrap_mmap(void*, std::size_t, int, int, int, std::int64_t);
std::int64_t APS5_VABI __wrap_readlink(const char*, char*, std::size_t);
int APS5_VABI __wrap_statvfs(const char*, LibcStatvfs*);
int APS5_VABI __wrap_symlink(const char*, const char*);
}

static void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (!condition) {
        std::fprintf(stderr, "POSIX filesystem check failed at line %u (guest errno %d)\n", location.line(), *__error_nid_postfix());
        std::abort();
    }
}

static void CheckWrappers(const std::string& file) {
    Require(__wrap_access(file.c_str(), 0) == 0);
    Require(__wrap_access("/anyps5-posix-test/absent", 0) == -1 && *__error_nid_postfix() == 2);
    Require(__wrap_access(file.c_str(), 8) == -1 && *__error_nid_postfix() == 22);

    std::array<char, 1024> wrapped{}, direct{};
    Require(__wrap_getcwd(wrapped.data(), wrapped.size()) == wrapped.data());
    Require(getcwd_nid_postfix(direct.data(), direct.size()) == direct.data());
    Require(std::strcmp(wrapped.data(), direct.data()) == 0);
    Require(__wrap_getcwd(wrapped.data(), 0) == nullptr && *__error_nid_postfix() == 22);

    Require(setenv_nid_postfix("WEBKIT_WRAP_TEST", "wrapped value", 1) == 0);
    const char* value = __wrap_getenv("WEBKIT_WRAP_TEST");
    Require(value != nullptr && std::strcmp(value, "wrapped value") == 0);
    Require(unsetenv_nid_postfix("WEBKIT_WRAP_TEST") == 0);
    Require(__wrap_getenv("WEBKIT_WRAP_TEST") == nullptr);

    KernelTimespec realtime{-1, -1}, monotonic{-1, -1};
    Require(__wrap_clock_gettime(0, &realtime) == 0 && realtime.tv_sec > 1500000000);
    Require(realtime.tv_nsec >= 0 && realtime.tv_nsec < 1000000000);
    Require(__wrap_clock_gettime(4, &monotonic) == 0 && monotonic.tv_nsec >= 0 && monotonic.tv_nsec < 1000000000);
    Require(__wrap_clock_gettime(4, nullptr) == -1 && *__error_nid_postfix() == 14);

    FileStat status{};
    Require(__wrap_lstat(file.c_str(), &status) == 0);
    Require((status.st_mode & 0170000) == 0100000 && status.st_size == 0);
    Require(__wrap_lstat("/anyps5-posix-test/absent", &status) == -1 && *__error_nid_postfix() == 2);

    const std::string hard = "/anyps5-posix-test/wrap-hard";
    Require(__wrap_link(file.c_str(), hard.c_str()) == 0);
    Require(__wrap_access(hard.c_str(), 0) == 0);
    Require(__wrap_link(file.c_str(), hard.c_str()) == -1 && *__error_nid_postfix() == 17);
    Require(__wrap_link("/anyps5-posix-test/absent", "/anyps5-posix-test/wrap-other") == -1 && *__error_nid_postfix() == 2);

    const std::string symbolic = "/anyps5-posix-test/wrap-link";
    const int linked = __wrap_symlink("fixed", symbolic.c_str());
#ifdef _WIN32
    if (linked == -1 && *__error_nid_postfix() == 1) {
        std::fputs("Windows symbolic-link privileges unavailable\n", stderr);
    } else
#endif
    {
        Require(linked == 0);
        std::array<char, 16> target{};
        Require(__wrap_readlink(symbolic.c_str(), target.data(), target.size()) == 5 && std::memcmp(target.data(), "fixed", 5) == 0);
        Require(__wrap_lstat(symbolic.c_str(), &status) == 0 && (status.st_mode & 0170000) == 0120000);
        Require(__wrap_symlink("fixed", symbolic.c_str()) == -1 && *__error_nid_postfix() == 17);
    }
    Require(__wrap_readlink(file.c_str(), wrapped.data(), wrapped.size()) == -1 && *__error_nid_postfix() == 22);
    Require(__wrap_readlink("/anyps5-posix-test/absent", wrapped.data(), wrapped.size()) == -1 && *__error_nid_postfix() == 2);

    std::string pattern = "/anyps5-posix-test/owned-XXXXXX";
    const int descriptor = mkstemp_nid_postfix(pattern.data());
    Require(descriptor >= 0);
    Require(__wrap_fchown(descriptor, 0xFFFFFFFFu, 0xFFFFFFFFu) == 0);
    Require(__wrap_fchown(descriptor, 0, 0) == 0);
    Require(close_nid_postfix(descriptor) == 0);
    Require(__wrap_fchown(descriptor, 0xFFFFFFFFu, 0xFFFFFFFFu) == -1 && *__error_nid_postfix() == 9);

    LibcStatvfs volume{};
    Require(__wrap_statvfs("/anyps5-posix-test", &volume) == 0);
    Require(volume.f_bsize > 0 && volume.f_frsize > 0 && volume.f_blocks > 0 && volume.f_namemax > 0);
    Require(__wrap_statvfs("/anyps5-posix-test/absent", &volume) == -1 && *__error_nid_postfix() == 2);

    constexpr std::size_t Length = 16384;
    void* const failed = reinterpret_cast<void*>(static_cast<std::uintptr_t>(-1));
    auto* mapped = static_cast<unsigned char*>(__wrap_mmap(nullptr, Length, 3, 0x1002, -1, 0));
    Require(mapped != failed && mapped != nullptr && mapped[0] == 0 && mapped[Length - 1] == 0);
    mapped[0] = 0x5a;
    mapped[Length - 1] = 0xa5;
    Require(mapped[0] == 0x5a && mapped[Length - 1] == 0xa5);
    Require(munmap_nid_postfix(mapped, Length) == 0);
    Require(__wrap_mmap(nullptr, 0, 3, 0x1002, -1, 0) == failed && *__error_nid_postfix() == 22);
    Require(__wrap_mmap(nullptr, Length, 3, 0x1002, 3, 0) == failed && *__error_nid_postfix() == 22);
}

int main() {
    std::random_device random;
    const auto root = std::filesystem::temp_directory_path() /
        ("anyps5-posix-test-" + std::to_string(random()) + "-" + std::to_string(random()));
    Require(std::filesystem::create_directory(root));
    AddPathAlias_nid_no_patch("/anyps5-posix-test", root.string().c_str());
    const std::string prefix = "/anyps5-posix-test/file-";
    std::string pattern = prefix + "XXXXXXXXX";
    const int descriptor = mkstemp_nid_postfix(pattern.data());
    Require(descriptor >= 0);
    Require(pattern.starts_with(prefix) && pattern.size() == prefix.size() + 9);
    Require(pattern.substr(prefix.size()).find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") == std::string::npos);
    Require(std::filesystem::file_size(ResolvePath_nid_no_patch(pattern.c_str())) == 0);
#ifndef _WIN32
    struct stat status{};
    Require(::fstat(descriptor, &status) == 0 && (status.st_mode & 0777) == 0600);
#endif
    const char payload[] = {'a', '\0', '\r', '\n', '\x1a', 'z'};
    Require(write_nid_postfix(descriptor, payload, sizeof(payload)) == sizeof(payload));
    Require(lseek_nid_postfix(descriptor, 0, SEEK_SET) == 0);
    std::array<char, sizeof(payload)> received{};
    Require(read_nid_postfix(descriptor, received.data(), received.size()) == static_cast<std::int64_t>(received.size()));
    Require(std::memcmp(payload, received.data(), sizeof(payload)) == 0);
    Require(isatty_nid_postfix(descriptor) == 0 && *__error_nid_postfix() == 25);
    Require(close_nid_postfix(descriptor) == 0);
    Require(isatty_nid_postfix(descriptor) == 0 && *__error_nid_postfix() == 9);
    std::string second = prefix + "XXXXXXXXX";
    const int other = mkstemp_nid_postfix(second.data());
    Require(other >= 0 && second != pattern);
    Require(close_nid_postfix(other) == 0);
    const std::string candidates = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (const char character : candidates) {
        std::ofstream stream(root / (std::string("occupied-") + character));
        stream << "preserved";
        Require(stream.good());
    }
    std::string occupied = "/anyps5-posix-test/occupied-X";
    Require(mkstemp_nid_postfix(occupied.data()) == -1 && *__error_nid_postfix() == 17);
    for (const char character : candidates) {
        std::ifstream stream(root / (std::string("occupied-") + character));
        std::string contents;
        stream >> contents;
        Require(contents == "preserved");
    }
    Require(std::filesystem::remove(root / "occupied-7"));
    occupied = "/anyps5-posix-test/occupied-X";
    const int collision = mkstemp_nid_postfix(occupied.data());
    Require(collision >= 0 && occupied == "/anyps5-posix-test/occupied-7");
    Require(close_nid_postfix(collision) == 0);
    std::string fixed = "/anyps5-posix-test/fixed";
    const int unchanged = mkstemp_nid_postfix(fixed.data());
    Require(unchanged >= 0 && fixed == "/anyps5-posix-test/fixed");
    Require(close_nid_postfix(unchanged) == 0);
    Require(mkstemp_nid_postfix(fixed.data()) == -1 && *__error_nid_postfix() == 17);
    std::string missing = "/anyps5-posix-test/missing/file-XXXXXX";
    Require(mkstemp_nid_postfix(missing.data()) == -1 && *__error_nid_postfix() == 2);
    std::string notDirectory = fixed + "/file-XXXXXX";
    Require(mkstemp_nid_postfix(notDirectory.data()) == -1);
    Require(*__error_nid_postfix() == 20 || *__error_nid_postfix() == 2);
    std::string empty;
    Require(mkstemp_nid_postfix(empty.data()) == -1 && *__error_nid_postfix() == 22);
    Require(mkstemp_nid_postfix(nullptr) == -1 && *__error_nid_postfix() == 14);
    std::string tooLong(1024, 'X');
    Require(mkstemp_nid_postfix(tooLong.data()) == -1 && *__error_nid_postfix() == 63);
    Require(tooLong == std::string(1024, 'X'));
    Require(isatty_nid_postfix(-1) == 0 && *__error_nid_postfix() == 9);
    Require(isatty_nid_postfix(100000) == 0 && *__error_nid_postfix() == 9);
    int pipes[2]{};
    Require(pipe_nid_postfix(pipes) == 0);
    for (const int pipe : pipes) {
        Require(isatty_nid_postfix(pipe) == 0 && *__error_nid_postfix() == 25);
        Require(close_nid_postfix(pipe) == 0);
    }
    const int socket = socket_nid_postfix(2, 2, 0);
    Require(socket >= 0 && socket < 1024);
    Require(isatty_nid_postfix(socket) == 0 && *__error_nid_postfix() == 25);
    Require(close_nid_postfix(socket) == 0);
    Require(isatty_nid_postfix(socket) == 0 && *__error_nid_postfix() == 9);
#ifdef _WIN32
    const int nullDevice = ::_open("NUL", _O_RDWR);
    Require(nullDevice >= 0);
    Require(isatty_nid_postfix(nullDevice) == 0 && *__error_nid_postfix() == 25);
    Require(::_close(nullDevice) == 0);
    const int console = ::_open("CONIN$", _O_RDONLY);
    if (console >= 0) {
        *__error_nid_postfix() = 123;
        Require(isatty_nid_postfix(console) == 1 && *__error_nid_postfix() == 123);
        Require(::_close(console) == 0);
    }
#else
    const int terminal = ::posix_openpt(O_RDWR | O_NOCTTY);
    Require(terminal >= 0 && ::grantpt(terminal) == 0 && ::unlockpt(terminal) == 0);
    const int slave = ::open(::ptsname(terminal), O_RDWR | O_NOCTTY);
    Require(slave >= 0);
    *__error_nid_postfix() = 123;
    Require(isatty_nid_postfix(slave) == 1 && *__error_nid_postfix() == 123);
    Require(::close(slave) == 0 && ::close(terminal) == 0);
#endif
    CheckWrappers(fixed);
    RemovePathAlias_nid_no_patch("/anyps5-posix-test");
    Require(std::filesystem::remove_all(root) > 0);
}
