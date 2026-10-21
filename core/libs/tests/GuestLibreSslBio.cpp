#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iterator>
#include <string>

struct GuestBio;
struct GuestBioMethod;
struct FileStream;

struct GuestBufMem {
    std::size_t length;
    char* data;
    std::size_t max;
};

extern "C" {
const GuestBioMethod* APS5_VABI BIO_s_mem_nid_postfix();
const GuestBioMethod* APS5_VABI BIO_s_file_nid_postfix();
GuestBio* APS5_VABI BIO_new_nid_postfix(const GuestBioMethod*);
GuestBio* APS5_VABI BIO_new_mem_buf_nid_postfix(void*, int);
GuestBio* APS5_VABI BIO_new_fp_nid_postfix(FileStream*, int);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
int APS5_VABI BIO_write_nid_postfix(GuestBio*, const void*, int);
int APS5_VABI BIO_puts_nid_postfix(GuestBio*, const char*);
int APS5_VABI BIO_printf_nid_postfix(GuestBio*, const char*, ...);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
FileStream* APS5_VABI fopen_nid_postfix(const char*, const char*);
int APS5_VABI fclose_nid_postfix(FileStream*);
}

constexpr int CtrlReset = 1;
constexpr int CtrlEof = 2;
constexpr int CtrlInfo = 3;
constexpr int CtrlGetClose = 8;
constexpr int CtrlSetClose = 9;
constexpr int CtrlPending = 10;
constexpr int CtrlFlush = 11;
constexpr int CtrlWpending = 13;
constexpr int SetFilePointer = 106;
constexpr int GetFilePointer = 107;
constexpr int SetFilename = 108;
constexpr int GetBufMemPointer = 115;
constexpr int FileSeek = 128;
constexpr int FileTell = 133;

static void Require(bool value) {
    if (!value) std::abort();
}

template <typename TCall>
static bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

static std::string Data(GuestBio* bio) {
    char* data = nullptr;
    const auto length = BIO_ctrl_nid_postfix(bio, CtrlInfo, 0, &data);
    return length > 0 ? std::string(data, static_cast<std::size_t>(length)) : std::string();
}

static std::string ReadFile(const char* path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

static void ClearErrors() {
    while (ERR_get_error_nid_postfix()) {
    }
}

static void TestMemory() {
    Require(BIO_s_mem_nid_postfix() == BIO_s_mem_nid_postfix() && BIO_s_mem_nid_postfix() != BIO_s_file_nid_postfix());
    GuestBio* bio = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    Require(bio != nullptr);
    Require(BIO_ctrl_nid_postfix(bio, CtrlEof, 0, nullptr) == 1);
    Require(BIO_ctrl_nid_postfix(bio, CtrlGetClose, 0, nullptr) == 1);
    Require(BIO_write_nid_postfix(bio, "Hello", 5) == 5);
    Require(BIO_write_nid_postfix(bio, "", 0) == 0);
    Require(BIO_puts_nid_postfix(bio, " World") == 6);
    Require(BIO_printf_nid_postfix(bio, " %d-%s-%05.2f-%llx", 42, "x", 3.25, 0x123456789abcull) == 24);
    Require(Data(bio) == "Hello World 42-x-03.25-123456789abc");
    Require(BIO_ctrl_nid_postfix(bio, CtrlPending, 0, nullptr) == 35);
    Require(BIO_ctrl_nid_postfix(bio, CtrlWpending, 0, nullptr) == 0);
    Require(BIO_ctrl_nid_postfix(bio, CtrlEof, 0, nullptr) == 0);
    Require(BIO_ctrl_nid_postfix(bio, CtrlFlush, 0, nullptr) == 1);
    GuestBufMem* memory = nullptr;
    Require(BIO_ctrl_nid_postfix(bio, GetBufMemPointer, 0, &memory) == 1);
    Require(memory && memory->length == 35 && memory->max >= 35 && std::memcmp(memory->data, "Hello World 42", 14) == 0);
    const std::string big(100000, 'z');
    Require(BIO_write_nid_postfix(bio, big.data(), static_cast<int>(big.size())) == 100000);
    Require(BIO_ctrl_nid_postfix(bio, CtrlPending, 0, nullptr) == 100035);
    Require(BIO_ctrl_nid_postfix(bio, CtrlReset, 0, nullptr) == 1);
    Require(BIO_ctrl_nid_postfix(bio, CtrlPending, 0, nullptr) == 0 && BIO_ctrl_nid_postfix(bio, CtrlEof, 0, nullptr) == 1);
    Require(BIO_ctrl_nid_postfix(bio, 9999, 0, nullptr) == 0);
    Require(BIO_ctrl_nid_postfix(bio, CtrlSetClose, 0, nullptr) == 1 && BIO_ctrl_nid_postfix(bio, CtrlGetClose, 0, nullptr) == 0);
    Require(BIO_ctrl_nid_postfix(bio, CtrlSetClose, 1, nullptr) == 1);
    Require(Throws([&] { BIO_write_nid_postfix(bio, nullptr, 3); }));
    Require(Throws([&] { BIO_write_nid_postfix(bio, "a", -1); }));
    Require(BIO_free_nid_postfix(bio) == 1);
    Require(BIO_free_nid_postfix(nullptr) == 0);

    char text[] = "read only";
    GuestBio* fixed = BIO_new_mem_buf_nid_postfix(text, -1);
    Require(fixed != nullptr);
    char* data = nullptr;
    Require(BIO_ctrl_nid_postfix(fixed, CtrlInfo, 0, &data) == 9 && data == text);
    ClearErrors();
    Require(BIO_write_nid_postfix(fixed, "x", 1) == -1);
    Require(ERR_get_error_nid_postfix() == ((32ull << 24) | (117ull << 12) | 126));
    Require(Data(fixed) == "read only");
    Require(BIO_free_nid_postfix(fixed) == 1);
    GuestBio* part = BIO_new_mem_buf_nid_postfix(text, 4);
    Require(Data(part) == "read");
    Require(BIO_ctrl_nid_postfix(part, CtrlReset, 0, nullptr) == 1 && Data(part) == "read");
    Require(BIO_free_nid_postfix(part) == 1);
    Require(Throws([] { BIO_new_mem_buf_nid_postfix(nullptr, 1); }));
}

static void TestFile(const char* path) {
    std::remove(path);
    GuestBio* bio = BIO_new_nid_postfix(BIO_s_file_nid_postfix());
    Require(bio != nullptr);
    Require(BIO_ctrl_nid_postfix(bio, SetFilename, 0x01 | 0x04, const_cast<char*>(path)) == 1);
    Require(BIO_puts_nid_postfix(bio, "line one\n") == 9);
    Require(BIO_printf_nid_postfix(bio, "value=%u\n", 7u) == 8);
    Require(BIO_ctrl_nid_postfix(bio, FileTell, 0, nullptr) == 17);
    Require(BIO_ctrl_nid_postfix(bio, CtrlFlush, 0, nullptr) == 1);
    Require(ReadFile(path) == "line one\nvalue=7\n");
    Require(BIO_ctrl_nid_postfix(bio, FileSeek, 5, nullptr) == 0);
    Require(BIO_write_nid_postfix(bio, "ONE", 3) == 3);
    Require(BIO_ctrl_nid_postfix(bio, CtrlReset, 0, nullptr) == 0 && BIO_ctrl_nid_postfix(bio, FileTell, 0, nullptr) == 0);
    Require(BIO_free_nid_postfix(bio) == 1);
    Require(ReadFile(path) == "line ONE\nvalue=7\n");

    bio = BIO_new_nid_postfix(BIO_s_file_nid_postfix());
    Require(BIO_ctrl_nid_postfix(bio, SetFilename, 0x01 | 0x08, const_cast<char*>(path)) == 1);
    Require(BIO_write_nid_postfix(bio, "tail", 4) == 4);
    Require(BIO_free_nid_postfix(bio) == 1);
    Require(ReadFile(path) == "line ONE\nvalue=7\ntail");

    bio = BIO_new_nid_postfix(BIO_s_file_nid_postfix());
    ClearErrors();
    char missing[] = "missing-libressl-dir/file.txt";
    Require(BIO_ctrl_nid_postfix(bio, SetFilename, 0x01 | 0x02, missing) == 0);
    Require(ERR_get_error_nid_postfix() == ((32ull << 24) | (116ull << 12) | 128));
    Require(Throws([&] { BIO_write_nid_postfix(bio, "x", 1); }));
    FileStream* stream = fopen_nid_postfix(path, "ab");
    Require(stream != nullptr);
    Require(BIO_ctrl_nid_postfix(bio, SetFilePointer, 0, stream) == 1);
    FileStream* got = nullptr;
    Require(BIO_ctrl_nid_postfix(bio, GetFilePointer, 0, &got) == 1 && got == stream);
    Require(BIO_ctrl_nid_postfix(bio, CtrlGetClose, 0, nullptr) == 0);
    Require(BIO_puts_nid_postfix(bio, "!") == 1);
    Require(BIO_free_nid_postfix(bio) == 1);
    Require(fclose_nid_postfix(stream) == 0);
    Require(ReadFile(path) == "line ONE\nvalue=7\ntail!");

    stream = fopen_nid_postfix(path, "wb");
    GuestBio* wrapped = BIO_new_fp_nid_postfix(stream, 0x01);
    Require(wrapped && BIO_ctrl_nid_postfix(wrapped, CtrlGetClose, 0, nullptr) == 1);
    Require(BIO_printf_nid_postfix(wrapped, "%s %c", "fp", 'z') == 4);
    Require(BIO_free_nid_postfix(wrapped) == 1);
    Require(ReadFile(path) == "fp z");
    Require(std::remove(path) == 0);
    Require(Throws([] { BIO_new_fp_nid_postfix(nullptr, 0); }));
}

int main(int argc, char** argv) {
    Require(argc == 2);
    TestMemory();
    TestFile(argv[1]);
    return 0;
}
