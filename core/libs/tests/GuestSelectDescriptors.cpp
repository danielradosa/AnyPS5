#include "SceTypes.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

struct GuestTimeval {
    std::int64_t seconds;
    std::int64_t microseconds;
};

struct GuestDescriptorSet {
    std::array<std::uint64_t, 1024 / 64> words{};

    void Add(int descriptor) {
        words[descriptor / 64] |= (std::uint64_t{1} << (descriptor % 64));
    }

    bool Has(int descriptor) const {
        return ((words[descriptor / 64] >> (descriptor % 64)) & 1u) != 0;
    }
};

extern "C" {
int APS5_VABI select_nid_postfix(int, void*, void*, void*, const void*);
int APS5_VABI pipe_nid_postfix(int*);
int APS5_VABI close_nid_postfix(int);
int APS5_VABI open_nid_postfix(const char*, int, int);
int APS5_VABI mkdir_nid_postfix(const char*, unsigned short);
int APS5_VABI kqueue_nid_postfix();
int APS5_VABI kevent_nid_postfix(int, const KernelEvent*, int, KernelEvent*, int, const KernelTimespec*);
std::int64_t APS5_VABI write_nid_postfix(int, const char*, std::int64_t);
std::int64_t APS5_VABI read_nid_postfix(int, void*, std::uint64_t);
int* APS5_VABI __error_nid_postfix();
}

static void Require(bool value) { if (!value) std::abort(); }

constexpr int GuestEbadf = 9;
constexpr int Directory = 0x00020000;
constexpr std::int16_t EvfiltUser = -11;
constexpr std::uint16_t EvAdd = 0x0001;
constexpr std::uint16_t EvClear = 0x0020;
constexpr std::uint32_t NoteTrigger = 0x01000000;

static int Select(int descriptor, GuestDescriptorSet* read, GuestDescriptorSet* write, GuestDescriptorSet* except, const GuestTimeval* limit) {
    return select_nid_postfix(descriptor + 1, read != nullptr ? read->words.data() : nullptr, write != nullptr ? write->words.data() : nullptr,
        except != nullptr ? except->words.data() : nullptr, limit);
}

static int UserEvent(int kq, std::uint16_t flags, std::uint32_t fflags) {
    KernelEvent change{};
    change.ident = 7;
    change.filter = EvfiltUser;
    change.flags = flags;
    change.fflags = fflags;
    return kevent_nid_postfix(kq, &change, 1, nullptr, 0, nullptr);
}

static void Pipes() {
    const GuestTimeval now{0, 0};
    int ends[2]{-1, -1};
    Require(pipe_nid_postfix(ends) == 0);
    GuestDescriptorSet read{};
    read.Add(ends[0]);
    Require(Select(ends[0], &read, nullptr, nullptr, &now) == 0 && !read.Has(ends[0]));
    std::thread late([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        Require(write_nid_postfix(ends[1], "ping", 4) == 4);
    });
    read.Add(ends[0]);
    const GuestTimeval second{1, 0};
    const auto start = std::chrono::steady_clock::now();
    Require(Select(ends[0], &read, nullptr, nullptr, &second) == 1 && read.Has(ends[0]));
    Require(std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(20));
    late.join();
    GuestDescriptorSet write{};
    GuestDescriptorSet except{};
    write.Add(ends[1]);
    except.Add(ends[1]);
    Require(Select(ends[1], nullptr, &write, &except, &now) == 1 && write.Has(ends[1]) && !except.Has(ends[1]));
    char buffer[8]{};
    Require(read_nid_postfix(ends[0], buffer, sizeof(buffer)) == 4);
    read.Add(ends[0]);
    Require(Select(ends[0], &read, nullptr, nullptr, &now) == 0);
    Require(close_nid_postfix(ends[1]) == 0);
    read.Add(ends[0]);
    Require(Select(ends[0], &read, nullptr, nullptr, &now) == 1 && read.Has(ends[0]));
    Require(close_nid_postfix(ends[0]) == 0);
    read.Add(ends[0]);
    Require(Select(ends[0], &read, nullptr, nullptr, &now) == -1 && *__error_nid_postfix() == GuestEbadf);
}

static void Directories() {
    const GuestTimeval now{0, 0};
    const auto path = std::string("anyps5-select-") + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    Require(mkdir_nid_postfix(path.c_str(), 0755) == 0);
    const int directory = open_nid_postfix(path.c_str(), Directory, 0);
    Require(directory >= 0);
    GuestDescriptorSet read{};
    GuestDescriptorSet write{};
    GuestDescriptorSet except{};
    read.Add(directory);
    write.Add(directory);
    except.Add(directory);
    Require(Select(directory, &read, &write, &except, &now) == 2);
    Require(read.Has(directory) && write.Has(directory) && !except.Has(directory));
    Require(close_nid_postfix(directory) == 0);
    Require(std::filesystem::remove(path));
}

static void Queues() {
    const GuestTimeval now{0, 0};
    const int kq = kqueue_nid_postfix();
    Require(kq >= 0);
    Require(UserEvent(kq, EvAdd | EvClear, 0) == 0);
    GuestDescriptorSet read{};
    GuestDescriptorSet write{};
    read.Add(kq);
    write.Add(kq);
    Require(Select(kq, &read, &write, nullptr, &now) == 0 && !read.Has(kq) && !write.Has(kq));
    std::thread trigger([kq] {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        Require(UserEvent(kq, 0, NoteTrigger) == 0);
    });
    read.Add(kq);
    const GuestTimeval second{1, 0};
    Require(Select(kq, &read, nullptr, nullptr, &second) == 1 && read.Has(kq));
    trigger.join();
    KernelEvent event{};
    const KernelTimespec zero{0, 0};
    Require(kevent_nid_postfix(kq, nullptr, 0, &event, 1, &zero) == 1 && event.ident == 7);
    read.Add(kq);
    Require(Select(kq, &read, nullptr, nullptr, &now) == 0);
    Require(close_nid_postfix(kq) == 0);
}

int main() {
    Pipes();
    Directories();
    Queues();
    return 0;
}
