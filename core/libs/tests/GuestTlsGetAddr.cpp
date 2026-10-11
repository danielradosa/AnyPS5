#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <link.h>
#endif

struct GuestTlsIndex {
    std::uint64_t module;
    std::uint64_t offset;
};

extern "C" {
void* APS5_VABI __tls_get_addr_nid_postfix(GuestTlsIndex*);
}

static void Require(bool value) { if (!value) std::abort(); }

#ifdef _WIN32
static IMAGE_TLS_DIRECTORY64* ImageTls() {
    auto* base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    const auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    Require(directory.VirtualAddress != 0 && directory.Size >= sizeof(IMAGE_TLS_DIRECTORY64));
    return reinterpret_cast<IMAGE_TLS_DIRECTORY64*>(base + directory.VirtualAddress);
}

static void CheckThreadBlocks() {
    auto* tls = ImageTls();
    auto* raw = reinterpret_cast<std::uint8_t*>(tls->StartAddressOfRawData);
    Require(tls->EndAddressOfRawData - tls->StartAddressOfRawData >= 8);
    std::uint8_t original[8];
    std::memcpy(original, raw, sizeof(original));
    GuestTlsIndex first{tls->AddressOfIndex, 0};
    auto* mine = static_cast<std::uint8_t*>(__tls_get_addr_nid_postfix(&first));
    Require(mine != raw && std::memcmp(mine, original, sizeof(original)) == 0);
    GuestTlsIndex last{tls->AddressOfIndex, 7};
    Require(static_cast<std::uint8_t*>(__tls_get_addr_nid_postfix(&last)) == mine + 7);
    const std::uint8_t pattern[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    std::memcpy(raw, pattern, sizeof(pattern));
    std::uint8_t seen[8]{};
    std::uintptr_t theirs = 0;
    std::thread([&] {
        for (std::uint64_t offset = 0; offset < sizeof(seen); ++offset) {
            GuestTlsIndex index{tls->AddressOfIndex, offset};
            seen[offset] = *static_cast<std::uint8_t*>(__tls_get_addr_nid_postfix(&index));
        }
        auto* block = static_cast<std::uint8_t*>(__tls_get_addr_nid_postfix(&first));
        theirs = reinterpret_cast<std::uintptr_t>(block);
        block[0] = 0x99;
    }).join();
    std::memcpy(raw, original, sizeof(original));
    Require(std::memcmp(seen, pattern, sizeof(pattern)) == 0);
    Require(theirs != reinterpret_cast<std::uintptr_t>(mine) && std::memcmp(mine, original, sizeof(original)) == 0);
}
#else
thread_local std::uint64_t probe = 0x5a17c0de2b0b5e11ull;

struct ProbeImage {
    std::uint64_t module = 0;
    std::uintptr_t block = 0;
};

static ProbeImage FindProbeImage() {
    ProbeImage found;
    dl_iterate_phdr([](dl_phdr_info* info, std::size_t size, void* data) {
        if (size < offsetof(dl_phdr_info, dlpi_tls_data) + sizeof(info->dlpi_tls_data)) return 0;
        const auto address = reinterpret_cast<std::uintptr_t>(&probe);
        const auto block = reinterpret_cast<std::uintptr_t>(info->dlpi_tls_data);
        for (std::uint16_t header = 0; header < info->dlpi_phnum; ++header) {
            const auto& tls = info->dlpi_phdr[header];
            if (tls.p_type != PT_TLS || block == 0 || address < block || address - block >= tls.p_memsz) continue;
            *static_cast<ProbeImage*>(data) = {info->dlpi_tls_modid, block};
            return 1;
        }
        return 0;
    }, &found);
    Require(found.module != 0);
    return found;
}

static void CheckThreadBlocks() {
    const auto image = FindProbeImage();
    GuestTlsIndex index{image.module, reinterpret_cast<std::uintptr_t>(&probe) - image.block};
    Require(__tls_get_addr_nid_postfix(&index) == &probe);
    probe = 1;
    std::uintptr_t theirs = 0;
    std::uint64_t seen = 0;
    bool own = false;
    std::thread([&] {
        auto* value = static_cast<std::uint64_t*>(__tls_get_addr_nid_postfix(&index));
        theirs = reinterpret_cast<std::uintptr_t>(value);
        seen = *value;
        own = value == &probe;
    }).join();
    Require(own && seen == 0x5a17c0de2b0b5e11ull && probe == 1 && theirs != reinterpret_cast<std::uintptr_t>(&probe));
}
#endif

int main() {
    CheckThreadBlocks();
    bool threw = false;
    try { __tls_get_addr_nid_postfix(nullptr); } catch (const std::invalid_argument&) { threw = true; }
    Require(threw);
}
