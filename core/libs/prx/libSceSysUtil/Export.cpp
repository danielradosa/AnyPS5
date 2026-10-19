#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceSysUtilSendSystemNotificationWithText(int32_t type, const char* message) {
    (void)type;
    if (message == nullptr) APS5_INVALID_ARG_EX;
    WriteLogLine(stderr, "[notification] %s\n", message);
    return 0;
}

}
