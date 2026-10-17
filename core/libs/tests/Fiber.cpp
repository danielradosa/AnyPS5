#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <xmmintrin.h>
#ifdef _WIN32
#include <windows.h>
#endif

using Entry = void (APS5_VABI*)(std::uint64_t, std::uint64_t);

extern "C" {
std::int32_t APS5_VABI _sceFiberInitializeImpl_nid_postfix(FiberObject*, const char*, Entry, std::uint64_t, void*, std::uint64_t, const void*, std::uint32_t);
std::int32_t APS5_VABI _sceFiberInitializeWithInternalOptionImpl(FiberObject*, const char*, Entry, std::uint64_t, void*, std::uint64_t, const void*, std::uint32_t, std::uint32_t);
std::int32_t APS5_VABI _sceFiberAttachContextAndRun(FiberObject*, void*, std::uint64_t, std::uint64_t, std::uint64_t*);
std::int32_t APS5_VABI _sceFiberAttachContextAndSwitch(FiberObject*, void*, std::uint64_t, std::uint64_t, std::uint64_t*);
std::int32_t APS5_VABI sceFiberFinalize(FiberObject*);
std::int32_t APS5_VABI sceFiberRun_nid_postfix(FiberObject*, std::uint64_t, std::uint64_t*);
std::int32_t APS5_VABI sceFiberSwitch(FiberObject*, std::uint64_t, std::uint64_t*);
std::int32_t APS5_VABI sceFiberReturnToThread(std::uint64_t, std::uint64_t*);
std::int32_t APS5_VABI sceFiberGetSelf(FiberObject**);
std::int32_t APS5_VABI sceFiberGetInfo(FiberObject*, FiberInfo*);
std::int32_t APS5_VABI sceFiberGetThreadFramePointerAddress(std::uint64_t*);
std::int32_t APS5_VABI _sceFiberGetThreadFramePointerAddress(std::uint64_t*);
std::int32_t APS5_VABI sceFiberStartContextSizeCheck(std::uint32_t);
std::int32_t APS5_VABI sceFiberStopContextSizeCheck(void);
}

static void Require(bool value, const char* what) {
    if (!value) { std::fprintf(stderr, "Fiber check failed: %s\n", what); std::abort(); }
}

namespace {

constexpr std::int32_t FiberErrorNull = static_cast<std::int32_t>(0x80590001);
constexpr std::int32_t FiberErrorAlignment = static_cast<std::int32_t>(0x80590002);
constexpr std::int32_t FiberErrorRange = static_cast<std::int32_t>(0x80590003);
constexpr std::int32_t FiberErrorInvalid = static_cast<std::int32_t>(0x80590004);
constexpr std::int32_t FiberErrorPermission = static_cast<std::int32_t>(0x80590005);
constexpr std::int32_t FiberErrorState = static_cast<std::int32_t>(0x80590006);

alignas(16) std::array<unsigned char, 64 * 1024> g_firstContext;
alignas(16) std::array<unsigned char, 64 * 1024> g_secondContext;
struct alignas(8) FiberStorage {
    unsigned char bytes[0x100];
};

FiberStorage g_firstStorage;
FiberStorage g_checkedStorage;
alignas(16) std::array<unsigned char, 16 * 1024> g_checkedContext;
std::uint64_t g_threadFramePointer = 0;
FiberStorage g_secondStorage;
auto* const g_first = reinterpret_cast<FiberObject*>(&g_firstStorage);
auto* const g_second = reinterpret_cast<FiberObject*>(&g_secondStorage);

void CheckActiveContext(FiberObject* fiber, std::uint64_t expectedSize) {
    FiberInfo info{};
    info.size = sizeof(info);
    Require(sceFiberGetInfo(fiber, &info) == 0, "active fiber context info");
    Require(info.size_context == expectedSize, "reported context keeps the requested size");
    volatile std::uint64_t marker = 0;
    const auto address = reinterpret_cast<std::uintptr_t>(&marker);
    const auto low = reinterpret_cast<std::uintptr_t>(info.addr_context);
    const auto high = low + info.size_context;
    Require(address >= low && address < high, "reported fiber context contains the executing stack");
    for (auto cursor = address; cursor < high; cursor += sizeof(std::uint64_t)) {
        const volatile auto word = *reinterpret_cast<const volatile std::uint64_t*>(cursor);
        (void)word;
    }
}

void APS5_VABI FirstEntry(std::uint64_t argOnInitialize, std::uint64_t argOnRun) {
    Require(argOnInitialize == 11 && argOnRun == 100, "first fiber arguments");
    FiberObject* self = nullptr;
    Require(sceFiberGetSelf(&self) == 0 && self == g_first, "first fiber self");
    CheckActiveContext(g_first, g_firstContext.size());
    std::uint64_t framePointer = 0;
    Require(sceFiberGetThreadFramePointerAddress(nullptr) == FiberErrorNull, "frame pointer null");
    Require(sceFiberGetThreadFramePointerAddress(&framePointer) == 0 && framePointer == g_threadFramePointer, "thread frame pointer");
    framePointer = 0;
    Require(_sceFiberGetThreadFramePointerAddress(nullptr) == FiberErrorNull, "underscore frame pointer null");
    Require(_sceFiberGetThreadFramePointerAddress(&framePointer) == 0 && framePointer == g_threadFramePointer, "underscore thread frame pointer");
    volatile double carried = 1.5;
    std::uint64_t received = 0;
    Require(sceFiberSwitch(g_second, 200, &received) == 0, "switch to second fiber");
    Require(received == 300 && carried == 1.5, "switch back to first fiber");
    CheckActiveContext(g_first, g_firstContext.size());
    for (;;) {
        Require(sceFiberReturnToThread(received + 1, &received) == 0, "first fiber return");
    }
}

void APS5_VABI SecondEntry(std::uint64_t argOnInitialize, std::uint64_t argOnRun) {
    Require(argOnInitialize == 22 && argOnRun == 200, "second fiber arguments");
    CheckActiveContext(g_second, g_secondContext.size());
    Require(sceFiberSwitch(g_second, 0, nullptr) == FiberErrorState, "switch to self");
    _mm_setcsr(_mm_getcsr() | 0x8000u);
    std::uint64_t received = 0;
    Require(sceFiberReturnToThread(250, &received) == 0 && received == 260, "second fiber resumed on another thread");
    CheckActiveContext(g_second, g_secondContext.size());
    Require((_mm_getcsr() & 0x8000u) != 0, "second fiber keeps its MXCSR");
    Require(sceFiberSwitch(g_first, 300, nullptr) == 0, "switch to first fiber");
    Require(false, "second fiber resumed after its last switch");
}

void APS5_VABI CheckedEntry(std::uint64_t, std::uint64_t) {
    volatile unsigned char used[4096];
    for (auto& byte : used) byte = 1;
    std::uint64_t received = 0;
    for (;;) sceFiberReturnToThread(0, &received);
}

constexpr std::size_t SmallContextSize = 2048;
constexpr unsigned char SmallGuardFill = 0xa5;
alignas(16) std::array<unsigned char, 64 * 1024> g_smallArea;
FiberStorage g_smallStorage;

void APS5_VABI SmallEntry(std::uint64_t, std::uint64_t) {
    std::uint64_t received = 0;
    for (;;) sceFiberReturnToThread(0, &received);
}

void RunSmallContextFirst() {
    auto* small = reinterpret_cast<FiberObject*>(&g_smallStorage);
    auto* context = g_smallArea.data() + g_smallArea.size() - SmallContextSize;
    g_smallArea.fill(SmallGuardFill);
    Require(sceFiberReturnToThread(0, nullptr) == FiberErrorPermission, "return to thread outside a fiber");
    Require(_sceFiberInitializeImpl_nid_postfix(small, "small", SmallEntry, 0, context, SmallContextSize, nullptr, 0) == 0, "initialize small");
    Require(sceFiberRun_nid_postfix(small, 0, nullptr) == 0, "first fiber run of the process on a 2 KiB context");
    Require(sceFiberRun_nid_postfix(small, 0, nullptr) == 0, "resume on a 2 KiB context");
    Require(sceFiberFinalize(small) == 0, "finalize small");
    for (auto* byte = g_smallArea.data(); byte != context; ++byte) {
        Require(*byte == SmallGuardFill, "nothing is written below a 2 KiB context");
    }
}

constexpr std::size_t HostCallContextSize = 16 * 1024;
constexpr std::size_t HostCallStack = 256 * 1024;
constexpr std::size_t HostCallFrameSize = 4096;
constexpr unsigned char HostCallGuardFill = 0x5c;
alignas(16) std::array<unsigned char, HostCallStack + 64 * 1024> g_hostCallArea;
FiberStorage g_hostCallStorage;

std::size_t DescendHostStack(std::size_t remaining);
std::size_t (*volatile g_descendHostStack)(std::size_t) = DescendHostStack;

std::size_t DescendHostStack(std::size_t remaining) {
    volatile unsigned char frame[HostCallFrameSize];
    frame[0] = 0x11;
    frame[HostCallFrameSize - 1] = 0x22;
    const std::size_t frames = remaining > HostCallFrameSize ? g_descendHostStack(remaining - HostCallFrameSize) : 0;
    Require(frame[0] == 0x11 && frame[HostCallFrameSize - 1] == 0x22, "host call frame intact");
    return frames + 1;
}

#ifdef _WIN32
void CheckWindowsHostStack() {
    const auto* tib = reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
    const auto low = reinterpret_cast<std::uintptr_t>(tib->StackLimit);
    const auto high = reinterpret_cast<std::uintptr_t>(tib->StackBase);
    const volatile unsigned char marker = 0;
    const auto frame = reinterpret_cast<std::uintptr_t>(&marker);
    Require(frame > low && frame < high && high - low >= HostCallContextSize + HostCallStack, "the thread's stack bounds are the fiber's host stack");
}
#endif

void APS5_VABI HostCallEntry(std::uint64_t, std::uint64_t) {
    CheckActiveContext(reinterpret_cast<FiberObject*>(&g_hostCallStorage), HostCallContextSize);
#ifdef _WIN32
    CheckWindowsHostStack();
#endif
    Require(g_descendHostStack(HostCallStack) == HostCallStack / HostCallFrameSize, "host call depth");
    std::uint64_t received = 0;
    for (;;) sceFiberReturnToThread(0, &received);
}

alignas(16) std::array<unsigned char, 64 * 1024> g_chainContext;
FiberStorage g_chainStorage;
bool g_chainEnded = false;

void APS5_VABI FrameChainEntry(std::uint64_t, std::uint64_t) {
    auto* frame = static_cast<std::uintptr_t*>(__builtin_frame_address(0));
    const auto low = reinterpret_cast<std::uintptr_t>(frame);
    const auto high = low + (1u << 20);
    for (int depth = 0; depth < 100 && frame != nullptr; ++depth) {
        const auto address = reinterpret_cast<std::uintptr_t>(frame);
        Require(address >= low && address + 2 * sizeof(std::uintptr_t) <= high, "frame chain stays on the fiber's stack");
        frame = reinterpret_cast<std::uintptr_t*>(frame[0]);
        Require(frame == nullptr || reinterpret_cast<std::uintptr_t>(frame) > address, "frame chain climbs the stack");
    }
    g_chainEnded = frame == nullptr;
    std::uint64_t received = 0;
    for (;;) sceFiberReturnToThread(0, &received);
}

void RunHostCallOnSmallContext() {
    auto* fiber = reinterpret_cast<FiberObject*>(&g_hostCallStorage);
    auto* context = g_hostCallArea.data() + g_hostCallArea.size() - HostCallContextSize;
    g_hostCallArea.fill(HostCallGuardFill);
    Require(_sceFiberInitializeImpl_nid_postfix(fiber, "host call", HostCallEntry, 0, context, HostCallContextSize, nullptr, 0) == 0, "initialize host call fiber");
    Require(sceFiberRun_nid_postfix(fiber, 0, nullptr) == 0, "run a 256 KiB host call from a 16 KiB fiber");
    Require(sceFiberFinalize(fiber) == 0, "finalize host call fiber");
    for (auto* byte = g_hostCallArea.data(); byte != context; ++byte) {
        Require(*byte == HostCallGuardFill, "a host call from a 16 KiB fiber writes nothing below its context");
    }
}

void RunFrameChain() {
    auto* fiber = reinterpret_cast<FiberObject*>(&g_chainStorage);
    Require(_sceFiberInitializeImpl_nid_postfix(fiber, "chain", FrameChainEntry, 0, g_chainContext.data(), g_chainContext.size(), nullptr, 0) == 0, "initialize chain");
    Require(sceFiberRun_nid_postfix(fiber, 0, nullptr) == 0, "run chain");
    Require(g_chainEnded, "frame chain of a fiber ends with a null frame");
    Require(sceFiberFinalize(fiber) == 0, "finalize chain");
}

[[gnu::noinline]] std::int32_t RunFirst(std::uint64_t* returned) {
    g_threadFramePointer = reinterpret_cast<std::uint64_t>(__builtin_frame_address(0));
    const std::int32_t result = sceFiberRun_nid_postfix(g_first, 100, returned);
    asm volatile("" ::: "memory");
    return result;
}

alignas(16) std::array<unsigned char, 16 * 1024> g_flagContext;
FiberStorage g_flagStorage;
std::uint32_t g_fiberCsr = 0;
std::uint16_t g_fiberControl = 0;

std::uint16_t FpuControl() {
    std::uint16_t control = 0;
    asm volatile("fnstcw %0" : "=m"(control));
    return control;
}

void SetFpuControl(std::uint16_t control) {
    asm volatile("fldcw %0" : : "m"(control));
}

void APS5_VABI FpuEntry(std::uint64_t, std::uint64_t) {
    g_fiberCsr = _mm_getcsr();
    g_fiberControl = FpuControl();
    std::uint64_t received = 0;
    for (;;) sceFiberReturnToThread(0, &received);
}

void RunFpuFiber(FiberObject* fiber, std::uint32_t flags, std::uint32_t* csrAfter, std::uint16_t* controlAfter) {
    const auto threadCsr = _mm_getcsr();
    const auto threadControl = FpuControl();
    Require(_sceFiberInitializeWithInternalOptionImpl(fiber, "fpu", FpuEntry, 0, g_flagContext.data(), g_flagContext.size(), nullptr, flags, 0) == 0, "initialize fpu fiber");
    _mm_setcsr(0x1f80);
    SetFpuControl(0x027f);
    const auto result = sceFiberRun_nid_postfix(fiber, 0, nullptr);
    *csrAfter = _mm_getcsr();
    *controlAfter = FpuControl();
    _mm_setcsr(threadCsr);
    SetFpuControl(threadControl);
    Require(result == 0, "run fpu fiber");
    Require(sceFiberFinalize(fiber) == 0, "finalize fpu fiber");
}

void CheckInternalOptionFlags() {
    auto* flagged = reinterpret_cast<FiberObject*>(&g_flagStorage);
    FiberInfo info{};
    info.size = sizeof(info);
    bool rejected = false;
    try {
        _sceFiberInitializeWithInternalOptionImpl(flagged, "unknown", FpuEntry, 0, g_flagContext.data(), g_flagContext.size(), nullptr, 0x2, 0);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    Require(rejected, "unknown internal option flag throws");

    Require(_sceFiberInitializeWithInternalOptionImpl(flagged, "sized", CheckedEntry, 0, g_flagContext.data(), g_flagContext.size(), nullptr, 0x10, 0) == 0, "initialize with the size check flag");
    Require(sceFiberGetInfo(flagged, &info) == 0 && info.size_context_margin == g_flagContext.size(), "size check flag fills the context");
    Require(sceFiberRun_nid_postfix(flagged, 0, nullptr) == 0, "run fiber size checked by flag");
    Require(sceFiberGetInfo(flagged, &info) == 0, "flag checked info");
    Require(info.size_context_margin > 0 && info.size_context_margin < g_flagContext.size() - 4096, "size check flag margin below the used stack");
    Require(sceFiberFinalize(flagged) == 0, "finalize fiber size checked by flag");

    std::uint32_t csrAfter = 0;
    std::uint16_t controlAfter = 0;
    RunFpuFiber(flagged, 0x1, &csrAfter, &controlAfter);
    Require(g_fiberCsr == 0x1f80 && g_fiberControl == 0x027f, "fiber without the fpu flag starts with the caller's MXCSR and x87 control word");
    RunFpuFiber(flagged, 0x101, &csrAfter, &controlAfter);
    Require(g_fiberCsr == 0x9fc0 && g_fiberControl == 0x037f, "fpu flag starts the fiber with MXCSR 0x9fc0 and x87 control word 0x037f");
    Require(csrAfter == 0x1f80 && controlAfter == 0x027f, "caller keeps its MXCSR and x87 control word after an fpu flag fiber");
}

alignas(16) std::array<unsigned char, 16 * 1024> g_lateContext;
alignas(16) std::array<unsigned char, 16 * 1024> g_otherContext;
FiberStorage g_lateStorage;
auto* const g_late = reinterpret_cast<FiberObject*>(&g_lateStorage);

void APS5_VABI LateEntry(std::uint64_t argOnInitialize, std::uint64_t argOnRun) {
    Require(argOnInitialize == 33 && argOnRun == 44, "attached fiber arguments");
    Require(_sceFiberAttachContextAndRun(g_late, nullptr, 0, 0, nullptr) == FiberErrorPermission, "attach and run inside a fiber");
    std::uint64_t received = 0;
    Require(sceFiberReturnToThread(45, &received) == 0 && received == 55, "attached fiber resumed by a plain run");
    for (;;) sceFiberReturnToThread(56, &received);
}

void CheckAttachContextAndRun() {
    FiberInfo info{};
    info.size = sizeof(info);
    Require(_sceFiberInitializeImpl_nid_postfix(g_late, "late", LateEntry, 33, nullptr, 0, nullptr, 0) == 0, "initialize without a context");
    Require(sceFiberGetInfo(g_late, &info) == 0 && info.addr_context == nullptr && info.size_context == 0, "no context before attach");
    bool threw = false;
    try {
        sceFiberRun_nid_postfix(g_late, 44, nullptr);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "running a fiber without a context throws");
    Require(_sceFiberAttachContextAndRun(nullptr, g_lateContext.data(), g_lateContext.size(), 44, nullptr) == FiberErrorNull, "attach and run a null fiber");
    Require(_sceFiberAttachContextAndRun(reinterpret_cast<FiberObject*>(reinterpret_cast<unsigned char*>(&g_lateStorage) + 4), nullptr, 0, 44, nullptr) == FiberErrorAlignment, "attach and run a misaligned fiber");
    Require(_sceFiberAttachContextAndRun(g_late, g_lateContext.data() + 8, 1024, 44, nullptr) == FiberErrorAlignment, "attach a misaligned context");
    Require(_sceFiberAttachContextAndRun(g_late, g_lateContext.data(), 256, 44, nullptr) == FiberErrorRange, "attach a context below 512 bytes");
    Require(_sceFiberAttachContextAndRun(g_late, g_lateContext.data(), 1032, 44, nullptr) == FiberErrorInvalid, "attach a size that is not a multiple of 16");
    Require(_sceFiberAttachContextAndRun(g_late, nullptr, 1024, 44, nullptr) == FiberErrorInvalid, "attach a size without a context");
    Require(_sceFiberAttachContextAndRun(g_late, g_lateContext.data(), 0, 44, nullptr) == FiberErrorInvalid, "attach a context without a size");
    Require(sceFiberGetInfo(g_late, &info) == 0 && info.addr_context == nullptr, "rejected attaches leave no context");
    std::uint64_t returned = 0;
    Require(_sceFiberAttachContextAndRun(g_late, g_lateContext.data(), g_lateContext.size(), 44, &returned) == 0 && returned == 45, "attach and run");
    Require(sceFiberGetInfo(g_late, &info) == 0 && info.addr_context != nullptr && info.size_context == g_lateContext.size(), "attached context reported");
    Require(_sceFiberAttachContextAndRun(g_late, g_otherContext.data(), g_otherContext.size(), 0, nullptr) == FiberErrorInvalid, "second attach");
    Require(_sceFiberAttachContextAndRun(g_late, nullptr, 0, 55, &returned) == 0 && returned == 56, "attach and run without a new context");
    Require(sceFiberFinalize(g_late) == 0, "finalize attached fiber");
    Require(_sceFiberAttachContextAndRun(g_late, g_otherContext.data(), g_otherContext.size(), 0, nullptr) == FiberErrorInvalid, "attach and run a finalized fiber");

    Require(_sceFiberInitializeWithInternalOptionImpl(g_late, "late checked", CheckedEntry, 0, nullptr, 0, nullptr, 0x10, 0) == 0, "initialize size checked without a context");
    Require(sceFiberGetInfo(g_late, &info) == 0 && info.size_context_margin == ~0ull, "no margin without a context");
    Require(_sceFiberAttachContextAndRun(g_late, g_otherContext.data(), g_otherContext.size(), 0, nullptr) == 0, "attach and run size checked");
    Require(sceFiberGetInfo(g_late, &info) == 0, "attached size checked info");
    Require(info.size_context_margin > 0 && info.size_context_margin < g_otherContext.size() - 4096, "attached context filled for the size check");
    Require(sceFiberFinalize(g_late) == 0, "finalize attached size checked fiber");
}

alignas(16) std::array<unsigned char, 16 * 1024> g_switcherContext;
alignas(16) std::array<unsigned char, 16 * 1024> g_switchedContext;
FiberStorage g_switcherStorage;
FiberStorage g_switchedStorage;
auto* const g_switcher = reinterpret_cast<FiberObject*>(&g_switcherStorage);
auto* const g_switched = reinterpret_cast<FiberObject*>(&g_switchedStorage);

void APS5_VABI SwitchedEntry(std::uint64_t argOnInitialize, std::uint64_t argOnRun) {
    Require(argOnInitialize == 77 && argOnRun == 66, "switched fiber arguments");
    Require(sceFiberSwitch(g_switcher, 88, nullptr) == 0, "switch back from the attached fiber");
    Require(false, "attached fiber resumed after its last switch");
}

void APS5_VABI SwitcherEntry(std::uint64_t, std::uint64_t) {
    Require(_sceFiberAttachContextAndSwitch(g_switched, g_switchedContext.data() + 8, g_switchedContext.size(), 0, nullptr) == FiberErrorAlignment, "switch attach of a misaligned context");
    Require(_sceFiberAttachContextAndSwitch(g_switched, g_switchedContext.data(), 256, 0, nullptr) == FiberErrorRange, "switch attach below 512 bytes");
    Require(_sceFiberAttachContextAndSwitch(g_switched, nullptr, 1024, 0, nullptr) == FiberErrorInvalid, "switch attach of a size without a context");
    std::uint64_t received = 0;
    Require(_sceFiberAttachContextAndSwitch(g_switched, g_switchedContext.data(), g_switchedContext.size(), 66, &received) == 0 && received == 88, "attach and switch");
    Require(_sceFiberAttachContextAndSwitch(g_switched, g_switchedContext.data(), g_switchedContext.size(), 0, nullptr) == FiberErrorInvalid, "second switch attach");
    for (;;) sceFiberReturnToThread(99, &received);
}

void CheckAttachContextAndSwitch() {
    FiberInfo info{};
    info.size = sizeof(info);
    Require(_sceFiberInitializeImpl_nid_postfix(g_switcher, "switcher", SwitcherEntry, 0, g_switcherContext.data(), g_switcherContext.size(), nullptr, 0) == 0, "initialize switcher");
    Require(_sceFiberInitializeImpl_nid_postfix(g_switched, "switched", SwitchedEntry, 77, nullptr, 0, nullptr, 0) == 0, "initialize switched without a context");
    Require(_sceFiberAttachContextAndSwitch(nullptr, g_switchedContext.data(), g_switchedContext.size(), 0, nullptr) == FiberErrorNull, "attach and switch a null fiber");
    Require(_sceFiberAttachContextAndSwitch(reinterpret_cast<FiberObject*>(reinterpret_cast<unsigned char*>(&g_switchedStorage) + 4), nullptr, 0, 0, nullptr) == FiberErrorAlignment, "attach and switch a misaligned fiber");
    Require(_sceFiberAttachContextAndSwitch(g_switched, g_switchedContext.data(), g_switchedContext.size(), 0, nullptr) == FiberErrorPermission, "attach and switch outside a fiber");
    Require(sceFiberGetInfo(g_switched, &info) == 0 && info.addr_context == nullptr, "nothing attached outside a fiber");
    std::uint64_t returned = 0;
    Require(sceFiberRun_nid_postfix(g_switcher, 0, &returned) == 0 && returned == 99, "run switcher");
    Require(sceFiberGetInfo(g_switched, &info) == 0 && info.addr_context != nullptr && info.size_context == g_switchedContext.size(), "switch attached the context");
    Require(sceFiberFinalize(g_switcher) == 0, "finalize switcher");
    Require(sceFiberFinalize(g_switched) == 0, "finalize switched");
    Require(_sceFiberAttachContextAndSwitch(g_switched, nullptr, 0, 0, nullptr) == FiberErrorInvalid, "attach and switch a finalized fiber");
}

}

int main() {
    RunSmallContextFirst();
    FiberObject* self = nullptr;
    Require(sceFiberGetSelf(&self) == FiberErrorPermission, "no fiber on the thread");
    Require(_sceFiberInitializeImpl_nid_postfix(g_first, "first", FirstEntry, 11, g_firstContext.data(), g_firstContext.size(), nullptr, 0) == 0, "initialize first");
    Require(_sceFiberInitializeImpl_nid_postfix(g_second, "second", SecondEntry, 22, g_secondContext.data(), g_secondContext.size(), nullptr, 0) == 0, "initialize second");
    const auto threadCsr = _mm_getcsr();
    std::uint64_t returned = 0;
    std::uint64_t framePointer = 0;
    Require(sceFiberGetThreadFramePointerAddress(&framePointer) == FiberErrorPermission, "frame pointer outside a fiber");
    Require(_sceFiberGetThreadFramePointerAddress(&framePointer) == FiberErrorPermission, "underscore frame pointer outside a fiber");
    Require(RunFirst(&returned) == 0 && returned == 250, "run first until second returns");
    Require(_mm_getcsr() == threadCsr, "thread MXCSR restored after the fibers");
    std::thread([&] {
        Require(sceFiberRun_nid_postfix(g_second, 260, &returned) == 0 && returned == 301, "resume second on another thread");
    }).join();
    Require(sceFiberFinalize(g_first) == 0, "finalize first");
    Require(sceFiberFinalize(g_second) == 0, "finalize second");

    auto* checked = reinterpret_cast<FiberObject*>(&g_checkedStorage);
    FiberInfo info{};
    info.size = sizeof(info);
    Require(sceFiberStopContextSizeCheck() == FiberErrorState, "stop before start");
    Require(sceFiberStartContextSizeCheck(1) == FiberErrorInvalid, "start with flags");
    Require(_sceFiberInitializeImpl_nid_postfix(checked, "unchecked", CheckedEntry, 0, g_checkedContext.data(), g_checkedContext.size(), nullptr, 0) == 0, "initialize unchecked");
    Require(sceFiberGetInfo(checked, &info) == 0 && info.size_context_margin == ~0ull, "no margin without size check");
    Require(sceFiberFinalize(checked) == 0, "finalize unchecked");
    Require(sceFiberStartContextSizeCheck(0) == 0, "start size check");
    Require(sceFiberStartContextSizeCheck(0) == FiberErrorState, "start twice");
    Require(_sceFiberInitializeImpl_nid_postfix(checked, "checked", CheckedEntry, 0, g_checkedContext.data(), g_checkedContext.size(), nullptr, 0) == 0, "initialize checked");
    Require(sceFiberGetInfo(checked, &info) == 0 && info.size_context_margin == g_checkedContext.size(), "untouched context is all margin");
    Require(sceFiberRun_nid_postfix(checked, 0, nullptr) == 0, "run checked");
    Require(sceFiberGetInfo(checked, &info) == 0, "checked info");
    Require(info.size_context_margin > 0 && info.size_context_margin < g_checkedContext.size() - 4096, "margin below the used stack");
    Require(info.size_context_margin % 8 == 0, "margin in whole words");
    Require(sceFiberStopContextSizeCheck() == 0, "stop size check");

    CheckInternalOptionFlags();
    CheckAttachContextAndRun();
    CheckAttachContextAndSwitch();
    RunHostCallOnSmallContext();
    RunFrameChain();
}
