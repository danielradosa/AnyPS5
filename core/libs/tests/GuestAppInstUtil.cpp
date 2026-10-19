#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceAppInstUtilInitialize(void);
int APS5_VABI sceAppInstUtilTerminate(void);
int APS5_VABI sceAppInstUtilAppExists(const char* titleId, std::int32_t* exists);
}

namespace {

void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "libSceAppInstUtil check failed: %s\n", what);
    std::abort();
}

std::int32_t Exists(const char* titleId) {
    std::int32_t exists = -1;
    Require(sceAppInstUtilAppExists(titleId, &exists) == 0, "sceAppInstUtilAppExists failed");
    return exists;
}

template <typename TCall>
bool RejectsArgument(TCall call) {
    try {
        call();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

}

int main() {
    Require(sceAppInstUtilInitialize() == 0, "the first initialization failed");
    Require(sceAppInstUtilInitialize() == 0, "a repeated initialization failed");
    Require(Exists("PPSA00000") == 1, "the running title is not installed");
    Require(Exists("PPSA99109") == 0, "another title is installed");
    Require(Exists("PPSA000000") == 0, "a longer title id is the running title");
    Require(Exists("") == 0, "an empty title id is installed");
    std::int32_t exists = 0;
    Require(RejectsArgument([&] { sceAppInstUtilAppExists(nullptr, &exists); }), "a null title id was accepted");
    Require(RejectsArgument([] { sceAppInstUtilAppExists("PPSA00000", nullptr); }), "a null output was accepted");
    Require(sceAppInstUtilTerminate() == 0, "termination failed");
    Require(Exists("PPSA00000") == 1, "the running title is not installed after termination");
    std::puts("libSceAppInstUtil tests passed");
    return 0;
}
