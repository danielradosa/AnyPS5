#include "SceTypes.hpp"
#include "prx/libSceSystemService/SystemService.hpp"
#include <cstdint>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string_view>

extern "C" int APS5_VABI sceLncUtilEnableSuspendNotification();
extern "C" int APS5_VABI sceLncUtilDisableSuspendNotification();
extern "C" int APS5_VABI sceSystemServiceEnableSuspendNotification();
extern "C" int APS5_VABI sceSystemServiceDisableSuspendNotification();
extern "C" int APS5_VABI sceShellCoreUtilGetRemotePlayStatus(bool* active);
extern "C" int APS5_VABI sceSystemServiceGetRemoteplayStatus(bool* active);
extern "C" bool APS5_VABI sceShellCoreUtilIsGameLiveStreamingOnAir();
extern "C" bool APS5_VABI sceSystemServiceIsGameLiveStreamingOnAir();
extern "C" bool APS5_VABI sceLncUtilIsShellUiFgAndGameBgCpuMode();
extern "C" bool APS5_VABI sceSystemServiceIsShellUiFgAndGameBgCpuMode();
extern "C" bool APS5_VABI sceShellCoreUtilIsImposeScreenOverlaid();
extern "C" bool APS5_VABI sceShellCoreUtilIsScreenSaverOn();
extern "C" bool APS5_VABI sceSystemServiceIsScreenSaverOn();
extern "C" bool APS5_VABI sceSystemStateMgrIsShellUIShutdownInProgress();
extern "C" bool APS5_VABI sceSystemStateMgrIsBdDriveReady();
extern "C" bool APS5_VABI sceShellCoreUtilIsUsbMassStorageMounted(std::uint32_t deviceIndex);
extern "C" bool APS5_VABI sceShellCoreUtilIsExternalStorageAppMoveInProgress();

namespace {

int failures = 0;

template<typename TCall>
void Case(const char* name, TCall call) {
    try {
        if (call()) return;
        std::printf("%s: wrong answer\n", name);
    } catch (const std::exception& error) {
        std::printf("%s: %s\n", name, error.what());
    }
    ++failures;
}

template<typename TCall>
bool Unimplemented(TCall call, std::string_view reason) {
    try {
        static_cast<void>(call());
    } catch (const std::runtime_error& error) {
        return std::string_view(error.what()).find(reason) != std::string_view::npos;
    }
    return false;
}

void TestSuspend() {
    Case("sceLncUtilEnableSuspendNotification", [] {
        return sceLncUtilEnableSuspendNotification() == SYSTEM_SERVICE_OK && sceLncUtilEnableSuspendNotification() == SYSTEM_SERVICE_OK;
    });
    Case("sceLncUtilDisableSuspendNotification", [] {
        return sceLncUtilDisableSuspendNotification() == SYSTEM_SERVICE_OK && sceLncUtilDisableSuspendNotification() == SYSTEM_SERVICE_OK;
    });
    Case("sceSystemServiceEnableSuspendNotification", [] {
        return sceSystemServiceEnableSuspendNotification() == sceLncUtilEnableSuspendNotification();
    });
    Case("sceSystemServiceDisableSuspendNotification", [] {
        return sceSystemServiceDisableSuspendNotification() == sceLncUtilDisableSuspendNotification();
    });
}

template<typename TCall>
bool ReportsInactive(TCall call) {
    bool active = true;
    return call(&active) == SYSTEM_SERVICE_OK && !active;
}

void TestSessions() {
    Case("sceShellCoreUtilGetRemotePlayStatus", [] { return ReportsInactive(sceShellCoreUtilGetRemotePlayStatus); });
    Case("sceShellCoreUtilGetRemotePlayStatus null output", [] {
        return Unimplemented([] { return sceShellCoreUtilGetRemotePlayStatus(nullptr); }, "null output");
    });
    Case("sceSystemServiceGetRemoteplayStatus", [] {
        return ReportsInactive(sceSystemServiceGetRemoteplayStatus) && sceSystemServiceGetRemoteplayStatus(nullptr) == SYSTEM_SERVICE_ERROR_PARAMETER;
    });
    Case("sceShellCoreUtilIsGameLiveStreamingOnAir", [] { return !sceShellCoreUtilIsGameLiveStreamingOnAir(); });
    Case("sceSystemServiceIsGameLiveStreamingOnAir", [] {
        return !sceSystemServiceIsGameLiveStreamingOnAir() && sceSystemServiceIsGameLiveStreamingOnAir() == sceShellCoreUtilIsGameLiveStreamingOnAir();
    });
}

void TestShell() {
    Case("sceLncUtilIsShellUiFgAndGameBgCpuMode", [] { return !sceLncUtilIsShellUiFgAndGameBgCpuMode(); });
    Case("sceSystemServiceIsShellUiFgAndGameBgCpuMode", [] {
        return !sceSystemServiceIsShellUiFgAndGameBgCpuMode() && sceSystemServiceIsShellUiFgAndGameBgCpuMode() == sceLncUtilIsShellUiFgAndGameBgCpuMode();
    });
    Case("sceShellCoreUtilIsImposeScreenOverlaid", [] { return !sceShellCoreUtilIsImposeScreenOverlaid(); });
    Case("sceShellCoreUtilIsScreenSaverOn", [] { return !sceShellCoreUtilIsScreenSaverOn(); });
    Case("sceSystemServiceIsScreenSaverOn", [] {
        return !sceSystemServiceIsScreenSaverOn() && sceSystemServiceIsScreenSaverOn() == sceShellCoreUtilIsScreenSaverOn();
    });
    Case("sceSystemStateMgrIsShellUIShutdownInProgress", [] { return !sceSystemStateMgrIsShellUIShutdownInProgress(); });
}

void TestDevices() {
    Case("sceSystemStateMgrIsBdDriveReady", [] { return !sceSystemStateMgrIsBdDriveReady(); });
    Case("sceShellCoreUtilIsUsbMassStorageMounted", [] {
        for (std::uint32_t device = 0; device < 8; ++device) {
            if (sceShellCoreUtilIsUsbMassStorageMounted(device)) return false;
        }
        return true;
    });
    Case("sceShellCoreUtilIsExternalStorageAppMoveInProgress", [] { return !sceShellCoreUtilIsExternalStorageAppMoveInProgress(); });
}

}

int main(int argc, char** argv) {
    const std::string_view mode = argc > 1 ? argv[1] : "";
    if (mode == "suspend") TestSuspend();
    else if (mode == "sessions") TestSessions();
    else if (mode == "shell") TestShell();
    else if (mode == "devices") TestDevices();
    else return 2;
    if (failures == 0) std::printf("all cases passed\n");
    return failures == 0 ? 0 : 1;
}
