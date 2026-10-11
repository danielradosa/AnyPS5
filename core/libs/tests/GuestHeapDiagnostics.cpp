#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

extern "C" {
void APS5_VABI sceLibcInternalBacktraceForGame_nid_postfix(const char*);
void APS5_VABI sceLibcInternalHeapErrorReportForGame_nid_postfix(void*, void*, std::uint32_t);
}

namespace {

constexpr std::size_t HeapNameOffset = 0x3b0;
constexpr std::size_t HeapSize = 0x520;

void Require(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "guest heap diagnostics: %s\n", message);
    std::abort();
}

std::string ExecutableName() {
#ifdef _WIN32
    char path[MAX_PATH] = "";
    Require(GetModuleFileNameA(nullptr, path, sizeof(path)) != 0, "GetModuleFileNameA failed");
    std::string name = path;
    const auto separator = name.find_last_of("\\/");
#else
    Dl_info self{};
    Require(dladdr(reinterpret_cast<void*>(&Require), &self) != 0 && self.dli_fname != nullptr, "dladdr of the test failed");
    std::string name = self.dli_fname;
    const auto separator = name.find_last_of('/');
#endif
    if (separator != std::string::npos) name.erase(0, separator + 1);
    return name;
}

template<typename Call>
std::string Capture(Call call) {
    auto* captured = std::tmpfile();
    Require(captured != nullptr, "tmpfile failed");
    std::fflush(stderr);
#ifdef _WIN32
    const int saved = ::_dup(::_fileno(stderr));
    Require(saved >= 0 && ::_dup2(::_fileno(captured), ::_fileno(stderr)) == 0, "stderr redirection failed");
#else
    const int saved = ::dup(::fileno(stderr));
    Require(saved >= 0 && ::dup2(::fileno(captured), ::fileno(stderr)) == ::fileno(stderr), "stderr redirection failed");
#endif
    call();
    std::fflush(stderr);
#ifdef _WIN32
    Require(::_dup2(saved, ::_fileno(stderr)) == 0 && ::_close(saved) == 0, "stderr restore failed");
#else
    Require(::dup2(saved, ::fileno(stderr)) == ::fileno(stderr) && ::close(saved) == 0, "stderr restore failed");
#endif
    std::rewind(captured);
    std::string text;
    char buffer[4096];
    for (std::size_t length; (length = std::fread(buffer, 1, sizeof(buffer), captured)) != 0;) text.append(buffer, length);
    Require(std::fclose(captured) == 0, "tmpfile close failed");
    return text;
}

bool Contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

void RequireCallerFrame(const std::string& text, const std::string& name, const std::string& executable) {
    const auto header = "[" + name + "] backtrace:\n";
    const auto start = text.find(header);
    Require(start != std::string::npos, "backtrace header missing");
    const auto first = "[" + name + "]   #0 ";
    const auto line = text.find(first, start + header.size());
    Require(line != std::string::npos, "no frame after the backtrace header");
    const auto end = text.find('\n', line);
    Require(end != std::string::npos, "frame line is not terminated");
    Require(Contains(text.substr(line, end - line), executable + "+0x"), "first frame is not in the calling module");
}

[[gnu::noinline]] void CheckBacktrace(const std::string& executable) {
    const auto named = Capture([] { sceLibcInternalBacktraceForGame_nid_postfix("TestHeap"); });
    RequireCallerFrame(named, "TestHeap", executable);
    const auto unnamed = Capture([] { sceLibcInternalBacktraceForGame_nid_postfix(nullptr); });
    RequireCallerFrame(unnamed, "(null)", executable);
}

[[gnu::noinline]] void CheckHeapErrorReport(const std::string& executable) {
    alignas(16) static unsigned char heap[HeapSize];
    std::memset(heap, 'Z', sizeof(heap));
    std::memcpy(heap + HeapNameOffset, "GuestTestHeap", sizeof("GuestTestHeap"));
    auto* block = reinterpret_cast<void*>(std::uintptr_t{0x12345678abcd0});
    const auto named = Capture([&] { sceLibcInternalHeapErrorReportForGame_nid_postfix(heap, block, 7); });
    Require(Contains(named, "[GuestTestHeap] heap error 7 on block 0x12345678abcd0\n"), "named report lacks the name, the error or the block");
    RequireCallerFrame(named, "GuestTestHeap", executable);

    std::memset(heap + HeapNameOffset, 'N', 32);
    const auto unterminated = Capture([&] { sceLibcInternalHeapErrorReportForGame_nid_postfix(heap, block, 3); });
    Require(Contains(unterminated, "[" + std::string(32, 'N') + "] heap error 3 on block"), "name was not cut at 32 bytes");
    Require(!Contains(unterminated, "NZ"), "name was read past its 32 bytes");

    const auto unnamed = Capture([&] { sceLibcInternalHeapErrorReportForGame_nid_postfix(nullptr, block, 2); });
    Require(Contains(unnamed, "[(null)] heap error 2 on block 0x12345678abcd0\n"), "report without a heap lacks the error or the block");
    RequireCallerFrame(unnamed, "(null)", executable);
}

}

int main() {
    const auto executable = ExecutableName();
    CheckBacktrace(executable);
    CheckHeapErrorReport(executable);
}
