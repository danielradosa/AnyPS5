#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

extern "C" {
int APS5_VABI sceNetInit_nid_postfix(void);
int APS5_VABI sceNetSocket(const char*, int, int, int);
int APS5_VABI sceNetBind_nid_postfix(int, const void*, std::uint32_t);
int APS5_VABI sceNetListen(int, int);
int APS5_VABI sceNetGetsockname(int, void*, std::uint32_t*);
int APS5_VABI sceNetConnect(int, const void*, std::uint32_t);
int APS5_VABI sceNetAccept(int, void*, std::uint32_t*);
int APS5_VABI sceNetSocketClose(int);
int APS5_VABI sceNetShowIfconfig(void);
int APS5_VABI sceNetShowNetstat(void);
int APS5_VABI sceNetShowIfconfigWithMemory(int);
int APS5_VABI sceNetPoolCreate(const char*, int, int);
int APS5_VABI sceNetPoolDestroy(int);
int* APS5_VABI sceNetErrnoLoc(void);
}

namespace {

void Require(bool condition, int line) {
    if (!condition) {
        std::fprintf(stdout, "net show check failed at line %d\n", line);
        std::fflush(stdout);
        std::abort();
    }
}
#define CHECK(condition) Require((condition), __LINE__)

int Duplicate(int descriptor) {
#ifdef _WIN32
    return _dup(descriptor);
#else
    return dup(descriptor);
#endif
}

int Redirect(int from, int to) {
#ifdef _WIN32
    return _dup2(from, to);
#else
    return dup2(from, to);
#endif
}

void Close(int descriptor) {
#ifdef _WIN32
    _close(descriptor);
#else
    close(descriptor);
#endif
}

int Descriptor(std::FILE* file) {
#ifdef _WIN32
    return _fileno(file);
#else
    return fileno(file);
#endif
}

std::string Capture(int (APS5_VABI* show)(void)) {
    const auto path = std::filesystem::temp_directory_path() /
        ("anyps5-net-show-" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ".txt");
    std::FILE* file = std::fopen(path.string().c_str(), "w+b");
    CHECK(file != nullptr);
    std::fflush(stderr);
    const int saved = Duplicate(2);
    CHECK(saved >= 0);
    CHECK(Redirect(Descriptor(file), 2) >= 0);
    const int result = show();
    std::fflush(stderr);
    CHECK(Redirect(saved, 2) >= 0);
    Close(saved);
    CHECK(result == 0);
    std::rewind(file);
    std::string text;
    std::array<char, 512> chunk{};
    for (std::size_t read; (read = std::fread(chunk.data(), 1, chunk.size(), file)) != 0;) text.append(chunk.data(), read);
    std::fclose(file);
    std::filesystem::remove(path);
    return text;
}

int g_pool = -1;

int APS5_VABI ShowIfconfigInPool(void) {
    return sceNetShowIfconfigWithMemory(g_pool);
}

std::array<std::uint8_t, 16> Loopback(std::uint16_t port) {
    std::array<std::uint8_t, 16> address{};
    address[0] = 16;
    address[1] = 2;
    address[2] = static_cast<std::uint8_t>(port >> 8);
    address[3] = static_cast<std::uint8_t>(port);
    address[4] = 127;
    address[7] = 1;
    return address;
}

std::uint16_t Port(int socket) {
    std::array<std::uint8_t, 16> address{};
    std::uint32_t length = address.size();
    CHECK(sceNetGetsockname(socket, address.data(), &length) == 0 && length == 16);
    return static_cast<std::uint16_t>((address[2] << 8) | address[3]);
}

std::string Line(const char* protocol, const std::string& local, const std::string& foreign, const char* state) {
    char line[160];
    std::snprintf(line, sizeof(line), "%-5s %6s %6s %-22s %-22s %s", protocol, "0", "0", local.c_str(), foreign.c_str(), state);
    std::string text = line;
    while (!text.empty() && text.back() == ' ') text.pop_back();
    return "\n" + text + "\n";
}

}

int main() {
    CHECK(sceNetInit_nid_postfix() == 0);
    const auto ifconfig = Capture(sceNetShowIfconfig);
    CHECK(ifconfig.find("lo0: flags=8049<UP,LOOPBACK,RUNNING,MULTICAST> metric 0 mtu 16384\n") == 0);
    CHECK(ifconfig.find("\tinet 127.0.0.1 netmask 0xff000000\n") != std::string::npos);
    CHECK(ifconfig.find("\tinet6 ::1 prefixlen 128\n") != std::string::npos);
    CHECK(ifconfig.find("eth0: flags=8843<UP,BROADCAST,RUNNING,SIMPLEX,MULTICAST> metric 0 mtu 1500\n") != std::string::npos);
    CHECK(ifconfig.find("\tether 02:50:53:35:00:01\n") != std::string::npos);
    CHECK(ifconfig.find("\tstatus: active\n") != std::string::npos || ifconfig.find("\tstatus: no carrier\n") != std::string::npos);
    CHECK(ifconfig.find("inet 127.0.0.1\n") == std::string::npos);
    g_pool = sceNetPoolCreate("net-show", 16 * 1024, 0);
    CHECK(g_pool >= 0);
    const auto pooled = Capture(ShowIfconfigInPool);
    CHECK(pooled.find("lo0: flags=8049<UP,LOOPBACK,RUNNING,MULTICAST> metric 0 mtu 16384\n") == 0);
    CHECK(pooled.find("\tether 02:50:53:35:00:01\n") != std::string::npos);
    CHECK(sceNetPoolDestroy(g_pool) == 0);
    CHECK(sceNetShowIfconfigWithMemory(g_pool) == static_cast<int>(0x80410109) && *sceNetErrnoLoc() == 9);

    const int listener = sceNetSocket("listener", 2, 1, 0);
    if (listener < 0) return 77;
    const auto any = Loopback(0);
    CHECK(sceNetBind_nid_postfix(listener, any.data(), any.size()) == 0);
    CHECK(sceNetListen(listener, 1) == 0);
    const auto listenerPort = Port(listener);
    const int client = sceNetSocket("client", 2, 1, 0);
    CHECK(client >= 0);
    const auto target = Loopback(listenerPort);
    CHECK(sceNetConnect(client, target.data(), target.size()) == 0);
    std::array<std::uint8_t, 16> peer{};
    std::uint32_t peerLength = peer.size();
    const int accepted = sceNetAccept(listener, peer.data(), &peerLength);
    CHECK(accepted >= 0);
    const auto clientPort = Port(client);
    const int datagram = sceNetSocket("datagram", 2, 2, 0);
    CHECK(datagram >= 0);
    CHECK(sceNetBind_nid_postfix(datagram, any.data(), any.size()) == 0);
    const auto datagramPort = Port(datagram);

    const auto listening = "127.0.0.1." + std::to_string(listenerPort);
    const auto connecting = "127.0.0.1." + std::to_string(clientPort);
    const auto bound = "127.0.0.1." + std::to_string(datagramPort);
    const auto netstat = Capture(sceNetShowNetstat);
    CHECK(netstat.find("Active Internet connections (including servers)\n"
        "Proto Recv-Q Send-Q Local Address          Foreign Address        (state)\n") == 0);
    CHECK(netstat.find(Line("tcp4", listening, "*.*", "LISTEN")) != std::string::npos);
    CHECK(netstat.find(Line("tcp4", connecting, listening, "ESTABLISHED")) != std::string::npos);
    CHECK(netstat.find(Line("tcp4", listening, connecting, "ESTABLISHED")) != std::string::npos);
    CHECK(netstat.find(Line("udp4", bound, "*.*", "")) != std::string::npos);

    CHECK(sceNetSocketClose(datagram) == 0);
    CHECK(sceNetSocketClose(accepted) == 0);
    CHECK(sceNetSocketClose(client) == 0);
    CHECK(sceNetSocketClose(listener) == 0);
    const auto closed = Capture(sceNetShowNetstat);
    CHECK(closed.find(listening) == std::string::npos && closed.find(bound) == std::string::npos);
    CHECK(closed.find("Active Internet connections (including servers)\n") == 0);
    return 0;
}
