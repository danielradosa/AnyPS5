#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_DRIVER_DRAWPREP_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_DRIVER_DRAWPREP_HPP

#include "prx/libSceAgcDriver/Execution/include/QueueState.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Context.hpp"
#include "Recompiler.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <thread>

namespace AgcDriver::DriverDetail {

struct Submission;

enum DrawPrepCount : std::size_t {
    PrepSubmissions, PrepDraws, PrepInserted, PrepRefreshed, PrepCurrent, PrepTooLate, PrepIndirect, PrepPrechecked, PrepNotPrepared,
    PrepAbortPending, PrepAbortUnmapped, PrepFailed, PrepRaced, PrepUncacheable,
    PrepStopPredication, PrepStopConditional, PrepStopRegisterCopy, PrepStopReplay, PrepStopOther, PrepWaitsGated, PrepThrottled, DrawPrepCountCount
};

struct DrawPrepAbort {};

struct DrawPrepState {
    static constexpr std::size_t Never = std::numeric_limits<std::size_t>::max();
    std::mutex mutex;
    std::condition_variable changed;
    std::condition_variable released;
    const Submission* submission = nullptr;
    QueueState scratch;
    std::optional<ShaderRecompiler::SpirvTarget> target;
    std::optional<Graphics::Context> context;
    std::uint64_t deviceSerial = 0;
    bool pipelineLibraries = false;
    bool listRestart = false;
    std::uint64_t generation = 0;
    bool busy = false;
    bool stop = false;
    std::thread thread;
    std::atomic<bool> cancel{false};
    alignas(64) std::atomic<std::size_t> cursor{0};
    std::atomic<std::size_t> wakeAt{Never};
    alignas(64) std::atomic<std::size_t> helperCursor{Never};
    std::atomic<std::size_t> abortReason{0};
    std::array<std::uint64_t, DrawPrepCountCount> counts{};
    // Where the pending-write aborts come from (helper thread only, APS5_PROFILE_DRAW): reads in the
    // vertex decode or in the capture, by what overlapped (bit 0 a recorded write, 1 a pending storage
    // image, 2 a unit shadow); capture policy aborts; and how many draws ahead of the worker.
    bool capturing = false;
    std::size_t drawsAhead = 0;
    std::array<std::array<std::uint64_t, 8>, 2> readAborts{};
    std::uint64_t policyAborts = 0;
    std::array<std::uint64_t, 6> abortsAhead{};
    double busyMs = 0, gatedMs = 0, throttledMs = 0;
    std::chrono::steady_clock::time_point reported = std::chrono::steady_clock::now();
};

}

#endif
