#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

extern "C" {
void APS5_VABI sceCoredumpDebugTextOut(const char*, int);
}
static void Require(bool value) { if (!value) std::abort(); }
int main() {
    auto* captured = std::tmpfile();
    Require(captured != nullptr);
    std::fflush(stderr);
#ifdef _WIN32
    const int saved = ::_dup(::_fileno(stderr));
    Require(saved >= 0 && ::_dup2(::_fileno(captured), ::_fileno(stderr)) == 0);
#else
    const int saved = ::dup(::fileno(stderr));
    Require(saved >= 0 && ::dup2(::fileno(captured), ::fileno(stderr)) == ::fileno(stderr));
#endif
    const char text[] = {'s', 't', 'o', 'p', '\0', 'p', 'e', 'd', '|', 'n', 'o', 't', ' ', 't', 'h', 'i', 's'};
    sceCoredumpDebugTextOut(text, 9);
    sceCoredumpDebugTextOut("ignored", 0);
    sceCoredumpDebugTextOut("ignored", -3);
    sceCoredumpDebugTextOut(nullptr, 4);
    sceCoredumpDebugTextOut("tail", 4);
    std::fflush(stderr);
#ifdef _WIN32
    Require(::_dup2(saved, ::_fileno(stderr)) == 0 && ::_close(saved) == 0);
#else
    Require(::dup2(saved, ::fileno(stderr)) == ::fileno(stderr) && ::close(saved) == 0);
#endif
    std::rewind(captured);
    char output[128]{};
    const auto length = std::fread(output, 1, sizeof(output), captured);
    const char expected[] = "[coredump] stop\0ped|[coredump] tail";
    Require(length == sizeof(expected) - 1 && std::memcmp(output, expected, length) == 0);
    Require(std::fclose(captured) == 0);
}
