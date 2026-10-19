#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::int32_t SCE_VR_TRACKER2_ERROR_NOT_INIT = static_cast<std::int32_t>(0x8A740001);
constexpr std::int32_t SCE_VR_TRACKER2_ERROR_ALREADY_INITIALIZED = static_cast<std::int32_t>(0x8A740002);
constexpr std::int32_t SCE_VR_TRACKER2_ERROR_DEVICE_NOT_REGISTERED = static_cast<std::int32_t>(0x8A740003);
constexpr std::int32_t SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID = static_cast<std::int32_t>(0x8A740006);
constexpr std::int32_t SCE_VR_TRACKER2_ERROR_INVALID_DEVICE_HANDLE = static_cast<std::int32_t>(0x8A740007);

struct InitializeParam {
    std::uint32_t size;
    std::uint8_t reserved0[28];
    void* workMemory;
    std::uint32_t workMemorySize;
    std::uint32_t workMemoryAlignment;
    std::uint8_t reserved1[24];
};

struct GetResultParam {
    std::uint32_t size;
    std::int32_t handle;
    std::uint8_t reserved0[8];
    std::uint64_t time;
    std::uint8_t reserved1[8];
};

static_assert(sizeof(InitializeParam) == 0x48);
static_assert(offsetof(InitializeParam, workMemory) == 0x20);
static_assert(offsetof(InitializeParam, workMemorySize) == 0x28);
static_assert(sizeof(GetResultParam) == 0x20);
static_assert(offsetof(GetResultParam, time) == 0x10);

std::atomic<bool> initialized{false};

std::int32_t NoDevice() {
    return initialized.load() ? SCE_VR_TRACKER2_ERROR_DEVICE_NOT_REGISTERED : SCE_VR_TRACKER2_ERROR_NOT_INIT;
}

void RequireUninitialized(const char* function) {
    if (initialized.load()) throw std::runtime_error(std::string(function) + ": the answer of an initialized tracker is unknown");
}

std::int32_t NotInitialized(const char* function) {
    RequireUninitialized(function);
    return SCE_VR_TRACKER2_ERROR_NOT_INIT;
}

std::int32_t ClearThenReportNotInitialized(const char* function, void* output, std::size_t size) {
    RequireUninitialized(function);
    if (output == nullptr) return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
    std::memset(output, 0, size);
    return SCE_VR_TRACKER2_ERROR_NOT_INIT;
}

}

extern "C" {

std::int32_t APS5_VABI sceVrTracker2Finalize() {
    return initialized.exchange(false) ? 0 : SCE_VR_TRACKER2_ERROR_NOT_INIT;
}

std::int32_t APS5_VABI sceVrTracker2GetCoordinateSystem() {
    return NotInitialized(__func__);
}

std::int32_t APS5_VABI sceVrTracker2GetPlayAreaBoundaryGeometry() {
    return NotInitialized(__func__);
}

std::int32_t APS5_VABI sceVrTracker2ResetLocalCoordinate() {
    return NoDevice();
}

std::int32_t APS5_VABI sceVrTracker2GetPlayAreaOrientedBoundingBox() {
    return NoDevice();
}

std::int32_t APS5_VABI sceVrTracker2GetResult(const GetResultParam* param, void* result) {
    if (!initialized.load()) return SCE_VR_TRACKER2_ERROR_NOT_INIT;
    if (!param || !result || param->size != sizeof(GetResultParam)) return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
    return SCE_VR_TRACKER2_ERROR_DEVICE_NOT_REGISTERED;
}

std::int32_t APS5_VABI sceVrTracker2Initialize(const InitializeParam* param) {
    if (initialized.load()) return SCE_VR_TRACKER2_ERROR_ALREADY_INITIALIZED;
    if (!param || param->size != sizeof(InitializeParam) || !param->workMemory || param->workMemorySize == 0)
        return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
    return initialized.exchange(true) ? SCE_VR_TRACKER2_ERROR_ALREADY_INITIALIZED : 0;
}

std::int32_t APS5_VABI sceVrTracker2LocateCoordinateSystem() {
    return NoDevice();
}

std::int32_t APS5_VABI sceVrTracker2QueryMemory(const void* parameter, void* requirement) {
    if (parameter == nullptr || requirement == nullptr) return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
    const auto* bytes = static_cast<const std::uint8_t*>(parameter);
    std::uint32_t size = 0;
    std::memcpy(&size, bytes, sizeof size);
    if (size != 0x20) return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
    for (std::size_t offset = sizeof size; offset < 0x20; ++offset)
        if (bytes[offset] != 0) return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
    const std::uint32_t fields[3] = {0x20, 32u << 20, 64u << 10};
    std::memset(requirement, 0, 0x20);
    std::memcpy(requirement, fields, sizeof fields);
    return 0;
}

std::int32_t APS5_VABI sceVrTracker2RegisterDevice(std::int32_t type, std::int32_t handle) {
    (void)type;
    (void)handle;
    if (!initialized.load()) return SCE_VR_TRACKER2_ERROR_NOT_INIT;
    return SCE_VR_TRACKER2_ERROR_INVALID_DEVICE_HANDLE;
}

std::int32_t APS5_VABI sceVrTracker2SetCoordinateSystem() {
    return NotInitialized(__func__);
}

std::int32_t APS5_VABI sceVrTracker2UnregisterDevice(std::int32_t handle) {
    (void)handle;
    if (!initialized.load()) return SCE_VR_TRACKER2_ERROR_NOT_INIT;
    return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
}

APS5_EXPORT("1m6DD9BBmSE", VrTracker2NotInitialized);
APS5_EXPORT("SgunslIO-C0", VrTracker2NotInitialized);
APS5_EXPORT("KMIToVnHrhc", VrTracker2NotInitialized);
APS5_EXPORT("XfEBVemeJyE", VrTracker2NotInitialized);
APS5_EXPORT("y79S3-zrJKg", VrTracker2NotInitialized);
APS5_EXPORT("b777Kpfe2J8", VrTracker2NotInitialized);
APS5_EXPORT("UfUYgiT7VK0", VrTracker2NotInitialized);
APS5_EXPORT("MB7B6396K6U", VrTracker2NotInitialized);
APS5_EXPORT("H2Mnx7gz4ac", VrTracker2NotInitialized);
APS5_EXPORT("SgNHzN6XaEc", VrTracker2NotInitialized);
APS5_EXPORT("sYBNLrMxjAM", VrTracker2NotInitialized);
APS5_EXPORT("3gQncidodmw", VrTracker2NotInitialized);
APS5_EXPORT("qnCmEbUGqWY", VrTracker2NotInitialized);
APS5_EXPORT("oGNn7JIdMHw", VrTracker2NotInitialized);
APS5_EXPORT("80IxOn8swIA", VrTracker2NotInitialized);
APS5_EXPORT("NTbJNAZA4ro", VrTracker2NotInitialized);
APS5_EXPORT("914+9oJxNts", VrTracker2NotInitialized);
APS5_EXPORT("c6SZtdEjH0E", VrTracker2NotInitialized);
APS5_EXPORT("74IXIMe+Lto", VrTracker2NotInitialized);
APS5_EXPORT("hmahOYMiYNY", VrTracker2NotInitialized);
APS5_EXPORT("pNXc6ZlLq-8", VrTracker2NotInitialized);
APS5_EXPORT("omJ7b8FqAN0", VrTracker2NotInitialized);
APS5_EXPORT("G2PWj1HLqn8", VrTracker2NotInitialized);
APS5_EXPORT("73zcxAFawYk", VrTracker2NotInitialized);
APS5_EXPORT("vegrWFNiQwc", VrTracker2NotInitialized);
APS5_EXPORT("6r01IawmypA", VrTracker2NotInitialized);
APS5_EXPORT("zeof7sF2fqE", VrTracker2NotInitialized);
APS5_EXPORT("Z0rfHOHQV1w", VrTracker2NotInitialized);
APS5_EXPORT("4l7x+uy3dWw", VrTracker2NotInitialized);
APS5_EXPORT("ZXlt-Xb3da4", VrTracker2NotInitialized);
APS5_EXPORT("BSwAdB3GM3o", VrTracker2NotInitialized);
APS5_EXPORT("9ATuhVfn9Dw", VrTracker2NotInitialized);
APS5_EXPORT("wjmVbmf58rE", VrTracker2NotInitialized);
APS5_EXPORT("4+B+TcHEHks", VrTracker2NotInitialized);
APS5_EXPORT("cjDz4kYJKTw", VrTracker2NotInitialized);
APS5_EXPORT("BjVg0fKV9VA", VrTracker2NotInitialized);
APS5_EXPORT("oDV5j-2Luic", VrTracker2NotInitialized);
APS5_EXPORT("t+86o9sCZ-Y", VrTracker2NotInitialized);
APS5_EXPORT("TIpYP35JzUs", VrTracker2NotInitialized);
APS5_EXPORT("LrSc+KuxIkg", VrTracker2NotInitialized);
APS5_EXPORT("Ur+M5wy+QBY", VrTracker2NotInitialized);
APS5_EXPORT("OvKC351Udqo", VrTracker2NotInitialized);
APS5_EXPORT("LQV4nz6wteY", VrTracker2NotInitialized);
APS5_EXPORT("x0pGthEwf88", VrTracker2NotInitialized);
APS5_EXPORT("d5Fjs0rHkBg", VrTracker2NotInitialized);
APS5_EXPORT("Yed3ZONcuMk", VrTracker2NotInitialized);
APS5_EXPORT("fkptW6uRdWc", VrTracker2NotInitialized);
APS5_EXPORT("HDlD-xE1Xuk", VrTracker2NotInitialized);
APS5_EXPORT("IGdE6nsMTAY", VrTracker2NotInitialized);
APS5_EXPORT("DSTergmOvvE", VrTracker2NotInitialized);
APS5_EXPORT("B7hPwvvg79E", VrTracker2NotInitialized);
APS5_EXPORT("ZwnIsDe1eNE", VrTracker2NotInitialized);
APS5_EXPORT("B2vw+s-0Dfo", VrTracker2NotInitialized);
APS5_EXPORT("4LNTLivfRuA", VrTracker2NotInitialized);
APS5_EXPORT("edajhZhz9Uo", VrTracker2NotInitialized);
APS5_EXPORT("QS+7oqiszuc", VrTracker2NotInitialized);
APS5_EXPORT("-dDyvzd2jf0", VrTracker2NotInitialized);
APS5_EXPORT("k8JDLkCrOgs", VrTracker2NotInitialized);
APS5_EXPORT("-HFx+dnDSOI", VrTracker2NotInitialized);
APS5_EXPORT("wgDeDXirl0g", VrTracker2NotInitialized);
APS5_EXPORT("a1kWSTYGBxg", VrTracker2NotInitialized);
APS5_EXPORT("zUwxBEKr3fc", VrTracker2NotInitialized);
APS5_EXPORT("TC7VTVzj0xc", VrTracker2NotInitialized);
APS5_EXPORT("1DwROxtuANs", VrTracker2NotInitialized);
APS5_EXPORT("wwbt4s8cbMQ", VrTracker2NotInitialized);
APS5_EXPORT("eGxBJ0IolOA", VrTracker2NotInitialized);
APS5_EXPORT("9BaFhspHF+E", VrTracker2NotInitialized);
APS5_EXPORT("lb9q5nJ4vzk", VrTracker2NotInitialized);
APS5_EXPORT("ZsD1-thjXS4", VrTracker2NotInitialized);
APS5_EXPORT("QdHQnM0XICI", VrTracker2NotInitialized);
APS5_EXPORT("Ek4WMOvwv6g", VrTracker2NotInitialized);
APS5_EXPORT("W1YAO6G+hNc", VrTracker2NotInitialized);
APS5_EXPORT("HAJB7irjl+0", VrTracker2NotInitialized);
APS5_EXPORT("x2mN50nKScs", VrTracker2NotInitialized);
APS5_EXPORT("cCKExge5d3A", VrTracker2NotInitialized);
APS5_EXPORT("gn6e5I4g9bw", VrTracker2NotInitialized);
APS5_EXPORT("OYTKt9diFOY", VrTracker2NotInitialized);
APS5_EXPORT("loU5lT92Zrw", VrTracker2NotInitialized);
APS5_EXPORT("o6fsgA3R2Yc", VrTracker2NotInitialized);
APS5_EXPORT("AeBtEIE0idk", VrTracker2NotInitialized);
APS5_EXPORT("DBa3wov7dKQ", VrTracker2NotInitialized);
APS5_EXPORT("ojTGClQ52r4", VrTracker2NotInitialized);
APS5_EXPORT("oOLE62H66z0", VrTracker2NotInitialized);
APS5_EXPORT("MHekhnRBKAw", VrTracker2NotInitialized);
APS5_EXPORT("JnSmj+5E8WY", VrTracker2NotInitialized);
APS5_EXPORT("N1P+melAgb8", VrTracker2NotInitialized);
APS5_EXPORT("-e7A3rAM3Yk", VrTracker2NotInitialized);
APS5_EXPORT("RefT1spBleg", VrTracker2NotInitialized);
APS5_EXPORT("DsOfryhbm4U", VrTracker2NotInitialized);
std::int32_t APS5_VABI VrTracker2NotInitialized() {
    return NotInitialized(__func__);
}

APS5_EXPORT("XknY0IDcOwU", VrTracker2ClearWideResult);
std::int32_t APS5_VABI VrTracker2ClearWideResult(void* result) {
    return ClearThenReportNotInitialized(__func__, result, 24);
}

APS5_EXPORT("ark13GkiskA", VrTracker2ClearNarrowResult);
std::int32_t APS5_VABI VrTracker2ClearNarrowResult(void* result) {
    return ClearThenReportNotInitialized(__func__, result, 16);
}

APS5_EXPORT("-YvjOUph8B0", VrTracker2CheckSecondThenReportNotInitialized);
std::int32_t APS5_VABI VrTracker2CheckSecondThenReportNotInitialized(const void* first, const void* second) {
    (void)first;
    RequireUninitialized(__func__);
    return second == nullptr ? SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID : SCE_VR_TRACKER2_ERROR_NOT_INIT;
}

APS5_EXPORT("QyZvFyf-QEk", VrTracker2Accept);
APS5_EXPORT("fTYZOZepkqE", VrTracker2Accept);
std::int32_t APS5_VABI VrTracker2Accept() {
    return 0;
}

APS5_EXPORT("TZIUIk9id3c", VrTracker2FillDefaults);
std::int32_t APS5_VABI VrTracker2FillDefaults(void* output) {
    if (output == nullptr) APS5_INVALID_ARG_EX;
    auto* bytes = static_cast<std::uint8_t*>(output);
    const std::uint32_t first = 0xC000;
    const std::uint32_t second = 0;
    const std::uint16_t third = 0x40;
    std::memcpy(bytes + 4, &first, sizeof first);
    std::memcpy(bytes + 8, &second, sizeof second);
    std::memcpy(bytes + 12, &third, sizeof third);
    std::memcpy(bytes + 14, &third, sizeof third);
    return 0;
}

APS5_EXPORT("IB0qcDVu8vU", VrTracker2ClearWord);
std::int32_t APS5_VABI VrTracker2ClearWord(void* output) {
    if (output == nullptr) APS5_INVALID_ARG_EX;
    std::memset(output, 0, sizeof(std::uint32_t));
    return 0;
}

APS5_EXPORT("jRmuXjsknsI", VrTracker2Unknown01);
std::int32_t APS5_VABI VrTracker2Unknown01() {
    NotImplemented_nid_no_patch("jRmuXjsknsI");
    return 0;
}

APS5_EXPORT("NBhWy+Gl0tY", VrTracker2Unknown02);
std::int32_t APS5_VABI VrTracker2Unknown02() {
    NotImplemented_nid_no_patch("NBhWy+Gl0tY");
    return 0;
}

APS5_EXPORT("9R6W33Au4B4", VrTracker2Unknown03);
std::int32_t APS5_VABI VrTracker2Unknown03() {
    NotImplemented_nid_no_patch("9R6W33Au4B4");
    return 0;
}

APS5_EXPORT("O8ZkTFtvUL4", VrTracker2Unknown04);
std::int32_t APS5_VABI VrTracker2Unknown04() {
    NotImplemented_nid_no_patch("O8ZkTFtvUL4");
    return 0;
}

APS5_EXPORT("940WPg+WwIk", VrTracker2Unknown05);
std::int32_t APS5_VABI VrTracker2Unknown05() {
    NotImplemented_nid_no_patch("940WPg+WwIk");
    return 0;
}

APS5_EXPORT("oZDm1bynCaA", VrTracker2Unknown06);
std::int32_t APS5_VABI VrTracker2Unknown06() {
    NotImplemented_nid_no_patch("oZDm1bynCaA");
    return 0;
}

APS5_EXPORT("U1VYczwqukY", VrTracker2Unknown07);
std::int32_t APS5_VABI VrTracker2Unknown07() {
    NotImplemented_nid_no_patch("U1VYczwqukY");
    return 0;
}

}
