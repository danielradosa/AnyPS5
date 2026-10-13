#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_SUBMIT_INCLUDE_COMMANDBUFFER_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_SUBMIT_INCLUDE_COMMANDBUFFER_HPP

#include "SceTypes.hpp"

extern "C" int APS5_VABI sceAgcDriverSubmitCommandBuffer(void* queueContext, const Packet* packet);
extern "C" int APS5_VABI sceAgcDriverSubmitMultiCommandBuffers(void* queueContext, std::uint32_t* const* commandBuffers, const std::uint32_t* sizesInDwords, std::uint32_t count);

#endif
