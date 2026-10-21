#ifndef CORE_LIBS_PRX_LIBKERNEL_FILE_INCLUDE_RESERVEDDESCRIPTOR_HPP
#define CORE_LIBS_PRX_LIBKERNEL_FILE_INCLUDE_RESERVEDDESCRIPTOR_HPP

#include "prx/libc/include/general/VabiMacros.hpp"

using GuestDescriptorObserver = void (APS5_VABI*)(int descriptor);

extern "C" {
int APS5_VABI GuestReserveDescriptor_nid_no_patch(void);
void APS5_VABI GuestWatchDescriptorClose_nid_no_patch(GuestDescriptorObserver observer);
}

#endif
