#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
extern "C" int APS5_VABI sceSystemServiceGetAppIdOfRunningBigApp(void);
extern "C" int APS5_VABI sceSystemServiceKillApp(int, int, int, int);
extern "C" int APS5_VABI sceLncUtilKillApp(std::uint32_t);
namespace {
bool cleaned = false;
bool finished = false;
void Cleanup() {
    try { LibcAwaitExit_nid_postfix(); }
    catch (const ProcessShutdown&) { return; }
    catch (const std::runtime_error&) { cleaned = true; }
}
void VerifyExit() {
    if (!cleaned) std::_Exit(1);
    std::puts("Guest shutdown and atexit completed");
}
void UnexpectedShutdown() {
    if (!finished) std::_Exit(3);
}
void Require(bool value) { if (!value) std::abort(); }
bool Rejects(int appId, int how, int reason, int coreDump, const char* expected) {
    try { sceSystemServiceKillApp(appId, how, reason, coreDump); }
    catch (const std::runtime_error& error) { return std::strstr(error.what(), expected) != nullptr; }
    return false;
}
bool LncRejects(std::uint32_t appId) {
    try { sceLncUtilKillApp(appId); }
    catch (const std::runtime_error& error) { return std::strstr(error.what(), "is not the running title's") != nullptr; }
    return false;
}
}
int main(int argc, char** argv) {
    const int appId = sceSystemServiceGetAppIdOfRunningBigApp();
    Require(appId > 0 && sceSystemServiceGetAppIdOfRunningBigApp() == appId);
    if (argc > 1 && std::strcmp(argv[1], "--kill") == 0) {
        LibcRegisterShutdown_nid_postfix(Cleanup);
        Require(std::atexit(VerifyExit) == 0);
        sceSystemServiceKillApp(appId, -1, 0, 0);
        std::_Exit(4);
    }
    if (argc > 1 && std::strcmp(argv[1], "--lnc-kill") == 0) {
        LibcRegisterShutdown_nid_postfix(Cleanup);
        Require(std::atexit(VerifyExit) == 0);
        sceLncUtilKillApp(static_cast<std::uint32_t>(appId));
        std::_Exit(4);
    }
    LibcRegisterShutdown_nid_postfix(UnexpectedShutdown);
    Require(std::atexit(UnexpectedShutdown) == 0);
    constexpr int noAppInfo = static_cast<int>(0x80A10009u);
    constexpr char otherArguments[] = "sceSystemServiceKillApp: arguments other than -1, 0 and 0";
    Require(sceSystemServiceKillApp(appId + 1, -1, 0, 0) == noAppInfo);
    Require(sceSystemServiceKillApp(-1, -1, 0, 0) == noAppInfo);
    Require(sceSystemServiceKillApp(0, -1, 0, 0) == noAppInfo);
    Require(sceSystemServiceKillApp(appId + 1, 0, 1, 1) == noAppInfo);
    Require(Rejects(appId, 0, 0, 0, otherArguments));
    Require(Rejects(appId, -2, 0, 0, otherArguments));
    Require(Rejects(appId, -1, 1, 0, otherArguments));
    Require(Rejects(appId, -1, 0, 1, otherArguments));
    Require(LncRejects(static_cast<std::uint32_t>(appId) + 1));
    Require(LncRejects(0));
    Require(LncRejects(0xFFFFFFFFu));
    finished = true;
}
