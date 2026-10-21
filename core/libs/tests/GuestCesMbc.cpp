#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {
void* APS5_VABI sceCesUcsProfileInitSJis1997Cp932(void*);
void* APS5_VABI sceCesUcsProfileInitEucJpCp51932(void*);
void* APS5_VABI sceCesUcsProfileInitGbkCp936(void*);
const std::uint8_t* APS5_VABI sceCesRefersUcsProfileCp1252(void);
int APS5_VABI sceCesMbcsUcsContextInit(void*, const void*);
int APS5_VABI sceCesMbcsUcsContextInitCopy(void*, const void*);
int APS5_VABI sceCesMbcsStrToUtf8Str(void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesMbcToUcs2(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*);
int APS5_VABI sceCesMbcToUtf8(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesMbcToUtf16(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesMbcToUtf16be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesMbcToUtf16le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesMbcToUtf32(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesMbcToUtf32be(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesMbcToUtf32le(const void*, const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceCesSJisGetCode(const std::uint8_t*, std::uint32_t, std::uint32_t*, std::uint16_t*);
int APS5_VABI sceCesSJisPutCode(std::uint16_t, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesSJisCodeToJisX0213(std::uint16_t, std::uint8_t*, std::uint8_t*, std::uint8_t*);
int APS5_VABI sceCesJisX0213ToSJisCode(std::uint8_t, std::uint8_t, std::uint8_t, std::uint16_t*);
void* APS5_VABI sceCesUcsProfileInitEucKr(void*);
void* APS5_VABI sceCesUcsProfileInitUhc(void*);
void* APS5_VABI sceCesUcsProfileInitBig5Cp950(void*);
void* APS5_VABI sceCesUcsProfileInitEucCnGb2312(void*);
int APS5_VABI sceCesUcs2ToSJis(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUcs2ToEucJp(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUcs2ToGb(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUcs2ToEucCn(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUcs2ToSbc(std::uint16_t, const std::uint8_t*, std::uint8_t*);
int APS5_VABI sceCesUcs2StrToMbcsStr(void*, const std::uint16_t*, std::uint32_t, std::uint32_t*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUcs2ToMbc(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf8ToMbc(const std::uint8_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16ToMbc(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16beToMbc(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf16leToMbc(const std::uint16_t*, std::uint32_t, std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32ToMbc(std::uint32_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32beToMbc(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
int APS5_VABI sceCesUtf32leToMbc(const std::uint32_t*, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);
}

using Ucs2Encoder = int (APS5_VABI*)(std::uint16_t, const void*, std::uint8_t*, std::uint32_t, std::uint32_t*);

static void Require(bool value) { if (!value) std::abort(); }

static const std::uint8_t* Bytes(const char* text) { return reinterpret_cast<const std::uint8_t*>(text); }

static void RequireX0213(std::uint16_t sjis, std::uint8_t men, std::uint8_t ku, std::uint8_t ten) {
    std::uint8_t m = 0;
    std::uint8_t k = 0;
    std::uint8_t t = 0;
    Require(sceCesSJisCodeToJisX0213(sjis, &m, &k, &t) == 0 && m == men && k == ku && t == ten);
    std::uint16_t back = 0;
    Require(sceCesJisX0213ToSJisCode(men, ku, ten, &back) == 0 && back == sjis);
}

static void StoreOrdered(void* target, std::uint32_t code, std::uint32_t size, bool bigEndian) {
    auto* bytes = static_cast<std::uint8_t*>(target);
    for (std::uint32_t i = 0; i < size; ++i) bytes[i] = static_cast<std::uint8_t>(code >> (8 * (bigEndian ? size - 1 - i : i)));
}

static std::uint32_t EncodeUtf8(std::uint32_t code, std::uint8_t* utf8) {
    if (code < 0x80) {
        utf8[0] = static_cast<std::uint8_t>(code);
        return 1;
    }
    if (code < 0x800) {
        utf8[0] = static_cast<std::uint8_t>(0xC0 | code >> 6);
        utf8[1] = static_cast<std::uint8_t>(0x80 | (code & 0x3F));
        return 2;
    }
    utf8[0] = static_cast<std::uint8_t>(0xE0 | code >> 12);
    utf8[1] = static_cast<std::uint8_t>(0x80 | (code >> 6 & 0x3F));
    utf8[2] = static_cast<std::uint8_t>(0x80 | (code & 0x3F));
    return 3;
}

static void RequireSameForm(int result, int expected, std::uint32_t written, std::uint32_t size, const std::uint8_t* encoded, const std::uint8_t* first) {
    Require(result == expected);
    if (expected != 0) {
        Require(written == 77 && std::memcmp(encoded, "\x55\x55\x55\x55", 4) == 0);
        return;
    }
    Require(written == size && std::memcmp(encoded, first, 4) == 0);
}

static int EncodeEveryForm(const void* profile, std::uint32_t code, std::uint8_t* first, std::uint32_t& size) {
    std::memset(first, 0x55, 4);
    size = 77;
    const int result = sceCesUcs2ToMbc(static_cast<std::uint16_t>(code), profile, first, 4, &size);
    if (result != 0) Require(size == 77 && std::memcmp(first, "\x55\x55\x55\x55", 4) == 0);
    std::uint8_t utf8[4] = {};
    const std::uint32_t utf8Size = EncodeUtf8(code, utf8);
    std::uint8_t encoded[4];
    int converted = 0;
    std::uint32_t read = 77;
    std::uint32_t written = 77;
    std::memset(encoded, 0x55, 4);
    converted = sceCesUtf8ToMbc(utf8, utf8Size, &read, profile, encoded, 4, &written);
    RequireSameForm(converted, result, written, size, encoded, first);
    Require(read == (result == 0 ? utf8Size : 77));
    const std::uint16_t native = static_cast<std::uint16_t>(code);
    std::uint16_t big = 0;
    std::uint16_t little = 0;
    StoreOrdered(&big, code, 2, true);
    StoreOrdered(&little, code, 2, false);
    read = 77;
    written = 77;
    std::memset(encoded, 0x55, 4);
    converted = sceCesUtf16ToMbc(&native, 1, &read, profile, encoded, 4, &written);
    RequireSameForm(converted, result, written, size, encoded, first);
    Require(read == (result == 0 ? 1u : 77u));
    read = 77;
    written = 77;
    std::memset(encoded, 0x55, 4);
    converted = sceCesUtf16beToMbc(&big, 1, &read, profile, encoded, 4, &written);
    RequireSameForm(converted, result, written, size, encoded, first);
    Require(read == (result == 0 ? 1u : 77u));
    read = 77;
    written = 77;
    std::memset(encoded, 0x55, 4);
    converted = sceCesUtf16leToMbc(&little, 1, &read, profile, encoded, 4, &written);
    RequireSameForm(converted, result, written, size, encoded, first);
    Require(read == (result == 0 ? 1u : 77u));
    std::uint32_t bigScalar = 0;
    std::uint32_t littleScalar = 0;
    StoreOrdered(&bigScalar, code, 4, true);
    StoreOrdered(&littleScalar, code, 4, false);
    written = 77;
    std::memset(encoded, 0x55, 4);
    converted = sceCesUtf32ToMbc(code, profile, encoded, 4, &written);
    RequireSameForm(converted, result, written, size, encoded, first);
    written = 77;
    std::memset(encoded, 0x55, 4);
    converted = sceCesUtf32beToMbc(&bigScalar, profile, encoded, 4, &written);
    RequireSameForm(converted, result, written, size, encoded, first);
    written = 77;
    std::memset(encoded, 0x55, 4);
    converted = sceCesUtf32leToMbc(&littleScalar, profile, encoded, 4, &written);
    RequireSameForm(converted, result, written, size, encoded, first);
    return result;
}

static void RequireMbcEncoder(const void* profile, Ucs2Encoder reference, void* context, std::uint32_t expectedAssigned) {
    constexpr int unassignedCode = static_cast<int>(0x805C0020);
    std::uint32_t assigned = 0;
    for (std::uint32_t code = 0; code < 0x10000; ++code) {
        if (code >= 0xD800 && code <= 0xDFFF) continue;
        std::uint8_t encoded[4];
        std::uint32_t size = 0;
        const int result = EncodeEveryForm(profile, code, encoded, size);
        if (reference) {
            std::uint8_t expected[4] = {0x55, 0x55, 0x55, 0x55};
            std::uint32_t expectedSize = 77;
            Require(reference(static_cast<std::uint16_t>(code), profile, expected, 4, &expectedSize) == result);
            if (result == 0) Require(expectedSize == size && std::memcmp(expected, encoded, size) == 0);
        }
        if (context && code != 0) {
            const std::uint16_t text[2] = {static_cast<std::uint16_t>(code), 0};
            std::uint8_t expected[4] = {};
            std::uint32_t read = 0;
            std::uint32_t written = 0;
            Require(sceCesUcs2StrToMbcsStr(context, text, 2, &read, expected, 4, &written) == result);
            if (result == 0) Require(read == 1 && written == size && std::memcmp(expected, encoded, size) == 0);
        }
        if (result != 0) {
            Require(result == unassignedCode);
            continue;
        }
        ++assigned;
        std::uint16_t back = 0;
        std::uint32_t consumed = 0;
        Require(sceCesMbcToUcs2(profile, encoded, size, &consumed, &back) == 0 && consumed == size && back == code);
    }
    Require(assigned == expectedAssigned);
}

static int APS5_VABI Sbc1252(std::uint16_t ucs2, const void* profile, std::uint8_t* sbc, std::uint32_t, std::uint32_t* sbcSize) {
    const int result = sceCesUcs2ToSbc(ucs2, static_cast<const std::uint8_t*>(profile), sbc);
    if (result == 0 && sbcSize) *sbcSize = 1;
    return result;
}

static void RequireMbcBytes(const void* profile, std::uint32_t code, const char* expected) {
    std::uint8_t encoded[4] = {};
    std::uint32_t size = 0;
    const auto length = static_cast<std::uint32_t>(std::strlen(expected));
    Require(EncodeEveryForm(profile, code, encoded, size) == 0 && size == length && std::memcmp(encoded, expected, length) == 0);
}

int main() {
    constexpr int invalidParameter = static_cast<int>(0x805C0001);
    constexpr int invalidSrcBuffer = static_cast<int>(0x805C0010);
    constexpr int srcBufferEnd = static_cast<int>(0x805C0011);
    constexpr int invalidEncode = static_cast<int>(0x805C0014);
    constexpr int unassignedCode = static_cast<int>(0x805C0020);
    constexpr int invalidDstBuffer = static_cast<int>(0x805C0030);
    constexpr int dstBufferEnd = static_cast<int>(0x805C0031);

    alignas(8) std::uint8_t sjisSheet[64] = {};
    alignas(8) std::uint8_t eucSheet[64] = {};
    alignas(8) std::uint8_t gbkSheet[64] = {};
    alignas(8) std::uint8_t blankSheet[64] = {};
    const void* sjis = sceCesUcsProfileInitSJis1997Cp932(sjisSheet);
    const void* euc = sceCesUcsProfileInitEucJpCp51932(eucSheet);
    const void* gbk = sceCesUcsProfileInitGbkCp936(gbkSheet);
    const void* cp1252 = sceCesRefersUcsProfileCp1252();

    std::uint32_t consumed = 0;
    std::uint32_t produced = 0;
    std::uint16_t ucs2 = 0;
    Require(sceCesMbcToUcs2(sjis, Bytes("\x82\xA0"), 2, &consumed, &ucs2) == 0 && consumed == 2 && ucs2 == 0x3042);
    Require(sceCesMbcToUcs2(sjis, Bytes("\xB1"), 1, &consumed, &ucs2) == 0 && consumed == 1 && ucs2 == 0xFF71);
    Require(sceCesMbcToUcs2(euc, Bytes("\xA4\xA2"), 2, &consumed, &ucs2) == 0 && consumed == 2 && ucs2 == 0x3042);
    Require(sceCesMbcToUcs2(gbk, Bytes("\xD6\xD0"), 2, &consumed, &ucs2) == 0 && consumed == 2 && ucs2 == 0x4E2D);
    Require(sceCesMbcToUcs2(cp1252, Bytes("\x80"), 1, &consumed, &ucs2) == 0 && consumed == 1 && ucs2 == 0x20AC);
    Require(sceCesMbcToUcs2(nullptr, Bytes("A"), 1, &consumed, &ucs2) == invalidParameter);
    Require(sceCesMbcToUcs2(blankSheet, Bytes("A"), 1, &consumed, &ucs2) == invalidParameter);
    Require(sceCesMbcToUcs2(sjis, nullptr, 1, &consumed, &ucs2) == invalidSrcBuffer);
    Require(sceCesMbcToUcs2(sjis, Bytes("A"), 0, &consumed, &ucs2) == srcBufferEnd);
    Require(sceCesMbcToUcs2(sjis, Bytes("A"), 1, &consumed, nullptr) == invalidDstBuffer);
    Require(sceCesMbcToUcs2(sjis, Bytes("\x82"), 1, &consumed, &ucs2) == srcBufferEnd);
    Require(sceCesMbcToUcs2(sjis, Bytes("\x80"), 1, &consumed, &ucs2) == invalidEncode);
    Require(sceCesMbcToUcs2(sjis, Bytes("\xF0\x40"), 2, &consumed, &ucs2) == unassignedCode);

    std::uint8_t utf8[4] = {};
    Require(sceCesMbcToUtf8(gbk, Bytes("\xD6\xD0"), 2, &consumed, utf8, sizeof(utf8), &produced) == 0 && consumed == 2 && produced == 3);
    Require(std::memcmp(utf8, "\xE4\xB8\xAD", 3) == 0);
    Require(sceCesMbcToUtf8(gbk, Bytes("\xD6\xD0"), 2, &consumed, utf8, 2, &produced) == dstBufferEnd);

    std::uint16_t utf16 = 0;
    Require(sceCesMbcToUtf16(sjis, Bytes("\x82\xA0"), 2, &consumed, &utf16, 1, &produced) == 0 && produced == 1 && utf16 == 0x3042);
    Require(sceCesMbcToUtf16le(euc, Bytes("\xA4\xA2"), 2, &consumed, &utf16, 1, &produced) == 0 && std::memcmp(&utf16, "\x42\x30", 2) == 0);
    Require(sceCesMbcToUtf16be(euc, Bytes("\xA4\xA2"), 2, &consumed, &utf16, 1, &produced) == 0 && std::memcmp(&utf16, "\x30\x42", 2) == 0);
    Require(sceCesMbcToUtf16(sjis, Bytes("\x82\xA0"), 2, &consumed, &utf16, 0, &produced) == dstBufferEnd);

    std::uint32_t utf32 = 0;
    Require(sceCesMbcToUtf32(gbk, Bytes("A"), 1, &consumed, &utf32) == 0 && consumed == 1 && utf32 == 'A');
    Require(sceCesMbcToUtf32le(gbk, Bytes("\xD6\xD0"), 2, &consumed, &utf32) == 0 && std::memcmp(&utf32, "\x2D\x4E\x00\x00", 4) == 0);
    Require(sceCesMbcToUtf32be(gbk, Bytes("\xD6\xD0"), 2, &consumed, &utf32) == 0 && std::memcmp(&utf32, "\x00\x00\x4E\x2D", 4) == 0);

    std::uint16_t code = 0;
    std::uint32_t size = 0;
    Require(sceCesSJisGetCode(Bytes("\x82\xA0"), 2, &size, &code) == 0 && size == 2 && code == 0x82A0);
    Require(sceCesSJisGetCode(Bytes("A\x82"), 2, &size, &code) == 0 && size == 1 && code == 'A');
    Require(sceCesSJisGetCode(Bytes("\xDF"), 1, &size, &code) == 0 && size == 1 && code == 0xDF);
    Require(sceCesSJisGetCode(Bytes("\xFC\xFC"), 2, nullptr, &code) == 0 && code == 0xFCFC);
    Require(sceCesSJisGetCode(Bytes("\x82"), 1, &size, &code) == srcBufferEnd);
    Require(sceCesSJisGetCode(Bytes("\x80"), 1, &size, &code) == invalidEncode);
    Require(sceCesSJisGetCode(Bytes("\xA0"), 1, &size, &code) == invalidEncode);
    Require(sceCesSJisGetCode(Bytes("\xFD\x40"), 2, &size, &code) == invalidEncode);
    Require(sceCesSJisGetCode(Bytes("\x82\x7F"), 2, &size, &code) == invalidEncode);
    Require(sceCesSJisGetCode(nullptr, 2, &size, &code) == invalidSrcBuffer);
    Require(sceCesSJisGetCode(Bytes("A"), 0, &size, &code) == srcBufferEnd);
    Require(sceCesSJisGetCode(Bytes("A"), 1, &size, nullptr) == invalidDstBuffer);

    std::uint8_t sjisOut[2] = {};
    Require(sceCesSJisPutCode(0x82A0, sjisOut, 2, &size) == 0 && size == 2 && sjisOut[0] == 0x82 && sjisOut[1] == 0xA0);
    Require(sceCesSJisPutCode('A', sjisOut, 1, &size) == 0 && size == 1 && sjisOut[0] == 'A');
    Require(sceCesSJisPutCode(0xB1, sjisOut, 1, nullptr) == 0 && sjisOut[0] == 0xB1);
    Require(sceCesSJisPutCode(0x82A0, sjisOut, 1, &size) == dstBufferEnd);
    Require(sceCesSJisPutCode('A', sjisOut, 0, &size) == dstBufferEnd);
    Require(sceCesSJisPutCode(0x80, sjisOut, 2, &size) == invalidEncode);
    Require(sceCesSJisPutCode(0x0141, sjisOut, 2, &size) == invalidEncode);
    Require(sceCesSJisPutCode(0x827F, sjisOut, 2, &size) == invalidEncode);
    Require(sceCesSJisPutCode(0x82A0, nullptr, 2, &size) == invalidDstBuffer);

    RequireX0213(0x8140, 1, 1, 1);
    RequireX0213(0x889F, 1, 16, 1);
    RequireX0213(0xEFFC, 1, 94, 94);
    RequireX0213(0xF040, 2, 1, 1);
    RequireX0213(0xF09F, 2, 8, 1);
    RequireX0213(0xF140, 2, 3, 1);
    RequireX0213(0xF19F, 2, 4, 1);
    RequireX0213(0xF240, 2, 5, 1);
    RequireX0213(0xF29F, 2, 12, 1);
    RequireX0213(0xF340, 2, 13, 1);
    RequireX0213(0xF39F, 2, 14, 1);
    RequireX0213(0xF440, 2, 15, 1);
    RequireX0213(0xF49F, 2, 78, 1);
    RequireX0213(0xF540, 2, 79, 1);
    RequireX0213(0xFCFC, 2, 94, 94);
    std::uint8_t men = 0;
    std::uint8_t ku = 0;
    std::uint8_t ten = 0;
    Require(sceCesSJisCodeToJisX0213(0x8040, &men, &ku, &ten) == invalidEncode);
    Require(sceCesSJisCodeToJisX0213(0xFD40, &men, &ku, &ten) == invalidEncode);
    Require(sceCesSJisCodeToJisX0213(0x827F, &men, &ku, &ten) == invalidEncode);
    Require(sceCesSJisCodeToJisX0213(0x82A0, nullptr, &ku, &ten) == invalidDstBuffer);
    Require(sceCesJisX0213ToSJisCode(2, 2, 1, &code) == invalidEncode);
    Require(sceCesJisX0213ToSJisCode(2, 77, 1, &code) == invalidEncode);
    Require(sceCesJisX0213ToSJisCode(3, 1, 1, &code) == invalidEncode);
    Require(sceCesJisX0213ToSJisCode(0, 1, 1, &code) == invalidEncode);
    Require(sceCesJisX0213ToSJisCode(1, 95, 1, &code) == invalidEncode);
    Require(sceCesJisX0213ToSJisCode(1, 1, 0, &code) == invalidEncode);
    Require(sceCesJisX0213ToSJisCode(1, 1, 1, nullptr) == invalidDstBuffer);

    alignas(8) std::uint8_t context[64] = {};
    alignas(8) std::uint8_t copy[64] = {};
    Require(sceCesMbcsUcsContextInit(context, gbk) == 0);
    Require(sceCesMbcsUcsContextInitCopy(copy, context) == 0);
    Require(sceCesMbcsStrToUtf8Str(copy, Bytes("\xD6\xD0"), 2, &consumed, utf8, sizeof(utf8), &produced) == 0 && consumed == 2 && produced == 3);
    Require(std::memcmp(utf8, "\xE4\xB8\xAD", 3) == 0);
    Require(sceCesMbcsUcsContextInitCopy(nullptr, context) == invalidParameter);
    Require(sceCesMbcsUcsContextInitCopy(copy, nullptr) == invalidParameter);

    alignas(8) std::uint8_t eucKrSheet[64] = {};
    alignas(8) std::uint8_t uhcSheet[64] = {};
    alignas(8) std::uint8_t big5Sheet[64] = {};
    alignas(8) std::uint8_t eucCnSheet[64] = {};
    const void* eucKr = sceCesUcsProfileInitEucKr(eucKrSheet);
    const void* uhc = sceCesUcsProfileInitUhc(uhcSheet);
    const void* big5 = sceCesUcsProfileInitBig5Cp950(big5Sheet);
    const void* eucCn = sceCesUcsProfileInitEucCnGb2312(eucCnSheet);
    alignas(8) std::uint8_t eucKrContext[64] = {};
    alignas(8) std::uint8_t uhcContext[64] = {};
    alignas(8) std::uint8_t big5Context[64] = {};
    Require(sceCesMbcsUcsContextInit(eucKrContext, eucKr) == 0);
    Require(sceCesMbcsUcsContextInit(uhcContext, uhc) == 0);
    Require(sceCesMbcsUcsContextInit(big5Context, big5) == 0);
    RequireMbcEncoder(sjis, sceCesUcs2ToSJis, nullptr, 7517);
    RequireMbcEncoder(euc, sceCesUcs2ToEucJp, nullptr, 13303);
    RequireMbcEncoder(gbk, sceCesUcs2ToGb, nullptr, 21920);
    RequireMbcEncoder(eucCn, sceCesUcs2ToEucCn, nullptr, 7606);
    RequireMbcEncoder(cp1252, Sbc1252, nullptr, 256);
    RequireMbcEncoder(eucKr, nullptr, eucKrContext, 8354);
    RequireMbcEncoder(uhc, nullptr, uhcContext, 17176);
    RequireMbcEncoder(big5, nullptr, big5Context, 13621);

    RequireMbcBytes(sjis, 0x3042, "\x82\xA0");
    RequireMbcBytes(sjis, 0xFF71, "\xB1");
    RequireMbcBytes(euc, 0xFF71, "\x8E\xB1");
    RequireMbcBytes(euc, 0x02D8, "\x8F\xA2\xAF");
    RequireMbcBytes(gbk, 0x4E2D, "\xD6\xD0");
    RequireMbcBytes(gbk, 0x20AC, "\x80");
    RequireMbcBytes(eucCn, 0x4E2D, "\xD6\xD0");
    RequireMbcBytes(cp1252, 0x20AC, "\x80");
    RequireMbcBytes(eucKr, 0xAC00, "\xB0\xA1");
    RequireMbcBytes(uhc, 0xB620, "\x8C\x63");
    RequireMbcBytes(big5, 0x4E00, "\xA4\x40");
    RequireMbcBytes(big5, 'A', "A");

    std::uint8_t mbc[4] = {0x55, 0x55, 0x55, 0x55};
    std::uint32_t read = 77;
    std::uint32_t written = 77;
    const std::uint16_t hiragana = 0x3042;
    std::uint32_t hiraganaBig = 0;
    StoreOrdered(&hiraganaBig, 0x3042, 4, true);
    Require(sceCesUcs2ToMbc(0x3042, nullptr, mbc, 4, &written) == invalidParameter);
    Require(sceCesUcs2ToMbc(0x3042, blankSheet, mbc, 4, &written) == invalidParameter);
    Require(sceCesUcs2ToMbc(0x3042, sjis, nullptr, 4, &written) == invalidDstBuffer);
    Require(sceCesUcs2ToMbc(0x3042, sjis, mbc, 1, &written) == dstBufferEnd);
    Require(sceCesUcs2ToMbc(0xD800, sjis, mbc, 4, &written) == invalidEncode);
    Require(sceCesUcs2ToMbc(0xDFFF, sjis, mbc, 4, &written) == invalidEncode);
    Require(sceCesUcs2ToMbc(0xAC00, sjis, mbc, 4, &written) == unassignedCode);
    Require(sceCesUtf8ToMbc(Bytes("\xE3\x81\x82"), 3, &read, blankSheet, mbc, 4, &written) == invalidParameter);
    Require(sceCesUtf8ToMbc(nullptr, 3, &read, sjis, mbc, 4, &written) == invalidSrcBuffer);
    Require(sceCesUtf8ToMbc(Bytes("\xE3\x81\x82"), 0, &read, sjis, mbc, 4, &written) == srcBufferEnd);
    Require(sceCesUtf8ToMbc(Bytes("\xE3\x81\x82"), 2, &read, sjis, mbc, 4, &written) == srcBufferEnd);
    Require(sceCesUtf8ToMbc(Bytes("\xE3\x81\x82"), 3, &read, sjis, nullptr, 4, &written) == invalidDstBuffer);
    Require(sceCesUtf8ToMbc(Bytes("\xE3\x81\x82"), 3, &read, sjis, mbc, 1, &written) == dstBufferEnd);
    Require(sceCesUtf8ToMbc(Bytes("\xC0\x80"), 2, &read, sjis, mbc, 4, &written) == invalidEncode);
    Require(sceCesUtf8ToMbc(Bytes("\xF0\x9F\x98\x80"), 4, &read, sjis, mbc, 4, &written) == unassignedCode);
    const std::uint16_t cutPair[1] = {0xD83D};
    const std::uint16_t brokenPair[2] = {0xD83D, 0x0041};
    const std::uint16_t lowSurrogate[1] = {0xDE00};
    const std::uint16_t emoji[2] = {0xD83D, 0xDE00};
    Require(sceCesUtf16ToMbc(&hiragana, 1, &read, nullptr, mbc, 4, &written) == invalidParameter);
    Require(sceCesUtf16ToMbc(nullptr, 1, &read, sjis, mbc, 4, &written) == invalidSrcBuffer);
    Require(sceCesUtf16ToMbc(&hiragana, 0, &read, sjis, mbc, 4, &written) == srcBufferEnd);
    Require(sceCesUtf16ToMbc(&hiragana, 1, &read, sjis, nullptr, 4, &written) == invalidDstBuffer);
    Require(sceCesUtf16ToMbc(&hiragana, 1, &read, sjis, mbc, 1, &written) == dstBufferEnd);
    Require(sceCesUtf16ToMbc(cutPair, 1, &read, sjis, mbc, 4, &written) == srcBufferEnd);
    Require(sceCesUtf16ToMbc(brokenPair, 2, &read, sjis, mbc, 4, &written) == invalidEncode);
    Require(sceCesUtf16ToMbc(lowSurrogate, 1, &read, sjis, mbc, 4, &written) == invalidEncode);
    Require(sceCesUtf16ToMbc(emoji, 2, &read, sjis, mbc, 4, &written) == unassignedCode);
    Require(sceCesUtf16beToMbc(&hiragana, 1, &read, blankSheet, mbc, 4, &written) == invalidParameter);
    Require(sceCesUtf16leToMbc(nullptr, 1, &read, sjis, mbc, 4, &written) == invalidSrcBuffer);
    Require(sceCesUtf32ToMbc(0x3042, blankSheet, mbc, 4, &written) == invalidParameter);
    Require(sceCesUtf32ToMbc(0x3042, sjis, nullptr, 4, &written) == invalidDstBuffer);
    Require(sceCesUtf32ToMbc(0x110000, sjis, mbc, 4, &written) == invalidEncode);
    Require(sceCesUtf32ToMbc(0xD800, sjis, mbc, 4, &written) == invalidEncode);
    Require(sceCesUtf32ToMbc(0x1F600, sjis, mbc, 4, &written) == unassignedCode);
    Require(sceCesUtf32beToMbc(&hiraganaBig, nullptr, mbc, 4, &written) == invalidParameter);
    Require(sceCesUtf32beToMbc(nullptr, sjis, mbc, 4, &written) == invalidSrcBuffer);
    Require(sceCesUtf32leToMbc(nullptr, sjis, mbc, 4, &written) == invalidSrcBuffer);
    Require(sceCesUtf32leToMbc(&hiraganaBig, sjis, mbc, 4, &written) == invalidEncode);
    Require(read == 77 && written == 77 && std::memcmp(mbc, "\x55\x55\x55\x55", 4) == 0);
    Require(sceCesUtf32beToMbc(&hiraganaBig, sjis, mbc, 4, nullptr) == 0 && mbc[0] == 0x82 && mbc[1] == 0xA0);
    Require(sceCesUtf16ToMbc(&hiragana, 1, nullptr, euc, mbc, 2, nullptr) == 0 && mbc[0] == 0xA4 && mbc[1] == 0xA2);
}
