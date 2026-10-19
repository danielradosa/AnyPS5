#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

struct BgftInitParams {
    void* heap;
    std::uint32_t heapSize;
    std::int32_t reserved;
};

namespace {

constexpr std::uint32_t MinimumHeapSize = 1024 * 1024;

}

extern "C" {

int APS5_VABI sceBgftServiceInit(const BgftInitParams* params) {
    if (params == nullptr || params->heap == nullptr || params->heapSize < MinimumHeapSize || params->reserved != 0) {
        throw std::invalid_argument(std::string(__func__) + ": the service takes a heap of at least 1 MiB and a zero reserved word");
    }
    return 0;
}
int APS5_VABI sceBgftServiceTerm(void) {
    return 0;
}
int APS5_VABI sceBgftServiceDownloadPauseTaskAll(void) {
    return 0;
}
int APS5_VABI sceBgftServiceDownloadResumeTaskAll(void) {
    return 0;
}
int APS5_VABI sceBgftServiceDownloadStartTaskAll(void) {
    return 0;
}
int APS5_VABI sceBgftServiceDownloadStopTaskAll(void) {
    return 0;
}
int APS5_VABI sceBgftServiceDownloadFindActivePatchTask() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceDownloadFindActivePupTask() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceDownloadFindTaskByContentId(const char* contentId, int32_t subType, int32_t* taskId) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceDownloadGetProgress(int32_t taskId, void* progress) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceDownloadPauseTask(int32_t taskId) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceDownloadResumeTask(int32_t taskId) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceDownloadStartTask(int32_t taskId) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceDownloadStopTask(int32_t taskId) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadFindActiveGameAndGameAcTask() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadFindActiveTask() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadGetGameAndGameAcProgress() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadGetPatchProgress() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadGetPlayGoProgress() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadGetProgress() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadGetTaskInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadPauseTask() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadRegisterTaskByStorageEx() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadResumeTask() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadStopTask() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadStartTask() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntDownloadUnregisterTask() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceBgftServiceIntInit() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
