#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>

struct BgftInitParams {
    void* heap;
    std::uint32_t heapSize;
    std::int32_t reserved;
};

extern "C" {
int APS5_VABI sceBgftServiceInit(const BgftInitParams* params);
int APS5_VABI sceBgftServiceTerm(void);
int APS5_VABI sceBgftServiceDownloadPauseTaskAll(void);
int APS5_VABI sceBgftServiceDownloadResumeTaskAll(void);
int APS5_VABI sceBgftServiceDownloadStartTaskAll(void);
int APS5_VABI sceBgftServiceDownloadStopTaskAll(void);
}

namespace {

void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "libSceBgft check failed: %s\n", what);
    std::abort();
}

bool Rejected(const BgftInitParams* params) {
    try {
        sceBgftServiceInit(params);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

}

int main() {
    constexpr std::uint32_t minimum = 1024 * 1024;
    std::vector<std::uint8_t> heap(minimum);
    BgftInitParams params{heap.data(), minimum, 0};
    Require(Rejected(nullptr), "a null parameter block was accepted");
    BgftInitParams noHeap{nullptr, minimum, 0};
    Require(Rejected(&noHeap), "a null heap was accepted");
    BgftInitParams small{heap.data(), minimum - 1, 0};
    Require(Rejected(&small), "a heap under 1 MiB was accepted");
    BgftInitParams reserved{heap.data(), minimum, 1};
    Require(Rejected(&reserved), "a non-zero reserved word was accepted");
    Require(sceBgftServiceInit(&params) == 0, "initialization failed");
    Require(sceBgftServiceDownloadStartTaskAll() == 0, "starting every task failed");
    Require(sceBgftServiceDownloadPauseTaskAll() == 0, "pausing every task failed");
    Require(sceBgftServiceDownloadResumeTaskAll() == 0, "resuming every task failed");
    Require(sceBgftServiceDownloadStopTaskAll() == 0, "stopping every task failed");
    Require(sceBgftServiceTerm() == 0, "termination failed");
    std::puts("libSceBgft tests passed");
    return 0;
}
