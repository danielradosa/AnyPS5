#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>

extern "C" {
int APS5_VABI sceNpSnsFacebookDialogClose(void);
int APS5_VABI sceNpSnsFacebookDialogGetStatus(void);
}

namespace {

constexpr int STATUS_NONE = 0;
constexpr int NOT_INITIALIZED = static_cast<int>(0x80B80003u);

bool Check(bool condition, const char* message) {
    if (!condition) std::fprintf(stderr, "NpDialogs: %s\n", message);
    return condition;
}

bool CheckFacebook() {
    return Check(sceNpSnsFacebookDialogGetStatus() == STATUS_NONE, "the Facebook dialog is never initialized")
        && Check(sceNpSnsFacebookDialogClose() == NOT_INITIALIZED, "closing the Facebook dialog is NOT_INITIALIZED");
}

}

int main() {
    if (!CheckFacebook()) return 1;
    std::puts("NpDialogs tests passed");
    return 0;
}
