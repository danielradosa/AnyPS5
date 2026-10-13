#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

constexpr int REMOTEPLAY_CONNECTION_STATUS_DISCONNECT = 0;

extern "C" {

int APS5_VABI sceShareGetCurrentStatus(uint32_t feature_flag, ShareCurrentStatus* status);
int APS5_VABI sceShareInitialize(size_t heap_size, int thread_priority, uint64_t affinity_mask);
int APS5_VABI sceShareTerminate(void);

int APS5_VABI RemotePlayGetConnectionStatus(int userId, int* status) {
 (void)userId;
 if (!status) APS5_INVALID_ARG_EX;
 *status = REMOTEPLAY_CONNECTION_STATUS_DISCONNECT;
 return 0;
}

int APS5_VABI ShareGetCurrentStatus(uint32_t feature_flag, ShareCurrentStatus* status) {
 return sceShareGetCurrentStatus(feature_flag, status);
}

int APS5_VABI ShareInitialize(size_t heap_size, int thread_priority, uint64_t affinity_mask) {
 return sceShareInitialize(heap_size, thread_priority, affinity_mask);
}

int APS5_VABI ShareTerminate(void) {
 return sceShareTerminate();
}

}
