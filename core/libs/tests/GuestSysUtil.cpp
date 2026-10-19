#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceSysUtilSendSystemNotificationWithText(std::int32_t type, const char* message);
}

static void Require(bool value) {
    if (!value) std::abort();
}

int main() {
    Require(sceSysUtilSendSystemNotificationWithText(0, "guest_sys_util notification") == 0);
    Require(sceSysUtilSendSystemNotificationWithText(222, "") == 0);
    bool threw = false;
    try {
        sceSysUtilSendSystemNotificationWithText(0, nullptr);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw);
    return 0;
}
