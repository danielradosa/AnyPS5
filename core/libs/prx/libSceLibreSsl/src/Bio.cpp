#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestHeap.hpp"

#include <atomic>
#include <cstdarg>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

struct FileStream;

extern "C" {
FileStream* APS5_VABI fopen_nid_postfix(const char* filename, const char* mode);
int APS5_VABI fclose_nid_postfix(FileStream* stream);
std::size_t APS5_VABI fread_nid_postfix(void* buffer, std::size_t size, std::size_t count, FileStream* stream);
std::size_t APS5_VABI fwrite_nid_postfix(const void* buffer, std::size_t size, std::size_t count, FileStream* stream);
int APS5_VABI fflush_nid_postfix(FileStream* stream);
int APS5_VABI fseek_nid_postfix(FileStream* stream, std::int64_t offset, int origin);
std::int64_t APS5_VABI ftell_nid_postfix(FileStream* stream);
int APS5_VABI feof_nid_postfix(FileStream* stream);
int APS5_VABI vsnprintf_nid_postfix(char* buffer, std::size_t size, const char* format, VaList* args);
}

namespace LibreSsl {

struct BioMethod {
    int type;
};

namespace {

constexpr int TypeMem = 1 | 0x0400;
constexpr int TypeFile = 2 | 0x0400;

constexpr int BioClose = 0x01;
constexpr int BioFpRead = 0x02;
constexpr int BioFpWrite = 0x04;
constexpr int BioFpAppend = 0x08;

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
constexpr int SetBufMem = 114;
constexpr int GetBufMemPointer = 115;
constexpr int FileSeek = 128;
constexpr int SetBufMemEofReturn = 130;
constexpr int FileTell = 133;

constexpr int BioFunctionFileCtrl = 116;
constexpr int BioFunctionMemWrite = 117;
constexpr int BioReasonNoSuchFile = 128;
constexpr int BioReasonWriteToReadOnly = 126;

constexpr BioMethod MemMethod{TypeMem};
constexpr BioMethod FileMethod{TypeFile};

struct BufMem {
    std::uint64_t length;
    char* data;
    std::uint64_t max;
};

static_assert(sizeof(BufMem) == 24);

}

struct Bio {
    const BioMethod* method;
    std::atomic<int> references{1};
    int shutdown = BioClose;
    BufMem* memory = nullptr;
    bool readOnly = false;
    char* original = nullptr;
    std::int64_t eofReturn = -1;
    FileStream* stream = nullptr;
};

namespace {

Bio& RequireBio(Bio* bio, const char* function) {
    if (!bio) throw std::invalid_argument(std::string(function) + ": null BIO");
    return *bio;
}

bool Reserve(BufMem& memory, std::size_t size) {
    if (size <= memory.max) return true;
    std::size_t capacity = memory.max ? static_cast<std::size_t>(memory.max) : 64;
    while (capacity < size) capacity = capacity > std::numeric_limits<std::size_t>::max() / 2 ? size : capacity * 2;
    void* grown = GuestHeap::GuestHeapReallocate_nid_postfix(memory.data, capacity);
    if (!grown) return false;
    memory.data = static_cast<char*>(grown);
    memory.max = capacity;
    return true;
}

void CloseFile(Bio& bio) {
    if (bio.stream && (bio.shutdown & BioClose)) fclose_nid_postfix(bio.stream);
    bio.stream = nullptr;
}

void Destroy(Bio* bio) {
    if (bio->method->type == TypeFile) {
        CloseFile(*bio);
    } else if (bio->memory && (bio->shutdown & BioClose)) {
        if (!bio->readOnly) GuestHeap::GuestHeapFree_nid_postfix(bio->memory->data);
        GuestHeap::GuestHeapFree_nid_postfix(bio->memory);
    }
    delete bio;
}

int Write(Bio& bio, const void* data, std::size_t length, const char* function) {
    if (!data && length) throw std::invalid_argument(std::string(function) + ": null data");
    if (length > static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::invalid_argument(std::string(function) + ": length above INT_MAX");
    if (bio.method->type == TypeFile) {
        if (!bio.stream) throw std::logic_error(std::string(function) + ": the file BIO has no stream");
        if (!length) return 0;
        return static_cast<int>(fwrite_nid_postfix(data, 1, length, bio.stream));
    }
    if (bio.readOnly) {
        PushError(ErrLibBio, BioFunctionMemWrite, BioReasonWriteToReadOnly);
        return -1;
    }
    if (!length) return 0;
    auto& memory = *bio.memory;
    if (!Reserve(memory, static_cast<std::size_t>(memory.length) + length)) {
        PushError(ErrLibBio, BioFunctionMemWrite, ErrReasonMallocFailure);
        return -1;
    }
    std::memcpy(memory.data + memory.length, data, length);
    memory.length += length;
    return static_cast<int>(length);
}

std::int64_t MemCtrl(Bio& bio, int cmd, std::int64_t larg, void* parg) {
    auto& memory = *bio.memory;
    switch (cmd) {
    case CtrlReset:
        if (bio.readOnly) {
            memory.data = bio.original;
            memory.length = memory.max;
        } else {
            memory.length = 0;
        }
        return 1;
    case CtrlEof:
        return memory.length == 0 ? 1 : 0;
    case CtrlInfo:
        if (parg) *static_cast<char**>(parg) = memory.data;
        return static_cast<std::int64_t>(memory.length);
    case CtrlPending:
        return static_cast<std::int64_t>(memory.length);
    case CtrlWpending:
        return 0;
    case CtrlFlush:
        return 1;
    case SetBufMemEofReturn:
        bio.eofReturn = larg;
        return 1;
    case GetBufMemPointer:
        if (parg) *static_cast<BufMem**>(parg) = &memory;
        return 1;
    case SetBufMem:
        throw std::invalid_argument("BIO_ctrl: BIO_set_mem_buf is not supported");
    default:
        return 0;
    }
}

std::int64_t FileCtrl(Bio& bio, int cmd, std::int64_t larg, void* parg) {
    switch (cmd) {
    case CtrlReset:
    case FileSeek:
        if (!bio.stream) throw std::logic_error("BIO_ctrl: the file BIO has no stream");
        return fseek_nid_postfix(bio.stream, cmd == CtrlReset ? 0 : larg, 0) == 0 ? 0 : -1;
    case FileTell:
        if (!bio.stream) throw std::logic_error("BIO_ctrl: the file BIO has no stream");
        return ftell_nid_postfix(bio.stream);
    case CtrlEof:
        if (!bio.stream) throw std::logic_error("BIO_ctrl: the file BIO has no stream");
        return feof_nid_postfix(bio.stream) ? 1 : 0;
    case CtrlFlush:
        if (!bio.stream) throw std::logic_error("BIO_ctrl: the file BIO has no stream");
        return fflush_nid_postfix(bio.stream) == 0 ? 1 : 0;
    case SetFilePointer:
        CloseFile(bio);
        bio.stream = static_cast<FileStream*>(parg);
        bio.shutdown = static_cast<int>(larg & BioClose);
        return 1;
    case GetFilePointer:
        if (parg) *static_cast<FileStream**>(parg) = bio.stream;
        return 1;
    case SetFilename: {
        if (!parg) throw std::invalid_argument("BIO_ctrl: null file name");
        const char* mode = nullptr;
        if (larg & BioFpAppend) mode = (larg & BioFpRead) ? "a+b" : "ab";
        else if ((larg & BioFpRead) && (larg & BioFpWrite)) mode = "r+b";
        else if (larg & BioFpWrite) mode = "wb";
        else if (larg & BioFpRead) mode = "rb";
        else throw std::invalid_argument("BIO_ctrl: no read, write or append flag for the file");
        CloseFile(bio);
        bio.stream = fopen_nid_postfix(static_cast<const char*>(parg), mode);
        if (!bio.stream) {
            PushError(ErrLibBio, BioFunctionFileCtrl, BioReasonNoSuchFile);
            return 0;
        }
        bio.shutdown = static_cast<int>(larg & BioClose);
        return 1;
    }
    default:
        return 0;
    }
}

}

std::vector<unsigned char> BioReadAll(Bio* bio, const char* function) {
    auto& stream = RequireBio(bio, function);
    std::vector<unsigned char> bytes;
    if (stream.method->type == TypeFile) {
        if (!stream.stream) throw std::logic_error(std::string(function) + ": the file BIO has no stream");
        unsigned char chunk[4096];
        while (true) {
            const std::size_t got = fread_nid_postfix(chunk, 1, sizeof(chunk), stream.stream);
            bytes.insert(bytes.end(), chunk, chunk + got);
            if (got < sizeof(chunk)) break;
        }
        return bytes;
    }
    auto& memory = *stream.memory;
    bytes.assign(memory.data, memory.data + memory.length);
    if (stream.readOnly) memory.data += memory.length;
    memory.length = 0;
    return bytes;
}

void BioPutBack(Bio* bio, const unsigned char* data, std::size_t length) {
    if (!length) return;
    if (bio->method->type == TypeFile) {
        if (fseek_nid_postfix(bio->stream, -static_cast<std::int64_t>(length), 1) != 0) throw std::runtime_error("BIO: cannot seek back over unread file data");
        return;
    }
    auto& memory = *bio->memory;
    if (bio->readOnly) {
        memory.data -= length;
        memory.length += length;
        return;
    }
    if (memory.length) throw std::logic_error("BIO: data was written while an object was being read");
    std::memmove(memory.data, data, length);
    memory.length = length;
}

}

using namespace LibreSsl;

extern "C" {

const BioMethod* APS5_VABI BIO_s_mem_nid_postfix() {
    return &MemMethod;
}

const BioMethod* APS5_VABI BIO_s_file_nid_postfix() {
    return &FileMethod;
}

Bio* APS5_VABI BIO_new_nid_postfix(const BioMethod* type) {
    if (type != &MemMethod && type != &FileMethod) throw std::invalid_argument("BIO_new: unsupported BIO method");
    auto* bio = new (std::nothrow) Bio{type};
    if (!bio) return nullptr;
    if (type == &MemMethod) {
        bio->memory = static_cast<BufMem*>(GuestHeap::GuestHeapAllocate_nid_postfix(sizeof(BufMem)));
        if (!bio->memory) {
            delete bio;
            return nullptr;
        }
        *bio->memory = {};
    }
    return bio;
}

Bio* APS5_VABI BIO_new_mem_buf_nid_postfix(void* buf, int len) {
    if (!buf) throw std::invalid_argument("BIO_new_mem_buf: null buffer");
    if (len < -1) throw std::invalid_argument("BIO_new_mem_buf: negative length");
    auto* bio = BIO_new_nid_postfix(&MemMethod);
    if (!bio) return nullptr;
    const std::size_t size = len == -1 ? std::strlen(static_cast<const char*>(buf)) : static_cast<std::size_t>(len);
    bio->readOnly = true;
    bio->original = static_cast<char*>(buf);
    *bio->memory = {size, static_cast<char*>(buf), size};
    bio->eofReturn = 0;
    return bio;
}

Bio* APS5_VABI BIO_new_fp_nid_postfix(FileStream* stream, int close_flag) {
    if (!stream) throw std::invalid_argument("BIO_new_fp: null stream");
    auto* bio = BIO_new_nid_postfix(&FileMethod);
    if (!bio) return nullptr;
    bio->stream = stream;
    bio->shutdown = close_flag & BioClose;
    return bio;
}

int LibreSslBioUpRef_nid_no_patch(Bio* bio) {
    RequireBio(bio, "BIO_up_ref").references.fetch_add(1);
    return 1;
}

int APS5_VABI BIO_free_nid_postfix(Bio* a) {
    if (!a) return 0;
    if (a->references.fetch_sub(1) > 1) return 1;
    Destroy(a);
    return 1;
}

int APS5_VABI BIO_write_nid_postfix(Bio* b, const void* data, int len) {
    auto& bio = RequireBio(b, __func__);
    if (len < 0) throw std::invalid_argument("BIO_write: negative length");
    return Write(bio, data, static_cast<std::size_t>(len), __func__);
}

int APS5_VABI BIO_puts_nid_postfix(Bio* bp, const char* buf) {
    auto& bio = RequireBio(bp, __func__);
    if (!buf) throw std::invalid_argument("BIO_puts: null string");
    return Write(bio, buf, std::strlen(buf), __func__);
}

int APS5_VABI BIO_printf_nid_postfix(Bio* bio, const char* format, ...) {
    auto& target = RequireBio(bio, __func__);
    if (!format) throw std::invalid_argument("BIO_printf: null format");
#ifdef _WIN32
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
#else
    std::va_list args;
    va_start(args, format);
#endif
    static_assert(sizeof(args) == sizeof(VaList));
    VaList copy;
    std::memcpy(&copy, args, sizeof(VaList));
    const int size = vsnprintf_nid_postfix(nullptr, 0, format, reinterpret_cast<VaList*>(args));
    std::string text;
    if (size > 0) {
        text.resize(static_cast<std::size_t>(size) + 1);
        vsnprintf_nid_postfix(text.data(), text.size(), format, &copy);
        text.resize(static_cast<std::size_t>(size));
    }
#ifdef _WIN32
    __builtin_sysv_va_end(args);
#else
    va_end(args);
#endif
    if (size < 0) return -1;
    if (size == 0) return 0;
    const int written = Write(target, text.data(), text.size(), __func__);
    return written == size ? written : -1;
}

std::int64_t APS5_VABI BIO_ctrl_nid_postfix(Bio* bp, int cmd, std::int64_t larg, void* parg) {
    auto& bio = RequireBio(bp, __func__);
    if (cmd == CtrlGetClose) return bio.shutdown;
    if (cmd == CtrlSetClose) {
        bio.shutdown = static_cast<int>(larg);
        return 1;
    }
    return bio.method->type == TypeFile ? FileCtrl(bio, cmd, larg, parg) : MemCtrl(bio, cmd, larg, parg);
}

}
