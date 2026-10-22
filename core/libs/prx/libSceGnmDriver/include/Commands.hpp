#ifndef CORE_LIBS_PRX_LIBSCEGNMDRIVER_INCLUDE_COMMANDS_HPP
#define CORE_LIBS_PRX_LIBSCEGNMDRIVER_INCLUDE_COMMANDS_HPP

#include <cstdint>
#include <initializer_list>
#include <utility>

namespace GnmDriver {

inline constexpr std::uint32_t OpClearState = 0x12;
inline constexpr std::uint32_t OpContextControl = 0x28;
inline constexpr std::uint32_t OpNumInstances = 0x2f;
inline constexpr std::uint32_t OpSetContextReg = 0x69;
inline constexpr std::uint32_t OpSetShReg = 0x76;
inline constexpr std::uint32_t OpSetUconfigReg = 0x79;
inline constexpr std::uint32_t HwInitPacketWords = 0x100;
inline constexpr std::uint32_t NeoVgtParamRegister = 0x40000258u;
inline constexpr std::uint32_t NeoVgtParamDefault = 0x6d007fu;
inline constexpr std::uint32_t ContextStateWords = 0x20;
inline constexpr std::uint32_t ContextState400Words = 0x100;
inline constexpr std::uint32_t Sdk400 = 0x4000000;

enum class HardwareState { Sdk100, Sdk175, Neo200, Neo350 };

using RegisterValue = std::pair<std::uint32_t, std::uint32_t>;

std::uint32_t* SetRegisters(std::uint32_t* at, std::uint32_t opcode, std::initializer_list<RegisterValue> values);
std::uint32_t* DefaultState(std::uint32_t* at, HardwareState state);
std::uint32_t DrawInitDefaultHardwareState(std::uint32_t* cmd, std::uint32_t size, HardwareState state);
std::uint32_t DrawInitToNeoContextState(std::uint32_t* cmd, std::uint32_t size, bool sdk400);
int SetPsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, bool clearShaderMask);
int SetCsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t modifier);

}

#endif
