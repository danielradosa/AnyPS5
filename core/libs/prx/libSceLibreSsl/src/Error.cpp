#include "prx/libSceLibreSsl/include/Internal.hpp"

#include <deque>

namespace LibreSsl {

namespace {

constexpr std::size_t QueueCapacity = 16;

std::deque<std::uint64_t>& Queue() {
    thread_local std::deque<std::uint64_t> queue;
    return queue;
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
