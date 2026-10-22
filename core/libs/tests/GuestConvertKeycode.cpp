#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <stdexcept>

extern "C" {
int APS5_VABI sceConvertKeycodeGetVirtualKeycode(std::uint16_t, std::uint32_t, std::uint16_t*);
int APS5_VABI sceConvertKeycodeGetImeKeyboardType(std::int32_t, std::uint32_t*);
int APS5_VABI sceConvertKeycodeGetCharacter(std::uint16_t, std::uint32_t, std::uint32_t, char16_t*);
}

namespace {
constexpr int InvalidAddress = static_cast<int>(0x80bc0031u);
constexpr int InvalidUserId = static_cast<int>(0x80bc0010u);
constexpr uint32_t LeftCtrl = 0x01u << 8;
constexpr uint32_t LeftShift = 0x02u << 8;
constexpr uint32_t RightShift = 0x20u << 8;
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

    std::uint16_t virtualKey = 0xABCD;
    Require(sceConvertKeycodeGetVirtualKeycode(4, 0, nullptr) == InvalidAddress);
    Require(sceConvertKeycodeGetVirtualKeycode(4, 0, &virtualKey) == static_cast<int>(0x80BC001Du));
    Require(virtualKey == 0xABCD);
    Require(sceConvertKeycodeGetVirtualKeycode(4, 40, &virtualKey) == static_cast<int>(0x80BC001Du));
    Require(sceConvertKeycodeGetVirtualKeycode(4, 4, &virtualKey) == 0 && virtualKey == 65);
    Require(sceConvertKeycodeGetVirtualKeycode(4, 9, &virtualKey) == 0 && virtualKey == 81);
    Require(sceConvertKeycodeGetVirtualKeycode(100, 4, &virtualKey) == 0 && virtualKey == 255);
    Require(sceConvertKeycodeGetVirtualKeycode(136, 24, &virtualKey) == 0 && virtualKey == 21);
    Require(sceConvertKeycodeGetVirtualKeycode(65535, 4, &virtualKey) == 0 && virtualKey == 255);
    for (std::uint32_t layout : {2u, 3u, 12u, 31u, 39u}) {
        virtualKey = 0xABCD;
        bool rejected = false;
        try {
            sceConvertKeycodeGetVirtualKeycode(28, layout, &virtualKey);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        Require(rejected && virtualKey == 0xABCD);
    }

    std::uint32_t type = 0xFFFFFFFFu;
    Require(sceConvertKeycodeGetImeKeyboardType(-1, &type) == InvalidUserId);
    Require(type == 0xFFFFFFFFu);

    bool rejected = false;
    try {
        sceConvertKeycodeGetImeKeyboardType(0, &type);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    Require(rejected && type == 0xFFFFFFFFu);

    Require(sceConvertKeycodeGetCharacter(4, 0, 0, nullptr) == InvalidAddress);
    char16_t character = u'?';
    Require(sceConvertKeycodeGetCharacter(4, 0, 0, &character) == static_cast<int>(0x80BC001Du));
    Require(character == u'?');
    Require(sceConvertKeycodeGetCharacter(4, 0, 40, &character) == static_cast<int>(0x80BC001Du));

    Require(sceConvertKeycodeGetCharacter(0x04, 0xffffffffu, 99, nullptr) == InvalidAddress);
    Require(Character(0x1d, LeftShift) == u'Z');
    Require(Character(0x04, CapsLock | RightShift) == u'a');
    Require(Character(0x1e, CapsLock) == u'1');
    Require(Character(0x27, RightShift) == u')');
    Require(Character(0x2d, 0) == u'-' && Character(0x2d, LeftShift) == u'_');
    Require(Character(0x31, 0) == u'\\' && Character(0x31, LeftShift) == u'|');
    Require(Character(0x38, 0) == u'/' && Character(0x38, RightShift) == u'?');
    Require(Character(0x2c, LeftShift) == u' ');
    Require(Character(0x28, 0) == u'\n' && Character(0x2a, 0) == u'\b' && Character(0x2b, 0) == u'\t');
    Require(Character(0x29, 0) == 0);
    Require(Character(0x4f, 0) == 0);
    Require(Character(0x54, 0) == u'/' && Character(0x57, 0) == u'+' && Character(0x58, 0) == u'\n');
    Require(Character(0x62, NumLock) == u'0' && Character(0x63, NumLock) == u'.');
    Require(Character(0x04, LeftCtrl | ScrollLock) == u'a');
    Require(Throws(0, 33));
    for (std::uint32_t state : {0u, 0x4000000u}) {
        Require(sceConvertKeycodeGetCharacter(4, state, 4, &character) == 0 && character == u'a');
        Require(sceConvertKeycodeGetCharacter(30, state, 4, &character) == 0 && character == u'1');
        Require(sceConvertKeycodeGetCharacter(58, state, 4, &character) == 0 && character == 0);
        Require(sceConvertKeycodeGetCharacter(65535, state, 4, &character) == 0 && character == 0);
        Require(sceConvertKeycodeGetCharacter(0x64, state, 4, &character) == 0 && character == u'\\');
        Require(sceConvertKeycodeGetCharacter(0x27, state, 4, &character) == 0 && character == u'0');
        Require(sceConvertKeycodeGetCharacter(0x2C, state, 4, &character) == 0 && character == u' ');
        Require(sceConvertKeycodeGetCharacter(0x54, state, 4, &character) == 0 && character == u'/');
        Require(sceConvertKeycodeGetCharacter(0x59, state | NumLock, 4, &character) == 0 && character == u'1');
        Require(sceConvertKeycodeGetCharacter(0x63, state | NumLock, 4, &character) == 0 && character == u'.');
    }
    Require(sceConvertKeycodeGetCharacter(4, 0x200, 4, &character) == 0 && character == u'A');
    Require(sceConvertKeycodeGetCharacter(4, 0x20000, 4, &character) == 0 && character == u'A');
    Require(sceConvertKeycodeGetCharacter(4, 0x20200, 4, &character) == 0 && character == u'a');
    Require(sceConvertKeycodeGetCharacter(30, 0x200, 4, &character) == 0 && character == u'!');
    Require(sceConvertKeycodeGetCharacter(89, 0, 4, &character) == 0 && character == 0);
    Require(sceConvertKeycodeGetCharacter(89, 0x10000, 4, &character) == 0 && character == u'1');
    Require(sceConvertKeycodeGetCharacter(89, 0x10200, 4, &character) == 0 && character == u'1');
    Require(sceConvertKeycodeGetCharacter(4, 0, 9, &character) == 0 && character == u'q');
    Require(sceConvertKeycodeGetCharacter(28, 0, 2, &character) == 0 && character == u'z');
    Require(sceConvertKeycodeGetCharacter(20, 0x4000, 2, &character) == 0 && character == u'@');
    Require(sceConvertKeycodeGetCharacter(47, 0x20000, 3, &character) == 0 && character == 220);
    Require(sceConvertKeycodeGetCharacter(47, 0x20200, 3, &character) == 0 && character == 200);
    Require(sceConvertKeycodeGetCharacter(4, 0x4000000, 24, &character) == 0 && character == 0x3141);
    Require(sceConvertKeycodeGetCharacter(5, 0x4000000, 30, &character) == 0 && character == 1575);
    Require(sceConvertKeycodeGetCharacter(5, 0x4000200, 30, &character) == 0 && character == 1570);
    Require(sceConvertKeycodeGetCharacter(30, 0x4000300, 30, &character) == 0 && character == 0x200D);
    Require(sceConvertKeycodeGetCharacter(31, 0x4000300, 30, &character) == 0 && character == 0x200C);
    Require(sceConvertKeycodeGetCharacter(32, 0x4000300, 30, &character) == 0 && character == 0x200E);
    Require(sceConvertKeycodeGetCharacter(33, 0x4000300, 30, &character) == 0 && character == 0x200F);
    for (std::uint32_t layout = 1; layout <= 30; ++layout) {
        for (std::uint16_t key : {83, 224, 225, 226, 227, 228, 229, 230, 231}) {
            Require(sceConvertKeycodeGetCharacter(key, 0xFFFFFFFFu, layout, &character) == 0 && character == 0);
        }
    }
    return 0;
}
