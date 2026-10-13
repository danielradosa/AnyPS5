#include "prx/libc/include/exceptions/Runtime.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <typeinfo>
#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#endif

using TerminateHandler = void (*)();

extern "C" {
[[noreturn]] void APS5_VABI __cxa_call_unexpected_nid_postfix(void* exception);
std::type_info* APS5_VABI __cxa_current_exception_type_nid_postfix();
TerminateHandler APS5_VABI _ZSt13set_terminatePFvvE_nid_postfix(TerminateHandler handler);
}

static constexpr int TerminateCalled = 61;
static constexpr int TrappedWhileCaught = 62;
static constexpr int TrappedUncaught = 63;
static constexpr int HandlerRejected = 64;

static void RequireAt(bool value, int line) {
    if (value) return;
    std::fprintf(stderr, "GuestAmprUnexpected.cpp:%d: requirement failed\n", line);
    std::abort();
}

#define Require(value) RequireAt((value), __LINE__)

[[noreturn]] static void ExitWith(int code) {
#ifdef _WIN32
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(code));
#endif
    _exit(code);
}

static void OnTerminate() {
    ExitWith(TerminateCalled);
}

static int TrapResult() {
    return __cxa_current_exception_type_nid_postfix() == &typeid(int) ? TrappedWhileCaught : TrappedUncaught;
}

#ifdef _WIN32
static LONG CALLBACK OnTrap(EXCEPTION_POINTERS* info) {
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION) return EXCEPTION_CONTINUE_SEARCH;
    ExitWith(TrapResult());
}
#else
static void OnTrap(int) {
    ExitWith(TrapResult());
}
#endif

static _Unwind_Exception* NativeException() {
    void* object = __cxa_allocate_exception_nid_postfix(sizeof(int));
    *static_cast<int*>(object) = 7;
    auto* header = LibcException::FromObject(object);
    header->type = const_cast<std::type_info*>(&typeid(int));
    header->unwind.exception_class = LibcException::PrimaryClass;
    return &header->unwind;
}

static int RunChild(const char* mode) {
    _ZSt13set_terminatePFvvE_nid_postfix(OnTerminate);
#ifdef _WIN32
    Require(AddVectoredExceptionHandler(1, OnTrap) != nullptr);
#else
    std::signal(SIGILL, OnTrap);
#endif
    auto* exception = NativeException();
    if (std::strcmp(mode, "handler") == 0) {
        LibcException::FromUnwind(exception)->unexpected = OnTerminate;
        try {
            __cxa_call_unexpected_nid_postfix(exception);
        } catch (const std::runtime_error&) {
            return HandlerRejected;
        }
    }
    __cxa_call_unexpected_nid_postfix(exception);
}

static int RunChildProcess(const char* mode) {
#ifdef _WIN32
    std::wstring executable(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    Require(length > 0 && length < executable.size());
    executable.resize(length);
    std::wstring command = L"\"" + executable + L"\" " + std::wstring(mode, mode + std::strlen(mode));
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    Require(CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process));
    Require(WaitForSingleObject(process.hProcess, 10000) == WAIT_OBJECT_0);
    DWORD code = 0;
    Require(GetExitCodeProcess(process.hProcess, &code));
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return static_cast<int>(code);
#else
    const pid_t child = fork();
    Require(child >= 0);
    if (child == 0) _exit(RunChild(mode));
    int status = 0;
    Require(waitpid(child, &status, 0) == child);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    if (argc == 2) return RunChild(argv[1]);
    Require(RunChildProcess("default") == TrappedWhileCaught);
    Require(RunChildProcess("handler") == HandlerRejected);
    std::puts("__cxa_call_unexpected default handler and installed handler: PASS");
    return 0;
}
