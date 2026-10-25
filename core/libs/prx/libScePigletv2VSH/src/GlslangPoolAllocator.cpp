#include "prx/libc/include/HostThreadLocal.hpp"

#include <glslang/Include/Common.h>

namespace glslang {

[[maybe_unused]] static TPoolAllocator& ReplacedGetThreadPoolAllocator();
[[maybe_unused]] static void ReplacedSetThreadPoolAllocator(TPoolAllocator* poolAllocator);

}

#define DISABLE_THREAD_SUPPORT
#define GetThreadPoolAllocator ReplacedGetThreadPoolAllocator
#define SetThreadPoolAllocator ReplacedSetThreadPoolAllocator
#include <glslang/MachineIndependent/PoolAlloc.cpp>
#undef SetThreadPoolAllocator
#undef GetThreadPoolAllocator
#undef DISABLE_THREAD_SUPPORT

namespace glslang {

namespace {

thread_local TPoolAllocator* threadPool = nullptr;

}

TPoolAllocator& GetThreadPoolAllocator() {
    struct DefaultPoolStorage {};
    return threadPool != nullptr ? *threadPool : HostThreadLocal<TPoolAllocator, DefaultPoolStorage>();
}

void SetThreadPoolAllocator(TPoolAllocator* poolAllocator) {
    threadPool = poolAllocator;
}

}
