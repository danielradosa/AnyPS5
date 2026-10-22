#include "prx/libc/include/general/VabiMacros.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI execvp_nid_postfix(const char*, char* const*);
int APS5_VABI execv_nid_postfix(const char*, char* const*);
int APS5_VABI execve_nid_postfix(const char*, char* const*, char* const*);
int APS5_VABI open_nid_postfix(const char*, int, int);
int APS5_VABI close_nid_postfix(int);
int APS5_VABI mkdir_nid_postfix(const char*, unsigned short);
std::int64_t APS5_VABI write_nid_postfix(int, const char*, std::int64_t);
int* APS5_VABI __error_nid_postfix();
extern char** environ_nid_postfix;
}

static constexpr int Enoent = 2;
static constexpr int E2big = 7;
static constexpr int Enoexec = 8;
static constexpr int Eacces = 13;
static constexpr int Efault = 14;
static constexpr int Enametoolong = 63;
static constexpr int WriteOnly = 0x0001;
static constexpr int Create = 0x0200;

static void Require(bool value) { if (!value) std::abort(); }

static int ExecvError(const char* path, char* const* arguments) {
    *__error_nid_postfix() = 0;
    Require(execv_nid_postfix(path, arguments) == -1);
    return *__error_nid_postfix();
}

static int ExecveError(const char* path, char* const* arguments, char* const* environment) {
    *__error_nid_postfix() = 0;
    Require(execve_nid_postfix(path, arguments, environment) == -1);
    return *__error_nid_postfix();
}

static int ExecvpError(const char* file, char* const* arguments) {
    *__error_nid_postfix() = 0;
    Require(execvp_nid_postfix(file, arguments) == -1);
    return *__error_nid_postfix();
}

static void CreateFile(const std::string& path, int mode) {
    const int file = open_nid_postfix(path.c_str(), WriteOnly | Create, mode);
    Require(file >= 0);
    Require(write_nid_postfix(file, "data", 4) == 4);
    Require(close_nid_postfix(file) == 0);
}

int main() {
    char shell[] = "sh";
    char option[] = "-c";
    char command[] = "exit 0";
    char* arguments[] = {shell, option, command, nullptr};
    Require(ExecvpError("sh", arguments) == Enoent);
    Require(ExecvpError("/bin/sh", arguments) == Enoent);
    Require(ExecvpError("", arguments) == Enoent);
    Require(ExecvError("/bin/sh", arguments) == Enoent);
    Require(ExecvError("", arguments) == Enoent);
    Require(ExecvError("/bin/sh", nullptr) == Efault);
    Require(ExecvError(nullptr, nullptr) == Efault);
    bool threw = false;
    try {
        execvp_nid_postfix(nullptr, arguments);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw);
    char variable[] = "HOME=/";
    char* replacement[] = {variable, nullptr};
    Require(ExecveError("/bin/sh", arguments, replacement) == Enoent);
    Require(ExecveError("/bin/sh", arguments, nullptr) == Enoent);
    Require(ExecveError("/bin/sh", nullptr, replacement) == Efault);
    const std::string longPath(1024, 'a');
    Require(ExecvError(longPath.c_str(), arguments) == Enametoolong);
    std::string huge(300000, 'x');
    char* oversized[] = {huge.data(), nullptr};
    Require(ExecvError("/bin/sh", oversized) == E2big);
    Require(ExecvpError("/bin/sh", oversized) == E2big);
    Require(ExecveError("/bin/sh", arguments, oversized) == E2big);
    Require(ExecveError(longPath.c_str(), arguments, replacement) == Enametoolong);

    const auto directory = "anyps5-exec-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    Require(mkdir_nid_postfix(directory.c_str(), 0755) == 0);
    Require(ExecvError(directory.c_str(), arguments) == Eacces);
    const auto plain = directory + "/plain.bin";
    CreateFile(plain, 0644);
    Require(ExecvError(plain.c_str(), arguments) == Eacces);
    Require(ExecvpError(plain.c_str(), arguments) == Eacces);
    const auto tool = directory + "/tool.exe";
    CreateFile(tool, 0755);
    Require(ExecvError(tool.c_str(), arguments) == Enoexec);
    Require(ExecveError(plain.c_str(), arguments, replacement) == Eacces);
    Require(ExecveError(tool.c_str(), arguments, replacement) == Enoexec);
    Require(ExecvpError(tool.c_str(), arguments) == Enoent);

    char** saved = environ_nid_postfix;
    std::string search = "PATH=" + std::string(1100, 'p') + ":" + directory;
    char* environment[] = {search.data(), nullptr};
    environ_nid_postfix = environment;
    Require(ExecvpError("plain.bin", arguments) == Eacces);
    Require(ExecvpError("tool.exe", arguments) == Enoent);
    Require(ExecvpError("missing", arguments) == Enoent);
    Require(ExecvpError("plain.bin", nullptr) == Efault);
    environ_nid_postfix = saved;
    Require(std::filesystem::remove_all(directory) == 3);
    return 0;
}
