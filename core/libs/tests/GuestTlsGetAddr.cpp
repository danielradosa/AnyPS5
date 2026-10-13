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
#include <tlhelp32.h>
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
static IMAGE_TLS_DIRECTORY64* ImageTls(std::uint8_t* base) {
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    const auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    if (directory.VirtualAddress == 0 || directory.Size < sizeof(IMAGE_TLS_DIRECTORY64)) return nullptr;
    return reinterpret_cast<IMAGE_TLS_DIRECTORY64*>(base + directory.VirtualAddress);
}

static std::size_t CheckOtherModules(const IMAGE_TLS_DIRECTORY64* own, const std::uint8_t* mine) {
    const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    Require(snapshot != INVALID_HANDLE_VALUE);
    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    std::size_t checked = 0;
    for (auto more = Module32FirstW(snapshot, &entry); more; more = Module32NextW(snapshot, &entry)) {
        const auto* tls = ImageTls(entry.modBaseAddr);
        if (tls == nullptr || tls == own || tls->AddressOfIndex == 0 || tls->EndAddressOfRawData == tls->StartAddressOfRawData) continue;
        GuestTlsIndex index{tls->AddressOfIndex, 0};
        const auto* block = static_cast<const std::uint8_t*>(__tls_get_addr_nid_postfix(&index));
        Require(block != nullptr && block != mine && block != reinterpret_cast<const std::uint8_t*>(tls->StartAddressOfRawData));
        ++checked;
    }
    Require(CloseHandle(snapshot) != 0);
    return checked;
}

static void CheckThreadBlocks() {
    auto* tls = ImageTls(reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr)));
    Require(tls != nullptr);
    auto* raw = reinterpret_cast<std::uint8_t*>(tls->StartAddressOfRawData);
    Require(tls->EndAddressOfRawData - tls->StartAddressOfRawData >= 8);
    std::uint8_t original[8];
    std::memcpy(original, raw, sizeof(original));
    GuestTlsIndex first{tls->AddressOfIndex, 0};
    auto* mine = static_cast<std::uint8_t*>(__tls_get_addr_nid_postfix(&first));
    Require(mine != raw && std::memcmp(mine, original, sizeof(original)) == 0);
    Require(CheckOtherModules(tls, mine) > 0);
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

struct OtherModules {
    std::uint64_t own = 0;
    std::size_t checked = 0;
};

static std::size_t CheckOtherModules(std::uint64_t own) {
    OtherModules modules{own};
    dl_iterate_phdr([](dl_phdr_info* info, std::size_t size, void* data) {
        auto& modules = *static_cast<OtherModules*>(data);
        if (size < offsetof(dl_phdr_info, dlpi_tls_data) + sizeof(info->dlpi_tls_data)) return 0;
        if (info->dlpi_tls_modid == 0 || info->dlpi_tls_modid == modules.own || info->dlpi_tls_data == nullptr) return 0;
        GuestTlsIndex index{info->dlpi_tls_modid, 0};
        Require(__tls_get_addr_nid_postfix(&index) == info->dlpi_tls_data);
        ++modules.checked;
        return 0;
    }, &modules);
    return modules.checked;
}

static void CheckThreadBlocks() {
    const auto image = FindProbeImage();
    GuestTlsIndex index{image.module, reinterpret_cast<std::uintptr_t>(&probe) - image.block};
    Require(__tls_get_addr_nid_postfix(&index) == &probe);
    Require(CheckOtherModules(image.module) > 0);
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
