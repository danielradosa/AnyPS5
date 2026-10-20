#include "prx/libSceAgc/Misc/include/Semaphore.hpp"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <mutex>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int GRAPHICS5_ERROR_INVALID_ALIGNMENT = static_cast<int>(0x8a6c0002u);
constexpr int GRAPHICS5_ERROR_OUT_OF_RANGE = static_cast<int>(0x8a6c000bu);
constexpr int GRAPHICS5_ERROR_SEMAPHORE_MEMORY_REGISTERED = static_cast<int>(0x8a6c0048u);
constexpr int GRAPHICS5_ERROR_SEMAPHORE_MEMORY_UNREGISTERED = static_cast<int>(0x8a6c0049u);
constexpr std::uint64_t semaphoreMemoryAlignment = 0x4000u;
constexpr std::uint64_t semaphoreMemoryLimit = 0x40000000u;
constexpr std::uint64_t semaphoreLabelBytes = 32u;

std::mutex semaphoreMutex;
std::uint8_t* semaphoreMemory = nullptr;
std::uint64_t semaphoreBytes = 0;

bool aligned(std::uint64_t value) {
    return (value & (semaphoreMemoryAlignment - 1u)) == 0;
}

int registerSemaphoreMemory(void* memory, std::uint64_t sizeInBytes, bool limited) {
    std::lock_guard lock(semaphoreMutex);
    if (semaphoreBytes != 0) return GRAPHICS5_ERROR_SEMAPHORE_MEMORY_REGISTERED;
    if (!aligned(sizeInBytes)) return GRAPHICS5_ERROR_INVALID_ALIGNMENT;
    if (limited && sizeInBytes > semaphoreMemoryLimit) return GRAPHICS5_ERROR_OUT_OF_RANGE;
    if (!aligned(reinterpret_cast<std::uintptr_t>(memory))) return GRAPHICS5_ERROR_INVALID_ALIGNMENT;
    semaphoreMemory = static_cast<std::uint8_t*>(memory);
    semaphoreBytes = sizeInBytes;
    if (sizeInBytes != 0) std::memset(memory, 0, sizeInBytes);
    return 0;
}

}

extern "C" {

int APS5_VABI sceAgcSetAmmSemaphoreMemory(void* memory, std::uint64_t sizeInBytes) {
    return registerSemaphoreMemory(memory, sizeInBytes, false);
}

int APS5_VABI sceAgcSetSemaphoreMemory(void* memory, std::uint64_t sizeInBytes) {
    return registerSemaphoreMemory(memory, sizeInBytes, true);
}

int APS5_VABI sceAgcGetSemaphoreLabel(std::uint32_t index, void** label) {
    std::lock_guard lock(semaphoreMutex);
    if (semaphoreBytes == 0) return GRAPHICS5_ERROR_SEMAPHORE_MEMORY_UNREGISTERED;
    const auto offset = std::uint64_t{index} * semaphoreLabelBytes;
    if (offset + semaphoreLabelBytes > semaphoreBytes) return GRAPHICS5_ERROR_OUT_OF_RANGE;
    *label = semaphoreMemory + offset;
    return 0;
}

}
