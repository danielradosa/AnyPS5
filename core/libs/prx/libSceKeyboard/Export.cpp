#include "prx/libc/include/General.hpp"
#include "prx/libSceKeyboard/include/KeyboardState.hpp"

extern "C" {

int APS5_VABI sceKeyboardInit(void) {
    return Keyboard::Initialize(Keyboard::Client::Standard);
}

int APS5_VABI sceKeyboardOpen(int user_id, int32_t type, int32_t index, const void* param) {
    (void)param;
    return Keyboard::Open(Keyboard::Client::Standard, user_id, type, index);
}

int APS5_VABI sceKeyboardClose(int32_t handle) {
    return Keyboard::Close(Keyboard::Client::Standard, handle);
}

int APS5_VABI sceKeyboardRead(int32_t handle, KeyboardData* data, int32_t num) {
    return Keyboard::Read(Keyboard::Client::Standard, handle, data, num);
}

int APS5_VABI sceKeyboardReadState(int32_t handle, KeyboardData* data) {
    return Keyboard::ReadState(Keyboard::Client::Standard, handle, data);
}

int APS5_VABI sceKeyboardGetKey2Char(int32_t handle, int32_t arrange, uint32_t led, uint32_t modifier_key, uint16_t key_code, KeyboardCharData* char_data) {
    return Keyboard::GetKey2Char(Keyboard::Client::Standard, handle, arrange, led, modifier_key, key_code, char_data);
}

int APS5_VABI sceKeyboardConnectPort(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardDebugGetDeviceId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardDeviceOpen(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardDisconnectAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardDisconnectDevice(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardDisconnectPort(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardGetConnection(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardGetDeviceInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardGetHandle(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardMbusInit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardPadEmulateClose(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardPadEmulateOpen(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardSetProcessFocus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKeyboardSetProcessPrivilege(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceDbgKeyboardInit(void) {
    return Keyboard::Initialize(Keyboard::Client::Debug);
}

int APS5_VABI sceDbgKeyboardOpen(int user_id, int32_t type, int32_t index, const void* param) {
    (void)param;
    return Keyboard::Open(Keyboard::Client::Debug, user_id, type, index);
}

int APS5_VABI sceDbgKeyboardClose(int32_t handle) {
    return Keyboard::Close(Keyboard::Client::Debug, handle);
}

int APS5_VABI sceDbgKeyboardRead(int32_t handle, KeyboardData* data, int32_t num) {
    return Keyboard::Read(Keyboard::Client::Debug, handle, data, num);
}

int APS5_VABI sceDbgKeyboardReadState(int32_t handle, KeyboardData* data) {
    return Keyboard::ReadState(Keyboard::Client::Debug, handle, data);
}

int APS5_VABI sceDbgKeyboardGetKey2Char(int32_t handle, int32_t arrange, uint32_t led, uint32_t modifier_key, uint16_t key_code, KeyboardCharData* char_data) {
    return Keyboard::GetKey2Char(Keyboard::Client::Debug, handle, arrange, led, modifier_key, key_code, char_data);
}

void KeyboardPublishInput_nid_postfix(const KeyboardInputEvent& event) {
    Keyboard::Publish(event);
}

}
