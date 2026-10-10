#include "BdaAbi.hpp"
#include "Recompiler.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <initializer_list>
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
  s_mov_b32 s8, 0
loop:
  s_add_u32 s8, s8, 1
  s_cmp_eq_u32 s8, 0
  s_cbranch_scc1 done
  s_branch loop
done:
  s_endpgm)";

constexpr std::array<std::uint32_t, 6> Code{0xbe880380u, 0x80088108u, 0xbf068008u, 0xbf850001u, 0xbf82fffcu, 0xbf810000u};

void Check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void SetGuard(const char* value) {
#ifdef _WIN32
    Check(_putenv_s("APS5_LOOP_GUARD", value) == 0, "loop guard: cannot set APS5_LOOP_GUARD");
#else
    Check((*value != '\0' ? setenv("APS5_LOOP_GUARD", value, 1) : unsetenv("APS5_LOOP_GUARD")) == 0, "loop guard: cannot set APS5_LOOP_GUARD");
#endif
}

struct Compiled {
    std::size_t faultBindings = 0;
    std::size_t tableBindings = 0;
    std::uint32_t bdaAbiVersion = 0;
    std::size_t loopLimitReports = 0;
};

std::size_t LoopLimitReports(std::span<const std::uint32_t> words) {
    Check(words.size() >= 5 && words[0] == spv::MagicNumber, "loop guard: missing SPIR-V module");
    std::uint32_t u32 = 0;
    std::map<std::uint32_t, std::uint32_t> constants;
    std::size_t reports = 0;
    for (std::size_t offset = 5; offset < words.size();) {
        const auto count = words[offset] >> spv::WordCountShift;
        Check(count != 0 && count <= words.size() - offset, "loop guard: malformed SPIR-V instruction");
        const auto opcode = words[offset] & spv::OpCodeMask;
        if (opcode == spv::OpTypeInt && count == 4 && words[offset + 2] == 32u && words[offset + 3] == 0u) u32 = words[offset + 1];
        if (opcode == spv::OpConstant && count == 4 && u32 != 0 && words[offset + 1] == u32) constants[words[offset + 2]] = words[offset + 3];
        if (opcode == spv::OpFunctionCall && count > 4) {
            const auto reason = constants.find(words[offset + 4]);
            if (reason != constants.end() && reason->second == static_cast<std::uint32_t>(BdaAbi::FaultReason::LoopLimit)) ++reports;
        }
        offset += count;
    }
    return reports;
}

Compiled Compile(std::uint32_t subgroupSize) {
    const std::array<std::uint32_t, 4> userData{0x10000000u, 0x00100000u, 0x40u, 0x00027facu};
    const std::array<std::uint32_t, 2> capabilities{spv::CapabilityShader, spv::CapabilityGroupNonUniform};
    const std::array<std::string_view, 1> extensions{"SPV_KHR_storage_buffer_storage_class"};
    RecompileRequest request{};
    request.shader = {ShaderStage::Compute, 0x20000u, Code, 0, {}};
    request.context.waveSize = 64;
    request.context.userDataBaseRegister = 0;
    request.context.userData = userData;
    request.context.compute = ShaderComputeStageInfo{{64u, 1u, 1u}, 0u, {false, false, false}, false, 1u};
    request.target.vulkanVersion = 0x00403000u;
    request.target.spirvVersion = 0x00010600u;
    request.target.subgroupSize = subgroupSize;
    request.target.bdaAbiVersion = BdaAbi::Version;
    request.target.supportedCapabilities = capabilities;
    request.target.supportedExtensions = extensions;
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
    compiled.loopLimitReports = LoopLimitReports(result.spirv.Words());
    return compiled;
}

void VerifyGuard(std::uint32_t subgroupSize) {
    const auto lanes = " (" + std::to_string(subgroupSize) + "-lane subgroup)";
    for (const char* value : {"", "0"}) {
        SetGuard(value);
        const auto off = Compile(subgroupSize);
        const std::string name = *value == '\0' ? "unset" : std::string("=") + value;
        Check(off.faultBindings == 0 && off.tableBindings == 0 && off.bdaAbiVersion == 0u, "loop guard: with APS5_LOOP_GUARD " + name + " a program without DMA got " + std::to_string(off.faultBindings) + " fault buffer bindings and BDA ABI " + std::to_string(off.bdaAbiVersion) + lanes);
        Check(off.loopLimitReports == 0, "loop guard: with APS5_LOOP_GUARD " + name + " the SPIR-V records a LoopLimit fault" + lanes);
    }
    SetGuard("1000");
    const auto guarded = Compile(subgroupSize);
    Check(guarded.faultBindings == 1 && guarded.tableBindings == 0, "loop guard: APS5_LOOP_GUARD=1000 gave a program without DMA " + std::to_string(guarded.faultBindings) + " fault buffer and " + std::to_string(guarded.tableBindings) + " BDA table bindings, expected one fault buffer and no table" + lanes);
    Check(guarded.bdaAbiVersion == BdaAbi::Version, "loop guard: the guarded program's fault buffer has BDA ABI " + std::to_string(guarded.bdaAbiVersion) + lanes);
    Check(guarded.loopLimitReports != 0, "loop guard: APS5_LOOP_GUARD=1000 left no LoopLimit fault record in the SPIR-V of an s_branch loop" + lanes);
}

}

int main() {
    const char* previous = std::getenv("APS5_LOOP_GUARD");
    const std::string saved = previous != nullptr ? previous : "";
    int status = 0;
    try {
        VerifyGuard(64u);
        VerifyGuard(32u);
        std::puts("loop guard tests passed");
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
