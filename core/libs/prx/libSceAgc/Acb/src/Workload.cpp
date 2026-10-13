#include "prx/libSceAgc/Acb/include/Workload.hpp"

#include "prx/libSceAgc/Command/include/Workload.hpp"
#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

std::uint32_t* APS5_VABI sceAgcAcbSetWorkloadsActive(CommandBuffer* buf, std::uint32_t streamId, const std::uint32_t* workloadIds, std::uint32_t workloadCount) {
    return Agc::Command::WriteWorkloadsActive(buf, true, streamId, workloadIds, workloadCount, __func__);
}

std::uint32_t* APS5_VABI sceAgcAcbSetWorkloadComplete(CommandBuffer* buf, std::uint32_t streamId, std::uint32_t workloadId) {
    return Agc::Command::WriteWorkloadComplete(buf, true, streamId, workloadId, __func__);
}

std::uint32_t* APS5_VABI sceAgcAcbSetWorkloadStreamInactive(CommandBuffer* buf, std::uint32_t streamId) {
    return Agc::Command::WriteWorkloadStreamInactive(buf, true, streamId, __func__);
}

}
