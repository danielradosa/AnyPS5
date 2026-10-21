#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" int APS5_VABI sceNpPartyGetState(std::uint16_t* state);
extern "C" int APS5_VABI sceNpPartyGetStateAsUserA(int asUserId, std::uint16_t* state);
extern "C" int APS5_VABI sceNpPartyGetMemberInfoA(std::uint16_t memberId, void* memberInfo);
extern "C" int APS5_VABI sceNpPartyGetMembersA(void* memberList);

int main() {
    std::uint16_t states[3];
    std::memset(states, 0xa5, sizeof(states));
    if (sceNpPartyGetState(nullptr) != static_cast<int>(0x80552504u)) {
        std::fprintf(stderr, "a null state was not refused with INVALID_ARGUMENT\n");
        return 1;
    }
    if (sceNpPartyGetState(&states[1]) != 0 || states[1] != 2) {
        std::fprintf(stderr, "the state is not NOT_IN_PARTY\n");
        return 1;
    }
    if (states[0] != 0xa5a5 || states[2] != 0xa5a5) {
        std::fprintf(stderr, "sceNpPartyGetState wrote past its 16-bit state\n");
        return 1;
    }
    std::uint16_t userState = 0xa5a5;
    if (sceNpPartyGetStateAsUserA(0x10000000, nullptr) != static_cast<int>(0x80552504u)) {
        std::fprintf(stderr, "sceNpPartyGetStateAsUserA accepted a null state\n");
        return 1;
    }
    if (sceNpPartyGetStateAsUserA(0x10000000, &userState) != 0 || userState != 2) {
        std::fprintf(stderr, "the user's state is not NOT_IN_PARTY\n");
        return 1;
    }
    unsigned char memberData[64];
    std::memset(memberData, 0x5a, sizeof(memberData));
    if (sceNpPartyGetMemberInfoA(1, memberData) != static_cast<int>(0x80552506u) || sceNpPartyGetMembersA(memberData) != static_cast<int>(0x80552506u)) {
        std::fprintf(stderr, "the member queries did not report NOT_IN_PARTY\n");
        return 1;
    }
    for (const unsigned char byte : memberData) {
        if (byte != 0x5a) {
            std::fprintf(stderr, "a member query wrote its output outside a party\n");
            return 1;
        }
    }
    std::puts("NpParty state tests passed");
    return 0;
}
