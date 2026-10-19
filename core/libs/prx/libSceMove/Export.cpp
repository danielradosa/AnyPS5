#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int MOVE_ERROR_NOT_INIT = static_cast<int>(0x80EE0001u);

}

extern "C" {

int APS5_VABI sceMoveClose(int32_t handle) {
    (void)handle;
    return MOVE_ERROR_NOT_INIT;
}
int APS5_VABI sceMoveGetExtensionPortInfo(int32_t handle, void* data) {
    (void)handle;
    (void)data;
    return MOVE_ERROR_NOT_INIT;
}
int APS5_VABI sceMoveReadStateLatest(int32_t handle, void* data) {
    (void)handle;
    (void)data;
    return MOVE_ERROR_NOT_INIT;
}
int APS5_VABI sceMoveReadStateRecent(int32_t handle, int64_t timestamp, void* data, int32_t* out_count) {
    (void)handle;
    (void)timestamp;
    (void)data;
    (void)out_count;
    return MOVE_ERROR_NOT_INIT;
}
int APS5_VABI sceMoveResetLightSphere(int32_t handle) {
    (void)handle;
    return MOVE_ERROR_NOT_INIT;
}
int APS5_VABI sceMoveSetLightSphere(int32_t handle, uint8_t red, uint8_t green, uint8_t blue) {
    (void)handle;
    (void)red;
    (void)green;
    (void)blue;
    return MOVE_ERROR_NOT_INIT;
}

}
