#include "SceTypes.hpp"
#include "Sigevent.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "prx/libkernel/File/include/ReservedDescriptor.hpp"
#include "prx/libkernel/System/include/SignalQueue.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct GuestMqAttr {
    std::int64_t flags;
    std::int64_t maxmsg;
    std::int64_t msgsize;
    std::int64_t curmsgs;
    std::int64_t reserved[4];
};
static_assert(sizeof(GuestMqAttr) == 64);

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI close_nid_postfix(int descriptor);
int APS5_VABI clock_gettime_nid_postfix(int clock, KernelTimespec* time);
bool GuestThreadIdIsLive_nid_no_patch(std::int64_t tid);
}

namespace {

constexpr int GuestEagain = 35;
constexpr int GuestEbadf = 9;
constexpr int GuestEbusy = 16;
constexpr int GuestEexist = 17;
constexpr int GuestEfault = 14;
constexpr int GuestEinval = 22;
constexpr int GuestEmsgsize = 40;
constexpr int GuestEnametoolong = 63;
constexpr int GuestEnfile = 23;
constexpr int GuestEnoent = 2;
constexpr int GuestEtimedout = 60;
constexpr int GuestOExec = 0x00040000;
constexpr int FileRead = 0x1;
constexpr int FileWrite = 0x2;
constexpr std::size_t NameMax = 255;
constexpr std::int64_t DefaultMaxmsg = 10;
constexpr std::int64_t DefaultMsgsize = 1024;
constexpr std::int64_t MaxMaxmsg = 100;
constexpr std::int64_t MaxMsgsize = 16384;
constexpr int MaxQueues = 100;
constexpr unsigned PriorityMax = 64;
constexpr std::int64_t NanosPerSecond = 1000000000;
constexpr int ClockRealtime = 0;
constexpr std::uint64_t NotifierKeyTag = 0x6d71000000000000ull;

int Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

std::atomic<int> g_queues{0};

struct Message {
    unsigned priority;
    std::vector<char> data;
};

struct Notifier {
    int descriptor;
    GuestSigevent event;
    std::uint64_t key;
};

struct Queue {
    std::mutex lock;
    std::condition_variable changed;
    std::list<Message> messages;
    std::int64_t maxmsg = DefaultMaxmsg;
    std::int64_t msgsize = DefaultMsgsize;
    int receivers = 0;
    int senders = 0;
    std::optional<Notifier> notifier;

    Queue() = default;
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    ~Queue();
};

using QueueRef = std::shared_ptr<Queue>;

struct OpenQueue {
    QueueRef queue;
    int flags = 0;
    std::uint64_t notifierKey = 0;
};

struct Registry {
    std::mutex lock;
    std::map<int, OpenQueue> descriptors;
    std::map<std::string, QueueRef> names;
    std::uint64_t generation = 0;
    bool watching = false;
};

Registry& State() {
    static auto* registry = new Registry();
    return *registry;
}

Queue::~Queue() {
    g_queues.fetch_sub(1);
}

void APS5_VABI DescriptorClosed(int descriptor) {
    OpenQueue closed;
    {
        auto& registry = State();
        const std::lock_guard lock(registry.lock);
        const auto found = registry.descriptors.find(descriptor);
        if (found == registry.descriptors.end()) return;
        closed = std::move(found->second);
        registry.descriptors.erase(found);
        if (closed.notifierKey != 0) {
            const std::lock_guard queueLock(closed.queue->lock);
            if (closed.queue->notifier && closed.queue->notifier->key == closed.notifierKey) closed.queue->notifier.reset();
            GuestSignalCancel_nid_no_patch(closed.notifierKey);
        }
    }
}

std::optional<OpenQueue> Find(int descriptor) {
    auto& registry = State();
    const std::lock_guard lock(registry.lock);
    const auto found = registry.descriptors.find(descriptor);
    if (found == registry.descriptors.end()) return std::nullopt;
    return found->second;
}

int NameError(const char* path) {
    if (path == nullptr) return GuestEfault;
    const std::size_t length = std::strlen(path);
    if (length > NameMax) return GuestEnametoolong;
    if (length < 2 || path[0] != '/' || std::strchr(path + 1, '/') != nullptr) return GuestEinval;
    return 0;
}

void Send(const std::optional<Notifier>& notifier) {
    if (!notifier || !SigeventSignals(notifier->event)) return;
    if (notifier->event.notify == GuestSigevThreadId && !GuestThreadIdIsLive_nid_no_patch(notifier->event.threadId)) return;
    const GuestSignalRecord record{GuestSiMesgq, 0, notifier->event.value, {notifier->descriptor, 0}, notifier->key, nullptr};
    const int saved = *__error_nid_postfix();
    GuestSignalQueue_nid_no_patch(notifier->event.signo, &record);
    *__error_nid_postfix() = saved;
}

std::optional<Notifier> TakeNotifier(Queue& queue) {
    std::optional<Notifier> taken;
    taken.swap(queue.notifier);
    return taken;
}

KernelTimespec RealtimeNow() {
    KernelTimespec now{};
    clock_gettime_nid_postfix(ClockRealtime, &now);
    return now;
}

std::optional<std::chrono::nanoseconds> Remaining(const KernelTimespec& deadline) {
    const KernelTimespec now = RealtimeNow();
    std::int64_t seconds = deadline.tv_sec - now.tv_sec;
    std::int64_t nanos = deadline.tv_nsec - now.tv_nsec;
    if (nanos < 0) {
        --seconds;
        nanos += NanosPerSecond;
    }
    if (seconds < 0 || (seconds == 0 && nanos <= 0)) return std::nullopt;
    constexpr std::int64_t LongestWaitSeconds = 3600;
    if (seconds > LongestWaitSeconds) return std::chrono::seconds(LongestWaitSeconds);
    return std::chrono::seconds(seconds) + std::chrono::nanoseconds(nanos);
}

template <typename Ready>
int WaitFor(Queue& queue, std::unique_lock<std::mutex>& lock, int& waiters, bool waitok, const KernelTimespec* deadline, Ready ready) {
    if (ready()) return 0;
    if (!waitok) return GuestEagain;
    if (deadline != nullptr && (deadline->tv_nsec < 0 || deadline->tv_nsec >= NanosPerSecond)) return GuestEinval;
    ++waiters;
    int error = 0;
    while (!ready()) {
        if (deadline == nullptr) {
            queue.changed.wait(lock);
            continue;
        }
        const auto remaining = Remaining(*deadline);
        if (!remaining) {
            error = GuestEtimedout;
            break;
        }
        queue.changed.wait_for(lock, *remaining);
    }
    --waiters;
    return error;
}

}

extern "C" {

int APS5_VABI kmq_open_nid_postfix(const char* path, int flags, std::uint16_t mode, const GuestMqAttr* attr) {
    static_cast<void>(mode);
    if ((flags & SCE_KERNEL_O_ACCMODE) == SCE_KERNEL_O_ACCMODE || (flags & GuestOExec) != 0) return Fail(GuestEinval);
    const bool create = (flags & SCE_KERNEL_O_CREAT) != 0;
    if (create && attr != nullptr) {
        if (attr->maxmsg <= 0 || attr->maxmsg > MaxMaxmsg) return Fail(GuestEinval);
        if (attr->msgsize <= 0 || attr->msgsize > MaxMsgsize) return Fail(GuestEinval);
    }
    if (const int error = NameError(path)) return Fail(error);
    const int descriptor = GuestReserveDescriptor_nid_no_patch();
    if (descriptor < 0) return -1;
    auto& registry = State();
    std::unique_lock lock(registry.lock);
    if (!registry.watching) {
        GuestWatchDescriptorClose_nid_no_patch(DescriptorClosed);
        registry.watching = true;
    }
    int error = 0;
    QueueRef queue;
    const auto found = registry.names.find(path + 1);
    if (found == registry.names.end()) {
        if (!create) {
            error = GuestEnoent;
        } else if (g_queues.load() >= MaxQueues) {
            error = GuestEnfile;
        } else {
            queue = std::make_shared<Queue>();
            g_queues.fetch_add(1);
            if (attr != nullptr) {
                queue->maxmsg = attr->maxmsg;
                queue->msgsize = attr->msgsize;
            }
            registry.names.emplace(path + 1, queue);
        }
    } else if ((flags & (SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL)) == (SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL)) {
        error = GuestEexist;
    } else {
        queue = found->second;
    }
    if (error != 0) {
        lock.unlock();
        close_nid_postfix(descriptor);
        return Fail(error);
    }
    const int fileFlags = (flags + 1) & (FileRead | FileWrite);
    registry.descriptors[descriptor] = OpenQueue{std::move(queue), fileFlags | (flags & SCE_KERNEL_O_NONBLOCK), 0};
    return descriptor;
}

int APS5_VABI kmq_unlink_nid_postfix(const char* path) {
    if (const int error = NameError(path)) return Fail(error);
    QueueRef released;
    auto& registry = State();
    std::unique_lock lock(registry.lock);
    const auto found = registry.names.find(path + 1);
    if (found == registry.names.end()) return Fail(GuestEnoent);
    released = std::move(found->second);
    registry.names.erase(found);
    lock.unlock();
    return 0;
}

int APS5_VABI kmq_setattr_nid_postfix(int descriptor, const GuestMqAttr* attr, GuestMqAttr* previous) {
    if (attr != nullptr && (attr->flags & ~static_cast<std::int64_t>(SCE_KERNEL_O_NONBLOCK)) != 0) return Fail(GuestEinval);
    GuestMqAttr old{};
    QueueRef queue;
    {
        auto& registry = State();
        const std::lock_guard lock(registry.lock);
        const auto found = registry.descriptors.find(descriptor);
        if (found == registry.descriptors.end()) return Fail(GuestEbadf);
        queue = found->second.queue;
        old.flags = found->second.flags & SCE_KERNEL_O_NONBLOCK;
        if (attr != nullptr) found->second.flags = (found->second.flags & ~SCE_KERNEL_O_NONBLOCK) | static_cast<int>(attr->flags & SCE_KERNEL_O_NONBLOCK);
    }
    {
        const std::lock_guard lock(queue->lock);
        old.maxmsg = queue->maxmsg;
        old.msgsize = queue->msgsize;
        old.curmsgs = static_cast<std::int64_t>(queue->messages.size());
    }
    if (previous != nullptr) *previous = old;
    return 0;
}

int APS5_VABI kmq_timedsend_nid_postfix(int descriptor, const char* message, std::size_t length, unsigned priority, const KernelTimespec* deadline) {
    const auto open = Find(descriptor);
    if (!open || (open->flags & FileWrite) == 0) return Fail(GuestEbadf);
    Queue& queue = *open->queue;
    if (priority >= PriorityMax) return Fail(GuestEinval);
    if (static_cast<std::uint64_t>(length) > static_cast<std::uint64_t>(queue.msgsize)) return Fail(GuestEmsgsize);
    if (message == nullptr && length != 0) return Fail(GuestEfault);
    Message entry{priority, std::vector<char>(message, message + length)};
    std::optional<Notifier> notifier;
    {
        std::unique_lock lock(queue.lock);
        const auto space = [&] { return static_cast<std::int64_t>(queue.messages.size()) < queue.maxmsg; };
        if (const int error = WaitFor(queue, lock, queue.senders, (open->flags & SCE_KERNEL_O_NONBLOCK) == 0, deadline, space)) return Fail(error);
        const auto position = std::find_if(queue.messages.begin(), queue.messages.end(), [&](const Message& queued) { return queued.priority < priority; });
        queue.messages.insert(position, std::move(entry));
        if (queue.receivers != 0) queue.changed.notify_all();
        else notifier = TakeNotifier(queue);
    }
    Send(notifier);
    return 0;
}

std::int64_t APS5_VABI kmq_timedreceive_nid_postfix(int descriptor, char* message, std::size_t length, unsigned* priority, const KernelTimespec* deadline) {
    const auto open = Find(descriptor);
    if (!open || (open->flags & FileRead) == 0) return Fail(GuestEbadf);
    Queue& queue = *open->queue;
    if (static_cast<std::uint64_t>(length) < static_cast<std::uint64_t>(queue.msgsize)) return Fail(GuestEmsgsize);
    Message received;
    std::optional<Notifier> notifier;
    {
        std::unique_lock lock(queue.lock);
        const auto available = [&] { return !queue.messages.empty(); };
        if (const int error = WaitFor(queue, lock, queue.receivers, (open->flags & SCE_KERNEL_O_NONBLOCK) == 0, deadline, available)) return Fail(error);
        received = std::move(queue.messages.front());
        queue.messages.pop_front();
        if (queue.senders != 0) queue.changed.notify_all();
        if (queue.notifier && queue.receivers == 0 && !queue.messages.empty()) notifier = TakeNotifier(queue);
    }
    Send(notifier);
    if (message == nullptr && !received.data.empty()) return Fail(GuestEfault);
    if (!received.data.empty()) std::memcpy(message, received.data.data(), received.data.size());
    if (priority != nullptr) *priority = received.priority;
    return static_cast<std::int64_t>(received.data.size());
}

int APS5_VABI kmq_notify_nid_postfix(int descriptor, const GuestSigevent* event) {
    if (event != nullptr && !SigeventValid(*event)) return Fail(GuestEinval);
    std::optional<Notifier> notifier;
    {
        auto& registry = State();
        const std::lock_guard lock(registry.lock);
        const auto found = registry.descriptors.find(descriptor);
        if (found == registry.descriptors.end()) return Fail(GuestEbadf);
        OpenQueue& open = found->second;
        Queue& queue = *open.queue;
        const std::lock_guard queueLock(queue.lock);
        if (event == nullptr) {
            if (open.notifierKey != 0) {
                if (queue.notifier && queue.notifier->key == open.notifierKey) queue.notifier.reset();
                GuestSignalCancel_nid_no_patch(open.notifierKey);
                open.notifierKey = 0;
            }
            return 0;
        }
        if (queue.notifier) return Fail(GuestEbusy);
        if (open.notifierKey != 0) GuestSignalCancel_nid_no_patch(open.notifierKey);
        else open.notifierKey = NotifierKeyTag | (++registry.generation << 20) | static_cast<std::uint64_t>(descriptor & 0xfffff);
        queue.notifier = Notifier{descriptor, *event, open.notifierKey};
        if (queue.receivers == 0 && !queue.messages.empty()) notifier = TakeNotifier(queue);
    }
    Send(notifier);
    return 0;
}

}
