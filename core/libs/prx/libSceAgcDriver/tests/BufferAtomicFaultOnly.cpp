#include "BdaAbi.hpp"
#include "Recompiler.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <map>
#include <span>
#include <spirv/unified1/spirv.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace ShaderRecompiler;

constexpr std::string_view Source = R"(
  [v_readfirstlane_b32 s7, v0]
  buffer_atomic_umax v1, off, s[4:7] offset:4
  s_endpgm
  (and the same with buffer_atomic_umax_x2 v[1:2] for the 64-bit form))";

void Check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void SetGuard(const char* value) {
#ifdef _WIN32
    Check(_putenv_s("APS5_LOOP_GUARD", value) == 0, "buffer atomic fault only: cannot set APS5_LOOP_GUARD");
#else
    Check((*value != '\0' ? setenv("APS5_LOOP_GUARD", value, 1) : unsetenv("APS5_LOOP_GUARD")) == 0, "buffer atomic fault only: cannot set APS5_LOOP_GUARD");
#endif
}

struct Compiled {
    std::size_t faultBindings = 0;
    std::size_t tableBindings = 0;
    std::uint32_t bdaAbiVersion = 0;
    bool int64 = false;
    bool physical = false;
    std::uint32_t addressing = 0;
    std::size_t unalignedReports = 0;
};

void Scan(std::span<const std::uint32_t> words, Compiled& compiled) {
    Check(words.size() >= 5 && words[0] == spv::MagicNumber, "buffer atomic fault only: missing SPIR-V module");
    std::uint32_t u32 = 0;
    std::map<std::uint32_t, std::uint32_t> constants;
    for (std::size_t offset = 5; offset < words.size();) {
        const auto count = words[offset] >> spv::WordCountShift;
        Check(count != 0 && count <= words.size() - offset, "buffer atomic fault only: malformed SPIR-V instruction");
        const auto opcode = words[offset] & spv::OpCodeMask;
        if (opcode == spv::OpCapability && words[offset + 1] == spv::CapabilityInt64) compiled.int64 = true;
        if (opcode == spv::OpCapability && words[offset + 1] == spv::CapabilityPhysicalStorageBufferAddresses) compiled.physical = true;
        if (opcode == spv::OpMemoryModel) compiled.addressing = words[offset + 1];
        if (opcode == spv::OpTypeInt && count == 4 && words[offset + 2] == 32u && words[offset + 3] == 0u) u32 = words[offset + 1];
        if (opcode == spv::OpConstant && count == 4 && u32 != 0 && words[offset + 1] == u32) constants[words[offset + 2]] = words[offset + 3];
        if (opcode == spv::OpFunctionCall && count == 9) {
            const auto reason = constants.find(words[offset + 4]);
            if (reason != constants.end() && reason->second == static_cast<std::uint32_t>(BdaAbi::FaultReason::Unaligned)) ++compiled.unalignedReports;
        }
        offset += count;
    }
}

Compiled Compile(std::uint32_t subgroupSize, bool gpu, bool wide) {
    std::vector<std::uint32_t> code;
    if (gpu) code.push_back(0x7e0e0500u);
    code.insert(code.end(), {wide ? 0xe1600004u : 0xe0e00004u, 0x80010100u, 0xbf810000u});
    const std::array<std::uint32_t, 8> userData{0u, 0u, 0u, 0u, 0x10000000u, 0u, 0x14u, 0x30005004u};
    const std::array<std::uint32_t, 2> plain{spv::CapabilityShader, spv::CapabilityGroupNonUniform};
    const std::array<std::uint32_t, 4> plainWide{spv::CapabilityShader, spv::CapabilityGroupNonUniform, spv::CapabilityInt64, spv::CapabilityInt64Atomics};
    const std::array<std::uint32_t, 5> addressed{spv::CapabilityShader, spv::CapabilityGroupNonUniform, spv::CapabilityInt64, spv::CapabilityPhysicalStorageBufferAddresses, spv::CapabilityStorageBuffer8BitAccess};
    const std::array<std::string_view, 1> plainExtensions{"SPV_KHR_storage_buffer_storage_class"};
    const std::array<std::string_view, 3> addressedExtensions{"SPV_KHR_storage_buffer_storage_class", "SPV_KHR_8bit_storage", "SPV_KHR_physical_storage_buffer"};
    RecompileRequest request{};
    request.shader = {ShaderStage::Compute, 0x20000u, code, 0, {}};
    request.context.waveSize = 64;
    request.context.userDataBaseRegister = 0;
    request.context.userData = userData;
    request.context.compute = ShaderComputeStageInfo{{64u, 1u, 1u}, 0u, {false, false, false}, false, 1u};
    request.target.vulkanVersion = 0x00403000u;
    request.target.spirvVersion = 0x00010600u;
    request.target.subgroupSize = subgroupSize;
    request.target.bdaAbiVersion = BdaAbi::Version;
    if (gpu) {
        request.target.supportedCapabilities = addressed;
        request.target.supportedExtensions = addressedExtensions;
    } else if (wide) {
        request.target.supportedCapabilities = plainWide;
        request.target.supportedExtensions = plainExtensions;
    } else {
        request.target.supportedCapabilities = plain;
        request.target.supportedExtensions = plainExtensions;
    }
    request.target.maxWorkgroupSize = {1024u, 1024u, 64u};
    request.target.maxWorkgroupInvocations = 1024;
    request.target.maxWorkgroupSharedMemoryBytes = 49152;
    request.layout = {0, 0, 0, 128};
    request.useCache = false;
    const auto result = Recompile(request);
    Compiled compiled;
    compiled.faultBindings = static_cast<std::size_t>(std::count_if(result.bindings.begin(), result.bindings.end(), [](const DescriptorBinding& binding) { return binding.role == DescriptorRole::FaultBuffer; }));
    compiled.tableBindings = static_cast<std::size_t>(std::count_if(result.bindings.begin(), result.bindings.end(), [](const DescriptorBinding& binding) { return binding.role == DescriptorRole::BdaPagetable; }));
    compiled.bdaAbiVersion = result.bdaAbiVersion;
    Scan(result.spirv.Words(), compiled);
    return compiled;
}

void Verify(std::uint32_t subgroupSize) {
    const auto lanes = " (" + std::to_string(subgroupSize) + "-lane subgroup)";
    Compiled atomic;
    try {
        atomic = Compile(subgroupSize, false, false);
    } catch (const std::exception& error) {
        throw std::runtime_error("buffer atomic fault only: an atomic on a validated V# needs no BDA support, but the recompile failed: " + std::string(error.what()) + lanes);
    }
    Check(atomic.tableBindings == 0, "buffer atomic fault only: an atomic on a validated V# got a BDA table binding, which makes the dispatch an address-based build" + lanes);
    Check(atomic.faultBindings == 1 && atomic.bdaAbiVersion == BdaAbi::Version, "buffer atomic fault only: an atomic on a validated V# got " + std::to_string(atomic.faultBindings) + " fault buffer bindings and BDA ABI " + std::to_string(atomic.bdaAbiVersion) + lanes);
    Check(!atomic.int64 && !atomic.physical && atomic.addressing == spv::AddressingModelLogical, "buffer atomic fault only: an atomic on a validated V# declared Int64 or physical storage buffer addressing" + lanes);
    Check(atomic.unalignedReports == 0, "buffer atomic fault only: a 32-bit atomic on a validated V# reported an unaligned address instead of acting on the dword that holds it" + lanes);
    Compiled wide;
    try {
        wide = Compile(subgroupSize, false, true);
    } catch (const std::exception& error) {
        throw std::runtime_error("buffer atomic fault only: a 64-bit atomic on a validated V# needs no BDA support, but the recompile failed: " + std::string(error.what()) + lanes);
    }
    Check(wide.tableBindings == 0 && wide.faultBindings == 1, "buffer atomic fault only: a 64-bit atomic on a validated V# got " + std::to_string(wide.tableBindings) + " BDA table and " + std::to_string(wide.faultBindings) + " fault buffer bindings" + lanes);
    Check(!wide.physical && wide.addressing == spv::AddressingModelLogical, "buffer atomic fault only: a 64-bit atomic on a validated V# declared physical storage buffer addressing" + lanes);
    Check(wide.unalignedReports != 0, "buffer atomic fault only: a 64-bit atomic on a validated V# lost its unaligned-address fault report" + lanes);
    const auto gpu = Compile(subgroupSize, true, false);
    Check(gpu.tableBindings == 1, "buffer atomic fault only: an atomic on a GPU-selected V# got " + std::to_string(gpu.tableBindings) + " BDA table bindings, expected 1" + lanes);
}

}

int main() {
    const char* previous = std::getenv("APS5_LOOP_GUARD");
    const std::string saved = previous != nullptr ? previous : "";
    int status = 0;
    try {
        SetGuard("");
        Verify(64u);
        Verify(32u);
        std::puts("buffer atomic fault only tests passed");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n%.*s\n", error.what(), static_cast<int>(Source.size()), Source.data());
        status = 1;
    }
    try {
        SetGuard(saved.c_str());
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        status = 1;
    }
    return status;
}
