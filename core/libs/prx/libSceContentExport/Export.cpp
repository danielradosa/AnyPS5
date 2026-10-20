#include <cstdint>
#include <cstddef>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace {
constexpr std::size_t MaxExecution = 10;
std::atomic<bool> g_initialized{false};
std::mutex g_sessionMutex;
std::set<std::int32_t> g_sessions;
std::int32_t g_lastSessionId = 0;
std::uint64_t g_nextExport = 1;

constexpr const char* ExportRoot = "/_ce";
constexpr int ErrorFileNotFound = static_cast<int>(0x809D3011u);
constexpr int ErrorNotSupportedFormat = static_cast<int>(0x809D3012u);
constexpr int ErrorLargeTitle = static_cast<int>(0x809D3013u);
constexpr int ErrorLargeComment = static_cast<int>(0x809D3015u);
constexpr int ErrorDiskFull = static_cast<int>(0x809D3017u);
constexpr int ErrorNotSupportedThumbnail = static_cast<int>(0x809D3019u);

template <std::size_t TSize>
bool IsTerminated(const char (&text)[TSize]) {
    return std::memchr(text, 0, TSize) != nullptr;
}

const char* ExtensionFor(std::string_view contentType) {
    if (contentType == "image/jpeg") return ".jpg";
    if (contentType == "image/png") return ".png";
    if (contentType == "image/gif") return ".gif";
    if (contentType == "video/mp4") return ".mp4";
    if (contentType == "video/webm") return ".webm";
    return nullptr;
}

bool IsThumbnailExtension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg";
}

bool IsRegularFile(const std::filesystem::path& path) {
    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

void RequireSession(const char* function, std::int32_t expId) {
    if (!g_initialized.load()) throw std::logic_error(std::string(function) + ": not initialized");
    if (!g_sessions.contains(expId)) throw std::invalid_argument(std::string(function) + ": unknown export session");
}

int CheckParam(const ContentExportParam& param) {
    if (!IsTerminated(param.title)) return ErrorLargeTitle;
    if (!IsTerminated(param.comment)) return ErrorLargeComment;
    if (!IsTerminated(param.content_type) || ExtensionFor(param.content_type) == nullptr) return ErrorNotSupportedFormat;
    return 0;
}

std::string ReserveName(const std::filesystem::path& directory, const char* extension) {
    std::string name;
    do {
        name = "export_" + std::to_string(g_nextExport++);
    } while (std::filesystem::exists(directory / (name + extension)));
    return name;
}

std::FILE* OpenForWrite(const std::filesystem::path& path) {
#ifdef _WIN32
    return ::_wfopen(path.c_str(), L"wb");
#else
    return std::fopen(path.c_str(), "wb");
#endif
}

class ProvidedFile {
public:
    ProvidedFile(std::string prefix, std::filesystem::path path) : m_prefix(std::move(prefix)), m_path(std::move(path)), m_file(OpenForWrite(m_path)) {
        if (m_file == nullptr) m_openError = errno;
    }
    ~ProvidedFile() {
        if (m_file != nullptr) std::fclose(m_file);
        if (m_kept) return;
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
    }
    ProvidedFile(const ProvidedFile&) = delete;
    ProvidedFile& operator=(const ProvidedFile&) = delete;
    int Open() const { return m_file != nullptr ? 0 : Failure(m_openError); }
    int Write(const void* data, std::uint64_t size) const {
        if (size == 0 || std::fwrite(data, 1, size, m_file) == size) return 0;
        return Failure(errno);
    }
    int Keep() {
        if (std::fclose(std::exchange(m_file, nullptr)) != 0) return Failure(errno);
        m_kept = true;
        return 0;
    }
private:
    int Failure(int error) const {
        if (error == ENOSPC) return ErrorDiskFull;
        throw std::system_error(error, std::generic_category(), m_prefix + "cannot write " + m_path.string());
    }
    std::string m_prefix;
    std::filesystem::path m_path;
    std::FILE* m_file;
    int m_openError = 0;
    bool m_kept = false;
};

int ExportFile(const char* funcName, std::int32_t exportId, const ContentExportParam* param, const char* sourcePath, const char* thumbnailPath, char* path, std::size_t pathBufLen) {
    const std::string prefix = std::string(funcName) + ": ";
    if (param == nullptr || sourcePath == nullptr || path == nullptr || pathBufLen == 0) throw std::invalid_argument(prefix + "invalid argument");
    std::lock_guard lock(g_sessionMutex);
    RequireSession(funcName, exportId);
    if (const int error = CheckParam(*param)) return error;
    const char* extension = ExtensionFor(param->content_type);
    const std::filesystem::path source = ResolvePath_nid_no_patch(sourcePath);
    if (!IsRegularFile(source)) return ErrorFileNotFound;
    std::filesystem::path thumbnail;
    if (thumbnailPath != nullptr) {
        thumbnail = ResolvePath_nid_no_patch(thumbnailPath);
        if (!IsRegularFile(thumbnail)) return ErrorFileNotFound;
        if (!IsThumbnailExtension(thumbnail)) return ErrorNotSupportedThumbnail;
    }
    const std::filesystem::path directory = ResolvePath_nid_no_patch(ExportRoot);
    const std::string name = ReserveName(directory, extension);
    const std::string guestPath = std::string(ExportRoot) + "/" + name + extension;
    if (guestPath.size() + 1 > pathBufLen) throw std::invalid_argument(prefix + "path buffer is too small");
    const std::filesystem::path destination = directory / (name + extension);
    const std::filesystem::path thumbnailDestination = directory / (name + ".thumbnail" + thumbnail.extension().string());
    try {
        std::filesystem::create_directories(directory);
        std::filesystem::copy_file(source, destination);
        RecordWrittenPath_nid_no_patch(destination);
        if (thumbnailPath != nullptr) {
            std::filesystem::copy_file(thumbnail, thumbnailDestination);
            RecordWrittenPath_nid_no_patch(thumbnailDestination);
        }
    } catch (const std::filesystem::filesystem_error& error) {
        if (error.code() == std::errc::no_space_on_device) return ErrorDiskFull;
        throw;
    }
    std::memcpy(path, guestPath.c_str(), guestPath.size() + 1);
    return 0;
}
}

extern "C" {

int APS5_VABI sceContentExportInit2(const ContentExportInitParam2* init_param) {
    if (init_param == nullptr) APS5_INVALID_ARG_EX;
    if (init_param->malloc_func == nullptr || init_param->free_func == nullptr) throw std::invalid_argument(std::string(__func__) + ": missing allocator functions");
    if (init_param->reserved0 != 0 || init_param->reserved1 != 0) throw std::invalid_argument(std::string(__func__) + ": reserved fields are not zero");
    bool expected = false;
    if (!g_initialized.compare_exchange_strong(expected, true)) throw std::logic_error(std::string(__func__) + ": already initialized");
    return 0;
}

int APS5_VABI sceContentExportFinish(std::int32_t expId) {
    std::lock_guard lock(g_sessionMutex);
    RequireSession(__func__, expId);
    g_sessions.erase(expId);
    return 0;
}

int APS5_VABI sceContentExportFromData(std::int32_t exportId, const ContentExportParam* param, std::size_t contentLength, ContentExportDataProvideFunction dataProvider, void* userData, char* path, std::size_t pathBufLen) {
    const std::string prefix = std::string(__func__) + ": ";
    if (param == nullptr || dataProvider == nullptr || path == nullptr || pathBufLen == 0) throw std::invalid_argument(prefix + "invalid argument");
    if (contentLength == 0) throw std::invalid_argument(prefix + "empty content");
    std::filesystem::path directory;
    std::filesystem::path destination;
    std::string guestPath;
    {
        std::lock_guard lock(g_sessionMutex);
        RequireSession(__func__, exportId);
        if (const int error = CheckParam(*param)) return error;
        const char* extension = ExtensionFor(param->content_type);
        directory = ResolvePath_nid_no_patch(ExportRoot);
        const std::string name = ReserveName(directory, extension);
        guestPath = std::string(ExportRoot) + "/" + name + extension;
        destination = directory / (name + extension);
    }
    if (guestPath.size() + 1 > pathBufLen) throw std::invalid_argument(prefix + "path buffer is too small");
    try {
        std::filesystem::create_directories(directory);
    } catch (const std::filesystem::filesystem_error& error) {
        if (error.code() == std::errc::no_space_on_device) return ErrorDiskFull;
        throw;
    }
    ProvidedFile file(prefix, destination);
    if (const int error = file.Open()) return error;
    std::uint64_t written = 0;
    for (;;) {
        const bool complete = written == contentLength;
        void* data = nullptr;
        std::uint64_t size = 0;
        const std::int32_t status = dataProvider(&data, &size, userData);
        if (status != 0 && status != 1) throw std::runtime_error(prefix + "data provider returned " + std::to_string(status));
        if (size > contentLength - written) throw std::runtime_error(prefix + "data provider returned more than the content length");
        if (size != 0 && data == nullptr) throw std::runtime_error(prefix + "data provider returned no data");
        if (const int error = file.Write(data, size)) return error;
        written += size;
        if (status == 1) break;
        if (complete) throw std::runtime_error(prefix + "data provider continued after the content length");
        if (size == 0) throw std::runtime_error(prefix + "data provider asked to continue without providing data");
    }
    if (written != contentLength) throw std::runtime_error(prefix + "data provider ended before the content length");
    if (const int error = file.Keep()) return error;
    RecordWrittenPath_nid_no_patch(destination);
    std::memcpy(path, guestPath.c_str(), guestPath.size() + 1);
    return 0;
}

int APS5_VABI sceContentExportFromFile(std::int32_t exportId, const ContentExportParam* param, const char* sourcePath, char* path, std::size_t pathBufLen) {
    return ExportFile(__func__, exportId, param, sourcePath, nullptr, path, pathBufLen);
}

int APS5_VABI sceContentExportFromFileWithThumbnail(std::int32_t exportId, const ContentExportParam* param, const char* sourcePath, const char* thumbnailPath, char* path, std::size_t pathBufLen) {
    if (thumbnailPath == nullptr) APS5_INVALID_ARG_EX;
    return ExportFile(__func__, exportId, param, sourcePath, thumbnailPath, path, pathBufLen);
}

int APS5_VABI sceContentExportAddToFavorites(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportDataProvideFunction(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportFree(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportFrom(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportFromFileForSf(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportFromFileWithContentIdList(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportFromFileWithTitleIdList(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportInit(const ContentExportInitParam2* init_param) {
    if (init_param == nullptr) APS5_INVALID_ARG_EX;
    if (init_param->malloc_func == nullptr || init_param->free_func == nullptr) throw std::invalid_argument(std::string(__func__) + ": missing allocator functions");
    bool expected = false;
    if (!g_initialized.compare_exchange_strong(expected, true)) throw std::logic_error(std::string(__func__) + ": already initialized");
    return 0;
}

int APS5_VABI sceContentExportMalloc(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportRemoveFromFavorites(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportRenameContent(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportValidateContents(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceContentExportStart(void) {
    std::lock_guard lock(g_sessionMutex);
    if (!g_initialized.load()) throw std::logic_error(std::string(__func__) + ": not initialized");
    if (g_sessions.size() >= MaxExecution) throw std::runtime_error(std::string(__func__) + ": behaviour beyond 10 live export sessions is unknown");
    if (g_lastSessionId == std::numeric_limits<std::int32_t>::max()) throw std::runtime_error(std::string(__func__) + ": export session ids exhausted");
    g_sessions.insert(++g_lastSessionId);
    return g_lastSessionId;
}

int APS5_VABI sceContentExportGetProgress(std::int32_t expId) {
    std::lock_guard lock(g_sessionMutex);
    RequireSession(__func__, expId);
    throw std::runtime_error(std::string(__func__) + ": progress of a session with no running export is unknown");
}

int APS5_VABI sceContentExportCancel(std::int32_t expId) {
    std::lock_guard lock(g_sessionMutex);
    RequireSession(__func__, expId);
    throw std::runtime_error(std::string(__func__) + ": cancelling a session with no running export is unknown");
}

int APS5_VABI sceContentExportFromDataWithThumbnail(std::int32_t expId, const ContentExportParam* param, const ContentExportCallbackParam* contentCallbackParam,
    const char* thumbnailType, const ContentExportCallbackParam* thumbnailCallbackParam, char* path, std::size_t pathBufferLength) {
    std::lock_guard lock(g_sessionMutex);
    RequireSession(__func__, expId);
    throw std::runtime_error(std::string(__func__) + ": the host has no media library to export into");
}

int APS5_VABI sceContentExportTerm(void) {
    std::lock_guard lock(g_sessionMutex);
    if (!g_sessions.empty()) throw std::runtime_error(std::string(__func__) + ": terminating with live export sessions is unknown");
    bool expected = true;
    if (!g_initialized.compare_exchange_strong(expected, false)) throw std::logic_error(std::string(__func__) + ": not initialized");
    return 0;
}

}
