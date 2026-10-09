#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
static int MakePipe(int* ends) { return ::_pipe(ends, 64, _O_BINARY); }
static int ClosePipe(int end) { return ::_close(end); }
#else
#include <unistd.h>
static int MakePipe(int* ends) { return ::pipe(ends); }
static int ClosePipe(int end) { return ::close(end); }
#endif

struct GuestIovec {
    void* base;
    std::size_t length;
};

extern "C" {
int APS5_VABI sceKernelOpen(const char*, int, std::uint16_t);
int APS5_VABI sceKernelClose(int);
int APS5_VABI sceKernelLseek(int, std::int64_t, int);
std::int64_t APS5_VABI readv_nid_postfix(int, const GuestIovec*, int);
std::int64_t APS5_VABI writev_nid_postfix(int, const GuestIovec*, int);
std::int64_t APS5_VABI preadv_nid_postfix(int, const GuestIovec*, int, std::int64_t);
std::int64_t APS5_VABI pwritev_nid_postfix(int, const GuestIovec*, int, std::int64_t);
int* APS5_VABI __error_nid_postfix();
}

static void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "POSIX vector check failed at line %d\n", line);
        std::abort();
    }
}
#define Require(value) Check((value), __LINE__)

static bool Fails(std::int64_t result, int error) {
    return result == -1 && *__error_nid_postfix() == error;
}

static std::string Contents(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

int main() {
    const auto root = std::filesystem::path("anyps5-posix-vector-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Require(std::filesystem::create_directory(root));
    const auto path = root / "data.bin";
    { std::ofstream stream(path, std::ios::binary); stream << "0123456789"; }

    const int file = sceKernelOpen(path.string().c_str(), SCE_KERNEL_O_RDWR, 0);
    Require(file >= 0);

    *__error_nid_postfix() = 13;
    char first[3] = {};
    char second[4] = {};
    GuestIovec reads[2] = {{first, sizeof(first)}, {second, sizeof(second)}};
    Require(readv_nid_postfix(file, reads, 2) == 7);
    Require(std::memcmp(first, "012", 3) == 0 && std::memcmp(second, "3456", 4) == 0);
    Require(readv_nid_postfix(file, reads, 2) == 3);
    Require(std::memcmp(first, "789", 3) == 0);
    Require(readv_nid_postfix(file, reads, 2) == 0);
    Require(preadv_nid_postfix(file, reads, 2, 1) == 7);
    Require(std::memcmp(first, "123", 3) == 0 && std::memcmp(second, "4567", 4) == 0);
    Require(sceKernelLseek(file, 0, 1) == 10);

    char ab[] = "AB";
    char cde[] = "CDE";
    GuestIovec writes[2] = {{ab, 2}, {cde, 3}};
    Require(pwritev_nid_postfix(file, writes, 2, 2) == 5);
    Require(sceKernelLseek(file, 0, 1) == 10);
    Require(writev_nid_postfix(file, writes, 2) == 5);
    Require(sceKernelLseek(file, 0, 1) == 15);
    Require(Contents(path) == "01ABCDE789ABCDE");
    Require(*__error_nid_postfix() == 13);

    Require(Fails(readv_nid_postfix(file, reads, -1), 22));
    Require(Fails(writev_nid_postfix(file, writes, 1025), 22));
    Require(Fails(readv_nid_postfix(file, nullptr, 1), 14));
    Require(Fails(pwritev_nid_postfix(file, nullptr, 1, 0), 14));
    Require(Fails(preadv_nid_postfix(file, reads, 2, -1), 22));
    Require(Fails(pwritev_nid_postfix(file, writes, 2, -1), 22));
    Require(Contents(path) == "01ABCDE789ABCDE");

    Require(sceKernelClose(file) == 0);
    Require(Fails(readv_nid_postfix(file, reads, 2), 9));
    Require(Fails(writev_nid_postfix(file, writes, 2), 9));
    Require(Fails(preadv_nid_postfix(file, reads, 2, 0), 9));
    Require(Fails(pwritev_nid_postfix(file, writes, 2, 0), 9));

    int ends[2] = {};
    Require(MakePipe(ends) == 0);
    char xyz[] = "xyz";
    GuestIovec message[1] = {{xyz, 3}};
    Require(writev_nid_postfix(ends[1], message, 1) == 3);
    Require(readv_nid_postfix(ends[0], reads, 2) == 3);
    Require(std::memcmp(first, "xyz", 3) == 0);
    Require(Fails(preadv_nid_postfix(ends[0], reads, 2, 0), 29));
    Require(Fails(pwritev_nid_postfix(ends[1], message, 1, 0), 29));
    Require(ClosePipe(ends[0]) == 0 && ClosePipe(ends[1]) == 0);

    std::filesystem::remove_all(root);
    return 0;
}
