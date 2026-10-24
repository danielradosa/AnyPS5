#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/HostThreadLocal.hpp"

#include <cstdio>
#include <cstring>
#include <deque>
#include <stdexcept>

namespace LibreSsl {

namespace {

constexpr std::size_t QueueCapacity = 15;

struct QueueTag;

std::deque<std::uint64_t>& Queue() {
    return HostThreadLocal<std::deque<std::uint64_t>, QueueTag>();
}

char errorBuffer[256];

}

void PushError(int library, int function, int reason) {
    auto& queue = Queue();
    if (queue.size() == QueueCapacity) queue.pop_front();
    queue.push_back((static_cast<std::uint64_t>(library) & 0xff) << 24 | (static_cast<std::uint64_t>(function) & 0xfff) << 12 |
        (static_cast<std::uint64_t>(reason) & 0xfff));
}

std::uint64_t PeekLastError() {
    const auto& queue = Queue();
    return queue.empty() ? 0 : queue.back();
}

}

using namespace LibreSsl;

extern "C" {

std::uint64_t LibreSslPeekLastError_nid_no_patch() {
    return PeekLastError();
}

std::uint64_t APS5_VABI ERR_get_error_nid_postfix() {
    auto& queue = Queue();
    if (queue.empty()) return 0;
    const auto code = queue.front();
    queue.pop_front();
    return code;
}

std::uint64_t APS5_VABI ERR_peek_error_nid_postfix() {
    const auto& queue = Queue();
    return queue.empty() ? 0 : queue.front();
}

void APS5_VABI ERR_clear_error_nid_postfix() {
    Queue().clear();
}

void APS5_VABI ERR_remove_thread_state_nid_postfix(const void* tid) {
    if (tid) throw std::invalid_argument("ERR_remove_thread_state: removing another thread's queue is not supported");
    Queue().clear();
}

void APS5_VABI ERR_free_strings_nid_postfix() {
}

void APS5_VABI ERR_error_string_n_nid_postfix(std::uint64_t error, char* buffer, std::size_t length) {
    if (buffer == nullptr && length != 0) NotImplemented_nid_no_patch(__func__);
    const int library = (error >> 24) & 255;
    const int function = (error >> 12) & 4095;
    const int reason = error & 4095;
    const int written = std::snprintf(buffer, length, "error:%08llX:lib(%d):func(%d):reason(%d)",
        static_cast<unsigned long long>(error), library, function, reason);
    if (written >= 0 && static_cast<std::size_t>(written) >= length && length > 4) {
        char* next = buffer;
        for (std::size_t index = 0; index < 4; ++index) {
            char* colon = std::strchr(next, ':');
            char* last = buffer + length - 5 + index;
            if (colon == nullptr || colon > last) {
                colon = last;
                *colon = ':';
            }
            next = colon + 1;
        }
    }
}

char* APS5_VABI ERR_error_string_nid_postfix(std::uint64_t error, char* buffer) {
    if (buffer == nullptr) buffer = errorBuffer;
    ERR_error_string_n_nid_postfix(error, buffer, sizeof(errorBuffer));
    return buffer;
}

}
