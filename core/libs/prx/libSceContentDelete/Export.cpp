#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <atomic>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {
std::atomic<bool> g_initialized{false};

constexpr std::string_view ContentRoot = "/_ce";
constexpr std::string_view ThumbnailInfix = ".thumbnail.";

void RequireInitialized(const char* function) {
    if (!g_initialized.load()) throw std::logic_error(std::string(function) + ": not initialized");
}

[[noreturn]] void NotInLibrary(const char* function, const std::string& content) {
    throw std::logic_error(std::string(function) + ": " + content + " is not in the host content library, and the console's error for it is unknown");
}

std::string_view ContentName(std::string_view path) {
    if (path.size() <= ContentRoot.size() + 1 || !path.starts_with(ContentRoot) || path[ContentRoot.size()] != '/') return {};
    const std::string_view name = path.substr(ContentRoot.size() + 1);
    if (name == "." || name == ".." || name.find_first_of("/\\") != std::string_view::npos || name.find(ThumbnailInfix) != std::string_view::npos) return {};
    return name;
}
}

extern "C" {

int APS5_VABI sceContentDeleteInitialize(const ContentDeleteInitParam* init_param) {
    if (init_param == nullptr) APS5_INVALID_ARG_EX;
    bool expected = false;
    if (!g_initialized.compare_exchange_strong(expected, true)) throw std::logic_error(std::string(__func__) + ": already initialized");
    return 0;
}

int APS5_VABI sceContentDeleteByPath(const char* content_path) {
    if (content_path == nullptr) APS5_INVALID_ARG_EX;
    RequireInitialized(__func__);
    if (ContentName(content_path).empty()) NotInLibrary(__func__, std::string("path ") + content_path);
    const std::filesystem::path content = ResolvePath_nid_no_patch(content_path);
    std::error_code error;
    if (!std::filesystem::is_regular_file(content, error)) NotInLibrary(__func__, std::string("path ") + content_path);
    const std::filesystem::path directory = content.parent_path();
    const std::string thumbnailPrefix = content.stem().string() + std::string(ThumbnailInfix);
    std::vector<std::filesystem::path> thumbnails;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().filename().string().starts_with(thumbnailPrefix)) thumbnails.push_back(entry.path());
    }
    std::filesystem::remove(content);
    for (const auto& thumbnail : thumbnails) std::filesystem::remove(thumbnail);
    RecordWrittenPath_nid_no_patch(directory);
    return 0;
}

int APS5_VABI sceContentDeleteById(std::int64_t content_id) {
    RequireInitialized(__func__);
    NotInLibrary(__func__, "content id " + std::to_string(content_id));
}

int APS5_VABI sceContentDeleteTerminate(void) {
    bool expected = true;
    if (!g_initialized.compare_exchange_strong(expected, false)) throw std::logic_error(std::string(__func__) + ": not initialized");
    return 0;
}

}
