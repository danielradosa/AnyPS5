#include <cstdint>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "HitLog.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceKeyboard/include/KeyCharacter.hpp"

namespace {
constexpr int kErrInvalidAddress = static_cast<int>(0x80BC0031u);
constexpr int kErrInvalidUserId = static_cast<int>(0x80BC0010u);
constexpr std::uint32_t kStatusModifiers = 0xff00u;
constexpr std::uint32_t kStatusLeds = 0x70000u;
constexpr std::uint32_t kIgnoredStateBits = 0x4000000u;
constexpr std::int32_t kKeyboardTypeEnglishUs = 4;
}  // namespace

extern "C" {

int APS5_VABI sceConvertKeycodeGetImeKeyboardType(int32_t userId, uint32_t* type) {
 if (type == nullptr) return kErrInvalidAddress;
 if (userId < 0) return kErrInvalidUserId;
 *type = 0;
 return 0;
}
int APS5_VABI sceConvertKeycodeGetVirtualKeycode(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3) {
 APS5_HIT("CONVERTKEYCODE", "sceConvertKeycodeGetVirtualKeycode args 0x%llx 0x%llx 0x%llx 0x%llx -> error (signature unknown)",
          static_cast<unsigned long long>(a0), static_cast<unsigned long long>(a1), static_cast<unsigned long long>(a2), static_cast<unsigned long long>(a3));
 return kErrInvalidAddress;
}

int APS5_VABI sceConvertKeycodeGetCharacterFromKeyboardData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceConvertKeycodeGetCharacter(std::uint16_t keycode, std::uint32_t status, std::int32_t keyboardType, char16_t* character) {
    if (character == nullptr) return kErrInvalidAddress;
    if ((status & ~(kStatusModifiers | kStatusLeds | kIgnoredStateBits)) != 0)
        throw std::runtime_error(std::string(__func__) + ": unsupported status bits " + std::to_string(status & ~(kStatusModifiers | kStatusLeds | kIgnoredStateBits)));
    if (keyboardType != 0 && keyboardType != kKeyboardTypeEnglishUs)
        throw std::runtime_error(std::string(__func__) + ": keyboard type " + std::to_string(keyboardType) + " not implemented");
    *character = static_cast<char16_t>(Keyboard::Character(false, (status & kStatusLeds) >> 16, (status & kStatusModifiers) >> 8, keycode));
    return 0;
}

}
