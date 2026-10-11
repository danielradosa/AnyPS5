#ifndef CORE_LIBS_PRX_LIBSCEAGC_COMMAND_INCLUDE_WORKLOAD_HPP
#define CORE_LIBS_PRX_LIBSCEAGC_COMMAND_INCLUDE_WORKLOAD_HPP

#include "prx/libSceAgc/Command/include/Packet.hpp"

namespace Agc::Command {

std::uint32_t* WriteWorkloadsActive(CommandBuffer* buffer, bool compute, std::uint32_t streamId, const std::uint32_t* workloadIds, std::uint32_t workloadCount, const char* function);
std::uint32_t* WriteWorkloadComplete(CommandBuffer* buffer, bool compute, std::uint32_t streamId, std::uint32_t workloadId, const char* function);
std::uint32_t* WriteWorkloadStreamInactive(CommandBuffer* buffer, bool compute, std::uint32_t streamId, const char* function);

}

#endif
