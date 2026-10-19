#include "prx/libSceFont/include/FontTypes.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <vector>

extern "C" {
int APS5_VABI sceFontMemoryInit(FontMemory*, void*, std::uint32_t, const FontMemoryInterface*, void*, FontMemoryDestroyFunction, void*);
int APS5_VABI sceFontMemoryTerm(FontMemory*);
int APS5_VABI sceFontTextSourceInit(FontTextSource*, const void*, std::uint32_t, FontTextParseFunction, void*);
int APS5_VABI sceFontTextSourceSetDefaultFont(FontTextSource*, FontHandle);
int APS5_VABI sceFontCreateString(const FontMemory*, FontTextSource*, const FontCreateStringDetail*, FontString*);
int APS5_VABI sceFontDestroyString(FontString*);
FontTextCharacter* APS5_VABI sceFontStringRefersTextCharacters(FontString, std::uint32_t*);
int APS5_VABI sceFontCharacterGetSyllableStringState(const FontTextCharacter*, int*);
int APS5_VABI sceFontCreateWords(const FontMemory*, FontTextSource*, const void*, FontWords*);
int APS5_VABI sceFontDestroyWords(FontWords*);
int APS5_VABI sceFontWordsFindWordCharacters(FontWords, const FontTextCharacter*, const FontTextCharacter*, FontTextCharacter**, FontTextCharacter**);
}

static void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "Font text check failed at line %d\n", line);
        std::abort();
    }
}
#define Require(value) Check((value), __LINE__)

template <typename F>
static bool Throws(F f) {
    try {
        f();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

static int allocations = 0;
static void* APS5_VABI Allocate(void*, std::uint32_t size) {
    ++allocations;
    return std::malloc(size);
}
static void APS5_VABI Release(void*, void* pointer) {
    if (pointer) --allocations;
    std::free(pointer);
}

static std::int32_t APS5_VABI ParseCodes(FontTextSource* source, void** order, FontTextParseResult* result) {
    const auto* current = static_cast<const std::uint32_t*>(source->current);
    *order = const_cast<std::uint32_t*>(current);
    if (current == source->end) {
        result->Terminate.terminateCode = 0;
        return 0;
    }
    result->FontCode.font = nullptr;
    result->FontCode.code = *current;
    source->current = current + 1;
    return 1;
}

struct Text {
    FontString string = nullptr;
    FontTextCharacter* characters = nullptr;
    std::uint32_t count = 0;
};

static Text CreateText(const FontMemory& memory, const std::vector<std::uint32_t>& codes, FontHandle font) {
    FontTextSource source{};
    Require(sceFontTextSourceInit(&source, codes.data(), static_cast<std::uint32_t>(codes.size() * sizeof(std::uint32_t)), ParseCodes, nullptr) == SCE_FONT_OK);
    Require(sceFontTextSourceSetDefaultFont(&source, font) == SCE_FONT_OK);
    Text text;
    Require(sceFontCreateString(&memory, &source, nullptr, &text.string) == SCE_FONT_OK && text.string != nullptr);
    text.characters = sceFontStringRefersTextCharacters(text.string, &text.count);
    Require(text.characters != nullptr && text.count == codes.size());
    return text;
}

static bool StateIs(const FontTextCharacter* character, int expected) {
    int state = -1;
    return sceFontCharacterGetSyllableStringState(character, &state) == SCE_FONT_OK && state == expected;
}

static std::int32_t APS5_VABI ParseFailure(FontTextSource*, void** order, FontTextParseResult* result) {
    *order = nullptr;
    result->Error.errorCode = SCE_FONT_ERROR_FATAL;
    return -1;
}

static bool WordIs(FontWords words, const FontTextCharacter* start, const FontTextCharacter* term, const FontTextCharacter* last, const FontTextCharacter* next) {
    FontTextCharacter* foundLast = nullptr;
    FontTextCharacter* foundNext = nullptr;
    return sceFontWordsFindWordCharacters(words, start, term, &foundLast, &foundNext) == SCE_FONT_OK && foundLast == last && foundNext == next;
}

static void TestWords(const FontMemory& memory, FontHandle font) {
    const std::vector<std::uint32_t> codes{'H', 'i', ',', ' ', 'y', 'o', 'u', ' ', ' ', 't', 'h', 'e', 'r', 'e', '\r', '\n', ' ', 'A', '-', 'b', ' ', '1', '-', '2',
                                           0x00A0, 'k', 'g', ' ', 0x6F22, 0x0301, 0x5B57, 0x3002, 0x300C, 0x304B, 0x300D, 0x3041};
    const int allocated = allocations;
    FontTextSource source{};
    Require(sceFontTextSourceInit(&source, codes.data(), static_cast<std::uint32_t>(codes.size() * sizeof(std::uint32_t)), ParseCodes, nullptr) == SCE_FONT_OK);
    FontWords words = nullptr;
    Require(sceFontCreateWords(&memory, &source, nullptr, &words) == SCE_FONT_OK && words != nullptr);
    Require(source.current == codes.data() && allocations == allocated + 1);
    Require(sceFontTextSourceSetDefaultFont(&source, font) == SCE_FONT_OK);
    FontString string = nullptr;
    Require(sceFontCreateString(&memory, &source, nullptr, &string) == SCE_FONT_OK);
    std::uint32_t count = 0;
    FontTextCharacter* c = sceFontStringRefersTextCharacters(string, &count);
    Require(c != nullptr && count == codes.size());

    Require(WordIs(words, &c[0], nullptr, &c[2], &c[4]));
    Require(WordIs(words, &c[4], nullptr, &c[6], &c[9]));
    Require(WordIs(words, &c[7], nullptr, &c[8], &c[9]));
    Require(WordIs(words, &c[9], nullptr, &c[13], &c[16]));
    Require(WordIs(words, &c[14], nullptr, &c[15], &c[16]));
    Require(WordIs(words, &c[15], nullptr, &c[15], &c[16]));
    Require(WordIs(words, &c[16], nullptr, &c[16], &c[17]));
    Require(WordIs(words, &c[17], nullptr, &c[18], &c[19]));
    Require(WordIs(words, &c[19], nullptr, &c[19], &c[21]));
    Require(WordIs(words, &c[21], nullptr, &c[26], &c[28]));
    Require(WordIs(words, &c[28], nullptr, &c[29], &c[30]));
    Require(WordIs(words, &c[30], nullptr, &c[31], &c[32]));
    Require(WordIs(words, &c[32], nullptr, &c[35], nullptr));
    Require(WordIs(words, &c[0], &c[2], &c[1], &c[2]));
    Require(WordIs(words, &c[4], &c[8], &c[6], &c[8]));
    Require(WordIs(words, &c[33], &c[34], &c[33], &c[34]));

    FontTextCharacter* next = nullptr;
    Require(sceFontWordsFindWordCharacters(words, &c[0], nullptr, nullptr, &next) == SCE_FONT_OK && next == &c[4]);
    FontTextCharacter* last = &c[1];
    next = &c[1];
    Require(sceFontWordsFindWordCharacters(nullptr, &c[0], nullptr, &last, &next) == SCE_FONT_ERROR_INVALID_PARAMETER && last == nullptr && next == nullptr);
    Require(sceFontWordsFindWordCharacters(words, nullptr, nullptr, &last, &next) == SCE_FONT_ERROR_INVALID_PARAMETER);
    std::uint64_t bogus[4]{};
    Require(sceFontWordsFindWordCharacters(reinterpret_cast<FontWords>(bogus), &c[0], nullptr, &last, &next) == SCE_FONT_ERROR_INVALID_WORDS);
    Require(Throws([&] { sceFontWordsFindWordCharacters(words, &c[3], &c[3], &last, &next); }));
    Text other = CreateText(memory, {'x', 'y'}, font);
    Require(Throws([&] { sceFontWordsFindWordCharacters(words, &other.characters[0], nullptr, &last, &next); }));
    Require(sceFontDestroyString(&other.string) == SCE_FONT_OK);

    FontWords failed = reinterpret_cast<FontWords>(bogus);
    Require(sceFontCreateWords(nullptr, &source, nullptr, &failed) == SCE_FONT_ERROR_INVALID_PARAMETER && failed == nullptr);
    Require(sceFontCreateWords(&memory, &source, nullptr, nullptr) == SCE_FONT_ERROR_INVALID_PARAMETER);
    FontTextSource unset{};
    unset.textParser = ParseCodes;
    Require(sceFontCreateWords(&memory, &unset, nullptr, &failed) == SCE_FONT_ERROR_INVALID_TEXT_SOURCE && failed == nullptr);
    FontTextSource failing{};
    Require(sceFontTextSourceInit(&failing, codes.data(), 4, ParseFailure, nullptr) == SCE_FONT_OK);
    Require(sceFontCreateWords(&memory, &failing, nullptr, &failed) == SCE_FONT_ERROR_FATAL && failed == nullptr);
    Require(allocations == allocated + 2);

    Require(sceFontDestroyString(&string) == SCE_FONT_OK);
    Require(sceFontDestroyWords(nullptr) == SCE_FONT_ERROR_INVALID_PARAMETER);
    Require(sceFontDestroyWords(&words) == SCE_FONT_OK && words == nullptr);
    Require(sceFontDestroyWords(&words) == SCE_FONT_ERROR_INVALID_WORDS);
    Require(allocations == allocated);
}

int main() {
    const FontMemoryInterface iface{Allocate, Release, nullptr, nullptr, nullptr, nullptr};
    FontMemory memory{};
    Require(sceFontMemoryInit(&memory, nullptr, 0, &iface, nullptr, nullptr, nullptr) == SCE_FONT_OK);
    FontHandleOpaque fontStorage{};
    const FontHandle font = &fontStorage;

    const std::vector<std::uint32_t> latin{'A', 'b', ' ', 0x00E9, 0x4E00};
    Text text = CreateText(memory, latin, font);
    FontTextCharacter* characters = text.characters;

    for (std::uint32_t i = 0; i < text.count; ++i) Require(StateIs(&characters[i], 0));
    int state = 7;
    Require(sceFontCharacterGetSyllableStringState(nullptr, &state) == SCE_FONT_ERROR_INVALID_PARAMETER && state == 0);
    Require(sceFontCharacterGetSyllableStringState(&characters[0], nullptr) == SCE_FONT_ERROR_INVALID_PARAMETER);
    Require(sceFontDestroyString(&text.string) == SCE_FONT_OK && text.string == nullptr);

    TestWords(memory, font);

    Require(allocations == 0);
    Require(sceFontMemoryTerm(&memory) == SCE_FONT_OK);
    std::puts("font text: ok");
}
