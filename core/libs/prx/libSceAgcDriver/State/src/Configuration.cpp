#include "prx/libSceAgcDriver/State/include/Configuration.hpp"

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

struct TessellationFactorRing {
 uintptr_t base = 0xff0000000;
 uint32_t size = 0x20000;
};

std::mutex tessellationFactorRingMutex;
TessellationFactorRing tessellationFactorRing;

struct HsOffchipParam {
 uint16_t first = 0;
 uint16_t second = 0;
};

std::mutex hsOffchipParamMutex;
HsOffchipParam hsOffchipParam;

}

extern "C" {

int APS5_VABI sceAgcDriverSetHsOffchipParam(uint16_t first, uint16_t second) {
 std::lock_guard lock(hsOffchipParamMutex);
 hsOffchipParam = {first, second};
 return 0;
}

int APS5_VABI sceAgcDriverGetHsOffchipParam(uint16_t* first, uint16_t* second) {
 if (first == nullptr || second == nullptr) throw std::invalid_argument("sceAgcDriverGetHsOffchipParam: null output");
 std::lock_guard lock(hsOffchipParamMutex);
 *first = hsOffchipParam.first;
 *second = hsOffchipParam.second;
 return 0;
}

int APS5_VABI sceAgcDriverSetTFRing(const volatile void* base, uint32_t size) {
 if (base == nullptr || size == 0) throw std::invalid_argument("sceAgcDriverSetTFRing: empty ring");
 std::lock_guard lock(tessellationFactorRingMutex);
 tessellationFactorRing = {reinterpret_cast<uintptr_t>(base), size};
 return 0;
}

int APS5_VABI sceAgcDriverGetTFRing(uintptr_t* base, uint32_t* size) {
 if (base == nullptr || size == nullptr) throw std::invalid_argument("sceAgcDriverGetTFRing: null output");
 std::lock_guard lock(tessellationFactorRingMutex);
 *base = tessellationFactorRing.base;
 *size = tessellationFactorRing.size;
 return 0;
}

}
