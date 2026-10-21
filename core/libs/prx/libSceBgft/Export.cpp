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

void CheckInitParams(const char* function, const BgftInitParams* params) {
    if (params == nullptr || params->heap == nullptr || params->heapSize < MinimumHeapSize || params->reserved != 0) {
        throw std::invalid_argument(std::string(function) + ": the service takes a heap of at least 1 MiB and a zero reserved word");
    }
}

}

extern "C" {

int APS5_VABI sceBgftServiceInit(const BgftInitParams* params) {
    CheckInitParams(__func__, params);
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
int APS5_VABI sceBgftServiceIntInit(const BgftInitParams* params) {
    CheckInitParams(__func__, params);
    return 0;
}
int APS5_VABI sceBgftServiceDownloadQueryTask(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDebugDownloadCorruptPkg(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDebugDownloadRequest(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDebugPlayGoGetPlayGoStatusString(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDebugPlayGoIsPaused(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDebugPlayGoResume(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDebugPlayGoRevertToFullState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDebugPlayGoRevertToInitialState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDebugPlayGoRevertToSnapshot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDebugPlayGoSnapshotByTitleId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDebugPlayGoSuspend(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadDebugDeleteBgftEnvFile(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadDebugDownloadBgftEnvFile(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadDebugGetBgftEnvInfoString(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadDebugGetStat(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadGetExtUsbUserStorageSize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadGetUserStorageSize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadPauseTaskAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadQueryTask(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadRegisterTaskByStorage(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadRestartTask(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadResumeTaskAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadStartTaskAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadStopTaskAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntDownloadUnregisterTaskAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntExtUsbCheckActiveDownloadTasks(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntGetTaskInfoValueInt(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntGetTaskInfoValueString(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntGetTaskInfoValueStringIndex(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntPlayGoFindTaskId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntPlayGoGetDiscIniChunksInstallProgress(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntPlayGoIsPaused(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntPlayGoResume(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntTerm(void) {
    return 0;
}

int APS5_VABI sceBgftServiceIntUploadGetProgress(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntUploadQueryTask(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntUploadRegisterTask(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntUploadRestartTask(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntUploadStartTask(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntUploadStartTaskAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntUploadStopTask(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntUploadStopTaskAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntUploadUnregisterTask(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBgftServiceIntUploadUnregisterTaskAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
