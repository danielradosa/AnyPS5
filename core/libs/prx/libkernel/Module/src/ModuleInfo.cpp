#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/KernelErrors.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#else
#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#endif

extern "C" std::int32_t ModuleIdForImage_nid_no_patch(const void* native);

namespace {

#ifndef _WIN32
constexpr char GuestModuleSuffix[] = ".guest.prx";
constexpr std::int32_t ProtRead = 1;
constexpr std::int32_t ProtWrite = 2;
constexpr std::int32_t ProtExecute = 4;

template<typename TInfo>
struct ImageSearch {
    std::uintptr_t address;
    TInfo* info;
    bool found;
};

bool Contains(const dl_phdr_info& image, std::uintptr_t begin, std::uint64_t size) {
    for (std::uint16_t i = 0; i < image.dlpi_phnum; ++i) {
        const auto& header = image.dlpi_phdr[i];
        if (header.p_type != PT_LOAD) continue;
        const std::uintptr_t start = image.dlpi_addr + header.p_vaddr;
        if (begin >= start && begin - start <= header.p_memsz && size <= header.p_memsz - (begin - start)) return true;
    }
    return false;
}

std::uint32_t ToU32(std::uint64_t value, const char* field) {
    if (value > std::numeric_limits<std::uint32_t>::max())
        throw std::runtime_error(std::string("module info: ") + field + " exceeds 32 bits");
    return static_cast<std::uint32_t>(value);
}

std::uintptr_t EhFrameAddress(const dl_phdr_info& image, std::uintptr_t header) {
    if (!Contains(image, header, 8))
        throw std::runtime_error("sceKernelGetModuleInfoFromAddr: eh_frame_hdr outside the image");
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(header);
    if (bytes[0] != 1) throw std::runtime_error("sceKernelGetModuleInfoFromAddr: unsupported eh_frame_hdr version");
    const std::uintptr_t field = header + 4;
    switch (bytes[1]) {
    case 0x1b: {
        std::int32_t offset;
        std::memcpy(&offset, bytes + 4, sizeof(offset));
        return field + static_cast<std::intptr_t>(offset);
    }
    case 0x03: {
        std::uint32_t value;
        std::memcpy(&value, bytes + 4, sizeof(value));
        return value;
    }
    case 0x00: {
        std::uint64_t value;
        if (!Contains(image, header, 12))
            throw std::runtime_error("sceKernelGetModuleInfoFromAddr: eh_frame_hdr outside the image");
        std::memcpy(&value, bytes + 4, sizeof(value));
        return static_cast<std::uintptr_t>(value);
    }
    default:
        throw std::runtime_error("sceKernelGetModuleInfoFromAddr: unsupported eh_frame_ptr encoding");
    }
}

std::uint64_t EhFrameSize(const dl_phdr_info& image, std::uintptr_t frame) {
    std::uintptr_t position = frame;
    for (;;) {
        if (!Contains(image, position, 4))
            throw std::runtime_error("sceKernelGetModuleInfoFromAddr: eh_frame outside the image");
        std::uint32_t length;
        std::memcpy(&length, reinterpret_cast<const void*>(position), sizeof(length));
        position += 4;
        if (length == 0) return position - frame;
        std::uint64_t recordLength = length;
        if (length == 0xffffffffu) {
            if (!Contains(image, position, 8))
                throw std::runtime_error("sceKernelGetModuleInfoFromAddr: eh_frame outside the image");
            std::memcpy(&recordLength, reinterpret_cast<const void*>(position), sizeof(recordLength));
            position += 8;
        }
        if (!Contains(image, position, recordLength))
            throw std::runtime_error("sceKernelGetModuleInfoFromAddr: eh_frame record outside the image");
        position += recordLength;
    }
}

std::string ImageName(const dl_phdr_info& image) {
    std::string path = image.dlpi_name ? image.dlpi_name : "";
    if (path.empty()) {
        char executable[4096];
        const auto length = ::readlink("/proc/self/exe", executable, sizeof(executable) - 1);
        if (length < 0) throw std::runtime_error("sceKernelGetModuleInfoFromAddr: cannot resolve the executable path");
        path.assign(executable, static_cast<std::size_t>(length));
    }
    std::string name = path.substr(path.find_last_of('/') + 1);
    if (name.size() > sizeof(GuestModuleSuffix) - 1 && name.ends_with(GuestModuleSuffix))
        name.resize(name.size() - (sizeof(GuestModuleSuffix) - 1));
    return name;
}

template<typename TInfo>
void FillNameAndSegments(const dl_phdr_info& image, TInfo& info) {
    const auto name = ImageName(image);
    if (name.size() >= sizeof(info.name))
        throw std::runtime_error("module info: module name too long: " + name);
    std::memcpy(info.name, name.c_str(), name.size() + 1);
    for (std::uint16_t i = 0; i < image.dlpi_phnum; ++i) {
        const auto& header = image.dlpi_phdr[i];
        if (header.p_type != PT_LOAD || info.segment_count >= std::size(info.segments)) continue;
        auto& segment = info.segments[info.segment_count++];
        segment.address = image.dlpi_addr + header.p_vaddr;
        segment.size = ToU32(header.p_memsz, "segment size");
        segment.prot = ((header.p_flags & PF_R) ? ProtRead : 0) | ((header.p_flags & PF_W) ? ProtWrite : 0) | ((header.p_flags & PF_X) ? ProtExecute : 0);
    }
}

void Fill(const dl_phdr_info& image, ModuleInfo& info) {
    FillNameAndSegments(image, info);
}

void Fill(const dl_phdr_info& image, ModuleInfoEx& info) {
    FillNameAndSegments(image, info);
    info.tls_index = ToU32(image.dlpi_tls_modid, "TLS module index");
    for (std::uint16_t i = 0; i < image.dlpi_phnum; ++i) {
        const auto& header = image.dlpi_phdr[i];
        const std::uintptr_t address = image.dlpi_addr + header.p_vaddr;
        if (header.p_type == PT_TLS) {
            info.tls_init_addr = address;
            info.tls_init_size = ToU32(header.p_filesz, "TLS image size");
            info.tls_size = ToU32(header.p_memsz, "TLS size");
            info.tls_align = ToU32(header.p_align, "TLS alignment");
        } else if (header.p_type == PT_GNU_EH_FRAME) {
            info.eh_frame_hdr_addr = address;
            info.eh_frame_hdr_size = ToU32(header.p_memsz, "eh_frame_hdr size");
            info.eh_frame_addr = EhFrameAddress(image, address);
            info.eh_frame_size = ToU32(EhFrameSize(image, info.eh_frame_addr), "eh_frame size");
        } else if (header.p_type == PT_DYNAMIC) {
            for (const auto* entry = reinterpret_cast<const ElfW(Dyn)*>(address); entry->d_tag != DT_NULL; ++entry) {
                if (entry->d_tag == DT_INIT) info.init_proc_addr = image.dlpi_addr + entry->d_un.d_ptr;
                else if (entry->d_tag == DT_FINI) info.fini_proc_addr = image.dlpi_addr + entry->d_un.d_ptr;
            }
        }
    }
    info.ref_count = 1;
}

template<typename TInfo>
int FindImage(dl_phdr_info* image, std::size_t, void* data) {
    auto& search = *static_cast<ImageSearch<TInfo>*>(data);
    if (!Contains(*image, search.address, 1)) return 0;
    Fill(*image, *search.info);
    search.found = true;
    return 1;
}

int CollectGuestImage(dl_phdr_info* image, std::size_t, void* data) {
    const std::string_view name = image->dlpi_name ? image->dlpi_name : "";
    if (!name.empty() && !name.ends_with(".prx")) return 0;
    for (std::uint16_t i = 0; i < image->dlpi_phnum; ++i) {
        const auto& header = image->dlpi_phdr[i];
        if (header.p_type != PT_LOAD) continue;
        static_cast<std::vector<std::uintptr_t>*>(data)->push_back(image->dlpi_addr + header.p_vaddr);
        return 0;
    }
    return 0;
}
#endif

struct GuestImage {
    const void* native;
    std::uintptr_t address;
};

std::vector<GuestImage> GuestImages() {
    std::vector<GuestImage> images;
#ifdef _WIN32
    const HANDLE process = GetCurrentProcess();
    std::vector<HMODULE> modules(64);
    for (;;) {
        DWORD needed = 0;
        const auto size = static_cast<DWORD>(modules.size() * sizeof(HMODULE));
        if (!EnumProcessModules(process, modules.data(), size, &needed))
            throw std::runtime_error("sceKernelGetModuleList: EnumProcessModules failed with error " + std::to_string(GetLastError()));
        if (needed <= size) {
            modules.resize(needed / sizeof(HMODULE));
            break;
        }
        modules.resize(needed / sizeof(HMODULE));
    }
    const HMODULE executable = GetModuleHandleW(nullptr);
    for (const auto module : modules) {
        if (module != executable) {
            std::vector<wchar_t> path(MAX_PATH);
            DWORD length = 0;
            for (;;) {
                length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
                if (length == 0)
                    throw std::runtime_error("sceKernelGetModuleList: GetModuleFileNameW failed with error " + std::to_string(GetLastError()));
                if (length < path.size()) break;
                path.resize(path.size() * 2);
            }
            const std::wstring_view name(path.data(), length);
            constexpr std::wstring_view suffix = L".prx";
            if (name.size() < suffix.size() || CompareStringOrdinal(name.data() + name.size() - suffix.size(), static_cast<int>(suffix.size()), suffix.data(), static_cast<int>(suffix.size()), TRUE) != CSTR_EQUAL)
                continue;
        }
        images.push_back({module, reinterpret_cast<std::uintptr_t>(module)});
    }
#else
    std::vector<std::uintptr_t> addresses;
    dl_iterate_phdr(CollectGuestImage, &addresses);
    for (const auto address : addresses) {
        Dl_info symbol{};
        link_map* native = nullptr;
        if (!dladdr1(reinterpret_cast<const void*>(address), &symbol, reinterpret_cast<void**>(&native), RTLD_DL_LINKMAP) || !native)
            throw std::runtime_error("sceKernelGetModuleList: no link map for a loaded image");
        images.push_back({native, address});
    }
#endif
    return images;
}

}

extern "C" {

int APS5_VABI sceKernelGetModuleInfoFromAddr(std::uint64_t address, int flags, ModuleInfoEx* info) {
    if (!info) return SCE_KERNEL_ERROR_EFAULT;
    if (flags != 2) throw std::invalid_argument("sceKernelGetModuleInfoFromAddr: unsupported flags " + std::to_string(flags));
    if (info->st_size != sizeof(ModuleInfoEx))
        throw std::invalid_argument("sceKernelGetModuleInfoFromAddr: unsupported st_size " + std::to_string(info->st_size));
#ifdef _WIN32
    (void)address;
    NotImplemented_nid_no_patch(__func__);
    return 0;
#else
    Dl_info symbol{};
    link_map* native = nullptr;
    if (!dladdr1(reinterpret_cast<const void*>(address), &symbol, reinterpret_cast<void**>(&native), RTLD_DL_LINKMAP) || !native)
        return SCE_KERNEL_ERROR_ESRCH;
    ModuleInfoEx result{};
    result.st_size = sizeof(ModuleInfoEx);
    ImageSearch<ModuleInfoEx> search{static_cast<std::uintptr_t>(address), &result, false};
    dl_iterate_phdr(FindImage<ModuleInfoEx>, &search);
    if (!search.found) return SCE_KERNEL_ERROR_ESRCH;
    result.id = ModuleIdForImage_nid_no_patch(native);
    *info = result;
    return 0;
#endif
}

int APS5_VABI sceKernelGetModuleList(KernelModule* handles, std::size_t count, std::size_t* actual) {
    if (!handles || !actual) return SCE_KERNEL_ERROR_EFAULT;
    const auto images = GuestImages();
    for (std::size_t i = 0; i < images.size() && i < count; ++i) handles[i] = ModuleIdForImage_nid_no_patch(images[i].native);
    if (images.size() > count) return SCE_KERNEL_ERROR_ENOMEM;
    *actual = images.size();
    return 0;
}

int APS5_VABI sceKernelGetModuleInfo(KernelModule handle, ModuleInfo* info) {
    if (!info) return SCE_KERNEL_ERROR_EFAULT;
    if (info->st_size != sizeof(ModuleInfo)) return SCE_KERNEL_ERROR_EINVAL;
    for (const auto& image : GuestImages()) {
        if (ModuleIdForImage_nid_no_patch(image.native) != handle) continue;
#ifdef _WIN32
        NotImplemented_nid_no_patch(__func__);
        return 0;
#else
        ModuleInfo result{};
        result.st_size = sizeof(ModuleInfo);
        ImageSearch<ModuleInfo> search{image.address, &result, false};
        dl_iterate_phdr(FindImage<ModuleInfo>, &search);
        if (!search.found) return SCE_KERNEL_ERROR_ESRCH;
        *info = result;
        return 0;
#endif
    }
    return SCE_KERNEL_ERROR_ESRCH;
}

}
