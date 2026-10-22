#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <sys/stat.h>
#ifndef _WIN32
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/FilesystemError.hpp"
#include "prx/libkernel/File/include/DirectoryDescriptor.hpp"
#include "prx/libc/include/CreationMask.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "prx/libkernel/File/include/File.hpp"
#include "prx/libkernel/System/include/CompiledSdkVersion.hpp"
#include "prx/libkernel/KernelErrors.hpp"
#include "prx/libkernel/Module/ModuleInfo.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <sys/stat.h>
#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#endif
#endif

extern "C" int* APS5_VABI __error_nid_postfix();
extern "C" int APS5_VABI getpid_nid_postfix();
extern "C" int APS5_VABI getlogin_r_nid_postfix(char* output, int size);

namespace {
#ifdef _WIN32
extern "C" _invalid_parameter_handler _set_thread_local_invalid_parameter_handler(_invalid_parameter_handler);
static void IgnoreInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, std::uintptr_t) {}
static int GuestFstat(int fd) {
    const auto previous = _set_thread_local_invalid_parameter_handler(IgnoreInvalidParameter);
    struct _stat64 status{};
    const int result = ::_fstat64(fd, &status);
    _set_thread_local_invalid_parameter_handler(previous);
    return result;
}
#endif

constexpr int GuestEnoent = 2;
constexpr int GuestEperm = 1;
constexpr int GuestEio = 5;
constexpr int GuestEacces = 13;
constexpr int GuestEbadf = 9;
constexpr int GuestEfault = 14;
constexpr int GuestEnotdir = 20;
constexpr int GuestEisdir = 21;
constexpr int GuestEinval = 22;
constexpr int GuestEexist = 17;
constexpr int GuestExdev = 18;
constexpr int GuestEdeadlk = 11;
constexpr int GuestEnospc = 28;
constexpr int GuestErofs = 30;
constexpr int GuestEagain = 35;
constexpr int GuestEloop = 62;
constexpr int GuestEnametoolong = 63;
constexpr int GuestEnotempty = 66;
constexpr int GuestEnosys = 78;
constexpr int GuestEoverflow = 84;
constexpr int GuestAtFdcwd = -100;
constexpr int GuestAtRemovedir = 0x800;
constexpr std::size_t GuestPathMax = 1024;
constexpr std::size_t GuestNameMax = 255;
constexpr std::size_t GuestLoginNameMax = 33;
constexpr std::size_t GuestSignalSetWords = 4;
constexpr int GuestSigSetmask = 3;
constexpr std::size_t GuestTermiosBytes = 44;

constexpr int PcLinkMax = 1;
constexpr int PcMaxCanon = 2;
constexpr int PcMaxInput = 3;
constexpr int PcNameMax = 4;
constexpr int PcPathMax = 5;
constexpr int PcPipeBuf = 6;
constexpr int PcChownRestricted = 7;
constexpr int PcNoTrunc = 8;
constexpr int PcVdisable = 9;
constexpr int PcAllocSizeMin = 10;
constexpr int PcFileSizeBits = 12;
constexpr int PcRecIncrXferSize = 14;
constexpr int PcRecMaxXferSize = 15;
constexpr int PcRecMinXferSize = 16;
constexpr int PcRecXferAlign = 17;
constexpr int PcSymlinkMax = 18;
constexpr int PcMinHoleSize = 21;
constexpr int PcAsyncIo = 53;
constexpr int PcPrioIo = 54;
constexpr int PcSyncIo = 55;
constexpr int PcAclExtended = 59;
constexpr int PcAclPathMax = 60;
constexpr int PcMacPresent = 63;
constexpr int PcAclNfs4 = 64;
constexpr int GuestAtSymlinkNofollow = 0x0200;
constexpr int GuestAtSymlinkFollow = 0x0400;
constexpr std::uint32_t GuestOwnerUnchanged = 0xFFFFFFFFu;
constexpr std::uint16_t GuestFileType = 0170000;
constexpr std::uint16_t GuestFifo = 0010000;
constexpr std::uint16_t GuestDirectory = 0040000;
constexpr std::uint16_t GuestRegular = 0100000;
constexpr std::uint16_t GuestSocket = 0140000;
constexpr std::int64_t GuestPosixAsynchronousIo = 200112;
constexpr std::int64_t GuestPipeBuf = 512;

int GuestFileError(int error) {
    switch (error) {
    case ENOENT: return GuestEnoent;
    case EACCES: return 13;
    case ENOTDIR: return 20;
    case EINVAL: return GuestEinval;
    case EPERM: return GuestEperm;
    case EROFS: return 30;
    case ENAMETOOLONG: return GuestEnametoolong;
    case EIO: return GuestEio;
#ifdef ELOOP
    case ELOOP: return GuestEloop;
#endif
    default: return GuestEio;
    }
}

std::int64_t Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

std::int64_t PathConfigurationValue(int name, const char* function) {
    switch (name) {
    case PcLinkMax: return 32767;
    case PcNameMax: return static_cast<std::int64_t>(GuestNameMax);
    case PcPathMax: return static_cast<std::int64_t>(GuestPathMax);
    case PcPipeBuf: return 512;
    case PcChownRestricted: return 1;
    case PcNoTrunc: return 1;
    case PcFileSizeBits: return 64;
    case PcRecMaxXferSize: return -1;
    case PcSymlinkMax: return static_cast<std::int64_t>(GuestPathMax);
    case PcPrioIo: return 0;
    case PcSyncIo: return 0;
    case PcMaxCanon:
    case PcMaxInput:
    case PcVdisable:
    case PcAllocSizeMin:
    case PcRecIncrXferSize:
    case PcRecMinXferSize:
    case PcRecXferAlign:
    case PcMinHoleSize:
    case PcAsyncIo:
    case PcAclExtended:
    case PcAclPathMax:
    case PcMacPresent:
    case PcAclNfs4:
        throw std::runtime_error(std::string(function) + ": name " + std::to_string(name) + " depends on the console filesystem or kernel configuration");
    default: return Fail(GuestEinval);
    }
}

int GuestError(const std::error_code& error) {
    const auto condition = error.default_error_condition();
    if (condition == std::errc::resource_unavailable_try_again) return GuestEagain;
    if (condition == std::errc::resource_deadlock_would_occur) return GuestEdeadlk;
    if (condition == std::errc::too_many_symbolic_link_levels) return GuestEloop;
    if (condition == std::errc::filename_too_long) return GuestEnametoolong;
    if (condition == std::errc::directory_not_empty) return GuestEnotempty;
    if (condition == std::errc::value_too_large) return GuestEoverflow;
    const int value = condition.value();
    return value > 0 && value <= 34 ? value : GuestEio;
}

std::int64_t CopyLinkTarget(const std::string& target, char* buffer, std::size_t size) {
    const auto count = std::min(size, target.size());
    if (count != 0 && buffer == nullptr) return Fail(GuestEfault);
    if (count != 0) std::memcpy(buffer, target.data(), count);
    return static_cast<std::int64_t>(count);
}

std::int64_t CopyGuestLinkTarget(const std::filesystem::path& host, char* buffer, std::size_t size) {
    const auto guest = GuestPathFromHost_nid_no_patch(host);
    if (guest.empty()) throw std::runtime_error("readlink: the link target " + host.string() + " is outside the guest filesystem");
    return CopyLinkTarget(guest, buffer, size);
}

#ifdef _WIN32
struct ReparseHeader {
    ULONG tag;
    USHORT dataLength;
    USHORT reserved;
    USHORT substituteNameOffset;
    USHORT substituteNameLength;
    USHORT printNameOffset;
    USHORT printNameLength;
};
constexpr ULONG SymbolicLinkRelative = 1;
using ReparseData = std::array<std::byte, MAXIMUM_REPARSE_DATA_BUFFER_SIZE>;

enum class ReparseLink { None, Relative, Absolute };

std::wstring ReparseName(const ReparseData& data, DWORD size, std::size_t names, USHORT offset, USHORT length) {
    if (names + offset + length > size) throw std::runtime_error("readlink: malformed link reparse data");
    std::wstring name(length / sizeof(wchar_t), L'\0');
    std::memcpy(name.data(), data.data() + names + offset, name.size() * sizeof(wchar_t));
    return name;
}

ReparseLink ReadReparseLink(const std::filesystem::path& native, std::filesystem::path& target) {
    const HANDLE handle = CreateFileW(native.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "readlink: CreateFileW");
    ReparseData data{};
    DWORD size = 0;
    const BOOL read = DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, nullptr, 0, data.data(), static_cast<DWORD>(data.size()), &size, nullptr);
    const DWORD code = GetLastError();
    CloseHandle(handle);
    if (!read) throw std::system_error(static_cast<int>(code), std::system_category(), "readlink: FSCTL_GET_REPARSE_POINT");
    ReparseHeader header{};
    if (size < sizeof(header)) return ReparseLink::None;
    std::memcpy(&header, data.data(), sizeof(header));
    std::size_t names = sizeof(header);
    bool relative = false;
    if (header.tag == IO_REPARSE_TAG_SYMLINK) {
        ULONG flags = 0;
        if (size < sizeof(header) + sizeof(flags)) throw std::runtime_error("readlink: malformed link reparse data");
        std::memcpy(&flags, data.data() + sizeof(header), sizeof(flags));
        names += sizeof(flags);
        relative = (flags & SymbolicLinkRelative) != 0;
    } else if (header.tag != IO_REPARSE_TAG_MOUNT_POINT) {
        return ReparseLink::None;
    }
    auto name = ReparseName(data, size, names, header.substituteNameOffset, header.substituteNameLength);
    if (relative) {
        target = std::filesystem::path(name);
        return ReparseLink::Relative;
    }
    auto printName = ReparseName(data, size, names, header.printNameOffset, header.printNameLength);
    if (!printName.empty()) name = std::move(printName);
    else if (name.starts_with(L"\\??\\")) name.erase(0, 4);
    target = std::filesystem::path(name);
    return ReparseLink::Absolute;
}
#endif

bool HasNonDirectoryPrefix(const std::filesystem::path& native) {
    std::error_code error;
    for (auto prefix = native.parent_path(); prefix.has_relative_path(); prefix = prefix.parent_path()) {
        const auto status = std::filesystem::status(prefix, error);
        if (std::filesystem::exists(status)) return !std::filesystem::is_directory(status);
    }
    return false;
}

bool IsNativeLink(const std::filesystem::path& native) {
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesW(native.wstring().c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    std::error_code error;
    return std::filesystem::is_symlink(std::filesystem::symlink_status(native, error));
#endif
}

#ifdef _WIN32
bool NativeLinkResolves(const std::filesystem::path& native) {
    const HANDLE handle = CreateFileW(native.wstring().c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
        return true;
    }
    const DWORD code = GetLastError();
    return code != ERROR_FILE_NOT_FOUND && code != ERROR_PATH_NOT_FOUND && code != ERROR_CANT_RESOLVE_FILENAME;
}
#endif

std::int64_t ReadNativeLink(const std::filesystem::path& native, char* buffer, std::size_t size) {
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesW(native.wstring().c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
        std::filesystem::path target;
        const auto link = ReadReparseLink(native, target);
        if (link == ReparseLink::None) return Fail(GuestEinval);
        if (link == ReparseLink::Relative) return CopyLinkTarget(target.generic_string(), buffer, size);
        return CopyGuestLinkTarget(target, buffer, size);
    }
#endif
    std::error_code error;
    const auto status = std::filesystem::symlink_status(native, error);
    if (error)
        return Fail(error == std::errc::no_such_file_or_directory && HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestError(error));
    if (status.type() == std::filesystem::file_type::not_found)
        return Fail(HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestEnoent);
    if (!std::filesystem::is_symlink(status)) return Fail(GuestEinval);
    const auto target = std::filesystem::read_symlink(native, error);
    if (error) return Fail(GuestError(error));
    if (target.is_absolute()) return CopyGuestLinkTarget(target, buffer, size);
    return CopyLinkTarget(target.generic_string(), buffer, size);
}

bool HasLongComponent(std::string_view path) {
    for (std::size_t start = 0; start < path.size();) {
        const auto end = std::min(path.find('/', start), path.size());
        if (end - start > GuestNameMax) {
            if (start == 0) return true;
            std::filesystem::path prefix;
            if (ResolvePathError_nid_no_patch(std::string(path.substr(0, start)).c_str(), prefix)) return false;
            std::error_code error;
            return std::filesystem::is_directory(prefix, error);
        }
        start = end + 1;
    }
    return false;
}

bool IsDotOrDotDot(std::string_view path) {
    while (!path.empty() && (path.back() == '/' || path.back() == '\\')) path.remove_suffix(1);
    const auto separator = path.find_last_of("/\\");
    const auto component = path.substr(separator == std::string_view::npos ? 0 : separator + 1);
    return component == "." || component == "..";
}

int ResolveAtPath(int fd, const char* path, std::filesystem::path& native) {
    if (fd == GuestAtFdcwd || *path == '/') return ResolvePathError_nid_no_patch(path, native);
    int error = 0;
    const char* base = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
    if (base == nullptr) return error;
    native = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) / std::filesystem::path(reinterpret_cast<const char8_t*>(path));
    return 0;
}

int LinkPaths(const std::filesystem::path& source, const std::filesystem::path& destination, bool follow) {
    std::error_code error;
    if (!follow && IsNativeLink(source))
        throw std::runtime_error("linkat: a hard link to the symbolic link " + source.string() + " itself is not supported");
    const auto status = std::filesystem::status(source, error);
    if (!std::filesystem::exists(status)) return HasNonDirectoryPrefix(source) ? GuestEnotdir : GuestEnoent;
    if (std::filesystem::is_directory(status)) return GuestEperm;
    if (std::filesystem::exists(std::filesystem::symlink_status(destination, error))) return GuestEexist;
    if (!std::filesystem::is_directory(destination.parent_path(), error)) return HasNonDirectoryPrefix(destination) ? GuestEnotdir : GuestEnoent;
    const auto target = std::filesystem::is_symlink(source, error) ? std::filesystem::canonical(source, error) : source;
    if (!error) std::filesystem::create_hard_link(target, destination, error);
    if (error == std::errc::cross_device_link) return GuestExdev;
    if (error) throw std::system_error(error, "link");
    RecordWrittenPath_nid_no_patch(destination);
    return 0;
}

int ChangeOwnerAt(int fd, const char* path, std::uint32_t uid, std::uint32_t gid, bool follow, const char* function) {
    if (path == nullptr) return GuestEfault;
    if (*path == '\0') return GuestEnoent;
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return GuestEnametoolong;
    std::filesystem::path native;
    if (const int error = ResolveAtPath(fd, path, native)) return error;
    std::error_code error;
    const auto status = follow ? std::filesystem::status(native, error) : std::filesystem::symlink_status(native, error);
    bool present = status.type() != std::filesystem::file_type::not_found;
#ifdef _WIN32
    if (IsNativeLink(native)) {
        present = !follow || NativeLinkResolves(native);
        error.clear();
    }
#endif
    if (!present) return HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestEnoent;
    if (error) return GuestError(error);
    if ((uid != 0 && uid != GuestOwnerUnchanged) || (gid != 0 && gid != GuestOwnerUnchanged)) {
        throw std::runtime_error(std::string(function) + ": giving " + path + " owner " + std::to_string(uid) + " and group " + std::to_string(gid) +
            " is not supported, the host keeps no guest ownership");
    }
    return 0;
}

int RenamePath(const std::filesystem::path& source, const std::filesystem::path& destination) {
    namespace fs = std::filesystem;
    std::error_code error;
    const auto sourceStatus = fs::symlink_status(source, error);
    if (error && error != std::errc::no_such_file_or_directory) return FilesystemError(error);
    if (sourceStatus.type() == fs::file_type::not_found)
        return HasNonDirectoryPrefix(source) ? GuestEnotdir : GuestEnoent;

    const auto parent = destination.parent_path().empty() ? fs::path(".") : destination.parent_path();
    error.clear();
    const auto parentStatus = fs::status(parent, error);
    if (error) return FilesystemError(error);
    if (!fs::exists(parentStatus))
        return HasNonDirectoryPrefix(destination) ? GuestEnotdir : GuestEnoent;
    if (!fs::is_directory(parentStatus)) return GuestEnotdir;

    error.clear();
    const auto destinationStatus = fs::symlink_status(destination, error);
    if (error && error != std::errc::no_such_file_or_directory) return FilesystemError(error);
    const bool destinationExists = destinationStatus.type() != fs::file_type::not_found;
    const bool sourceIsDirectory = fs::is_directory(sourceStatus);
    const bool destinationIsDirectory = destinationExists && fs::is_directory(destinationStatus);
    bool sameFile = false;
    if (destinationExists) {
        error.clear();
        sameFile = fs::equivalent(source, destination, error);
        if (error == std::errc::no_such_file_or_directory) error.clear();
        if (error) return FilesystemError(error);
        if (!sameFile && sourceIsDirectory && !destinationIsDirectory) return GuestEnotdir;
        if (!sameFile && !sourceIsDirectory && destinationIsDirectory) return GuestEisdir;
    }
    if (sourceIsDirectory && !sameFile) {
        error.clear();
        const auto canonicalSource = fs::weakly_canonical(source, error);
        if (error) return FilesystemError(error);
        const auto canonicalDestination = fs::weakly_canonical(destination, error);
        if (error) return FilesystemError(error);
        const auto relative = canonicalDestination.lexically_relative(canonicalSource);
        if (!relative.empty() && *relative.begin() != "..") return GuestEinval;
    }
    if (destinationExists && !sameFile && destinationIsDirectory) {
        error.clear();
        if (!fs::is_empty(destination, error)) {
            return error ? FilesystemError(error) : GuestEnotempty;
        }
    }
#ifdef _WIN32
    if (!sameFile && sourceIsDirectory && destinationIsDirectory) {
        error.clear();
        if (!fs::remove(destination, error)) return error ? FilesystemError(error) : GuestEio;
    }
#endif

    error.clear();
    fs::rename(source, destination, error);
    return error ? FilesystemError(error) : 0;
}
}

extern "C" {

int APS5_VABI fork_nid_postfix(void) {
    return static_cast<int>(Fail(GuestEnosys));
}

int APS5_VABI vfork_nid_postfix(void) {
    return static_cast<int>(Fail(GuestEnosys));
}

char* APS5_VABI getlogin_nid_postfix(void) {
    static std::mutex lock;
    static char name[GuestLoginNameMax];
    static bool valid = false;
    const std::lock_guard guard(lock);
    if (!valid) {
        if (const int error = getlogin_r_nid_postfix(name, sizeof(name))) {
            *__error_nid_postfix() = error;
            return nullptr;
        }
        valid = true;
    }
    return name[0] != '\0' ? name : nullptr;
}

int APS5_VABI setsid_nid_postfix(void) {
    return static_cast<int>(Fail(GuestEperm));
}

std::uint16_t APS5_VABI umask_nid_postfix(std::uint16_t mask) {
    return File::ExchangeCreationMask_nid_no_patch(mask);
}

std::int64_t APS5_VABI pwrite_nid_postfix(int d, const void* buf, std::size_t nbytes, std::int64_t offset) {
    return PosixPwrite_nid_no_patch(d, buf, nbytes, offset);
}

int APS5_VABI flock_nid_postfix(int d, int operation);

int APS5_VABI sceKernelFlock(int d, int operation) {
    return flock_nid_postfix(d, operation) == 0 ? 0 : SceKernelError(*__error_nid_postfix());
}

int APS5_VABI _fstatfs_nid_postfix(int d, void* buf);

int SetLogin_nid_no_patch(const char* name);

int APS5_VABI setlogin_nid_postfix(const char* name) {
    if (const int error = SetLogin_nid_no_patch(name)) return static_cast<int>(Fail(error));
    return 0;
}

int APS5_VABI fstatfs_nid_postfix(int d, void* buf) {
    return _fstatfs_nid_postfix(d, buf);
}

std::int64_t APS5_VABI _fstat_nid_postfix(int d, FileStat* sb);

int APS5_VABI fstat_nid_postfix(int d, FileStat* sb) {
    return static_cast<int>(_fstat_nid_postfix(d, sb));
}

int APS5_VABI sceKernelGetCompiledSdkVersion(std::int32_t* version) {
    return CompiledSdkVersion_nid_no_patch(version);
}

std::int64_t APS5_VABI readlink_nid_postfix(const char* path, char* buffer, std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) return Fail(GuestEinval);
    if (path == nullptr) return Fail(GuestEfault);
    if (*path == '\0') return Fail(GuestEnoent);
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return Fail(GuestEnametoolong);
    std::filesystem::path native;
    if (const int error = ResolvePathError_nid_no_patch(path, native)) return Fail(error);
    return ReadNativeLink(native, buffer, size);
}

std::int64_t APS5_VABI readlinkat_nid_postfix(int fd, const char* path, char* buffer, std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) return Fail(GuestEinval);
    if (path == nullptr) return Fail(GuestEfault);
    if (*path == '\0') return Fail(GuestEnoent);
    if (fd == GuestAtFdcwd || *path == '/') return readlink_nid_postfix(path, buffer, size);
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return Fail(GuestEnametoolong);
    int error = 0;
    const char* base = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
    if (base == nullptr) return Fail(error);
    return ReadNativeLink(std::filesystem::path(reinterpret_cast<const char8_t*>(base)) /
                          std::filesystem::path(reinterpret_cast<const char8_t*>(path)), buffer, size);
}

std::int64_t APS5_VABI pathconf_nid_postfix(const char* path, int name) {
    if (path == nullptr) return Fail(GuestEfault);
    if (*path == '\0') return Fail(GuestEnoent);
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return Fail(GuestEnametoolong);
    std::filesystem::path native;
    if (const int error = ResolvePathError_nid_no_patch(path, native)) return Fail(error);
    std::error_code error;
    const auto status = std::filesystem::status(native, error);
    if (error) return Fail(error == std::errc::no_such_file_or_directory && HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestError(error));
    if (!std::filesystem::exists(status)) return Fail(HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestEnoent);
    return PathConfigurationValue(name, "pathconf");
}

std::int64_t APS5_VABI _fpathconf_nid_postfix(int fd, int name) {
    FileStat status{};
    if (_fstat_nid_postfix(fd, &status) != 0) return -1;
    if (name == PcAsyncIo) return GuestPosixAsynchronousIo;
    const auto type = status.st_mode & GuestFileType;
    if (type == GuestRegular || type == GuestDirectory) return PathConfigurationValue(name, "fpathconf");
    if (type == GuestFifo || type == GuestSocket) return name == PcPipeBuf ? GuestPipeBuf : Fail(GuestEinval);
    throw std::runtime_error("fpathconf: descriptor " + std::to_string(fd) + " is not a file, a directory, a pipe or a socket, and the console's answer for its device or object is not known");
}

std::int64_t APS5_VABI fpathconf_nid_postfix(int fd, int name) {
    return _fpathconf_nid_postfix(fd, name);
}

int APS5_VABI select_nid_postfix(int nfds, void* readfds, void* writefds, void* exceptfds, const void* timeout);
int APS5_VABI sigprocmask_nid_postfix(int how, const void* set, void* previousSet);
int APS5_VABI mlockall_nid_postfix(int flags);

int APS5_VABI pselect_nid_postfix(int nfds, void* readfds, void* writefds, void* exceptfds, const KernelTimespec* timeout, const void* mask) {
    KernelTimeval limit{};
    if (timeout != nullptr) {
        limit.tv_sec = timeout->tv_sec;
        limit.tv_usec = timeout->tv_nsec / 1000;
    }
    std::array<std::uint32_t, GuestSignalSetWords> previous{};
    if (mask != nullptr && sigprocmask_nid_postfix(GuestSigSetmask, mask, previous.data()) != 0) return -1;
    const int result = select_nid_postfix(nfds, readfds, writefds, exceptfds, timeout != nullptr ? &limit : nullptr);
    if (mask != nullptr) {
        const int saved = *__error_nid_postfix();
        if (sigprocmask_nid_postfix(GuestSigSetmask, previous.data(), nullptr) != 0)
            throw std::runtime_error("pselect: restoring the signal mask failed");
        *__error_nid_postfix() = saved;
    }
    return result;
}

int APS5_VABI settimeofday_nid_postfix(const KernelTimeval* tv, const KernelTimezone* tz) {
    if (tv != nullptr && (tv->tv_usec < 0 || tv->tv_usec >= 1000000 || tv->tv_sec < 0)) return static_cast<int>(Fail(GuestEinval));
    if (tv != nullptr) {
        throw std::runtime_error("settimeofday: setting the time to " + std::to_string(tv->tv_sec) + " s " + std::to_string(tv->tv_usec) +
            " us is not supported, the host clock is not changed");
    }
    if (tz != nullptr && (tz->tz_minuteswest != 0 || tz->tz_dsttime != 0)) {
        throw std::runtime_error("settimeofday: setting the kernel time zone to " + std::to_string(tz->tz_minuteswest) + " minutes west, DST " +
            std::to_string(tz->tz_dsttime) + " is not supported, gettimeofday reports 0 and 0");
    }
    return 0;
}

int APS5_VABI sceKernelSettimeofday(const KernelTimeval* tv, const KernelTimezone* tz) {
    if (settimeofday_nid_postfix(tv, tz) == 0) return 0;
    return static_cast<int>(0x80020000u | static_cast<std::uint32_t>(*__error_nid_postfix()));
}

int APS5_VABI sceKernelGetModuleInfoInternal(KernelModule handle, ModuleInfoEx* info) {
    return ModuleInfoExForHandle_nid_no_patch(handle, info);
}

int APS5_VABI sceKernelIsInSandbox(void) {
    return 1;
}

int APS5_VABI tcgetattr_nid_postfix(int descriptor, void* attributes);
int APS5_VABI getsid_nid_postfix(int pid);

int APS5_VABI tcgetsid_nid_postfix(int descriptor) {
    std::array<std::byte, GuestTermiosBytes> attributes{};
    if (tcgetattr_nid_postfix(descriptor, attributes.data()) != 0) return -1;
    throw std::runtime_error("tcgetsid: descriptor " + std::to_string(descriptor) + " is a terminal, and terminal sessions are not modelled");
}

int APS5_VABI tcsetsid_nid_postfix(int descriptor, int pid) {
    const int saved = *__error_nid_postfix();
    const int session = getsid_nid_postfix(0);
    *__error_nid_postfix() = saved;
    if (pid != session) return static_cast<int>(Fail(GuestEinval));
    std::array<std::byte, GuestTermiosBytes> attributes{};
    if (tcgetattr_nid_postfix(descriptor, attributes.data()) != 0) return -1;
    throw std::runtime_error("tcsetsid: descriptor " + std::to_string(descriptor) + " is a terminal, and making it the controlling terminal is not modelled");
}

int APS5_VABI sceKernelMlockall(int flags) {
    if (mlockall_nid_postfix(flags) == 0) return 0;
    return static_cast<int>(0x80020000u | static_cast<std::uint32_t>(*__error_nid_postfix()));
}

int APS5_VABI link_nid_postfix(const char* existing, const char* created) {
    if (existing == nullptr || created == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*existing == '\0' || *created == '\0') return static_cast<int>(Fail(GuestEnoent));
    std::filesystem::path source, destination;
    if (const int error = ResolvePathError_nid_no_patch(existing, source)) return static_cast<int>(Fail(error));
    if (const int error = ResolvePathError_nid_no_patch(created, destination)) return static_cast<int>(Fail(error));
    if (const int error = LinkPaths(source, destination, true)) return static_cast<int>(Fail(error));
    return 0;
}

int APS5_VABI linkat_nid_postfix(int fd1, const char* existing, int fd2, const char* created, int flag) {
    if (flag & ~GuestAtSymlinkFollow) return static_cast<int>(Fail(GuestEinval));
    if (existing == nullptr || created == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*existing == '\0' || *created == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (std::strlen(existing) >= GuestPathMax || HasLongComponent(existing) ||
        std::strlen(created) >= GuestPathMax || HasLongComponent(created))
        return static_cast<int>(Fail(GuestEnametoolong));
    std::filesystem::path source, destination;
    if (const int error = ResolveAtPath(fd1, existing, source)) return static_cast<int>(Fail(error));
    if (const int error = ResolveAtPath(fd2, created, destination)) return static_cast<int>(Fail(error));
    if (const int error = LinkPaths(source, destination, (flag & GuestAtSymlinkFollow) != 0)) return static_cast<int>(Fail(error));
    return 0;
}

int APS5_VABI fchdir_nid_postfix(int fd) {
    int error = 0;
    const char* path = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
    if (path == nullptr) return static_cast<int>(Fail(error));
#ifndef _WIN32
    if (::faccessat(fd, ".", X_OK, AT_EACCESS) != 0) return static_cast<int>(Fail(errno));
#endif
    error = SetWorkingDirectory_nid_no_patch(path);
    if (error != 0) return static_cast<int>(Fail(error));
    return 0;
}

int APS5_VABI chroot_nid_postfix(const char* path) {
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return static_cast<int>(Fail(GuestEnametoolong));
    std::filesystem::path native;
    if (const int error = ResolvePathError_nid_no_patch(path, native)) return static_cast<int>(Fail(error));
    std::error_code error;
    const auto status = std::filesystem::status(native, error);
    if (status.type() == std::filesystem::file_type::not_found) return static_cast<int>(Fail(HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestEnoent));
    if (error) return static_cast<int>(Fail(GuestError(error)));
    if (!std::filesystem::is_directory(status)) return static_cast<int>(Fail(GuestEnotdir));
    const bool root = std::filesystem::equivalent(native, ResolvePath_nid_no_patch("/"), error);
    if (error) return static_cast<int>(Fail(GuestError(error)));
    if (root) return 0;
    throw std::runtime_error(std::string("chroot: changing the root directory to ") + path + " is not supported, the guest path resolver keeps a fixed root");
}

static int CreateSymlinkAt(const char* target, int dirfd, const char* created) {
    if (target == nullptr || created == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (std::strlen(target) >= GuestPathMax) return static_cast<int>(Fail(GuestEnametoolong));
    if (*target == '\0' || *created == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (std::strlen(created) >= GuestPathMax || HasLongComponent(created)) return static_cast<int>(Fail(GuestEnametoolong));
    if (*target != '/') {
        const std::filesystem::path relative(target);
        if (relative.has_root_name() || relative.has_root_directory()) {
            NotImplemented_nid_no_patch("symlink: targets rooted outside the guest tree");
            return -1;
        }
        for (const auto& part : relative) {
            if (part == "..") {
                NotImplemented_nid_no_patch("symlink: parent-directory targets");
                return -1;
            }
        }
    }
    std::filesystem::path native;
    if (std::strlen(created) >= GuestPathMax || HasLongComponent(created)) return static_cast<int>(Fail(GuestEnametoolong));
    if (dirfd == GuestAtFdcwd || *created == '/') {
        if (const int error = ResolvePathError_nid_no_patch(created, native)) return static_cast<int>(Fail(error));
    } else {
        int error = 0;
        const char* base = GetDirectoryDescriptorPath_nid_no_patch(dirfd, &error);
        if (!base) return static_cast<int>(Fail(error));
        native = std::filesystem::path(base) / std::filesystem::path(reinterpret_cast<const char8_t*>(created));
    }
    std::error_code error;
#ifdef _WIN32
    if (GetFileAttributesW(native.wstring().c_str()) != INVALID_FILE_ATTRIBUTES) return static_cast<int>(Fail(GuestEexist));
#else
    if (std::filesystem::exists(std::filesystem::symlink_status(native, error))) return static_cast<int>(Fail(GuestEexist));
#endif
    const auto parent = native.parent_path();
    const auto parentStatus = std::filesystem::status(parent, error);
    if (!std::filesystem::is_directory(parentStatus))
        return static_cast<int>(Fail(std::filesystem::exists(parentStatus) || HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestEnoent));
    std::filesystem::path contents(target);
    if (*target == '/') {
        std::filesystem::path host;
        if (ResolvePathError_nid_no_patch(target, host) != 0)
            throw std::runtime_error(std::string("symlink: the absolute target ") + target + " has no host path");
        contents = std::filesystem::absolute(host);
    }
    const bool directory = std::filesystem::is_directory(parent / contents, error);
#ifdef _WIN32
    DWORD flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
    if (directory) flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    if (!CreateSymbolicLinkW(native.wstring().c_str(), std::filesystem::path(contents).make_preferred().wstring().c_str(), flags)) {
        const DWORD code = GetLastError();
        switch (code) {
        case ERROR_PRIVILEGE_NOT_HELD: return static_cast<int>(Fail(GuestEperm));
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS: return static_cast<int>(Fail(GuestEexist));
        case ERROR_PATH_NOT_FOUND: return static_cast<int>(Fail(GuestEnoent));
        case ERROR_ACCESS_DENIED: return static_cast<int>(Fail(GuestEacces));
        case ERROR_WRITE_PROTECT: return static_cast<int>(Fail(GuestErofs));
        case ERROR_DISK_FULL:
        case ERROR_HANDLE_DISK_FULL: return static_cast<int>(Fail(GuestEnospc));
        case ERROR_FILENAME_EXCED_RANGE: return static_cast<int>(Fail(GuestEnametoolong));
        default: throw std::system_error(static_cast<int>(code), std::system_category(), "symlink");
        }
    }
#else
    if (directory)
        std::filesystem::create_directory_symlink(contents, native, error);
    else
        std::filesystem::create_symlink(contents, native, error);
    if (error) {
        const int guest = FilesystemError(error);
        if (guest == GuestEio) throw std::system_error(error, "symlink");
        return static_cast<int>(Fail(guest));
    }
#endif
    RecordWrittenPath_nid_no_patch(native);
    return 0;
}

int APS5_VABI open_nid_postfix(const char* path, int flags, int mode);

int APS5_VABI symlink_nid_postfix(const char* target, const char* created) {
    return CreateSymlinkAt(target, GuestAtFdcwd, created);
}

int APS5_VABI symlinkat_nid_postfix(const char* target, int directory, const char* created) {
    if (target == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (std::strlen(target) >= GuestPathMax) return static_cast<int>(Fail(GuestEnametoolong));
    return CreateSymlinkAt(target, directory, created);
}

int APS5_VABI openat_nid_postfix(int fd, const char* path, int flags, ...) {
    int mode = 0;
    if (flags & SCE_KERNEL_O_CREAT) {
#ifdef _WIN32
        __builtin_sysv_va_list arguments;
        __builtin_sysv_va_start(arguments, flags);
        mode = __builtin_va_arg(arguments, int);
        __builtin_sysv_va_end(arguments);
#else
        std::va_list arguments;
        va_start(arguments, flags);
        mode = va_arg(arguments, int);
        va_end(arguments);
#endif
    }
    if (File::InvalidOpenFlags(flags)) return static_cast<int>(Fail(GuestEinval));
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (fd == GuestAtFdcwd || *path == '/') return open_nid_postfix(path, flags, mode);
    const int result = OpenInDirectory_nid_no_patch(fd, path, flags, static_cast<std::uint16_t>(mode));
    if (result < 0) return static_cast<int>(Fail(result & 0xffff));
    return result;
}

int APS5_VABI mkdir_nid_postfix(const char* path, std::uint16_t mode);

int APS5_VABI mkdirat_nid_postfix(int fd, const char* path, std::uint16_t mode) {
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (fd == GuestAtFdcwd || *path == '/') return mkdir_nid_postfix(path, mode);
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return static_cast<int>(Fail(GuestEnametoolong));
    int error = 0;
    const char* base = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
    if (base == nullptr) return static_cast<int>(Fail(error));
    auto native = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) /
                  std::filesystem::path(reinterpret_cast<const char8_t*>(path));
    while (!native.has_filename() && native.has_relative_path()) native = native.parent_path();
#ifdef _WIN32
    if (!CreateDirectoryW(native.wstring().c_str(), nullptr)) {
        const DWORD code = GetLastError();
        if (code == ERROR_ALREADY_EXISTS || code == ERROR_FILE_EXISTS) return static_cast<int>(Fail(GuestEexist));
        if (code == ERROR_ACCESS_DENIED) return static_cast<int>(Fail(GuestEperm));
        if (code == ERROR_FILENAME_EXCED_RANGE) return static_cast<int>(Fail(GuestEnametoolong));
        if (code == ERROR_PATH_NOT_FOUND) {
            if (HasNonDirectoryPrefix(native)) return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestEnoent));
        }
        return static_cast<int>(Fail(GuestEio));
    }
#else
    if (::mkdir(native.c_str(), static_cast<mode_t>(mode & 0777)) != 0) {
        const int nativeError = errno;
        if ((nativeError == ENOENT || nativeError == ENOTDIR) && HasNonDirectoryPrefix(native))
            return static_cast<int>(Fail(GuestEnotdir));
        return static_cast<int>(Fail(GuestError(std::error_code(nativeError, std::generic_category()))));
    }
#endif
    RecordWrittenPath_nid_no_patch(native);
    return 0;
}

int APS5_VABI fchmodat_nid_postfix(int fd, const char* path, int mode, int flag) {
    if (flag != 0 && flag != GuestAtSymlinkNofollow) return static_cast<int>(Fail(GuestEinval));
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    std::filesystem::path native;
    if (fd == GuestAtFdcwd || *path == '/') {
        if (const int error = ResolvePathError_nid_no_patch(path, native)) return static_cast<int>(Fail(error));
    } else {
        int error = 0;
        const char* base = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
        if (base == nullptr) return static_cast<int>(Fail(error));
        native = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) / std::filesystem::path(reinterpret_cast<const char8_t*>(path));
    }
    if (flag == GuestAtSymlinkNofollow) {
#ifdef _WIN32
        const DWORD attributes = GetFileAttributesW(native.wstring().c_str());
        const bool symbolic = attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
        std::error_code status;
        const bool symbolic = std::filesystem::is_symlink(std::filesystem::symlink_status(native, status));
#endif
        if (symbolic) {
#ifdef __APPLE__
        if (::fchmodat(AT_FDCWD, native.c_str(), static_cast<mode_t>(mode & 07777), AT_SYMLINK_NOFOLLOW) != 0) {
            return static_cast<int>(Fail(GuestFileError(errno)));
        }
#endif
        RecordWrittenPath_nid_no_patch(native);
        return 0;
    }
    }
#ifdef _WIN32
    if (::_wchmod(native.wstring().c_str(), mode & 07777) != 0) return static_cast<int>(Fail(errno == ENOENT && HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestFileError(errno)));
#else
    if (::chmod(native.c_str(), static_cast<mode_t>(mode & 07777)) != 0) return static_cast<int>(Fail(errno == ENOENT && HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestFileError(errno)));
#endif
    RecordWrittenPath_nid_no_patch(native);
    return 0;
}

int APS5_VABI fchown_nid_postfix(int fd, std::uint32_t uid, std::uint32_t gid) {
    constexpr std::uint32_t Unchanged = 0xFFFFFFFFu;
#ifdef _WIN32
    if (GuestFstat(fd) != 0) return static_cast<int>(Fail(GuestEbadf));
#else
    struct stat status{};
    if (::fstat(fd, &status) != 0) return static_cast<int>(Fail(GuestEbadf));
#endif
    if ((uid != 0 && uid != Unchanged) || (gid != 0 && gid != Unchanged)) {
        throw std::runtime_error("fchown: giving descriptor " + std::to_string(fd) + " owner " + std::to_string(uid) + " and group " + std::to_string(gid) +
            " is not supported, the host keeps no guest ownership");
    }
    return 0;
}

int APS5_VABI chown_nid_postfix(const char* path, std::uint32_t uid, std::uint32_t gid) {
    if (const int error = ChangeOwnerAt(GuestAtFdcwd, path, uid, gid, true, "chown")) return static_cast<int>(Fail(error));
    return 0;
}

int APS5_VABI lchown_nid_postfix(const char* path, std::uint32_t uid, std::uint32_t gid) {
    if (const int error = ChangeOwnerAt(GuestAtFdcwd, path, uid, gid, false, "lchown")) return static_cast<int>(Fail(error));
    return 0;
}

int APS5_VABI fchownat_nid_postfix(int fd, const char* path, std::uint32_t uid, std::uint32_t gid, int flag) {
    if (flag & ~GuestAtSymlinkNofollow) return static_cast<int>(Fail(GuestEinval));
    if (const int error = ChangeOwnerAt(fd, path, uid, gid, (flag & GuestAtSymlinkNofollow) == 0, "fchownat")) return static_cast<int>(Fail(error));
    return 0;
}

int APS5_VABI lchmod_nid_postfix(const char* path, std::uint16_t mode) {
    return fchmodat_nid_postfix(GuestAtFdcwd, path, mode, GuestAtSymlinkNofollow);
}

int APS5_VABI stat_nid_postfix(const char* path, FileStat* sb);
int APS5_VABI lstat_nid_postfix(const char* path, FileStat* sb);

int APS5_VABI fstatat_nid_postfix(int fd, const char* path, FileStat* sb, int flag) {
    if (flag & ~GuestAtSymlinkNofollow) return static_cast<int>(Fail(GuestEinval));
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (fd == GuestAtFdcwd || *path == '/') return flag ? lstat_nid_postfix(path, sb) : stat_nid_postfix(path, sb);
    const int result = StatInDirectory_nid_no_patch(fd, path, sb, flag == 0);
    if (result < 0) return static_cast<int>(Fail(result & 0xffff));
    return 0;
}

int ChangeFileFlags_nid_no_patch(const char* function, const char* path, std::uint64_t flags, bool follow);

int APS5_VABI lchflags_nid_postfix(const char* path, std::uint64_t flags) {
    return ChangeFileFlags_nid_no_patch("lchflags", path, flags, false);
}

int APS5_VABI unlink_nid_postfix(const char* path);
int APS5_VABI rmdir_nid_postfix(const char* path);

int APS5_VABI unlinkat_nid_postfix(int fd, const char* path, int flag) {
    if (flag & ~GuestAtRemovedir) return static_cast<int>(Fail(GuestEinval));
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (fd == GuestAtFdcwd || *path == '/') return (flag & GuestAtRemovedir) ? rmdir_nid_postfix(path) : unlink_nid_postfix(path);
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return static_cast<int>(Fail(GuestEnametoolong));
    int error = 0;
    const char* base = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
    if (base == nullptr) return static_cast<int>(Fail(error));
    auto native = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) /
                  std::filesystem::path(reinterpret_cast<const char8_t*>(path));
    const bool trailingSlash = !native.has_filename() && native.has_relative_path();
    while (!native.has_filename() && native.has_relative_path()) native = native.parent_path();
    std::error_code statusError;
    const auto symlinkStatus = std::filesystem::symlink_status(native, statusError);
    if (flag & GuestAtRemovedir) {
        if (IsDotOrDotDot(path)) return static_cast<int>(Fail(GuestEinval));
        if (std::filesystem::is_symlink(symlinkStatus)) return static_cast<int>(Fail(GuestEnotdir));
        if (!std::filesystem::is_directory(symlinkStatus)) {
            if (HasNonDirectoryPrefix(native) || std::filesystem::exists(symlinkStatus))
                return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestEnoent));
        }
        if (!std::filesystem::is_empty(native, statusError)) return static_cast<int>(Fail(GuestEnotempty));
#ifdef _WIN32
        struct _stat64 status{};
        const bool unlocked = ::_wstat64(native.c_str(), &status) == 0 && !(status.st_mode & _S_IWRITE) &&
            ::_wchmod(native.c_str(), _S_IREAD | _S_IWRITE) == 0;
        if (!std::filesystem::remove(native, statusError)) {
            if (unlocked) ::_wchmod(native.c_str(), _S_IREAD);
            return static_cast<int>(Fail(GuestEio));
        }
#else
        if (::rmdir(native.c_str()) != 0) {
            const int nativeError = errno;
            if ((nativeError == ENOENT || nativeError == ENOTDIR) && HasNonDirectoryPrefix(native))
                return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestError(std::error_code(nativeError, std::generic_category()))));
        }
#endif
    } else {
        if (trailingSlash) {
            if (std::filesystem::is_directory(symlinkStatus)) return static_cast<int>(Fail(GuestEperm));
            if (std::filesystem::exists(symlinkStatus) || HasNonDirectoryPrefix(native))
                return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestEnoent));
        }
        if (std::filesystem::is_directory(symlinkStatus)) return static_cast<int>(Fail(GuestEperm));
#ifdef _WIN32
        struct _stat64 status{};
        const bool unlocked = ::_wstat64(native.c_str(), &status) == 0 && (status.st_mode & _S_IFMT) == _S_IFREG &&
            !(status.st_mode & _S_IWRITE) && ::_wchmod(native.c_str(), _S_IREAD | _S_IWRITE) == 0;
        if (::_wunlink(native.c_str()) != 0) {
            const int nativeError = errno;
            if (unlocked) ::_wchmod(native.c_str(), _S_IREAD);
            if (HasNonDirectoryPrefix(native)) return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestError(std::error_code(nativeError, std::generic_category()))));
        }
#else
        if (::unlink(native.c_str()) != 0) {
            const int nativeError = errno;
            if ((nativeError == ENOENT || nativeError == ENOTDIR) && HasNonDirectoryPrefix(native))
                return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestError(std::error_code(nativeError, std::generic_category()))));
        }
#endif
    }
    RecordWrittenPath_nid_no_patch(native);
    return 0;
}

int APS5_VABI renameat_nid_postfix(int oldfd, const char* from, int newfd, const char* to) {
    if (from == nullptr || to == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*from == '\0' || *to == '\0') return static_cast<int>(Fail(GuestEnoent));
    if ((oldfd == GuestAtFdcwd || *from == '/') && (newfd == GuestAtFdcwd || *to == '/'))
        return RenameGuestPath_nid_no_patch(from, to);
    if (std::strlen(from) >= GuestPathMax || HasLongComponent(from) ||
        std::strlen(to) >= GuestPathMax || HasLongComponent(to))
        return static_cast<int>(Fail(GuestEnametoolong));
    if (IsDotOrDotDot(from) || IsDotOrDotDot(to)) return static_cast<int>(Fail(GuestEinval));

    std::filesystem::path source;
    if (oldfd == GuestAtFdcwd || *from == '/') {
        if (const int error = ResolvePathError_nid_no_patch(from, source)) return static_cast<int>(Fail(error));
    } else {
        int error = 0;
        const char* base = GetDirectoryDescriptorPath_nid_no_patch(oldfd, &error);
        if (base == nullptr) return static_cast<int>(Fail(error));
        source = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) /
                 std::filesystem::path(reinterpret_cast<const char8_t*>(from));
    }

    std::filesystem::path destination;
    if (newfd == GuestAtFdcwd || *to == '/') {
        if (const int error = ResolvePathError_nid_no_patch(to, destination)) return static_cast<int>(Fail(error));
    } else {
        int error = 0;
        const char* base = GetDirectoryDescriptorPath_nid_no_patch(newfd, &error);
        if (base == nullptr) return static_cast<int>(Fail(error));
        destination = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) /
                      std::filesystem::path(reinterpret_cast<const char8_t*>(to));
    }

    const std::string_view fromView(from);
    const std::string_view toView(to);
    const bool oldTrailingSlash = !fromView.empty() && fromView.back() == '/';
    const bool newTrailingSlash = !toView.empty() && toView.back() == '/';

    while (!source.has_filename() && source.has_relative_path()) source = source.parent_path();
    while (!destination.has_filename() && destination.has_relative_path()) destination = destination.parent_path();

    std::error_code statusError;
    const auto sourceStatus = std::filesystem::symlink_status(source, statusError);
    if (oldTrailingSlash) {
        if (sourceStatus.type() == std::filesystem::file_type::not_found)
            return static_cast<int>(Fail(HasNonDirectoryPrefix(source) ? GuestEnotdir : GuestEnoent));
        if (!std::filesystem::is_directory(sourceStatus)) return static_cast<int>(Fail(GuestEnotdir));
    }
    if (newTrailingSlash && sourceStatus.type() != std::filesystem::file_type::not_found &&
        !std::filesystem::is_directory(sourceStatus))
        return static_cast<int>(Fail(GuestEnotdir));

    const int error = RenamePath(source, destination);
    if (error) return static_cast<int>(Fail(error));
    RecordWrittenPath_nid_no_patch(source);
    RecordWrittenPath_nid_no_patch(destination);
    return 0;
}

struct KernelTitleWorkaround {
    std::int32_t version;
    std::int32_t align;
    std::uint64_t ids[2];
};

int APS5_VABI sceKernelTitleWorkaroundIsEnabled(const KernelTitleWorkaround* workaround, std::int32_t bit, std::int32_t* enabled) {
    if (!workaround || !enabled) return static_cast<int>(0x8002000E);
    if (static_cast<std::uint32_t>(bit) >= 0x3A) return static_cast<int>(0x80020016);
    *enabled = static_cast<std::int32_t>((workaround->ids[bit >> 6] >> (bit & 0x3F)) & 1);
    return 0;
}

int APS5_VABI sceKernelGetProcessType(int pid) {
    if (pid != getpid_nid_postfix() && pid != -1 && pid != 0) return static_cast<int>(0x80020003);
    return 0;
}


bool SetDefaultStackSize_nid_no_patch(std::size_t size);
void APS5_VABI pthread_set_name_np_nid_postfix(void* thread, const char* name);

int APS5_VABI pthread_set_defaultstacksize_np_nid_postfix(std::size_t size) {
    return SetDefaultStackSize_nid_no_patch(size) ? 0 : GuestEinval;
}

int APS5_VABI scePthreadSetDefaultstacksize(std::size_t size) {
    return SetDefaultStackSize_nid_no_patch(size) ? 0 : SceKernelError(GuestEinval);
}

void APS5_VABI scePthreadSetName(void* thread, const char* name) {
    pthread_set_name_np_nid_postfix(thread, name);
}

}
