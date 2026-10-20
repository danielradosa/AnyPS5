#include "SceTypes.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
void APS5_VABI sceImeParamInit(Param* param);
int APS5_VABI sceImeGetPanelSize(const Param* param, uint32_t* width, uint32_t* height);
int APS5_VABI sceImeClose_nid_postfix(void);
int APS5_VABI sceImeSetConsecutiveClauseConversionEnabled(bool enabled);
int APS5_VABI sceImeGetPanelPositionAndForm(PositionAndForm* form);
int APS5_VABI sceImeSetCaret(const Caret* caret);
int APS5_VABI sceImeSetText(const char16_t* text, uint32_t length);
int APS5_VABI sceImeSetTextGeometry(TextAreaMode mode, const TextGeometry* geometry);
int APS5_VABI sceImeKeyboardOpen(int32_t user_id, const KeyboardParam* param);
int APS5_VABI sceImeKeyboardClose(int32_t user_id);
int APS5_VABI sceImeKeyboardGetResourceId(int32_t user_id, KeyboardResourceIdArray* resource_ids);
int APS5_VABI sceImeOpen_nid_postfix(const Param* param, const ExtendedParam* extended);
int APS5_VABI sceImeUpdate(EventHandler handler);
}

static_assert(sizeof(Param) == 96);
static_assert(offsetof(Param, option) == 32);
static_assert(offsetof(Param, reserved) == 88);

static void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "%s\n", message);
    std::abort();
}

static void CheckInitialization() {
    struct GuardedParam {
        std::array<unsigned char, 8> before;
        Param param;
        std::array<unsigned char, 8> after;
    } guarded;
    std::memset(&guarded, 0xa5, sizeof(guarded));
    sceImeParamInit(nullptr);
    sceImeParamInit(&guarded.param);
    Require(guarded.param.user_id == -1, "initial user must be invalid");
    const auto* bytes = reinterpret_cast<const unsigned char*>(&guarded.param);
    for (size_t i = sizeof(guarded.param.user_id); i < sizeof(Param); ++i) {
        Require(bytes[i] == 0, "all remaining parameter bytes must be cleared");
    }
    for (unsigned char value : guarded.before) Require(value == 0xa5, "parameter underrun");
    for (unsigned char value : guarded.after) Require(value == 0xa5, "parameter overrun");
    guarded.param.type = 4;
    guarded.param.option = 0x4000;
    guarded.param.max_text_length = 32;
    sceImeParamInit(&guarded.param);
    Require(guarded.param.user_id == -1 && guarded.param.type == 0 &&
            guarded.param.option == 0 && guarded.param.max_text_length == 0,
            "reinitialization must reset previous values");
}

static void CheckPanelSizes() {
    Param param;
    std::memset(&param, 0xa5, sizeof(param));
    for (uint32_t type = 0; type <= 4; ++type) {
        for (uint32_t options : {0u, 0x4000u, 0x7bffu}) {
            param.type = type;
            param.option = options;
            const Param original = param;
            struct GuardedSize { uint32_t before; uint32_t value; uint32_t after; };
            GuardedSize width{0x12345678, 0, 0x87654321};
            GuardedSize height = width;
            Require(sceImeGetPanelSize(&param, &width.value, &height.value) == 0,
                    "valid panel query failed");
            const bool scaled = (options & 0x4000) != 0;
            Require(width.value == (type == 4 ? (scaled ? 740u : 370u) : (scaled ? 1586u : 793u)),
                    "incorrect panel width");
            Require(height.value == (type == 4 ? (scaled ? 804u : 402u) : (scaled ? 816u : 408u)),
                    "incorrect panel height");
            Require(width.before == 0x12345678 && width.after == 0x87654321 &&
                    height.before == 0x12345678 && height.after == 0x87654321,
                    "panel query wrote outside its outputs");
            Require(std::memcmp(&param, &original, sizeof(param)) == 0,
                    "panel query modified parameters");
        }
    }
}

static void CheckErrors() {
    constexpr int InvalidAddress = static_cast<int>(0x80bc0031u);
    constexpr int InvalidType = static_cast<int>(0x80bc0011u);
    constexpr int InvalidOption = static_cast<int>(0x80bc0015u);
    Param param{};
    uint32_t width = 0x12345678;
    uint32_t height = 0x87654321;
    Require(sceImeGetPanelSize(nullptr, &width, &height) == InvalidAddress, "null parameter");
    Require(sceImeGetPanelSize(&param, nullptr, &height) == InvalidAddress, "null width");
    Require(sceImeGetPanelSize(&param, &width, nullptr) == InvalidAddress, "null height");
    for (uint32_t type : {5u, 0xffffffffu}) {
        param.type = type;
        Require(sceImeGetPanelSize(&param, &width, &height) == InvalidType, "invalid type");
    }
    param.type = 0;
    for (uint32_t bit = 0; bit < 32; ++bit) {
        param.option = 1u << bit;
        const bool valid = (param.option & 0x7bffu) != 0;
        uint32_t queriedWidth = width;
        uint32_t queriedHeight = height;
        Require(sceImeGetPanelSize(&param, &queriedWidth, &queriedHeight) == (valid ? 0 : InvalidOption),
                "option bit validation");
        if (!valid) Require(queriedWidth == width && queriedHeight == height, "outputs changed on invalid option");
    }
    param.type = 5;
    param.option = 0x80000000;
    Require(sceImeGetPanelSize(&param, &width, &height) == InvalidType, "type error precedence");
    Require(sceImeGetPanelSize(&param, nullptr, &height) == InvalidAddress, "address error precedence");
    Require(width == 0x12345678 && height == 0x87654321, "outputs changed on error");
}

static void CheckClosedPanel() {
    constexpr int notOpened = static_cast<int>(0x80bc0002u);
    Caret caret{};
    TextGeometry geometry{};
    const char16_t text[] = u"text";
    Require(sceImeSetCaret(&caret) == notOpened, "caret needs an open panel");
    Require(sceImeSetCaret(nullptr) == notOpened, "caret checks the panel first");
    Require(sceImeSetText(text, 4) == notOpened, "text needs an open panel");
    Require(sceImeSetTextGeometry(TextAreaMode::Edit, &geometry) == notOpened, "geometry needs an open panel");
    Require(sceImeClose_nid_postfix() == notOpened, "closing needs an open panel");
    PositionAndForm form;
    std::memset(&form, 0xa5, sizeof(form));
    Require(sceImeGetPanelPositionAndForm(&form) == notOpened, "panel position needs an open panel");
    Require(sceImeGetPanelPositionAndForm(nullptr) == notOpened, "panel position checks the panel first");
    const auto* formBytes = reinterpret_cast<const unsigned char*>(&form);
    for (size_t i = 0; i < sizeof(form); ++i) Require(formBytes[i] == 0xa5, "panel position written without an open panel");
}

static void CheckKeyboardResourceIds() {
    constexpr int NotOpened = static_cast<int>(0x80bc0002u);
    constexpr int ConnectionFailed = static_cast<int>(0x80bc0004u);
    constexpr int InvalidUserId = static_cast<int>(0x80bc0010u);
    constexpr int InvalidAddress = static_cast<int>(0x80bc0031u);
    KeyboardResourceIdArray ids;
    std::memset(&ids, 0xa5, sizeof(ids));
    Require(sceImeKeyboardGetResourceId(1, nullptr) == InvalidAddress, "null resource id array");
    Require(sceImeKeyboardGetResourceId(-1, &ids) == InvalidUserId, "invalid user");
    const auto* bytes = reinterpret_cast<const unsigned char*>(&ids);
    for (size_t i = 0; i < sizeof(ids); ++i) Require(bytes[i] == 0xa5, "outputs changed on argument error");
    Require(sceImeKeyboardGetResourceId(1, &ids) == NotOpened, "keyboard not opened");
    Require(ids.user_id == 1, "user not reported for an unopened keyboard");
    for (uint32_t id : ids.resource_id) Require(id == 0, "resource id reported for an unopened keyboard");
    KeyboardParam param{};
    Require(sceImeKeyboardOpen(1, &param) == 0, "keyboard open failed");
    std::memset(&ids, 0xa5, sizeof(ids));
    Require(sceImeKeyboardGetResourceId(1, &ids) == ConnectionFailed, "a keyboard was reported as connected");
    Require(ids.user_id == 1, "user not reported");
    for (uint32_t id : ids.resource_id) Require(id == 0, "resource id reported without a keyboard");
    Require(sceImeKeyboardGetResourceId(2, &ids) == NotOpened, "keyboard of another user reported as opened");
    Require(ids.user_id == 2, "other user not reported");
    Require(sceImeKeyboardClose(1) == 0, "keyboard close failed");
    Require(sceImeKeyboardGetResourceId(1, &ids) == NotOpened, "closed keyboard reported as opened");
}

struct Received {
    int calls = 0;
    void* arg = nullptr;
    ImeEvent event{};
};

static void APS5_VABI RecordEvent(void* arg, const ImeEvent* event) {
    auto* received = static_cast<Received*>(arg);
    ++received->calls;
    received->arg = arg;
    received->event = *event;
}

static EventHandler RecordingHandler() {
    return reinterpret_cast<EventHandler>(&RecordEvent);
}

static void CheckOpenPanel() {
    constexpr int busy = static_cast<int>(0x80bc0001u);
    constexpr int notOpened = static_cast<int>(0x80bc0002u);
    constexpr int invalidText = static_cast<int>(0x80bc0006u);
    constexpr int invalidUserId = static_cast<int>(0x80bc0010u);
    constexpr int invalidType = static_cast<int>(0x80bc0011u);
    constexpr int invalidEnterLabel = static_cast<int>(0x80bc0013u);
    constexpr int invalidInputMethod = static_cast<int>(0x80bc0014u);
    constexpr int invalidOption = static_cast<int>(0x80bc0015u);
    constexpr int invalidMaxTextLength = static_cast<int>(0x80bc0016u);
    constexpr int invalidInputTextBuffer = static_cast<int>(0x80bc0017u);
    constexpr int invalidPosX = static_cast<int>(0x80bc0018u);
    constexpr int invalidPosY = static_cast<int>(0x80bc0019u);
    constexpr int invalidHorizontalAlignment = static_cast<int>(0x80bc001au);
    constexpr int invalidVerticalAlignment = static_cast<int>(0x80bc001bu);
    constexpr int invalidExtended = static_cast<int>(0x80bc001cu);
    constexpr int invalidWork = static_cast<int>(0x80bc0020u);
    constexpr int invalidHandler = static_cast<int>(0x80bc0022u);
    constexpr int invalidParam = static_cast<int>(0x80bc0030u);
    constexpr int invalidAddress = static_cast<int>(0x80bc0031u);
    constexpr int invalidReserved = static_cast<int>(0x80bc0032u);

    alignas(8) static unsigned char work[0x5000];
    char16_t buffer[33] = u"name";
    char16_t newline[] = u"a\nb";
    Received received;
    Param param;
    sceImeParamInit(&param);
    param.user_id = 1;
    param.max_text_length = 32;
    param.input_text_buffer = buffer;
    param.posx = 100.0f;
    param.posy = 200.0f;
    param.work = work;
    param.arg = &received;
    param.handler = RecordingHandler();

    const auto opens = [&](auto change, int expected) {
        Param changed = param;
        change(changed);
        return sceImeOpen_nid_postfix(&changed, nullptr) == expected;
    };
    Require(sceImeOpen_nid_postfix(nullptr, nullptr) == invalidAddress, "open without parameters");
    Require(opens([](Param& p) { p.user_id = -1; }, invalidUserId), "open for an invalid user");
    Require(opens([](Param& p) { p.user_id = 0xff; }, invalidUserId), "open for the system user");
    Require(opens([](Param& p) { p.type = 5; }, invalidType), "open with an invalid type");
    Require(opens([](Param& p) { p.enter_label = 4; }, invalidEnterLabel), "open with an invalid enter label");
    Require(opens([](Param& p) { p.input_method = 1; }, invalidInputMethod), "open with an invalid input method");
    Require(opens([](Param& p) { p.option = 0x400; }, invalidOption), "open with an invalid option");
    Require(opens([](Param& p) { p.option = 5; p.type = 1; }, invalidParam), "multiline password");
    Require(opens([](Param& p) { p.option = 1; p.type = 4; }, invalidParam), "multiline number");
    Require(opens([](Param& p) { p.option = 4; p.type = 0; }, invalidParam), "password of the default type");
    Require(opens([](Param& p) { p.max_text_length = 0; }, invalidMaxTextLength), "open without text length");
    Require(opens([](Param& p) { p.max_text_length = 2049; }, invalidMaxTextLength), "open with too long a text");
    Require(opens([](Param& p) { p.input_text_buffer = nullptr; }, invalidInputTextBuffer), "open without a text buffer");
    Require(opens([](Param& p) { p.posx = 1920.0f; }, invalidPosX), "open past the screen width");
    Require(opens([](Param& p) { p.posy = -1.0f; }, invalidPosY), "open above the screen");
    Require(opens([](Param& p) { p.horizontal_alignment = 3; }, invalidHorizontalAlignment), "open with an invalid horizontal alignment");
    Require(opens([](Param& p) { p.vertical_alignment = 3; }, invalidVerticalAlignment), "open with an invalid vertical alignment");
    Require(opens([](Param& p) { p.work = nullptr; }, invalidWork), "open without work memory");
    Require(opens([](Param& p) { p.work = work + 1; }, invalidWork), "open with unaligned work memory");
    Require(opens([](Param& p) { p.handler = nullptr; }, invalidHandler), "open without a handler");
    Require(opens([](Param& p) { p.reserved[3] = 1; }, invalidReserved), "open with reserved bytes set");
    Require(opens([&](Param& p) { p.input_text_buffer = newline; }, invalidText), "a newline in a single line text");
    Require(opens([&](Param& p) { p.input_text_buffer = newline; p.option = 1; }, 0), "a newline in a multiline text");
    Require(sceImeClose_nid_postfix() == 0, "the multiline panel closes");
    Require(opens([](Param& p) { p.posx = 3000.0f; p.posy = 2000.0f; p.option = 0x4000; }, 0), "4K coordinates with the over 2K option");
    Require(sceImeClose_nid_postfix() == 0, "the 4K panel closes");
    ExtendedParam extended{};
    extended.priority = 4;
    Require(sceImeOpen_nid_postfix(&param, &extended) == invalidExtended, "open with an invalid panel priority");
    extended.priority = 0;
    extended.reserved[0] = 1;
    Require(sceImeOpen_nid_postfix(&param, &extended) == invalidExtended, "open with reserved extended bytes set");
    extended.reserved[0] = 0;

    Require(sceImeOpen_nid_postfix(&param, &extended) == 0, "the panel opens");
    Require(sceImeOpen_nid_postfix(&param, nullptr) == busy, "a second panel is busy");
    Require(sceImeUpdate(RecordingHandler()) == 0, "the update delivers events");
    Require(received.calls == 1 && received.arg == &received, "the open event reaches the handler with its argument");
    Require(received.event.id == 0, "the first event opens the panel");
    Require(received.event.param.rect.x == 100.0f && received.event.param.rect.y == 200.0f &&
            received.event.param.rect.width == 793 && received.event.param.rect.height == 408, "the open event carries the panel rectangle");
    Require(sceImeUpdate(RecordingHandler()) == 0 && received.calls == 1, "events are delivered once");

    PositionAndForm form{};
    Require(sceImeGetPanelPositionAndForm(nullptr) == invalidAddress, "panel position needs an output");
    Require(sceImeGetPanelPositionAndForm(&form) == 0, "panel position of the open panel");
    Require(form.posx == 100.0f && form.posy == 200.0f && form.width == 793 && form.height == 408, "panel position and size");

    Caret caret{};
    caret.index = 4;
    Require(sceImeSetCaret(nullptr) == invalidAddress, "caret needs a value");
    Require(sceImeSetCaret(&caret) == 0, "caret at the end of the text");
    caret.index = 5;
    Require(sceImeSetCaret(&caret) == invalidParam, "caret past the text");
    Require(sceImeSetText(nullptr, 3) == invalidAddress, "text needs a value");
    Require(sceImeSetText(newline, 3) == invalidText, "a newline in single line text");
    Require(sceImeSetText(u"longer name", 11) == 0, "text replaced");
    Require(sceImeSetCaret(&caret) == 0, "caret inside the longer text");
    Require(sceImeSetText(u"ab", 2) == 0, "text shortened");
    Require(sceImeSetCaret(&caret) == invalidParam, "caret past the shortened text");
    Require(std::memcmp(buffer, u"ab", sizeof(u"ab")) == 0, "the title's buffer holds the text set last");

    TextGeometry geometry{10.0f, 10.0f, 100, 20};
    Require(sceImeSetTextGeometry(TextAreaMode::Preedit, nullptr) == invalidAddress, "geometry needs a value");
    Require(sceImeSetTextGeometry(TextAreaMode::Edit, &geometry) == invalidParam, "geometry of an edit area");
    Require(sceImeSetTextGeometry(TextAreaMode::Select, &geometry) == 0, "geometry of a selection");
    geometry.x = 1920.0f;
    Require(sceImeSetTextGeometry(TextAreaMode::Preedit, &geometry) == invalidParam, "geometry off the screen");

    Require(sceImeClose_nid_postfix() == 0, "the panel closes");
    Require(sceImeClose_nid_postfix() == notOpened, "a closed panel does not close again");
    Require(sceImeSetText(u"ab", 2) == notOpened, "text after closing");
    Require(sceImeUpdate(RecordingHandler()) == 0 && received.calls == 1, "no events after closing");
}

static void CheckConsecutiveClauseConversion() {
    Require(sceImeSetConsecutiveClauseConversionEnabled(true) == 0, "enable must succeed");
    Require(sceImeSetConsecutiveClauseConversionEnabled(false) == 0, "disable must succeed");
}

int main() {
    CheckInitialization();
    CheckPanelSizes();
    CheckErrors();
    CheckClosedPanel();
    CheckKeyboardResourceIds();
    CheckOpenPanel();
    CheckConsecutiveClauseConversion();
}
