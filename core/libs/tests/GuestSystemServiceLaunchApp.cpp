#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceSystemService/SystemService.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

extern "C" int APS5_VABI sceSystemServiceLaunchApp(const char* titleId, const char* const* arguments, const void* param);

namespace {

void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "sceSystemServiceLaunchApp check failed: %s\n", what);
    std::abort();
}

bool RelaunchThrows(const char* titleId) {
    try {
        static_cast<void>(sceSystemServiceLaunchApp(titleId, nullptr, nullptr));
    } catch (const std::runtime_error& error) {
        return std::string(error.what()).find("launching the running title") != std::string::npos;
    }
    return false;
}

}

int main() {
    Require(sceSystemServiceLaunchApp(nullptr, nullptr, nullptr) == SYSTEM_SERVICE_ERROR_PARAMETER, "a null title id is a parameter error");
    Require(sceSystemServiceLaunchApp("PPSA99109", nullptr, nullptr) == SYSTEM_SERVICE_ERROR_LAUNCH_NOT_INSTALLED, "another title is not installed");
    const char* arguments[] = {"--backup", nullptr};
    unsigned char param[0x20];
    std::memset(param, 0x5a, sizeof(param));
    Require(sceSystemServiceLaunchApp("PPSA99110", arguments, param) == SYSTEM_SERVICE_ERROR_LAUNCH_NOT_INSTALLED, "another title with arguments is not installed");
    for (unsigned char byte : param) Require(byte == 0x5a, "the launch parameter is not written");
    Require(std::strcmp(arguments[0], "--backup") == 0 && arguments[1] == nullptr, "the arguments are not written");
    Require(sceSystemServiceLaunchApp("", nullptr, nullptr) == SYSTEM_SERVICE_ERROR_LAUNCH_NOT_INSTALLED, "an empty title id names no installed title");
    Require(sceSystemServiceLaunchApp("PPSA000000", nullptr, nullptr) == SYSTEM_SERVICE_ERROR_LAUNCH_NOT_INSTALLED, "a longer title id is another title");
    Require(RelaunchThrows("PPSA00000"), "launching the running title is reported");
    std::puts("sceSystemServiceLaunchApp tests passed");
    return 0;
}
