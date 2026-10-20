#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" int APS5_VABI sceContentExportAddToFavorites(void);
extern "C" int APS5_VABI sceContentExportDataProvideFunction(void);
extern "C" int APS5_VABI sceContentExportFree(void);
extern "C" int APS5_VABI sceContentExportFrom(void);
extern "C" int APS5_VABI sceContentExportFromFileForSf(void);
extern "C" int APS5_VABI sceContentExportFromFileWithContentIdList(void);
extern "C" int APS5_VABI sceContentExportFromFileWithTitleIdList(void);
extern "C" int APS5_VABI sceContentExportInit(void);
extern "C" int APS5_VABI sceContentExportMalloc(void);
extern "C" int APS5_VABI sceContentExportRemoveFromFavorites(void);
extern "C" int APS5_VABI sceContentExportRenameContent(void);
extern "C" int APS5_VABI sceContentExportValidateContents(void);

extern "C" int APS5_VABI sceContentExportInit2(const ContentExportInitParam2* init_param);
extern "C" int APS5_VABI sceContentExportTerm(void);
extern "C" int APS5_VABI sceContentExportStart(void);
extern "C" int APS5_VABI sceContentExportFinish(std::int32_t expId);
extern "C" int APS5_VABI sceContentExportGetProgress(std::int32_t expId);
extern "C" int APS5_VABI sceContentExportCancel(std::int32_t expId);
extern "C" int APS5_VABI sceContentExportFromData(int exportId, const ContentExportParam* param, std::size_t contentLength, ContentExportDataProvideFunction dataProvider, void* userData, char* path, std::size_t pathBufLen);
extern "C" int APS5_VABI sceContentExportFromDataWithThumbnail(std::int32_t expId, const ContentExportParam* param, const ContentExportCallbackParam* contentCallbackParam,
    const char* thumbnailType, const ContentExportCallbackParam* thumbnailCallbackParam, char* path, std::size_t pathBufferLength);
extern "C" int APS5_VABI sceContentExportFromFile(int exportId, const ContentExportParam* param, const char* sourcePath, char* path, std::size_t pathBufLen);
extern "C" int APS5_VABI sceContentExportFromFileWithThumbnail(int exportId, const ContentExportParam* param, const char* sourcePath, const char* thumbnailPath, char* path, std::size_t pathBufLen);

namespace {

void Require(bool value) { if (!value) std::abort(); }

template <typename TException = std::logic_error, typename TAction>
bool Throws(TAction action) {
    try {
        action();
    } catch (const TException&) {
        return true;
    }
    return false;
}

template <typename TAction>
bool ThrowsProviderError(TAction action) {
    try {
        action();
    } catch (const std::runtime_error& error) {
        return std::string(error.what()).find("data provider") != std::string::npos;
    }
    return false;
}

struct WholeBuffer {
    std::uint8_t unused[0x48];
    const void* data;
    std::uint64_t size;
};

std::int32_t APS5_VABI ProvideWholeBuffer(void** data, std::uint64_t* size, void* userData) {
    const auto* whole = static_cast<const WholeBuffer*>(userData);
    *data = const_cast<void*>(whole->data);
    *size = whole->size;
    return 1;
}

struct Chunks {
    std::vector<std::string> parts;
    std::vector<std::int32_t> results;
    std::size_t calls = 0;
};

std::int32_t APS5_VABI ProvideChunks(void** data, std::uint64_t* size, void* userData) {
    auto& chunks = *static_cast<Chunks*>(userData);
    const std::size_t index = chunks.calls++;
    Require(index < chunks.parts.size());
    *data = chunks.parts[index].empty() ? nullptr : chunks.parts[index].data();
    *size = chunks.parts[index].size();
    return chunks.results[index];
}

std::size_t CountExports() {
    std::error_code error;
    std::size_t count = 0;
    for (std::filesystem::directory_iterator entry(ResolvePath_nid_no_patch("/_ce"), error), end; !error && entry != end; entry.increment(error)) ++count;
    return count;
}

std::string ReadAll(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

void WriteAll(const std::filesystem::path& path, const std::string& content) {
    std::ofstream stream(path, std::ios::binary);
    stream << content;
}

int allocator = 0;
char path[1024]{};

void Export(std::int32_t expId) {
    ContentExportParam param{};
    ContentExportCallbackParam content{};
    ContentExportCallbackParam thumbnail{};
    sceContentExportFromDataWithThumbnail(expId, &param, &content, "image/png", &thumbnail, path, sizeof(path));
}

constexpr int ErrorFileNotFound = static_cast<int>(0x809D3011u);
constexpr int ErrorNotSupportedFormat = static_cast<int>(0x809D3012u);
constexpr int ErrorLargeTitle = static_cast<int>(0x809D3013u);
constexpr int ErrorNotSupportedThumbnail = static_cast<int>(0x809D3019u);

ContentExportParam MakeParam(const char* contentType) {
    ContentExportParam param{};
    std::strcpy(param.title, "title");
    std::strcpy(param.comment, "comment");
    std::strcpy(param.content_type, contentType);
    return param;
}

void ExportFromFilesAndData(const ContentExportInitParam2& param) {
    const std::filesystem::path source = ResolvePath_nid_no_patch("/content_export_test_source.bin");
    const std::filesystem::path thumbnail = ResolvePath_nid_no_patch("/content_export_test_thumbnail.png");
    const std::filesystem::path textThumbnail = ResolvePath_nid_no_patch("/content_export_test_thumbnail.txt");
    WriteAll(source, "payload");
    WriteAll(thumbnail, "thumb");
    WriteAll(textThumbnail, "text");

    Require(sceContentExportInit2(&param) == 0);
    const int first = sceContentExportStart();
    const int second = sceContentExportStart();
    Require(first > 0 && second > 0 && first != second);

    char path[64];
    const ContentExportParam image = MakeParam("image/png");
    Require(Throws([&] { sceContentExportFromFile(first + second + 1, &image, "/content_export_test_source.bin", path, sizeof(path)); }));
    Require(Throws([&] { sceContentExportFromFile(first, nullptr, "/content_export_test_source.bin", path, sizeof(path)); }));
    Require(Throws([&] { sceContentExportFromFile(first, &image, nullptr, path, sizeof(path)); }));
    Require(Throws([&] { sceContentExportFromFile(first, &image, "/content_export_test_source.bin", path, 4); }));
    Require(sceContentExportFromFile(first, &image, "/content_export_test_missing.bin", path, sizeof(path)) == ErrorFileNotFound);
    const ContentExportParam unknown = MakeParam("audio/mpeg");
    Require(sceContentExportFromFile(first, &unknown, "/content_export_test_source.bin", path, sizeof(path)) == ErrorNotSupportedFormat);
    ContentExportParam longTitle = image;
    std::memset(longTitle.title, 'a', sizeof(longTitle.title));
    Require(sceContentExportFromFile(first, &longTitle, "/content_export_test_source.bin", path, sizeof(path)) == ErrorLargeTitle);

    Require(sceContentExportFromFile(first, &image, "/content_export_test_source.bin", path, sizeof(path)) == 0);
    Require(std::strncmp(path, "/_ce/", 5) == 0);
    const std::filesystem::path exported = ResolvePath_nid_no_patch(path);
    Require(ReadAll(exported) == "payload");
    Require(ReadAll(source) == "payload");

    char thumbnailed[64];
    const ContentExportParam video = MakeParam("video/mp4");
    Require(sceContentExportFromFileWithThumbnail(second, &video, "/content_export_test_source.bin", "/content_export_test_thumbnail.txt", thumbnailed, sizeof(thumbnailed)) == ErrorNotSupportedThumbnail);
    Require(sceContentExportFromFileWithThumbnail(second, &video, "/content_export_test_source.bin", "/content_export_test_missing.png", thumbnailed, sizeof(thumbnailed)) == ErrorFileNotFound);
    Require(Throws([&] { sceContentExportFromFileWithThumbnail(second, &video, "/content_export_test_source.bin", nullptr, thumbnailed, sizeof(thumbnailed)); }));
    Require(sceContentExportFromFileWithThumbnail(second, &video, "/content_export_test_source.bin", "/content_export_test_thumbnail.png", thumbnailed, sizeof(thumbnailed)) == 0);
    Require(std::strcmp(thumbnailed, path) != 0);
    const std::filesystem::path exportedVideo = ResolvePath_nid_no_patch(thumbnailed);
    Require(ReadAll(exportedVideo) == "payload");
    std::filesystem::path exportedThumbnail = exportedVideo;
    exportedThumbnail.replace_extension(".thumbnail.png");
    Require(ReadAll(exportedThumbnail) == "thumb");

    const std::string png("\x89PNG\r\n\x1a\n\0\0\0\rIHDR", 16);
    WholeBuffer whole{};
    whole.data = png.data();
    whole.size = png.size();
    char dataPath[64];
    Require(sceContentExportFromData(first, &image, png.size(), ProvideWholeBuffer, &whole, dataPath, sizeof(dataPath)) == 0);
    Require(std::strncmp(dataPath, "/_ce/", 5) == 0 && std::strcmp(dataPath + std::strlen(dataPath) - 4, ".png") == 0);
    Require(std::strcmp(dataPath, path) != 0 && std::strcmp(dataPath, thumbnailed) != 0);
    const std::filesystem::path exportedData = ResolvePath_nid_no_patch(dataPath);
    Require(ReadAll(exportedData) == png);

    Chunks chunks{{"first ", "second ", "third"}, {0, 0, 1}};
    char chunkedPath[64];
    Require(sceContentExportFromData(first, &video, 18, ProvideChunks, &chunks, chunkedPath, sizeof(chunkedPath)) == 0);
    Require(chunks.calls == 3 && std::strcmp(chunkedPath + std::strlen(chunkedPath) - 4, ".mp4") == 0);
    const std::filesystem::path exportedChunks = ResolvePath_nid_no_patch(chunkedPath);
    Require(ReadAll(exportedChunks) == "first second third");

    Chunks emptyLast{{"whole", ""}, {0, 1}};
    char emptyLastPath[64];
    Require(sceContentExportFromData(first, &image, 5, ProvideChunks, &emptyLast, emptyLastPath, sizeof(emptyLastPath)) == 0);
    Require(emptyLast.calls == 2);
    const std::filesystem::path exportedEmptyLast = ResolvePath_nid_no_patch(emptyLastPath);
    Require(ReadAll(exportedEmptyLast) == "whole");

    const std::size_t exportCount = CountExports();
    char rejected[64];
    Require(sceContentExportFromData(first, &unknown, png.size(), ProvideWholeBuffer, &whole, rejected, sizeof(rejected)) == ErrorNotSupportedFormat);
    Require(sceContentExportFromData(first, &longTitle, png.size(), ProvideWholeBuffer, &whole, rejected, sizeof(rejected)) == ErrorLargeTitle);
    Require(Throws([&] { sceContentExportFromData(first + second + 1, &image, png.size(), ProvideWholeBuffer, &whole, rejected, sizeof(rejected)); }));
    Require(Throws([&] { sceContentExportFromData(first, &image, png.size(), nullptr, &whole, rejected, sizeof(rejected)); }));
    Require(Throws([&] { sceContentExportFromData(first, &image, png.size(), ProvideWholeBuffer, &whole, rejected, 4); }));
    Chunks shorter{{"short"}, {1}};
    Require(ThrowsProviderError([&] { sceContentExportFromData(first, &image, 10, ProvideChunks, &shorter, rejected, sizeof(rejected)); }));
    Chunks longer{{"too long"}, {1}};
    Require(ThrowsProviderError([&] { sceContentExportFromData(first, &image, 5, ProvideChunks, &longer, rejected, sizeof(rejected)); }));
    Chunks continued{{"whole", ""}, {0, 0}};
    Require(ThrowsProviderError([&] { sceContentExportFromData(first, &image, 5, ProvideChunks, &continued, rejected, sizeof(rejected)); }));
    Require(continued.calls == 2);
    Chunks failed{{""}, {-1}};
    Require(ThrowsProviderError([&] { sceContentExportFromData(first, &image, 5, ProvideChunks, &failed, rejected, sizeof(rejected)); }));
    Require(CountExports() == exportCount);

    Require(sceContentExportFinish(first) == 0);
    Require(Throws([&] { sceContentExportFinish(first); }));
    Require(Throws([&] { sceContentExportFromFile(first, &image, "/content_export_test_source.bin", path, sizeof(path)); }));
    Require(sceContentExportFinish(second) == 0);
    Require(sceContentExportTerm() == 0);
    Require(sceContentExportInit2(&param) == 0);
    Require(Throws([&] { sceContentExportFromFile(second, &video, "/content_export_test_source.bin", path, sizeof(path)); }));
    Require(sceContentExportTerm() == 0);

    std::error_code error;
    std::filesystem::remove(exported, error);
    std::filesystem::remove(exportedVideo, error);
    std::filesystem::remove(exportedThumbnail, error);
    std::filesystem::remove(exportedData, error);
    std::filesystem::remove(exportedChunks, error);
    std::filesystem::remove(exportedEmptyLast, error);
    std::filesystem::remove(ResolvePath_nid_no_patch("/_ce"), error);
    std::filesystem::remove(source, error);
    std::filesystem::remove(thumbnail, error);
    std::filesystem::remove(textThumbnail, error);
}

}

int main() {
    ContentExportInitParam2 param{&allocator, &allocator, nullptr, 0x10000, 0, 0};
    Require(Throws([] { sceContentExportInit2(nullptr); }));
    Require(Throws([] { sceContentExportTerm(); }));
    Require(Throws([] { sceContentExportStart(); }));
    Require(Throws([] { sceContentExportFinish(1); }));
    ContentExportInitParam2 missing = param;
    missing.free_func = nullptr;
    Require(Throws([&] { sceContentExportInit2(&missing); }));
    ContentExportInitParam2 reserved = param;
    reserved.reserved1 = 1;
    Require(Throws([&] { sceContentExportInit2(&reserved); }));
    Require(sceContentExportInit2(&param) == 0);
    Require(Throws([&] { sceContentExportInit2(&param); }));
    Require(sceContentExportTerm() == 0);
    Require(Throws([] { sceContentExportTerm(); }));

    Require(sceContentExportInit2(&param) == 0);

    std::set<int> sessions;
    for (int i = 0; i < 10; ++i) {
        int session = sceContentExportStart();
        Require(session > 0);
        Require(sessions.insert(session).second);
    }
    Require(Throws<std::runtime_error>([] { sceContentExportStart(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportTerm(); }));
    int first = *sessions.begin();
    Require(Throws<std::runtime_error>([&] { sceContentExportGetProgress(first); }));
    Require(Throws<std::runtime_error>([&] { sceContentExportCancel(first); }));
    Require(Throws<std::runtime_error>([&] { Export(first); }));
    Require(path[0] == 0);
    Require(sceContentExportFinish(first) == 0);
    Require(Throws<std::invalid_argument>([&] { sceContentExportFinish(first); }));
    Require(Throws<std::invalid_argument>([&] { sceContentExportGetProgress(first); }));
    Require(Throws<std::invalid_argument>([&] { sceContentExportCancel(first); }));
    Require(Throws<std::invalid_argument>([&] { Export(first); }));
    Require(Throws<std::invalid_argument>([] { sceContentExportFinish(0); }));
    Require(Throws<std::invalid_argument>([] { sceContentExportFinish(-1); }));
    sessions.erase(first);
    int reopened = sceContentExportStart();
    Require(reopened > 0 && reopened != first);
    Require(sessions.insert(reopened).second);
    for (int session : sessions) Require(sceContentExportFinish(session) == 0);
    Require(sceContentExportTerm() == 0);
    Require(Throws([] { sceContentExportStart(); }));
    Require(Throws([&] { sceContentExportFinish(reopened); }));

    ExportFromFilesAndData(param);
    Require(Throws<std::runtime_error>([] { sceContentExportAddToFavorites(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportDataProvideFunction(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportFree(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportFrom(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportFromFileForSf(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportFromFileWithContentIdList(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportFromFileWithTitleIdList(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportInit(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportMalloc(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportRemoveFromFavorites(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportRenameContent(); }));
    Require(Throws<std::runtime_error>([] { sceContentExportValidateContents(); }));
}

