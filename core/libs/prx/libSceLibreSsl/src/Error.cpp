#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/HostThreadLocal.hpp"

#include <deque>
#include <stdexcept>

namespace LibreSsl {

namespace {

constexpr std::size_t QueueCapacity = 16;

struct QueueTag;

std::deque<std::uint64_t>& Queue() {
    return HostThreadLocal<std::deque<std::uint64_t>, QueueTag>();
}

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

}
