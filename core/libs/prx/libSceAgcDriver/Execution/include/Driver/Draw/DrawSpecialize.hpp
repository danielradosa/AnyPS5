#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_DRIVER_DRAWSPECIALIZE_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_DRIVER_DRAWSPECIALIZE_HPP

#include "prx/libSceAgcDriver/Execution/include/Driver/Dispatch/DispatchCache.hpp"
#include "Recompiler.hpp"
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace AgcDriver::DriverDetail {

struct DrawDecode;

struct SpecializeTarget {
    ShaderRecompiler::SpirvTarget target;
    std::vector<std::uint32_t> capabilities;
    std::vector<std::string> extensions;
    std::vector<std::string_view> extensionViews;
};

struct SpecializeJob {
    std::uint64_t drawKey = 0;
    std::size_t stage = 0;
    std::weak_ptr<DispatchVariant> variant;
    std::shared_ptr<const DrawDecode> decode;
    std::shared_ptr<const SpecializeTarget> target;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> runs;
    std::vector<std::uint32_t> words;
    std::vector<std::uint32_t> userData;
    ShaderRecompiler::RecompileRequest request{};
};

enum SpecializeCount : std::size_t { SpecializeParked, SpecializeQueued, SpecializeDropped, SpecializeDone, SpecializePublished, SpecializeStale, SpecializeFailed, SpecializeNotSpecialized, SpecializeCountCount };

struct SpecializeCounters {
    std::array<std::uint64_t, SpecializeCountCount> counts{};
    std::uint64_t jobNanoseconds = 0;
    std::size_t waiting = 0;
    std::size_t specializedVariants = 0;
};

struct SpecializeState {
    static constexpr std::size_t QueueBound = 4096;
    std::mutex mutex;
    std::condition_variable changed;
    std::condition_variable idle;
    std::deque<std::shared_ptr<SpecializeJob>> queue;
    std::vector<pthread_t> threads;
    std::size_t running = 0;
    std::atomic<bool> open{false};
    std::atomic<bool> stop{false};
    std::atomic<std::size_t> depth{0};
    std::mutex targetMutex;
    std::uint64_t targetSerial = 0;
    std::shared_ptr<const SpecializeTarget> target;
    std::array<std::atomic<std::uint64_t>, SpecializeCountCount> counts{};
    std::atomic<std::uint64_t> jobNanoseconds{0};
};

}

#endif
