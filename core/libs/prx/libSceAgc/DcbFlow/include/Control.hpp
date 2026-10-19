#ifndef CORE_LIBS_PRX_LIBSCEAGC_DCBFLOW_INCLUDE_CONTROL_HPP
#define CORE_LIBS_PRX_LIBSCEAGC_DCBFLOW_INCLUDE_CONTROL_HPP

#include "SceTypes.hpp"
#include <cstdint>

extern "C" {
std::uint32_t* APS5_VABI sceAgcDcbAtomicGds(CommandBuffer* buf, std::uint32_t engine, std::uint32_t operation, std::uint32_t completion, std::uint32_t dataMode, std::uint32_t autoIncrementBytes, std::uint16_t base, std::uint16_t size, std::uint32_t offsets, std::uint32_t destination, std::uint64_t source0, std::uint64_t source1);
std::uint32_t* APS5_VABI sceAgcDcbAtomicGds_0900(CommandBuffer* buf, std::uint32_t engine, std::uint32_t operation, std::uint32_t completion, std::uint32_t dataMode, std::uint32_t autoIncrementBytes, std::uint16_t base, std::uint16_t size, std::uint32_t offsets, std::uint32_t destination, std::uint64_t source0, std::uint64_t source1);
}

#endif
