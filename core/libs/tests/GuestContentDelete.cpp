#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceContentDeleteInitialize(const ContentDeleteInitParam* init_param);
int APS5_VABI sceContentDeleteByPath(const char* content_path);
int APS5_VABI sceContentDeleteById(std::int64_t content_id);
int APS5_VABI sceContentDeleteTerminate(void);
int APS5_VABI sceContentExportInit2(const ContentExportInitParam2* init_param);
int APS5_VABI sceContentExportTerm(void);
int APS5_VABI sceContentExportStart(void);
int APS5_VABI sceContentExportFinish(std::int32_t expId);
int APS5_VABI sceContentExportFromFile(int exportId, const ContentExportParam* param, const char* sourcePath, char* path, std::size_t pathBufLen);
int APS5_VABI sceContentExportFromFileWithThumbnail(int exportId, const ContentExportParam* param, const char* sourcePath, const char* thumbnailPath, char* path, std::size_t pathBufLen);
}

namespace {

void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "content delete check failed: %s\n", what);
    std::abort();
}

template <typename TException, typename TAction>
bool Throws(TAction action) {
    try {
        action();
    } catch (const TException&) {
        return true;
    }
    return false;
}

void WriteAll(const std::filesystem::path& path, const char* content) {
    std::ofstream(path, std::ios::binary) << content;
}

ContentExportParam MakeParam(const char* contentType) {
    ContentExportParam param{};
    std::strcpy(param.title, "title");
    std::strcpy(param.content_type, contentType);
    return param;
}

}

int main() {
    WriteAll(ResolvePath_nid_no_patch("/content_delete_source.bin"), "payload");
    WriteAll(ResolvePath_nid_no_patch("/content_delete_thumbnail.png"), "thumb");
    int allocator = 0;
    ContentExportInitParam2 exportInit{};
    exportInit.malloc_func = &allocator;
    exportInit.free_func = &allocator;
    Require(sceContentExportInit2(&exportInit) == 0, "content export initializes");
    const int session = sceContentExportStart();
    Require(session > 0, "an export session starts");
    const ContentExportParam image = MakeParam("image/png");
    const ContentExportParam video = MakeParam("video/mp4");
    char picture[64];
    char clip[64];
    char other[64];
    Require(sceContentExportFromFile(session, &image, "/content_delete_source.bin", picture, sizeof(picture)) == 0, "a picture is exported");
    Require(sceContentExportFromFileWithThumbnail(session, &video, "/content_delete_source.bin", "/content_delete_thumbnail.png", clip, sizeof(clip)) == 0, "a clip with a thumbnail is exported");
    Require(sceContentExportFromFile(session, &image, "/content_delete_source.bin", other, sizeof(other)) == 0, "another picture is exported");
    Require(sceContentExportFinish(session) == 0, "the export session finishes");
    Require(sceContentExportTerm() == 0, "content export terminates");
    const std::filesystem::path picturePath = ResolvePath_nid_no_patch(picture);
    const std::filesystem::path clipPath = ResolvePath_nid_no_patch(clip);
    const std::filesystem::path otherPath = ResolvePath_nid_no_patch(other);
    std::filesystem::path clipThumbnail = clipPath;
    clipThumbnail.replace_extension(".thumbnail.png");
    Require(std::filesystem::exists(clipThumbnail), "the clip's thumbnail is in the library");

    Require(Throws<std::logic_error>([&] { sceContentDeleteByPath(picture); }), "deleting before initialization");
    Require(Throws<std::logic_error>([] { sceContentDeleteById(1); }), "deleting an id before initialization");
    Require(Throws<std::invalid_argument>([] { sceContentDeleteInitialize(nullptr); }), "a null initialization parameter");
    ContentDeleteInitParam init{};
    init.heap_size = 0x10000;
    Require(sceContentDeleteInitialize(&init) == 0, "content delete initializes");
    Require(Throws<std::logic_error>([&] { sceContentDeleteInitialize(&init); }), "a second initialization");

    Require(Throws<std::invalid_argument>([] { sceContentDeleteByPath(nullptr); }), "a null path");
    Require(sceContentDeleteByPath(picture) == 0, "the picture is deleted");
    Require(!std::filesystem::exists(picturePath), "the picture file is gone");
    Require(std::filesystem::exists(otherPath), "the other picture stays");
    Require(sceContentDeleteByPath(clip) == 0, "the clip is deleted");
    Require(!std::filesystem::exists(clipPath) && !std::filesystem::exists(clipThumbnail), "the clip and its thumbnail are gone");
    Require(Throws<std::logic_error>([&] { sceContentDeleteByPath(picture); }), "a deleted picture is not in the library");
    const std::string thumbnailGuestPath = std::string("/_ce/") + clipThumbnail.filename().string();
    Require(Throws<std::logic_error>([&] { sceContentDeleteByPath(thumbnailGuestPath.c_str()); }), "a thumbnail is not content");
    Require(Throws<std::logic_error>([] { sceContentDeleteByPath("/content_delete_source.bin"); }), "a file outside the library");
    Require(Throws<std::logic_error>([] { sceContentDeleteByPath("/_ce/../content_delete_source.bin"); }), "a path leaving the library");
    Require(Throws<std::logic_error>([] { sceContentDeleteByPath("/_ce/"); }), "the library itself");
    Require(Throws<std::logic_error>([] { sceContentDeleteById(1); }) && Throws<std::logic_error>([] { sceContentDeleteById(-1); }), "no content id is in the library");
    Require(std::filesystem::exists(ResolvePath_nid_no_patch("/content_delete_source.bin")), "the source file stays");
    Require(std::filesystem::exists(otherPath), "the other picture still stays");

    Require(sceContentDeleteTerminate() == 0, "content delete terminates");
    Require(Throws<std::logic_error>([] { sceContentDeleteTerminate(); }), "a second termination");
    std::filesystem::remove(otherPath);
    std::puts("content delete tests passed");
    return 0;
}
