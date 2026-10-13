#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int ErrorBusy = static_cast<int>(0x80bc0001u);
constexpr int ErrorNotOpened = static_cast<int>(0x80bc0002u);
constexpr int ErrorConnectionFailed = static_cast<int>(0x80bc0004u);
constexpr int ErrorInvalidText = static_cast<int>(0x80bc0006u);
constexpr int ErrorInvalidUserId = static_cast<int>(0x80bc0010u);
constexpr int ErrorInvalidType = static_cast<int>(0x80bc0011u);
constexpr int ErrorInvalidEnterLabel = static_cast<int>(0x80bc0013u);
constexpr int ErrorInvalidInputMethod = static_cast<int>(0x80bc0014u);
constexpr int ErrorInvalidOption = static_cast<int>(0x80bc0015u);
constexpr int ErrorInvalidMaxTextLength = static_cast<int>(0x80bc0016u);
constexpr int ErrorInvalidInputTextBuffer = static_cast<int>(0x80bc0017u);
constexpr int ErrorInvalidPosX = static_cast<int>(0x80bc0018u);
constexpr int ErrorInvalidPosY = static_cast<int>(0x80bc0019u);
constexpr int ErrorInvalidHorizontalAlignment = static_cast<int>(0x80bc001au);
constexpr int ErrorInvalidVerticalAlignment = static_cast<int>(0x80bc001bu);
constexpr int ErrorInvalidExtended = static_cast<int>(0x80bc001cu);
constexpr int ErrorInvalidWork = static_cast<int>(0x80bc0020u);
constexpr int ErrorInvalidHandler = static_cast<int>(0x80bc0022u);
constexpr int ErrorInvalidAddress = static_cast<int>(0x80bc0031u);
constexpr int ErrorNoResourceId = static_cast<int>(0x80bc0023u);
constexpr int ErrorInvalidMode = static_cast<int>(0x80bc0024u);
constexpr int ErrorInvalidParam = static_cast<int>(0x80bc0030u);
constexpr int ErrorInvalidReserved = static_cast<int>(0x80bc0032u);
constexpr uint32_t ValidKeyboardModes = 0x0000007f;
constexpr uint32_t TypeDefault = 0;
constexpr uint32_t TypeBasicLatin = 1;
constexpr uint32_t TypeNumber = 4;
constexpr uint32_t ValidOptions = 0x00007bff;
constexpr uint32_t OptionMultiline = 0x00000001;
constexpr uint32_t OptionPassword = 0x00000004;
constexpr uint32_t OptionUseOver2K = 0x00004000;
constexpr uint32_t MaxEnterLabel = 3;
constexpr uint32_t MaxAlignment = 2;
constexpr uint32_t MaxPanelPriority = 3;
constexpr uint32_t MaxTextLength = 2048;
constexpr uint32_t EventOpen = 0;
constexpr uint32_t PanelTypeHide = 0;
constexpr float ScreenWidth = 1920.0f;
constexpr float ScreenHeight = 1080.0f;
constexpr int32_t UserIdInvalid = -1;
constexpr int32_t UserIdSystem = 0xff;

using GuestEventHandler = void (APS5_VABI*)(void* arg, const ImeEvent* event);

struct OpenIme {
    Param param{};
    std::u16string text;
    uint32_t caret = 0;
    std::vector<ImeEvent> events;
};

std::mutex g_keyboardMutex;
std::set<int32_t> g_openKeyboards;
std::mutex g_imeMutex;
std::optional<OpenIme> g_ime;

void PanelSize(const Param& param, uint32_t* width, uint32_t* height) {
    const uint32_t scale = (param.option & OptionUseOver2K) != 0 ? 2 : 1;
    *width = (param.type == TypeNumber ? 370u : 793u) * scale;
    *height = (param.type == TypeNumber ? 402u : 408u) * scale;
}

float CoordinateScale(const Param& param) {
    return (param.option & OptionUseOver2K) != 0 ? 2.0f : 1.0f;
}

bool InRange(float value, float limit) {
    return value >= 0.0f && value < limit;
}

bool IsValidText(const char16_t* text, uint32_t length, bool multiline) {
    for (uint32_t i = 0; i < length && text[i] != u'\0'; ++i) {
        const char16_t character = text[i];
        if (!multiline && (character == u'\n' || character == u'\r')) return false;
        if ((static_cast<uint16_t>(character) & 0xf800u) == 0xd800u) return false;
    }
    return true;
}

std::u16string BoundedText(const char16_t* text, uint32_t length) {
    uint32_t end = 0;
    while (end < length && text[end] != u'\0') ++end;
    return std::u16string(text, end);
}

template <std::size_t TSize>
bool IsZeroed(const std::int8_t (&bytes)[TSize]) {
    return std::all_of(std::begin(bytes), std::end(bytes), [](std::int8_t value) { return value == 0; });
}

int ValidateOpen(const Param& param, const ExtendedParam* extended) {
    if (param.user_id < 0 || param.user_id == UserIdSystem) return ErrorInvalidUserId;
    if (param.type > TypeNumber) return ErrorInvalidType;
    if (param.enter_label > MaxEnterLabel) return ErrorInvalidEnterLabel;
    if (param.input_method != 0) return ErrorInvalidInputMethod;
    if ((param.option & ~ValidOptions) != 0) return ErrorInvalidOption;
    const bool multiline = (param.option & OptionMultiline) != 0;
    const bool password = (param.option & OptionPassword) != 0;
    if (multiline && password) return ErrorInvalidParam;
    if (multiline && param.type != TypeDefault && param.type != TypeBasicLatin) return ErrorInvalidParam;
    if (password && param.type != TypeBasicLatin && param.type != TypeNumber) return ErrorInvalidParam;
    if (param.max_text_length == 0 || param.max_text_length > MaxTextLength) return ErrorInvalidMaxTextLength;
    if (!param.input_text_buffer) return ErrorInvalidInputTextBuffer;
    const float scale = CoordinateScale(param);
    if (!InRange(param.posx, ScreenWidth * scale)) return ErrorInvalidPosX;
    if (!InRange(param.posy, ScreenHeight * scale)) return ErrorInvalidPosY;
    if (param.horizontal_alignment > MaxAlignment) return ErrorInvalidHorizontalAlignment;
    if (param.vertical_alignment > MaxAlignment) return ErrorInvalidVerticalAlignment;
    if (extended && (extended->priority > MaxPanelPriority || !IsZeroed(extended->reserved))) return ErrorInvalidExtended;
    if (!param.work || (reinterpret_cast<std::uintptr_t>(param.work) & 3u) != 0) return ErrorInvalidWork;
    if (!param.handler) return ErrorInvalidHandler;
    if (!IsZeroed(param.reserved)) return ErrorInvalidReserved;
    if (!IsValidText(param.input_text_buffer, param.max_text_length, multiline)) return ErrorInvalidText;
    return 0;
}

}

extern "C" {

int APS5_VABI sceImeClose_nid_postfix(void) {
 std::lock_guard lock(g_imeMutex);
 if (!g_ime) return ErrorNotOpened;
 g_ime.reset();
 return 0;
}

int APS5_VABI sceImeGetPanelPositionAndForm(PositionAndForm* form) {
 std::lock_guard lock(g_imeMutex);
 if (!g_ime) return ErrorNotOpened;
 if (!form) return ErrorInvalidAddress;
 *form = {};
 form->type = PanelTypeHide;
 form->posx = g_ime->param.posx;
 form->posy = g_ime->param.posy;
 form->horizontal_alignment = g_ime->param.horizontal_alignment;
 form->vertical_alignment = g_ime->param.vertical_alignment;
 PanelSize(g_ime->param, &form->width, &form->height);
 return 0;
}

int APS5_VABI sceImeGetPanelSize(const Param* param, uint32_t* width, uint32_t* height) {
 if (!param || !width || !height) return ErrorInvalidAddress;
 if (param->type > TypeNumber) return ErrorInvalidType;
 if ((param->option & ~ValidOptions) != 0) return ErrorInvalidOption;
 PanelSize(*param, width, height);
 return 0;
}

int APS5_VABI sceImeKeyboardClose(int32_t user_id) {
 std::lock_guard lock(g_keyboardMutex);
 if (g_openKeyboards.erase(user_id) == 0) throw std::logic_error("sceImeKeyboardClose: keyboard not open for user " + std::to_string(user_id));
 return 0;
}

int APS5_VABI sceImeKeyboardGetInfo(uint32_t resource_id, KeyboardInfo* info) {
 (void)resource_id;
 if (!info) return ErrorInvalidAddress;
 std::lock_guard lock(g_keyboardMutex);
 return g_openKeyboards.empty() ? ErrorNotOpened : ErrorNoResourceId;
}

int APS5_VABI sceImeKeyboardGetResourceId(int32_t user_id, KeyboardResourceIdArray* resource_ids) {
 if (!resource_ids) return ErrorInvalidAddress;
 if (user_id == UserIdInvalid) return ErrorInvalidUserId;
 *resource_ids = {};
 resource_ids->user_id = user_id;
 std::lock_guard lock(g_keyboardMutex);
 return g_openKeyboards.contains(user_id) ? ErrorConnectionFailed : ErrorNotOpened;
}

int APS5_VABI sceImeKeyboardOpen(int32_t user_id, const KeyboardParam* param) {
 if (!param) APS5_INVALID_ARG_EX;
 std::lock_guard lock(g_keyboardMutex);
 if (!g_openKeyboards.insert(user_id).second) throw std::logic_error("sceImeKeyboardOpen: keyboard already open for user " + std::to_string(user_id));
 return 0;
}

int APS5_VABI sceImeKeyboardSetMode(int32_t user_id, uint32_t mode) {
 if (user_id == UserIdInvalid) return ErrorInvalidUserId;
 std::lock_guard lock(g_keyboardMutex);
 if (!g_openKeyboards.contains(user_id)) return ErrorNotOpened;
 return (mode & ~ValidKeyboardModes) == 0 ? 0 : ErrorInvalidMode;
}

int APS5_VABI sceImeOpen_nid_postfix(const Param* param, const ExtendedParam* extended) {
 std::lock_guard lock(g_imeMutex);
 if (g_ime) return ErrorBusy;
 if (!param) return ErrorInvalidAddress;
 if (const int error = ValidateOpen(*param, extended)) return error;
 OpenIme ime;
 ime.param = *param;
 ime.text = BoundedText(param->input_text_buffer, param->max_text_length);
 ime.caret = static_cast<uint32_t>(ime.text.size());
 ImeEvent open{};
 open.id = EventOpen;
 open.param.rect.x = param->posx;
 open.param.rect.y = param->posy;
 PanelSize(*param, &open.param.rect.width, &open.param.rect.height);
 ime.events.push_back(open);
 g_ime = std::move(ime);
 return 0;
}

void APS5_VABI sceImeParamInit(Param* param) {
 if (!param) return;
 std::memset(param, 0, sizeof(*param));
 param->user_id = -1;
}

int APS5_VABI sceImeSetCaret(const Caret* caret) {
 std::lock_guard lock(g_imeMutex);
 if (!g_ime) return ErrorNotOpened;
 if (!caret) return ErrorInvalidAddress;
 if (caret->index > g_ime->text.size()) return ErrorInvalidParam;
 g_ime->caret = caret->index;
 return 0;
}

int APS5_VABI sceImeSetText(const char16_t* text, uint32_t length) {
 std::lock_guard lock(g_imeMutex);
 if (!g_ime) return ErrorNotOpened;
 if (!text) return ErrorInvalidAddress;
 if (!IsValidText(text, length, (g_ime->param.option & OptionMultiline) != 0)) return ErrorInvalidText;
 g_ime->text = BoundedText(text, std::min(length, g_ime->param.max_text_length));
 g_ime->caret = std::min(g_ime->caret, static_cast<uint32_t>(g_ime->text.size()));
 return 0;
}

int APS5_VABI sceImeSetTextGeometry(TextAreaMode mode, const TextGeometry* geometry) {
 std::lock_guard lock(g_imeMutex);
 if (!g_ime) return ErrorNotOpened;
 if (!geometry) return ErrorInvalidAddress;
 if (!InRange(geometry->x, ScreenWidth) || !InRange(geometry->y, ScreenHeight)) return ErrorInvalidParam;
 if (mode != TextAreaMode::Select && mode != TextAreaMode::Preedit) return ErrorInvalidParam;
 return 0;
}

int APS5_VABI sceImeUpdate(EventHandler handler) {
 if (!handler) APS5_INVALID_ARG_EX;
 std::vector<ImeEvent> events;
 void* arg = nullptr;
 {
  std::lock_guard lock(g_imeMutex);
  if (g_ime) {
   events.swap(g_ime->events);
   arg = g_ime->param.arg;
  }
 }
 const auto guestHandler = reinterpret_cast<GuestEventHandler>(handler);
 for (const ImeEvent& event : events) guestHandler(arg, &event);
 return 0;
}

}
