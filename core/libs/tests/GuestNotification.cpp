#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <vector>

extern "C" {
int APS5_VABI sceNotificationSend(int type, std::uint8_t flag, std::uint8_t* message);
int APS5_VABI sceNotificationSendById(int type, std::uint8_t flag, std::uint8_t* id, std::uint8_t* jsonData);
int APS5_VABI sceNotificationShowPsButtonPersistentBanner(std::uint8_t* jsonParam);
void APS5_VABI sceNotificationHidePsButtonPersistentBanner();
}

namespace {

constexpr int BadMessage = static_cast<int>(0x81980001u);

void Require(bool value) {
    if (!value) std::abort();
}

std::uint8_t* Bytes(const char* text) {
    return reinterpret_cast<std::uint8_t*>(const_cast<char*>(text));
}

template <typename TException, typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const TException&) {
        return true;
    }
    return false;
}

}

int main() {
    Require(sceNotificationSend(0xFE, 0, Bytes("{\"rawData\":{\"viewTemplateType\":\"InteractiveToastTemplateB\"}}")) == 0);
    Require(sceNotificationSend(0xFE, 1, nullptr) == BadMessage);
    Require(sceNotificationSend(0xFE, 1, Bytes("")) == BadMessage);
    std::vector<std::uint8_t> unterminated(0x1800, 'a');
    Require(Throws<std::runtime_error>([&] { sceNotificationSend(0xFE, 0, unterminated.data()); }));
    std::vector<std::uint8_t> longest(0x1800, 'a');
    longest.back() = 0;
    Require(sceNotificationSend(0xFE, 0, longest.data()) == 0);

    Require(sceNotificationSendById(0xFE, 0, Bytes("guest-notification"), Bytes("{}")) == 0);
    Require(sceNotificationSendById(0xFE, 0, Bytes("guest-notification"), nullptr) == BadMessage);
    Require(sceNotificationSendById(0xFE, 0, Bytes("guest-notification"), Bytes("")) == BadMessage);
    Require(Throws<std::invalid_argument>([] { sceNotificationSendById(0xFE, 0, nullptr, Bytes("{}")); }));
    std::vector<std::uint8_t> longId(0x20, 'i');
    Require(Throws<std::runtime_error>([&] { sceNotificationSendById(0xFE, 0, longId.data(), Bytes("{}")); }));

    sceNotificationHidePsButtonPersistentBanner();
    Require(sceNotificationShowPsButtonPersistentBanner(nullptr) == 0);
    Require(sceNotificationShowPsButtonPersistentBanner(Bytes("{\"text\":\"guest\"}")) == 0);
    sceNotificationHidePsButtonPersistentBanner();
    sceNotificationHidePsButtonPersistentBanner();
    return 0;
}
