#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceSystemService/SystemService.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

extern "C" int APS5_VABI sceSystemServiceGetAppIdOfRunningBigApp(void);
extern "C" int APS5_VABI sceSystemServiceGetAppId(const char* titleId);
extern "C" int APS5_VABI sceSystemServiceGetAppTitleId(int appId, char* titleId);
extern "C" int APS5_VABI sceSystemServiceGetMainAppTitleId(char* titleId);
extern "C" int APS5_VABI sceLncUtilGetAppTitleId(std::uint32_t appId, char* titleId);

namespace {

constexpr char RunningTitle[] = "PPSA00000";

void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "title ID check failed: %s\n", what);
    std::abort();
}

bool GetAppIdThrows(const char* titleId) {
    try {
        static_cast<void>(sceSystemServiceGetAppId(titleId));
    } catch (const std::runtime_error& error) {
        return std::string(error.what()).find("is not the running title") != std::string::npos;
    }
    return false;
}

bool HoldsRunningTitle(const char* field, std::size_t size) {
    if (std::memcmp(field, RunningTitle, sizeof(RunningTitle)) != 0) return false;
    for (std::size_t index = sizeof(RunningTitle); index < size; ++index) {
        if (field[index] != 'x') return false;
    }
    return true;
}

}

int main() {
    const int appId = sceSystemServiceGetAppIdOfRunningBigApp();
    Require(sceSystemServiceGetAppId(RunningTitle) == appId, "the running title has the running application ID");
    Require(GetAppIdThrows("PPSA00001"), "another title has no application ID here");
    Require(GetAppIdThrows(""), "an empty title ID is not the running title");
    Require(GetAppIdThrows("PPSA000000"), "a longer title ID is not the running title");

    char titleId[16];
    std::memset(titleId, 'x', sizeof(titleId));
    Require(sceSystemServiceGetAppTitleId(appId, titleId) == SYSTEM_SERVICE_OK, "the running application has a title ID");
    Require(HoldsRunningTitle(titleId, sizeof(titleId)), "the title ID and its terminator are written, and nothing after them");
    Require(sceSystemServiceGetAppId(titleId) == appId, "the title ID maps back to the application ID");
    Require(sceSystemServiceGetAppTitleId(appId + 1, titleId) == SYSTEM_SERVICE_ERROR_NO_APP_INFO, "another application has no information");

    std::memset(titleId, 'x', sizeof(titleId));
    Require(sceSystemServiceGetMainAppTitleId(titleId) == SYSTEM_SERVICE_OK, "the main application title ID is read");
    Require(HoldsRunningTitle(titleId, sizeof(titleId)), "the main application is the running title");

    std::memset(titleId, 'x', sizeof(titleId));
    Require(sceLncUtilGetAppTitleId(static_cast<std::uint32_t>(appId), titleId) == 0, "LncUtil reads the running title ID");
    Require(HoldsRunningTitle(titleId, sizeof(titleId)), "LncUtil writes the same title ID");
    std::puts("title ID tests passed");
    return 0;
}
