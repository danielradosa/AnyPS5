#include "SceTypes.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
std::int32_t APS5_VABI sceSystemGestureOpen(std::int32_t input_type, const void* param);
int APS5_VABI sceSystemGestureAppendTouchRecognizer(std::int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer);
int APS5_VABI sceSystemGestureRemoveTouchRecognizer(std::int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer);
int APS5_VABI sceSystemGestureResetTouchRecognizer(std::int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer);
int APS5_VABI sceSystemGestureUpdateTouchRecognizerRectangle(std::int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer, const SystemGestureRectangle* rectangle);
int APS5_VABI sceSystemGestureResetPrimitiveTouchRecognizer(std::int32_t gesture_handle);
int APS5_VABI sceSystemGestureUpdateAllTouchRecognizer(std::int32_t gesture_handle);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventsCount(std::int32_t gesture_handle);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByIndex(std::int32_t gesture_handle, std::uint32_t index, SystemGesturePrimitiveTouchEvent* event);
int APS5_VABI sceSystemGestureGetTouchEvents(std::int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, SystemGestureTouchEvent* event_buffer, std::uint32_t capacity_of_buffer, std::uint32_t* number_of_event);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEvents(std::int32_t gesture_handle, SystemGesturePrimitiveTouchEvent* event_buffer, std::uint32_t capacity_of_buffer, std::uint32_t* number_of_event);
int APS5_VABI sceSystemGestureCreateTouchRecognizer(std::int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer, std::int32_t type, const SystemGestureRectangle* rectangle, const void* param);
int APS5_VABI sceSystemGestureGetTouchRecognizerInformation(std::int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, SystemGestureTouchRecognizerInformation* information);
int APS5_VABI sceSystemGestureGetTouchEventByEventID(std::int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, std::uint32_t eventId, SystemGestureTouchEvent* event);
int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(std::int32_t gesture_handle, std::uint16_t primitiveId, SystemGesturePrimitiveTouchEvent* event);
int APS5_VABI sceSystemGestureUpdateTouchRecognizer(std::int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer);
int APS5_VABI sceSystemGestureUpdatePrimitiveTouchRecognizer(std::int32_t gesture_handle, const void* param);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr std::int32_t kBadHandle = 999;
constexpr int kErrInvalidArgument = static_cast<int>(0x80D10002);
constexpr int kErrInvalidHandle = static_cast<int>(0x80D10003);
constexpr int kErrIndexOutOfArray = static_cast<int>(0x80D10005);
constexpr int kErrEventDataNotFound = static_cast<int>(0x80890005);
constexpr std::int32_t kGestureTypeDrag = 2;

bool InformationThrows(std::int32_t handle, const SystemGestureTouchRecognizer* recognizer) {
    SystemGestureTouchRecognizerInformation information{};
    try { sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, &information); } catch (const std::runtime_error&) { return true; }
    return false;
}

void CheckRecognizerInformation(std::int32_t handle) {
    static SystemGestureTouchRecognizer created;
    std::memset(&created, 0xAB, sizeof(created));
    SystemGestureRectangle area{};
    area.x = 10.0f;
    area.y = 20.0f;
    area.width = 300.0f;
    area.height = 400.0f;
    Require(sceSystemGestureCreateTouchRecognizer(handle, &created, kGestureTypeDrag, &area, nullptr) == 0);

    SystemGestureTouchRecognizerInformation information;
    std::memset(&information, 0xCD, sizeof(information));
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, &created, &information) == 0);
    Require(information.gesture_type == kGestureTypeDrag);
    Require(information.rectangle.x == 10.0f && information.rectangle.y == 20.0f);
    Require(information.rectangle.width == 300.0f && information.rectangle.height == 400.0f);
    Require(information.updated_time == 0);
    Require(information.reserve[0] == 0 && information.reserve[255] == 0);

    SystemGestureRectangle moved{};
    moved.x = 1.0f;
    moved.y = 2.0f;
    moved.width = 3.0f;
    moved.height = 4.0f;
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, &created, &moved) == 0);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, &created, nullptr) == kErrInvalidArgument);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, &created, &information) == 0);
    Require(information.gesture_type == kGestureTypeDrag);
    Require(information.rectangle.x == 1.0f && information.rectangle.y == 2.0f);
    Require(information.rectangle.width == 3.0f && information.rectangle.height == 4.0f);

    Require(sceSystemGestureGetTouchRecognizerInformation(kBadHandle, &created, &information) == kErrInvalidHandle);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, nullptr, &information) == kErrInvalidArgument);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, &created, nullptr) == kErrInvalidArgument);

    static SystemGestureTouchRecognizer withoutRectangle;
    Require(sceSystemGestureCreateTouchRecognizer(handle, &withoutRectangle, kGestureTypeDrag, nullptr, nullptr) == 0);
    Require(InformationThrows(handle, &withoutRectangle));
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, &withoutRectangle, &area) == 0);
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, &withoutRectangle, &information) == 0);
    Require(information.rectangle.width == 300.0f && information.rectangle.height == 400.0f);

    static SystemGestureTouchRecognizer neverCreated{};
    Require(InformationThrows(handle, &neverCreated));
}

void CheckEventLookups(std::int32_t handle) {
    static SystemGestureTouchRecognizer recognizer;
    Require(sceSystemGestureCreateTouchRecognizer(handle, &recognizer, kGestureTypeDrag, nullptr, nullptr) == 0);
    Require(sceSystemGestureAppendTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureUpdateAllTouchRecognizer(handle) == 0);

    SystemGestureTouchEvent touchEvent;
    std::memset(&touchEvent, 0xAB, sizeof(touchEvent));
    Require(sceSystemGestureGetTouchEventByEventID(handle, &recognizer, 0, &touchEvent) == kErrEventDataNotFound);
    Require(touchEvent.reserve[0] == 0 && touchEvent.reserve[167] == 0);
    Require(sceSystemGestureGetTouchEventByEventID(handle, &recognizer, 7, nullptr) == kErrEventDataNotFound);
    Require(sceSystemGestureGetTouchEventByEventID(kBadHandle, &recognizer, 0, &touchEvent) == kErrInvalidHandle);
    Require(sceSystemGestureGetTouchEventByEventID(handle, nullptr, 0, &touchEvent) == kErrInvalidArgument);

    SystemGesturePrimitiveTouchEvent primitive;
    std::memset(&primitive, 0xAB, sizeof(primitive));
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle, 0, &primitive) == kErrEventDataNotFound);
    Require(primitive.event_state == 0 && primitive.primitive_id == 0 && primitive.elapsed_time == 0);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(handle, 1, nullptr) == kErrEventDataNotFound);
    Require(sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(kBadHandle, 0, &primitive) == kErrInvalidHandle);

    Require(sceSystemGestureRemoveTouchRecognizer(handle, &recognizer) == 0);
}

bool Untouched(const void* data, std::size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < size; ++i) if (bytes[i] != 0x5a) return false;
    return true;
}

void CheckNoTouchHandleAndNullChecks(std::int32_t handle) {
    SystemGesturePrimitiveTouchEvent primitives[4];
    SystemGestureTouchEvent touchEvents[4];
    std::memset(primitives, 0x5a, sizeof(primitives));
    std::memset(touchEvents, 0x5a, sizeof(touchEvents));
    std::uint32_t count = 7;
    Require(sceSystemGestureGetPrimitiveTouchEvents(handle, primitives, 4, &count) == 0);
    Require(count == 0);

    count = 7;
    Require(sceSystemGestureGetPrimitiveTouchEvents(handle, primitives, 4, nullptr) == kErrInvalidArgument);
    Require(sceSystemGestureGetPrimitiveTouchEvents(handle + 1, primitives, 4, &count) == kErrInvalidHandle);
    Require(count == 7);

    Require(sceSystemGestureGetPrimitiveTouchEventsCount(handle) == 0);
    Require(sceSystemGestureGetPrimitiveTouchEventsCount(handle + 1) == kErrInvalidHandle);

    static SystemGestureTouchRecognizer recognizer{};
    SystemGestureRectangle rectangle{};
    count = 7;
    Require(sceSystemGestureGetTouchEvents(handle, &recognizer, touchEvents, 4, &count) == 0 && count == 0);
    Require(sceSystemGestureGetTouchEvents(handle, &recognizer, touchEvents, 4, nullptr) == kErrInvalidArgument);
    Require(sceSystemGestureGetTouchEvents(handle + 1, &recognizer, touchEvents, 4, &count) == kErrInvalidHandle);
    Require(sceSystemGestureGetPrimitiveTouchEventByIndex(handle, 0, primitives) == kErrIndexOutOfArray);
    Require(sceSystemGestureGetPrimitiveTouchEventByIndex(handle + 1, 0, primitives) == kErrInvalidHandle);
    Require(Untouched(primitives, sizeof(primitives)) && Untouched(touchEvents, sizeof(touchEvents)));

    Require(sceSystemGestureAppendTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureRemoveTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureResetTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureUpdateTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, &recognizer, &rectangle) == 0);
    Require(sceSystemGestureResetPrimitiveTouchRecognizer(handle) == 0);
    Require(sceSystemGestureUpdateAllTouchRecognizer(handle) == 0);
    Require(sceSystemGestureUpdatePrimitiveTouchRecognizer(handle, nullptr) == 0);
    Require(sceSystemGestureAppendTouchRecognizer(handle, nullptr) == kErrInvalidArgument);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, &recognizer, nullptr) == kErrInvalidArgument);
    Require(sceSystemGestureAppendTouchRecognizer(handle + 1, &recognizer) == kErrInvalidHandle);
    Require(sceSystemGestureRemoveTouchRecognizer(handle, nullptr) == kErrInvalidArgument);
    Require(sceSystemGestureResetTouchRecognizer(handle, nullptr) == kErrInvalidArgument);
    Require(sceSystemGestureUpdateTouchRecognizer(handle, nullptr) == kErrInvalidArgument);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, nullptr, &rectangle) == kErrInvalidArgument);
    Require(sceSystemGestureRemoveTouchRecognizer(handle + 1, &recognizer) == kErrInvalidHandle);
    Require(sceSystemGestureResetTouchRecognizer(handle + 1, &recognizer) == kErrInvalidHandle);
    Require(sceSystemGestureUpdateTouchRecognizer(handle + 1, &recognizer) == kErrInvalidHandle);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle + 1, &recognizer, &rectangle) == kErrInvalidHandle);
    Require(sceSystemGestureResetPrimitiveTouchRecognizer(handle + 1) == kErrInvalidHandle);
    Require(sceSystemGestureUpdateAllTouchRecognizer(handle + 1) == kErrInvalidHandle);
    Require(sceSystemGestureUpdatePrimitiveTouchRecognizer(handle + 1, nullptr) == kErrInvalidHandle);
}

}

int main() {
    const std::int32_t handle = sceSystemGestureOpen(0, nullptr);
    Require(handle > 0);

    static SystemGestureTouchRecognizer recognizer{};
    SystemGestureRectangle rectangle{};

    Require(sceSystemGestureAppendTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureAppendTouchRecognizer(kBadHandle, &recognizer) == kErrInvalidHandle);
    Require(sceSystemGestureAppendTouchRecognizer(handle, nullptr) == kErrInvalidArgument);
    Require(sceSystemGestureRemoveTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureResetTouchRecognizer(handle, &recognizer) == 0);
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, &recognizer, &rectangle) == 0);

    Require(sceSystemGestureResetPrimitiveTouchRecognizer(handle) == 0);
    Require(sceSystemGestureResetPrimitiveTouchRecognizer(kBadHandle) == kErrInvalidHandle);
    Require(sceSystemGestureUpdateAllTouchRecognizer(handle) == 0);

    Require(sceSystemGestureGetPrimitiveTouchEventsCount(handle) == 0);
    Require(sceSystemGestureGetPrimitiveTouchEventsCount(kBadHandle) == kErrInvalidHandle);

    SystemGesturePrimitiveTouchEvent primitive{};
    Require(sceSystemGestureGetPrimitiveTouchEventByIndex(handle, 0, &primitive) == kErrIndexOutOfArray);

    SystemGestureTouchEvent touchEvents[4]{};
    std::uint32_t count = 7;
    Require(sceSystemGestureGetTouchEvents(handle, &recognizer, touchEvents, 4, &count) == 0);
    Require(count == 0);

    SystemGesturePrimitiveTouchEvent primitives[4]{};
    count = 7;
    Require(sceSystemGestureGetPrimitiveTouchEvents(handle, primitives, 4, &count) == 0);
    Require(count == 0);

    CheckRecognizerInformation(handle);
    CheckEventLookups(handle);
    CheckNoTouchHandleAndNullChecks(handle);
}
