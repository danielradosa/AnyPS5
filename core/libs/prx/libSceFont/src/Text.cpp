// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#include "prx/libSceFont/include/FontInternal.hpp"

namespace {

using namespace Font;

constexpr std::uint16_t TEXT_SOURCE_MAGIC = 0x0F04;
constexpr std::uint16_t FONT_STRING_MAGIC = 0x0F05;
constexpr std::uint16_t FONT_WRITING_MAGIC = 0x0F06;
constexpr std::uint16_t FONT_WORDS_MAGIC = 0x0F0A;
constexpr std::uint16_t CREATE_STRING_DETAIL_ID = 0x0FD4;
constexpr std::uint32_t WRITING_FORM_HORIZONTAL = 0x10;
constexpr std::uint32_t WRITING_FORM_HORIZONTAL_LTR = 0x12;
constexpr std::int32_t PARSER_RESULT_ERROR = -1;
constexpr std::int32_t PARSER_RESULT_TERMINATE = 0;
constexpr std::int32_t PARSER_RESULT_FONT_CODE = 1;
constexpr std::size_t MAX_PARSED_CHARACTERS = 4096;
constexpr std::int32_t WRITING_MASK_NONE = 0;
constexpr std::int32_t WRITING_MASK_FORMAT_CHARACTERS = 1;
constexpr std::uint64_t CHARACTER_FLAG_FORMAT = 1ull << 42;

[[noreturn]] void Unsupported(const char* function, const std::string& what) {
    throw std::runtime_error(std::string(function) + ": " + what);
}

struct CharacterStorage {
    std::atomic<std::uint32_t> refCount{1};
    std::vector<FontTextCharacter> characters;
};

struct FontStringData {
    std::uint16_t magic = FONT_STRING_MAGIC;
    std::uint8_t flags = 0;
    std::uint8_t reserved0 = 0;
    std::uint32_t writingForm = WRITING_FORM_HORIZONTAL;
    const FontMemory* memory = nullptr;
    FontTextSource* source = nullptr;
    FontHandle defaultFont = nullptr;
    std::uint32_t terminateCode = 0;
    void* terminateOrder = nullptr;
    std::uint32_t textCount = 0;
    std::uint32_t renderCount = 0;
    CharacterStorage* storage = nullptr;
};

struct FontWritingData {
    std::uint16_t magic = FONT_WRITING_MAGIC;
    std::uint16_t reserved0 = 0;
    std::uint32_t writingForm = WRITING_FORM_HORIZONTAL;
    FontString string = nullptr;
    CharacterStorage* storage = nullptr;
    FontHandle defaultFont = nullptr;
    std::size_t nextIndex = 0;
    std::size_t currentIndex = 0;
    float penX = 0.0f;
    float penY = 0.0f;
    bool hasAggregateMetrics = false;
    bool hasCurrentStep = false;
    bool maskFormatCharacters = true;
    bool ownsStorage = false;
    bool hasRawGlyphMetrics = false;
    FontWritingStep step{};
    FontWritingMetrics metrics{};
    FontGlyphMetrics rawGlyphMetrics{};
    FontWritingLetterStep letterStep{};
};

static_assert(sizeof(FontWritingData) <= sizeof(FontWriting));

FontStringData* GetStringData(FontString fontString) {
    auto* data = reinterpret_cast<FontStringData*>(fontString);
    return data && data->magic == FONT_STRING_MAGIC ? data : nullptr;
}

FontWritingData* GetWritingData(FontWriting* fontWriting) {
    if (!fontWriting) return nullptr;
    auto* data = reinterpret_cast<FontWritingData*>(fontWriting->systemUse);
    return data->magic == FONT_WRITING_MAGIC ? data : nullptr;
}

void ReleaseStorage(CharacterStorage*& storage) {
    if (!storage) return;
    if (storage->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) delete storage;
    storage = nullptr;
}

void ReleaseWritingStorage(FontWritingData& writing) {
    if (!writing.ownsStorage) return;
    ReleaseStorage(writing.storage);
    writing.ownsStorage = false;
}

std::uint16_t TextSourceMagic(const FontTextSource* source) {
    return static_cast<std::uint16_t>(source->systemUse0 & 0xFFFFu);
}

bool IsWhitespaceCode(std::uint32_t code) {
    switch (code) {
    case 0x0009: case 0x000A: case 0x000B: case 0x000C: case 0x000D: case 0x0020: case 0x0085:
    case 0x00A0: case 0x1680: case 0x2028: case 0x2029: case 0x202F: case 0x205F: case 0x3000:
        return true;
    default:
        return code >= 0x2000 && code <= 0x200A;
    }
}

bool IsFormatCode(std::uint32_t code) {
    switch (code) {
    case 0x00AD: case 0x061C: case 0x200B: case 0x200C: case 0x200D: case 0x200E: case 0x200F:
    case 0x2060: case 0x2061: case 0x2062: case 0x2063: case 0x2064: case 0x2066: case 0x2067:
    case 0x2068: case 0x2069: case 0xFE0E: case 0xFE0F: case 0xFEFF:
        return true;
    default:
        return false;
    }
}

struct ParsedText {
    void* terminateOrder = nullptr;
    std::uint32_t terminateCode = 0;
};

int CheckTextCreation(const FontMemory* fontMemory, const FontTextSource* textSource) {
    if (TextSourceMagic(textSource) != TEXT_SOURCE_MAGIC) return SCE_FONT_ERROR_INVALID_TEXT_SOURCE;
    if (fontMemory->mem_kind != MEMORY_MAGIC || !fontMemory->iface || !fontMemory->iface->alloc || !fontMemory->iface->dealloc) return SCE_FONT_ERROR_INVALID_MEMORY;
    return SCE_FONT_OK;
}

template <typename TCodeHandler>
std::int32_t ParseTextSource(FontTextSource* textSource, ParsedText& parsed, TCodeHandler handleCode) {
    void* textOrder = nullptr;
    for (std::size_t count = 0; count < MAX_PARSED_CHARACTERS; ++count) {
        FontTextParseResult result{};
        const std::int32_t state = textSource->textParser(textSource, &textOrder, &result);
        if (state == PARSER_RESULT_FONT_CODE) {
            if (!handleCode(result.FontCode.font, result.FontCode.code, textOrder)) return SCE_FONT_ERROR_INVALID_TEXT_SOURCE;
            continue;
        }
        if (state == PARSER_RESULT_TERMINATE) {
            parsed.terminateOrder = textOrder;
            parsed.terminateCode = result.Terminate.terminateCode;
            return SCE_FONT_OK;
        }
        if (state == PARSER_RESULT_ERROR && result.Error.errorCode < 0) return result.Error.errorCode;
        return SCE_FONT_ERROR_INVALID_TEXT_SOURCE;
    }
    return SCE_FONT_ERROR_INVALID_TEXT_SOURCE;
}

enum class WordClass : std::uint8_t { Letter, Space, LineBreak, Glue, Combining, Ideographic, Close, WideClose, Open, WideOpen, Hyphen, BreakAfter };

struct WordsEntry {
    void* textOrder = nullptr;
    std::uint32_t code = 0;
    WordClass wordClass = WordClass::Letter;
    bool breakBefore = false;
};

struct FontWordsData {
    std::uint16_t magic = FONT_WORDS_MAGIC;
    std::uint16_t reserved0 = 0;
    std::uint32_t reserved1 = 0;
    const FontMemory* memory = nullptr;
    std::vector<WordsEntry> entries;
};

FontWordsData* GetWordsData(FontWords fontWords) {
    auto* data = reinterpret_cast<FontWordsData*>(fontWords);
    return data && data->magic == FONT_WORDS_MAGIC ? data : nullptr;
}

bool InRange(std::uint32_t code, std::uint32_t first, std::uint32_t last) {
    return code >= first && code <= last;
}

bool IsLineBreakCode(std::uint32_t code) {
    switch (code) {
    case 0x000A: case 0x000B: case 0x000C: case 0x000D: case 0x0085: case 0x2028: case 0x2029:
        return true;
    default:
        return false;
    }
}

bool IsGlueCode(std::uint32_t code) {
    switch (code) {
    case 0x00A0: case 0x2007: case 0x200D: case 0x202F: case 0x2060: case 0xFEFF:
        return true;
    default:
        return false;
    }
}

bool IsCombiningCode(std::uint32_t code) {
    switch (code) {
    case 0x061C: case 0x200C: case 0x200E: case 0x200F:
        return true;
    default:
        return InRange(code, 0x0300, 0x036F) || InRange(code, 0x1AB0, 0x1AFF) || InRange(code, 0x1DC0, 0x1DFF) || InRange(code, 0x202A, 0x202E) ||
               InRange(code, 0x2066, 0x2069) || InRange(code, 0x20D0, 0x20FF) || InRange(code, 0x3099, 0x309A) || InRange(code, 0xFE00, 0xFE0F) ||
               InRange(code, 0xFE20, 0xFE2F) || InRange(code, 0x1F3FB, 0x1F3FF) || InRange(code, 0xE0100, 0xE01EF);
    }
}

bool IsCloseCode(std::uint32_t code) {
    switch (code) {
    case 0x0021: case 0x0029: case 0x002C: case 0x002E: case 0x003A: case 0x003B: case 0x003F: case 0x005D: case 0x007D:
    case 0x2019: case 0x201D: case 0x2026:
        return true;
    default:
        return false;
    }
}

bool IsOpenCode(std::uint32_t code) {
    switch (code) {
    case 0x0028: case 0x005B: case 0x007B: case 0x00A1: case 0x00BF: case 0x2018: case 0x201C:
        return true;
    default:
        return false;
    }
}

bool IsWideCloseCode(std::uint32_t code) {
    switch (code) {
    case 0x3001: case 0x3002: case 0x3005: case 0x3009: case 0x300B: case 0x300D: case 0x300F: case 0x3011: case 0x3015: case 0x3017:
    case 0x3019: case 0x301B: case 0x301C: case 0x301E: case 0x301F: case 0x303B: case 0x3041: case 0x3043: case 0x3045: case 0x3047:
    case 0x3049: case 0x3063: case 0x3083: case 0x3085: case 0x3087: case 0x308E: case 0x3095: case 0x3096: case 0x309B: case 0x309C:
    case 0x309D: case 0x309E: case 0x30A0: case 0x30A1: case 0x30A3: case 0x30A5: case 0x30A7: case 0x30A9: case 0x30C3: case 0x30E3:
    case 0x30E5: case 0x30E7: case 0x30EE: case 0x30F5: case 0x30F6: case 0x30FB: case 0x30FC: case 0x30FD: case 0x30FE: case 0xFF01:
    case 0xFF09: case 0xFF0C: case 0xFF0E: case 0xFF1A: case 0xFF1B: case 0xFF1F: case 0xFF3D: case 0xFF5D: case 0xFF60: case 0xFF61:
    case 0xFF63: case 0xFF64: case 0xFF65: case 0xFF9E: case 0xFF9F:
        return true;
    default:
        return InRange(code, 0x31F0, 0x31FF) || InRange(code, 0xFF67, 0xFF70);
    }
}

bool IsWideOpenCode(std::uint32_t code) {
    switch (code) {
    case 0x3008: case 0x300A: case 0x300C: case 0x300E: case 0x3010: case 0x3014: case 0x3016: case 0x3018: case 0x301A: case 0x301D:
    case 0xFF08: case 0xFF3B: case 0xFF5B: case 0xFF5F: case 0xFF62:
        return true;
    default:
        return false;
    }
}

bool IsIdeographicCode(std::uint32_t code) {
    return InRange(code, 0x2E80, 0x2FFF) || InRange(code, 0x3003, 0x30FF) || InRange(code, 0x3100, 0x31FF) || InRange(code, 0x3200, 0x4DBF) ||
           InRange(code, 0x4E00, 0x9FFF) || InRange(code, 0xA000, 0xA4CF) || InRange(code, 0xAC00, 0xD7A3) || InRange(code, 0xF900, 0xFAFF) ||
           InRange(code, 0xFE30, 0xFE4F) || InRange(code, 0xFF01, 0xFF60) || InRange(code, 0xFF66, 0xFF9D) || InRange(code, 0xFFE0, 0xFFE6) ||
           InRange(code, 0x1F000, 0x1FAFF) || InRange(code, 0x20000, 0x3FFFD);
}

WordClass ClassifyWordCode(std::uint32_t code) {
    if (IsLineBreakCode(code)) return WordClass::LineBreak;
    if (IsGlueCode(code)) return WordClass::Glue;
    if (code == 0x200B || IsWhitespaceCode(code)) return WordClass::Space;
    if (IsCombiningCode(code)) return WordClass::Combining;
    if (code == 0x002D) return WordClass::Hyphen;
    if (code == 0x00AD || code == 0x2010 || code == 0x2012 || code == 0x2013) return WordClass::BreakAfter;
    if (IsCloseCode(code)) return WordClass::Close;
    if (IsOpenCode(code)) return WordClass::Open;
    if (IsWideCloseCode(code)) return WordClass::WideClose;
    if (IsWideOpenCode(code)) return WordClass::WideOpen;
    if (IsIdeographicCode(code)) return WordClass::Ideographic;
    return WordClass::Letter;
}

bool IsSpaceClass(WordClass wordClass) {
    return wordClass == WordClass::Space || wordClass == WordClass::LineBreak;
}

bool IsWideClass(WordClass wordClass) {
    return wordClass == WordClass::Ideographic || wordClass == WordClass::WideClose || wordClass == WordClass::WideOpen;
}

bool BreaksBetween(WordClass previous, const WordsEntry& current) {
    switch (current.wordClass) {
    case WordClass::Space: case WordClass::LineBreak:
        return true;
    case WordClass::Combining: case WordClass::Glue: case WordClass::Close: case WordClass::WideClose:
        return false;
    default:
        break;
    }
    switch (previous) {
    case WordClass::Glue: case WordClass::Open: case WordClass::WideOpen:
        return false;
    case WordClass::BreakAfter:
        return true;
    case WordClass::Hyphen:
        return !InRange(current.code, '0', '9');
    default:
        return IsWideClass(previous) || IsWideClass(current.wordClass);
    }
}

void MarkWordBreaks(std::vector<WordsEntry>& entries) {
    WordClass previous = WordClass::Space;
    for (WordsEntry& entry : entries) {
        entry.breakBefore = BreaksBetween(previous, entry);
        if (entry.wordClass != WordClass::Combining) previous = entry.wordClass;
        else if (IsSpaceClass(previous)) previous = WordClass::Letter;
    }
}

const WordsEntry& RequireWordsEntry(const char* function, const FontWordsData& words, std::size_t index, const FontTextCharacter* character) {
    if (index >= words.entries.size() || words.entries[index].textOrder != character->textOrder || words.entries[index].code != character->characterCode)
        Unsupported(function, "a character that is not from the text of the words is not modelled");
    return words.entries[index];
}

void FindWord(const char* function, const FontWordsData& words, FontTextCharacter* start, const FontTextCharacter* term, FontTextCharacter*& last, FontTextCharacter*& next) {
    std::size_t index = 0;
    for (const FontTextCharacter* previous = start->prev; previous && index < words.entries.size(); previous = previous->prev) ++index;
    const WordsEntry* entry = &RequireWordsEntry(function, words, index, start);
    FontTextCharacter* character = start;
    const auto following = [&]() -> const WordsEntry* {
        const FontTextCharacter* candidate = character->next;
        if (!candidate || candidate == term) return nullptr;
        return &RequireWordsEntry(function, words, index + 1, candidate);
    };
    const auto advance = [&](const WordsEntry* upcoming) {
        character = character->next;
        ++index;
        entry = upcoming;
        return following();
    };
    FontTextCharacter* bodyLast = nullptr;
    const WordsEntry* nextEntry = following();
    if (!IsSpaceClass(entry->wordClass)) {
        while (nextEntry && !nextEntry->breakBefore) nextEntry = advance(nextEntry);
        bodyLast = character;
        if (nextEntry && IsSpaceClass(nextEntry->wordClass)) nextEntry = advance(nextEntry);
    }
    while (entry->wordClass == WordClass::Space && nextEntry && IsSpaceClass(nextEntry->wordClass)) nextEntry = advance(nextEntry);
    if (entry->code == 0x000D && nextEntry && nextEntry->code == 0x000A) advance(nextEntry);
    last = bodyLast ? bodyLast : character;
    next = character->next;
}

FontTextCharacter MakeCharacter(FontHandle font, std::uint32_t code, void* textOrder) {
    FontTextCharacter character{};
    character.textOrder = textOrder;
    character.font = font;
    character.characterCode = code;
    character.clusterSpan = 1;
    character.flags = static_cast<std::uint64_t>(IsWhitespaceCode(code) ? 0x0E : 0) << 8;
    if (IsFormatCode(code)) character.flags |= CHARACTER_FLAG_FORMAT;
    return character;
}

void LinkCharacters(std::vector<FontTextCharacter>& characters) {
    for (std::size_t i = 0; i < characters.size(); ++i) {
        characters[i].prev = i == 0 ? nullptr : &characters[i - 1];
        characters[i].next = i + 1 < characters.size() ? &characters[i + 1] : nullptr;
    }
}

FontTextCharacter* TextCharacters(FontStringData* data) {
    return data && data->storage && !data->storage->characters.empty() ? data->storage->characters.data() : nullptr;
}

std::uint32_t LevelOf(const FontTextCharacter* character) {
    return static_cast<std::uint32_t>((character->flags >> 24) & 0xFFu);
}

void FillWritingMetricsFromGlyph(FontWritingData& writing, const FontGlyphMetrics& glyphMetrics) {
    writing.rawGlyphMetrics = glyphMetrics;
    writing.hasRawGlyphMetrics = true;
    writing.step.advanceX = glyphMetrics.Horizontal.advance;
    writing.step.advanceY = glyphMetrics.Vertical.advance;
    writing.step.GlyphMetrics = glyphMetrics;
    writing.step.Positioning.x = writing.step.x;
    writing.step.Positioning.y = writing.step.y;
    writing.step.GlyphMetrics.width = writing.step.x;
    writing.step.GlyphMetrics.height = writing.step.y;
}

bool PopulateWritingStep(FontWritingData& writing, std::size_t index) {
    if (!writing.storage || index >= writing.storage->characters.size()) return false;
    const FontTextCharacter& character = writing.storage->characters[index];
    const FontHandle font = character.font ? character.font : writing.defaultFont;
    writing.step.x = writing.penX;
    writing.step.y = writing.penY;
    writing.step.advanceX = 0.0f;
    writing.step.advanceY = 0.0f;
    writing.step.font = font;
    writing.step.Profile = {};
    writing.step.Profile.characterCount = 1;
    writing.step.glyphCode = character.characterCode;
    writing.step.Positioning.x = 0.0f;
    writing.step.Positioning.y = 0.0f;
    writing.step.GlyphMetrics = {};
    writing.rawGlyphMetrics = {};
    writing.hasRawGlyphMetrics = false;
    if ((character.flags & CHARACTER_FLAG_FORMAT) != 0 && writing.maskFormatCharacters) writing.step.Profile.invisibleGlyph = 1;
    if (font && character.characterCode != 0) {
        FontGlyphMetrics glyphMetrics{};
        int rc = GetCharGlyphMetrics(font, character.characterCode, &glyphMetrics, true);
        if (rc != SCE_FONT_OK) rc = GetCharGlyphMetrics(font, character.characterCode, &glyphMetrics, false);
        if (rc == SCE_FONT_OK) FillWritingMetricsFromGlyph(writing, glyphMetrics);
    }
    writing.penX += writing.step.advanceX;
    writing.penY += writing.step.advanceY;
    return true;
}

void AccumulateWritingMetrics(FontWritingData& writing) {
    const FontWritingMetrics previous = writing.metrics;
    const FontGlyphMetrics& glyphMetrics = writing.hasRawGlyphMetrics ? writing.rawGlyphMetrics : writing.step.GlyphMetrics;
    const float left = writing.step.x + glyphMetrics.Horizontal.bearingX;
    const float right = left + glyphMetrics.width;
    const float top = writing.step.y - glyphMetrics.Horizontal.bearingY;
    const float bottom = top + glyphMetrics.height;
    writing.metrics.advanceX = writing.step.x + writing.step.advanceX;
    writing.metrics.advanceY = writing.step.y + writing.step.advanceY;
    if (!writing.hasAggregateMetrics) {
        writing.metrics.Extent = {top, bottom, left, right};
        writing.hasAggregateMetrics = true;
        return;
    }
    writing.metrics.Extent.left = std::min(previous.Extent.left, left);
    writing.metrics.Extent.right = std::max(previous.Extent.right, right);
    writing.metrics.Extent.top = std::min(previous.Extent.top, top);
    writing.metrics.Extent.bottom = std::max(previous.Extent.bottom, bottom);
}

constexpr std::size_t TEXT_CODES_CURRENT = 5;
constexpr std::size_t TEXT_CODES_START = 6;
constexpr std::size_t TEXT_CODES_TERM = 7;

void StepTextCodes(FontTextCodes* textCodes, const FontTextCharacter* character) {
    textCodes->text.order = character->textOrder;
    textCodes->text.code = character->characterCode;
    textCodes->systemUse[TEXT_CODES_CURRENT] = const_cast<FontTextCharacter*>(character);
}

const FontTextCharacter* TextCodesCharacter(const FontTextCodes* textCodes, std::size_t slot) {
    return static_cast<const FontTextCharacter*>(textCodes->systemUse[slot]);
}

}

#pragma GCC visibility push(default)

extern "C" {

int APS5_VABI sceFontTextSourceInit(FontTextSource* textSource, const void* textAddress, std::uint32_t textSizeByte, FontTextParseFunction textParser, void* textObject) {
    if (!textSource) return SCE_FONT_ERROR_INVALID_PARAMETER;
    std::memset(textSource, 0, sizeof(*textSource));
    if (!textParser) return SCE_FONT_ERROR_INVALID_PARAMETER;
    const void* end = textSizeByte != 0 ? static_cast<const std::uint8_t*>(textAddress) + textSizeByte : nullptr;
    textSource->systemUse0 = (static_cast<std::uint64_t>(WRITING_FORM_HORIZONTAL) << 32) | TEXT_SOURCE_MAGIC;
    textSource->start = textAddress;
    textSource->end = end;
    textSource->current = textAddress;
    textSource->textParser = textParser;
    textSource->textObject = textObject;
    textSource->defaultFont = nullptr;
    textSource->systemUse[0] = const_cast<void*>(textAddress);
    textSource->systemUse[1] = const_cast<void*>(end);
    textSource->systemUse[2] = reinterpret_cast<void*>(textParser);
    textSource->systemUse[3] = textObject;
    textSource->systemUse[4] = nullptr;
    return SCE_FONT_OK;
}

int APS5_VABI sceFontTextSourceRewind(FontTextSource* textSource) {
    if (!textSource || TextSourceMagic(textSource) != TEXT_SOURCE_MAGIC) return SCE_FONT_ERROR_INVALID_PARAMETER;
    textSource->start = textSource->systemUse[0];
    textSource->end = textSource->systemUse[1];
    textSource->current = textSource->systemUse[0];
    textSource->textParser = reinterpret_cast<FontTextParseFunction>(textSource->systemUse[2]);
    textSource->textObject = textSource->systemUse[3];
    textSource->defaultFont = static_cast<FontHandle>(textSource->systemUse[4]);
    return SCE_FONT_OK;
}

int APS5_VABI sceFontTextSourceSetDefaultFont(FontTextSource* textSource, FontHandle defaultFont) {
    if (!textSource || TextSourceMagic(textSource) != TEXT_SOURCE_MAGIC) return SCE_FONT_ERROR_INVALID_PARAMETER;
    textSource->defaultFont = defaultFont;
    textSource->systemUse[4] = defaultFont;
    return SCE_FONT_OK;
}

int APS5_VABI sceFontTextSourceSetWritingForm(FontTextSource* textSource, std::int32_t writingForm) {
    if (!textSource || TextSourceMagic(textSource) != TEXT_SOURCE_MAGIC) return SCE_FONT_ERROR_INVALID_PARAMETER;
    if (writingForm < static_cast<std::int32_t>(WRITING_FORM_HORIZONTAL) || writingForm > static_cast<std::int32_t>(WRITING_FORM_HORIZONTAL_LTR)) return SCE_FONT_ERROR_INVALID_PARAMETER;
    textSource->systemUse0 = (textSource->systemUse0 & 0xFFFFFFFFull) | (static_cast<std::uint64_t>(writingForm) << 32);
    return SCE_FONT_OK;
}

int APS5_VABI sceFontCreateString(const FontMemory* fontMemory, FontTextSource* textSource, const FontCreateStringDetail* detail, FontString* pFontString) {
    if (!fontMemory || !textSource || !pFontString || !textSource->textParser) {
        if (pFontString) *pFontString = nullptr;
        return SCE_FONT_ERROR_INVALID_PARAMETER;
    }
    *pFontString = nullptr;
    if (const int rc = CheckTextCreation(fontMemory, textSource); rc != SCE_FONT_OK) return rc;
    if (detail) {
        constexpr std::uint32_t validOrders = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 31);
        if (detail->detailId != CREATE_STRING_DETAIL_ID || detail->detailType > 1 || (detail->detections & ~0x07u) != 0 || (detail->ordersOption & ~validOrders) != 0) return SCE_FONT_ERROR_INVALID_PARAMETER;
    }
    void* raw = fontMemory->iface->alloc(fontMemory->mspace_handle, sizeof(FontStringData));
    if (!raw) return SCE_FONT_ERROR_ALLOCATION_FAILED;
    auto* data = new (raw) FontStringData{};
    data->memory = fontMemory;
    data->source = textSource;
    data->defaultFont = textSource->defaultFont;
    data->storage = new CharacterStorage{};
    if (!data->defaultFont && detail) data->defaultFont = detail->defaultFont;
    data->writingForm = static_cast<std::uint32_t>(textSource->systemUse0 >> 32);
    ParsedText parsed{};
    const std::int32_t state = ParseTextSource(textSource, parsed, [data](FontHandle font, std::uint32_t code, void* textOrder) {
        if (!font) font = data->defaultFont;
        if (!font) return false;
        data->storage->characters.push_back(MakeCharacter(font, code, textOrder));
        return true;
    });
    if (state != SCE_FONT_OK) {
        ReleaseStorage(data->storage);
        data->~FontStringData();
        fontMemory->iface->dealloc(fontMemory->mspace_handle, raw);
        return state;
    }
    data->terminateOrder = parsed.terminateOrder;
    data->terminateCode = parsed.terminateCode;
    LinkCharacters(data->storage->characters);
    data->textCount = static_cast<std::uint32_t>(data->storage->characters.size());
    data->renderCount = data->textCount;
    *pFontString = reinterpret_cast<FontString>(data);
    return SCE_FONT_OK;
}

int APS5_VABI sceFontDestroyString(FontString* pFontString) {
    if (!pFontString) return SCE_FONT_ERROR_INVALID_PARAMETER;
    auto* data = GetStringData(*pFontString);
    if (!data) {
        *pFontString = nullptr;
        return SCE_FONT_ERROR_INVALID_STRING;
    }
    const FontMemory* memory = data->memory;
    ReleaseStorage(data->storage);
    data->~FontStringData();
    memory->iface->dealloc(memory->mspace_handle, data);
    *pFontString = nullptr;
    return SCE_FONT_OK;
}

std::uint32_t APS5_VABI sceFontStringGetTerminateCode(FontString fontString) {
    const auto* data = GetStringData(fontString);
    return data ? data->terminateCode : 0;
}

void* APS5_VABI sceFontStringGetTerminateOrder(FontString fontString) {
    const auto* data = GetStringData(fontString);
    return data ? data->terminateOrder : nullptr;
}

int APS5_VABI sceFontStringGetWritingForm(FontString fontString) {
    const auto* data = GetStringData(fontString);
    return data ? static_cast<int>(data->writingForm) : 0;
}

FontTextCharacter* APS5_VABI sceFontStringRefersTextCharacters(FontString fontString, std::uint32_t* characterCount) {
    auto* data = GetStringData(fontString);
    if (!data || data->textCount == 0) {
        if (characterCount) *characterCount = 0;
        return nullptr;
    }
    if (characterCount) *characterCount = data->textCount;
    return TextCharacters(data);
}

const FontTextCharacter* APS5_VABI sceFontStringRefersRenderCharacters(FontString fontString, FontTextCharacter* startCharacter, FontTextCharacter* lastCharacter, std::uint32_t* characterCount) {
    auto* data = GetStringData(fontString);
    if (!data || !characterCount || data->renderCount == 0) {
        if (characterCount) *characterCount = 0;
        return nullptr;
    }
    FontTextCharacter* characters = TextCharacters(data);
    if (!characters) {
        *characterCount = 0;
        return nullptr;
    }
    const FontTextCharacter* begin = characters;
    const FontTextCharacter* end = characters + data->renderCount;
    if (!startCharacter) startCharacter = characters;
    if (startCharacter < begin || startCharacter >= end || (lastCharacter && (lastCharacter < begin || lastCharacter >= end))) {
        *characterCount = 0;
        return nullptr;
    }
    FontTextCharacter* probe = startCharacter;
    if (startCharacter->clusterKind > 1) {
        do {
            startCharacter = probe;
            probe = startCharacter->prev;
            if (!probe) break;
        } while (probe->synthetic < 0);
    }
    auto startFlags = static_cast<std::uint32_t>(startCharacter->flags);
    const std::uint32_t paragraphLevel = (data->writingForm >> 8) & 1u;
    if ((startFlags & 0xFF00u) == 0x100u && startCharacter->prev) {
        if (startCharacter->prev->clusterKind == 1) startCharacter = startCharacter->prev;
        startFlags = static_cast<std::uint32_t>(startCharacter->flags);
    }
    std::uint32_t startLevel = startFlags >> 24;
    std::uint32_t lastLevel = startLevel;
    std::uint32_t minLevel = startLevel;
    std::uint32_t count = startCharacter->clusterIndex == 0 ? 1u : 0u;
    probe = startCharacter;
    while (probe != lastCharacter) {
        probe = probe->next;
        if (!probe) {
            if (lastCharacter) {
                *characterCount = 0;
                return nullptr;
            }
            break;
        }
        if (probe->clusterIndex == 0) {
            lastLevel = LevelOf(probe);
            if (lastLevel < minLevel && ((lastLevel ^ paragraphLevel) & 1u) == 0) minLevel = lastLevel;
            ++count;
        }
    }
    if (startLevel != lastLevel || startLevel != minLevel) {
        probe = startCharacter->prev;
        while (probe) {
            if (probe->clusterIndex == 0) {
                if (LevelOf(probe) <= minLevel) break;
                ++count;
            }
            startCharacter = probe;
            probe = probe->prev;
        }
        if (lastCharacter) {
            probe = lastCharacter->next;
            while (probe) {
                if (probe->clusterIndex == 0) {
                    if (LevelOf(probe) <= minLevel) break;
                    ++count;
                    lastCharacter = probe;
                }
                probe = probe->next;
            }
        }
    }
    if (lastCharacter) {
        probe = lastCharacter->next;
        std::uint32_t clusterTrim = 0;
        while (probe) {
            std::uint32_t nextTrim = clusterTrim;
            if (probe->clusterIndex == 0) {
                if (probe->synthetic == 0) break;
                nextTrim = 0;
                FontTextCharacter* nextLast = probe;
                if (probe->synthetic < 1) {
                    if (probe->clusterKind == 1) break;
                    nextTrim = clusterTrim + 1;
                    nextLast = lastCharacter;
                }
                lastCharacter = nextLast;
                ++count;
            }
            probe = probe->next;
            count -= nextTrim;
            clusterTrim = nextTrim;
        }
    }
    startLevel = LevelOf(startCharacter);
    if (startLevel != paragraphLevel) {
        probe = startCharacter;
        if (!lastCharacter || LevelOf(lastCharacter) != startLevel) {
            do {
                probe = probe->next;
                if (!probe) {
                    if (lastCharacter) startCharacter = nullptr;
                    break;
                }
                FontTextCharacter* best = startCharacter;
                std::uint32_t bestLevel = startLevel;
                if (probe->clusterIndex == 0) {
                    const std::uint32_t probeLevel = LevelOf(probe);
                    if (probeLevel <= paragraphLevel) break;
                    if ((startLevel & 1u) == paragraphLevel && probeLevel < startLevel && (probeLevel & 1u) != paragraphLevel) {
                        best = probe;
                        bestLevel = probeLevel;
                    }
                }
                startLevel = bestLevel;
                startCharacter = best;
            } while (probe != lastCharacter);
        } else if ((LevelOf(startCharacter) & 1u) != paragraphLevel) {
            startCharacter = lastCharacter;
        }
    }
    if (!startCharacter) count = 0;
    *characterCount = count;
    return startCharacter;
}

int APS5_VABI sceFontCharacterGetBidiLevel(const FontTextCharacter* textCharacter, int* bidiLevel) {
    if (!textCharacter || !bidiLevel) return SCE_FONT_ERROR_INVALID_PARAMETER;
    *bidiLevel = static_cast<int>(LevelOf(textCharacter));
    return SCE_FONT_OK;
}

int APS5_VABI sceFontCharacterGetSyllableStringState(const FontTextCharacter* textCharacter, int* syllableStringState) {
    if (!textCharacter || !syllableStringState) {
        if (syllableStringState) *syllableStringState = 0;
        return SCE_FONT_ERROR_INVALID_PARAMETER;
    }
    if (textCharacter->characterCode >= 0x0E00 && textCharacter->characterCode <= 0x0E7F) throw std::runtime_error("libSceFont: sceFontCharacterGetSyllableStringState for Thai syllable clusters is not implemented");
    *syllableStringState = 0;
    return SCE_FONT_OK;
}

int APS5_VABI sceFontCharacterGetTextFontCode(const FontTextCharacter* textCharacter, FontHandle* pFontHandle, std::uint32_t* textCode) {
    if (!textCharacter) {
        if (pFontHandle) *pFontHandle = nullptr;
        if (textCode) *textCode = 0;
        return SCE_FONT_ERROR_INVALID_PARAMETER;
    }
    if (!pFontHandle || !textCode) return SCE_FONT_ERROR_INVALID_PARAMETER;
    *pFontHandle = textCharacter->font;
    *textCode = textCharacter->characterCode;
    return SCE_FONT_OK;
}

int APS5_VABI sceFontCharacterGetTextOrder(const FontTextCharacter* textCharacter, void** pTextOrder) {
    if (!pTextOrder) return SCE_FONT_ERROR_INVALID_PARAMETER;
    if (!textCharacter) {
        *pTextOrder = nullptr;
        return SCE_FONT_ERROR_INVALID_PARAMETER;
    }
    *pTextOrder = textCharacter->textOrder;
    return SCE_FONT_OK;
}

std::uint32_t APS5_VABI sceFontCharacterLooksFormatCharacters(const FontTextCharacter* textCharacter) {
    if (!textCharacter) return 0;
    return (textCharacter->flags & CHARACTER_FLAG_FORMAT) != 0 ? textCharacter->characterCode : 0;
}

std::uint32_t APS5_VABI sceFontCharacterLooksWhiteSpace(const FontTextCharacter* textCharacter) {
    if (!textCharacter) return 0;
    return ((textCharacter->flags >> 8) & 0xFFu) == 0x0E ? textCharacter->characterCode : 0;
}

FontTextCharacter* APS5_VABI sceFontCharacterRefersTextBack(const FontTextCharacter* textCharacter) {
    if (!textCharacter) return nullptr;
    for (FontTextCharacter* current = textCharacter->prev; current; current = current->prev) {
        if (current->synthetic == 0 && current->clusterIndex == 0) return current;
    }
    return nullptr;
}

FontTextCharacter* APS5_VABI sceFontCharacterRefersTextNext(const FontTextCharacter* textCharacter) {
    if (!textCharacter) return nullptr;
    for (FontTextCharacter* current = textCharacter->next; current; current = current->next) {
        if (current->synthetic == 0 && current->clusterIndex == 0) return current;
    }
    return nullptr;
}

FontTextCodes* APS5_VABI sceFontCharactersRefersTextCodes(const FontTextCharacter* textCharacter, const FontTextCharacter* termCharacter, FontTextCodes* textCodes) {
    if (!textCodes) return nullptr;
    if (!textCharacter) throw std::runtime_error("libSceFont: sceFontCharactersRefersTextCodes without a text character");
    if (textCharacter->clusterIndex != 0 || textCharacter->synthetic != 0) return nullptr;
    std::memset(textCodes, 0, sizeof(FontTextCodes));
    StepTextCodes(textCodes, textCharacter);
    textCodes->systemUse[TEXT_CODES_START] = const_cast<FontTextCharacter*>(textCharacter);
    textCodes->systemUse[TEXT_CODES_TERM] = const_cast<FontTextCharacter*>(termCharacter);
    return textCodes;
}

FontTextCodes* APS5_VABI sceFontTextCodesStepNext(FontTextCodes* textCodesStep) {
    if (!textCodesStep) return nullptr;
    const FontTextCharacter* current = TextCodesCharacter(textCodesStep, TEXT_CODES_CURRENT);
    if (!current || current == TextCodesCharacter(textCodesStep, TEXT_CODES_TERM)) return nullptr;
    const FontTextCharacter* next = current->next;
    while (next && next->synthetic != 0) next = next->next;
    if (!next) return nullptr;
    StepTextCodes(textCodesStep, next);
    return textCodesStep;
}

FontTextCodes* APS5_VABI sceFontTextCodesStepBack(FontTextCodes* textCodesStep) {
    if (!textCodesStep) return nullptr;
    const FontTextCharacter* current = TextCodesCharacter(textCodesStep, TEXT_CODES_CURRENT);
    if (!current || current == TextCodesCharacter(textCodesStep, TEXT_CODES_START)) return nullptr;
    const FontTextCharacter* previous = current->prev;
    while (previous && previous->synthetic != 0) previous = previous->prev;
    if (!previous) return nullptr;
    StepTextCodes(textCodesStep, previous);
    return textCodesStep;
}

int APS5_VABI sceFontCreateWords(const FontMemory* fontMemory, FontTextSource* textSource, const void* detail, FontWords* pFontWords) {
    (void)detail;
    if (!fontMemory || !textSource || !pFontWords || !textSource->textParser) {
        if (pFontWords) *pFontWords = nullptr;
        return SCE_FONT_ERROR_INVALID_PARAMETER;
    }
    *pFontWords = nullptr;
    if (const int rc = CheckTextCreation(fontMemory, textSource); rc != SCE_FONT_OK) return rc;
    void* raw = fontMemory->iface->alloc(fontMemory->mspace_handle, sizeof(FontWordsData));
    if (!raw) return SCE_FONT_ERROR_ALLOCATION_FAILED;
    auto* words = new (raw) FontWordsData{};
    words->memory = fontMemory;
    const FontTextSource position = *textSource;
    ParsedText parsed{};
    const std::int32_t state = ParseTextSource(textSource, parsed, [words](FontHandle, std::uint32_t code, void* textOrder) {
        words->entries.push_back({textOrder, code, ClassifyWordCode(code), false});
        return true;
    });
    *textSource = position;
    if (state != SCE_FONT_OK) {
        words->~FontWordsData();
        fontMemory->iface->dealloc(fontMemory->mspace_handle, raw);
        return state;
    }
    MarkWordBreaks(words->entries);
    *pFontWords = reinterpret_cast<FontWords>(words);
    return SCE_FONT_OK;
}

int APS5_VABI sceFontDestroyWords(FontWords* pFontWords) {
    if (!pFontWords) return SCE_FONT_ERROR_INVALID_PARAMETER;
    auto* words = GetWordsData(*pFontWords);
    *pFontWords = nullptr;
    if (!words) return SCE_FONT_ERROR_INVALID_WORDS;
    const FontMemory* memory = words->memory;
    words->magic = 0;
    words->~FontWordsData();
    memory->iface->dealloc(memory->mspace_handle, words);
    return SCE_FONT_OK;
}

int APS5_VABI sceFontWordsFindWordCharacters(FontWords fontWords, const FontTextCharacter* startCharacter, const FontTextCharacter* termCharacter, FontTextCharacter** pLastCharacter, FontTextCharacter** pNextCharacter) {
    if (pLastCharacter) *pLastCharacter = nullptr;
    if (pNextCharacter) *pNextCharacter = nullptr;
    if (!fontWords || !startCharacter) return SCE_FONT_ERROR_INVALID_PARAMETER;
    const auto* words = GetWordsData(fontWords);
    if (!words) return SCE_FONT_ERROR_INVALID_WORDS;
    if (startCharacter == termCharacter) Unsupported(__func__, "a start character equal to the term character is not modelled");
    FontTextCharacter* last = nullptr;
    FontTextCharacter* next = nullptr;
    FindWord(__func__, *words, const_cast<FontTextCharacter*>(startCharacter), termCharacter, last, next);
    if (pLastCharacter) *pLastCharacter = last;
    if (pNextCharacter) *pNextCharacter = next;
    return SCE_FONT_OK;
}

int APS5_VABI sceFontWritingInit(FontWriting* fontWriting, FontString fontString, const FontTextCharacter* fontCharacter) {
    if (!fontWriting || !fontString || !fontCharacter) return SCE_FONT_ERROR_INVALID_PARAMETER;
    auto* data = GetStringData(fontString);
    if (!data) return SCE_FONT_ERROR_INVALID_STRING;
    if (auto* previous = GetWritingData(fontWriting)) ReleaseWritingStorage(*previous);
    std::memset(fontWriting, 0, sizeof(*fontWriting));
    auto* writing = new (fontWriting->systemUse) FontWritingData{};
    writing->string = fontString;
    writing->storage = data->storage;
    writing->defaultFont = data->defaultFont;
    if (writing->storage) writing->storage->refCount.fetch_add(1, std::memory_order_relaxed);
    writing->ownsStorage = true;
    writing->writingForm = data->writingForm;
    const FontTextCharacter* characters = TextCharacters(data);
    if (!characters || fontCharacter < characters || fontCharacter >= characters + data->storage->characters.size()) {
        ReleaseWritingStorage(*writing);
        return SCE_FONT_ERROR_INVALID_PARAMETER;
    }
    writing->nextIndex = static_cast<std::size_t>(fontCharacter - characters);
    writing->currentIndex = writing->nextIndex;
    writing->hasCurrentStep = false;
    if (!PopulateWritingStep(*writing, writing->nextIndex)) {
        ReleaseWritingStorage(*writing);
        return SCE_FONT_ERROR_INVALID_PARAMETER;
    }
    writing->penX = 0.0f;
    writing->penY = 0.0f;
    writing->hasAggregateMetrics = false;
    writing->metrics = {};
    return SCE_FONT_OK;
}

int APS5_VABI sceFontWritingGetRenderMetrics(FontWriting* fontWriting, FontWritingMetrics* writingMetrics) {
    if (!fontWriting || !writingMetrics) {
        if (writingMetrics) *writingMetrics = {};
        return SCE_FONT_ERROR_INVALID_PARAMETER;
    }
    const auto* writing = GetWritingData(fontWriting);
    if (!writing) {
        *writingMetrics = {};
        return SCE_FONT_ERROR_INVALID_WRITING;
    }
    *writingMetrics = writing->metrics;
    return SCE_FONT_OK;
}

const FontWritingStep* APS5_VABI sceFontWritingRefersRenderStep(FontWriting* fontWriting) {
    auto* writing = GetWritingData(fontWriting);
    if (!writing || !writing->storage || writing->nextIndex >= writing->storage->characters.size()) return nullptr;
    if (!PopulateWritingStep(*writing, writing->nextIndex)) return nullptr;
    writing->currentIndex = writing->nextIndex;
    writing->hasCurrentStep = true;
    AccumulateWritingMetrics(*writing);
    ++writing->nextIndex;
    return &writing->step;
}

FontTextCharacter* APS5_VABI sceFontWritingRefersRenderStepCharacter(FontWriting* fontWriting, const FontWritingLetterStep** pLetterStep) {
    if (pLetterStep) *pLetterStep = nullptr;
    auto* writing = GetWritingData(fontWriting);
    if (!writing || !writing->hasCurrentStep || !writing->storage || writing->currentIndex >= writing->storage->characters.size()) return nullptr;
    auto& letterStep = writing->letterStep;
    letterStep = {};
    letterStep.x = writing->step.x;
    letterStep.y = writing->step.y;
    letterStep.advanceX = writing->step.advanceX;
    letterStep.advanceY = writing->step.advanceY;
    letterStep.Components.textsCount = 1;
    letterStep.Components.textsIndex = static_cast<std::uint32_t>(writing->currentIndex);
    letterStep.Components.glyphsCount = 1;
    letterStep.Components.glyphsIndex = static_cast<std::uint32_t>(writing->currentIndex);
    letterStep.Components.characterTextCount = static_cast<std::uint8_t>(std::max<std::uint32_t>(writing->step.Profile.characterCount, 1));
    if (pLetterStep) *pLetterStep = &letterStep;
    return &writing->storage->characters[writing->currentIndex];
}

int APS5_VABI sceFontWritingSetMaskInvisible(FontWriting* fontWriting, std::int32_t mask) {
    auto* writing = GetWritingData(fontWriting);
    if (!writing) return SCE_FONT_ERROR_INVALID_WRITING;
    if (mask == WRITING_MASK_FORMAT_CHARACTERS) {
        writing->maskFormatCharacters = false;
        return SCE_FONT_OK;
    }
    if (mask == WRITING_MASK_NONE) {
        writing->maskFormatCharacters = true;
        return SCE_FONT_OK;
    }
    return SCE_FONT_ERROR_INVALID_PARAMETER;
}

}

#pragma GCC visibility pop
