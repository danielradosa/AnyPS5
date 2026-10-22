#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/AppMetadata/include/AppMetadata.hpp"

struct AppInstallProgressInfo;

extern "C" {

int APS5_VABI sceAppInstUtilInitialize(void) {
    return 0;
}
int APS5_VABI sceAppInstUtilTerminate(void) {
    return 0;
}
int APS5_VABI sceAppInstUtilAppInstallAll(void* reserved) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppInstallTitleDir(const char* titleId, const char* titleDir, void* reserved) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppIsInInstalling(int32_t* installing) {
    if (installing == nullptr) APS5_INVALID_ARG_EX;
    *installing = 0;
    return 0;
}
int APS5_VABI sceAppInstUtilAppIsInUpdating(const char* titleId, bool* updating) {
    if (titleId == nullptr || updating == nullptr) APS5_INVALID_ARG_EX;
    if (std::string_view(titleId) != GetAppTitleId_nid_postfix().value) {
        throw std::invalid_argument(std::string(__func__) + ": no installed application has this title id");
    }
    *updating = false;
    return 0;
}
int APS5_VABI sceAppInstUtilAppExists(const char* titleId, int32_t* exists) {
    if (titleId == nullptr || exists == nullptr) APS5_INVALID_ARG_EX;
    *exists = std::string_view(titleId) == GetAppTitleId_nid_postfix().value ? 1 : 0;
    return 0;
}
int APS5_VABI sceAppInstUtilAppGetSize(const char* titleId, std::uint64_t* size) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppInstallPkg(const char* packagePath, std::uint64_t serviceArgument) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppPrepareOverwritePkg() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppRecoverApp() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppUnInstall(const char* titleId) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppUnInstallAddcont(const char* titleId, const char* contentLabel) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppUnInstallPat(const char* titleId) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppUnInstallTheme() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilAppUnInstallTypes(const char* titleId, std::uint32_t types) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilGetInstallProgressInfo(const char* titleId, AppInstallProgressInfo* info) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilGetInstallProgress(const char* titleId, void* progress) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceAppInstUtilGetPrimaryAppSlot(const char* titleId, void* slot) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
