#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <stdexcept>

extern "C" {
int APS5_VABI sceConvertKeycodeGetImeKeyboardType(int32_t userId, uint32_t* type);
int APS5_VABI sceConvertKeycodeGetCharacter(uint16_t keycode, uint32_t status, int32_t keyboardType, char16_t* character);
}

namespace {
constexpr int InvalidAddress = static_cast<int>(0x80bc0031u);
constexpr int InvalidUserId = static_cast<int>(0x80bc0010u);
constexpr uint32_t LeftCtrl = 0x01u << 8;
constexpr uint32_t LeftShift = 0x02u << 8;
constexpr uint32_t RightShift = 0x20u << 8;
constexpr uint32_t RightAlt = 0x40u << 8;
constexpr uint32_t NumLock = 1u << 16;
constexpr uint32_t CapsLock = 2u << 16;
constexpr uint32_t ScrollLock = 4u << 16;
constexpr int32_t EnglishUs = 4;

void Require(bool value) {
    if (!value) std::abort();
}

char16_t Character(uint16_t keycode, uint32_t status, int32_t type = EnglishUs) {
    char16_t character = 0xffff;
    Require(sceConvertKeycodeGetCharacter(keycode, status, type, &character) == 0);
    return character;
}

bool Throws(uint32_t status, int32_t type) {
    char16_t character = 0xffff;
    try {
        sceConvertKeycodeGetCharacter(0x04, status, type, &character);
    } catch (const std::runtime_error& error) {
        return std::strstr(error.what(), "sceConvertKeycodeGetCharacter") != nullptr && character == 0xffff;
    }
    return false;
}
}

int main() {
    Require(sceConvertKeycodeGetImeKeyboardType(0, nullptr) == InvalidAddress);
    Require(sceConvertKeycodeGetImeKeyboardType(-1, nullptr) == InvalidAddress);

    uint32_t type = 0xffffffffu;
    Require(sceConvertKeycodeGetImeKeyboardType(-1, &type) == InvalidUserId);
    Require(type == 0xffffffffu);
    Require(sceConvertKeycodeGetImeKeyboardType(1, &type) == 0);
    Require(type == 0);

    type = 0xffffffffu;
    Require(sceConvertKeycodeGetImeKeyboardType(0, &type) == 0);
    Require(type == 0);
    Require(Character(0x04, 0, static_cast<int32_t>(type)) == u'a');
    Require(Character(0x1e, LeftShift, static_cast<int32_t>(type)) == u'!');

    Require(sceConvertKeycodeGetCharacter(0x04, 0, EnglishUs, nullptr) == InvalidAddress);
    Require(sceConvertKeycodeGetCharacter(0x04, 0xffffffffu, 99, nullptr) == InvalidAddress);

    Require(Character(0x04, 0) == u'a');
    Require(Character(0x1d, LeftShift) == u'Z');
    Require(Character(0x04, CapsLock) == u'A');
    Require(Character(0x04, CapsLock | RightShift) == u'a');
    Require(Character(0x1e, 0) == u'1');
    Require(Character(0x1e, LeftShift) == u'!');
    Require(Character(0x1e, CapsLock) == u'1');
    Require(Character(0x27, RightShift) == u')');
    Require(Character(0x2d, 0) == u'-' && Character(0x2d, LeftShift) == u'_');
    Require(Character(0x31, 0) == u'\\' && Character(0x31, LeftShift) == u'|');
    Require(Character(0x38, 0) == u'/' && Character(0x38, RightShift) == u'?');
    Require(Character(0x2c, LeftShift) == u' ');
    Require(Character(0x28, 0) == u'\n' && Character(0x2a, 0) == u'\b' && Character(0x2b, 0) == u'\t');
    Require(Character(0x29, 0) == 0);
    Require(Character(0x3a, 0) == 0);
    Require(Character(0x4f, 0) == 0);
    Require(Character(0xe1, LeftShift) == 0);
    Require(Character(0x54, 0) == u'/' && Character(0x57, 0) == u'+' && Character(0x58, 0) == u'\n');
    Require(Character(0x59, 0) == 0 && Character(0x59, NumLock) == u'1');
    Require(Character(0x62, NumLock) == u'0' && Character(0x63, NumLock) == u'.');
    Require(Character(0x04, LeftCtrl | RightAlt | ScrollLock) == u'a');
    Require(Character(0x04, 0, 0) == Character(0x04, 0, EnglishUs));

    Require(Throws(1u, EnglishUs));
    Require(Throws(0x80u, EnglishUs));
    Require(Throws(0x80000u, EnglishUs));
    Require(Throws(0x80000000u, EnglishUs));
    Require(Throws(0, 1));
    Require(Throws(0, 5));
    Require(Throws(0, 33));
    Require(Throws(0, -1));

    for (std::uint32_t state : {0u, 0x4000000u}) {
        char16_t character[3] = {u'?', 0, 0};
        Require(sceConvertKeycodeGetCharacter(0x04, state, 0, character) == 0);
        Require(character[0] == u'a');
        Require(sceConvertKeycodeGetCharacter(0x1E, state, 0, character) == 0);
        Require(character[0] == u'1');
        Require(sceConvertKeycodeGetCharacter(0x3A, state, 0, character) == 0);
        Require(character[0] == 0);
    }
    return 0;
}
