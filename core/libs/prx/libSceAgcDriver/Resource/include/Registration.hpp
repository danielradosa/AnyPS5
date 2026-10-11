#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_RESOURCE_INCLUDE_REGISTRATION_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_RESOURCE_INCLUDE_REGISTRATION_HPP

#include <cstddef>
#include <cstdint>

namespace AgcDriver {

bool WorkloadSlotTableCovers(std::uint64_t address, std::size_t bytes);

}

extern "C" std::uint64_t AgcDriverWorkloadStreamSlot_nid_no_patch(std::uint32_t streamId);

#endif
