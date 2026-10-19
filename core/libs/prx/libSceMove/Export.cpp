#include <atomic>
#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int MOVE_OK = 0;
constexpr int MOVE_ERROR_NO_CONTROLLER_CONNECTED = 1;
constexpr int MOVE_ERROR_NOT_INIT = static_cast<int>(0x80EE0001u);
constexpr int MOVE_ERROR_ALREADY_INIT = static_cast<int>(0x80EE0002u);
constexpr int MOVE_ERROR_INVALID_ARG = static_cast<int>(0x80EE0003u);
constexpr int32_t MOVE_HANDLE_BASE = 0x30b0000;
constexpr int32_t MOVE_HANDLE_STEP = 0x100;

std::atomic_bool g_initialized{false};
std::atomic<int32_t> g_lastHandle{MOVE_HANDLE_BASE};

bool Initialized() {
    return g_initialized.load(std::memory_order_acquire);
}

}

extern "C" {

int APS5_VABI sceMoveInit(void) {
    bool expected = false;
    if (!g_initialized.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return MOVE_ERROR_ALREADY_INIT;
    return MOVE_OK;
}
int APS5_VABI sceMoveTerm(void) {
    bool expected = true;
    if (!g_initialized.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) return MOVE_ERROR_NOT_INIT;
    return MOVE_OK;
}
int APS5_VABI sceMoveOpen(int32_t userId, int32_t type, int32_t index) {
    (void)userId;
    (void)type;
    (void)index;
    if (!Initialized()) return MOVE_ERROR_NOT_INIT;
    return g_lastHandle.fetch_add(MOVE_HANDLE_STEP, std::memory_order_relaxed) + MOVE_HANDLE_STEP;
}
int APS5_VABI sceMoveClose(int32_t handle) {
    (void)handle;
    if (!Initialized()) return MOVE_ERROR_NOT_INIT;
    return MOVE_OK;
}
int APS5_VABI sceMoveGetDeviceInfo(int32_t handle, void* info) {
    (void)handle;
    if (!Initialized()) return MOVE_ERROR_NOT_INIT;
    if (info == nullptr) return MOVE_ERROR_INVALID_ARG;
    return MOVE_ERROR_NO_CONTROLLER_CONNECTED;
}
int APS5_VABI sceMoveGetExtensionPortInfo(int32_t handle, void* data) {
    (void)handle;
    if (!Initialized()) return MOVE_ERROR_NOT_INIT;
    if (data == nullptr) return MOVE_ERROR_INVALID_ARG;
    return MOVE_ERROR_NO_CONTROLLER_CONNECTED;
}
int APS5_VABI sceMoveReadStateLatest(int32_t handle, void* data) {
    (void)handle;
    if (!Initialized()) return MOVE_ERROR_NOT_INIT;
    if (data == nullptr) return MOVE_ERROR_INVALID_ARG;
    return MOVE_ERROR_NO_CONTROLLER_CONNECTED;
}
int APS5_VABI sceMoveReadStateRecent(int32_t handle, int64_t timestamp, void* data, int32_t* out_count) {
    (void)handle;
    if (!Initialized()) return MOVE_ERROR_NOT_INIT;
    if (timestamp < 0 || data == nullptr || out_count == nullptr) return MOVE_ERROR_INVALID_ARG;
    return MOVE_ERROR_NO_CONTROLLER_CONNECTED;
}
int APS5_VABI sceMoveResetLightSphere(int32_t handle) {
    (void)handle;
    if (!Initialized()) return MOVE_ERROR_NOT_INIT;
    return MOVE_OK;
}
int APS5_VABI sceMoveSetLightSphere(int32_t handle, uint8_t red, uint8_t green, uint8_t blue) {
    (void)handle;
    (void)red;
    (void)green;
    (void)blue;
    if (!Initialized()) return MOVE_ERROR_NOT_INIT;
    return MOVE_ERROR_NO_CONTROLLER_CONNECTED;
}
int APS5_VABI sceMoveSetVibration(int32_t handle, uint8_t motor) {
    (void)handle;
    (void)motor;
    if (!Initialized()) return MOVE_ERROR_NOT_INIT;
    return MOVE_ERROR_NO_CONTROLLER_CONNECTED;
}

}
