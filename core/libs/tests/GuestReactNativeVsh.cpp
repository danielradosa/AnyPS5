#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <initializer_list>

extern "C" {
int APS5_VABI RemotePlayGetConnectionStatus(int, int*);
int APS5_VABI ShareGetCurrentStatus(std::uint32_t, ShareCurrentStatus*);
int APS5_VABI ShareInitialize(std::size_t, int, std::uint64_t);
int APS5_VABI ShareTerminate(void);
}

static void Require(bool value) { if (!value) std::abort(); }

int main() {
    for (int user : {-1, 0, 1, 0x10000}) {
        int status = 0x5a5a5a5a;
        Require(RemotePlayGetConnectionStatus(user, &status) == 0);
        Require(status == 0);
    }
    bool rejected = false;
    try {
        RemotePlayGetConnectionStatus(1, nullptr);
    } catch (const std::exception&) {
        rejected = true;
    }
    Require(rejected);

    constexpr int shareInvalidParam = static_cast<int>(0x81960002u);
    Require(ShareInitialize(128 * 1024, 700, 0) == 0);
    ShareCurrentStatus status;
    std::memset(&status, 0xFF, sizeof(status));
    Require(ShareGetCurrentStatus(0, &status) == shareInvalidParam);
    Require(ShareGetCurrentStatus(1, nullptr) == shareInvalidParam);
    Require(ShareGetCurrentStatus(1, &status) == 0);
    const auto* bytes = reinterpret_cast<const unsigned char*>(&status);
    for (std::size_t i = 0; i < sizeof(status); ++i) Require(bytes[i] == 0);
    Require(ShareTerminate() == 0);
}
