#include "SceTypes.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

struct NpWebApi2MemoryPoolStats {
    std::size_t pool_size;
    std::size_t max_inuse_size;
    std::size_t current_inuse_size;
    std::int32_t reserved;
};

struct NpWebApi2IntInitializeArgs {
    std::int32_t lib_http_ctx_id;
    std::int32_t reserved;
    std::uint64_t pool_size;
    const char* name;
    std::uint64_t struct_size;
};

struct NpWebApi2IntInitialize2Args {
    std::int32_t lib_http_ctx_id;
    std::int32_t reserved;
    std::uint64_t pool_size;
    const char* name;
    std::uint32_t push_config_group;
    std::int32_t reserved2;
    std::uint64_t struct_size;
};

extern "C" {
int APS5_VABI sceNpWebApi2Initialize(int, std::size_t);
int APS5_VABI sceNpWebApi2InitializeForPresence(int, std::uint64_t);
int APS5_VABI sceNpWebApi2IntInitialize(const NpWebApi2IntInitializeArgs*);
int APS5_VABI sceNpWebApi2IntInitialize2(const NpWebApi2IntInitialize2Args*);
int APS5_VABI sceNpWebApi2CreateMultipartRequest(int, const char*, const char*, const char*, std::int64_t*);
int APS5_VABI sceNpWebApi2SendMultipartRequest(std::int64_t, int, void*, std::uint64_t, NpWebApi2ResponseInformationOption*);
int APS5_VABI sceNpWebApi2Terminate(int);
int APS5_VABI sceNpWebApi2GetMemoryPoolStats(int, NpWebApi2MemoryPoolStats*);
int APS5_VABI sceNpWebApi2PushEventCreateHandle(int);
int APS5_VABI sceNpWebApi2PushEventAbortHandle(int, int);
int APS5_VABI sceNpWebApi2PushEventSetHandleTimeout(int, int, std::uint32_t);
int APS5_VABI sceNpWebApi2PushEventDeleteHandle(int, int);
int APS5_VABI sceNpWebApi2PushEventCreateFilter(int, int, const char*, std::uint32_t, const void*, std::size_t);
int APS5_VABI sceNpWebApi2CreateUserContext(int, int);
int APS5_VABI sceNpWebApi2CreateRequest(int, const char*, const char*, const char*, const void*, std::int64_t*);
int APS5_VABI sceNpWebApi2SetRequestTimeout(std::int64_t, std::uint32_t);
int APS5_VABI sceNpWebApi2DeleteRequest(std::int64_t);
int APS5_VABI sceNpWebApi2AddWebTraceTag(std::int64_t, const char*);
}

static constexpr int LibContextNotFound = static_cast<int>(0x80553404);
static constexpr int Aborted = static_cast<int>(0x80553409);
static constexpr int HandleNotFound = static_cast<int>(0x8055340D);

static void Require(bool value) { if (!value) std::abort(); }

template <typename TCall>
static bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

static void TestPushEventAbortHandle() {
    const int context = sceNpWebApi2Initialize(1, 0x8000);
    const int other = sceNpWebApi2Initialize(1, 0x8000);
    const std::uint64_t filterParam[2]{};
    const int handle = sceNpWebApi2PushEventCreateHandle(context);
    const int kept = sceNpWebApi2PushEventCreateHandle(context);
    Require(handle > 0 && kept > 0 && handle != kept);
    Require(sceNpWebApi2PushEventCreateFilter(context, handle, nullptr, 0, filterParam, 1) > 0);
    Require(sceNpWebApi2PushEventAbortHandle(context, handle) == 0);
    Require(sceNpWebApi2PushEventCreateFilter(context, handle, nullptr, 0, filterParam, 1) == Aborted);
    Require(sceNpWebApi2PushEventAbortHandle(context, handle) == 0);
    Require(sceNpWebApi2PushEventCreateFilter(context, kept, nullptr, 0, filterParam, 1) > 0);
    Require(sceNpWebApi2PushEventAbortHandle(other, handle) == HandleNotFound);
    Require(sceNpWebApi2PushEventAbortHandle(context, 0x7FFFFFFF) == HandleNotFound);
    Require(sceNpWebApi2PushEventAbortHandle(0x7FFFFFFF, handle) == LibContextNotFound);
    Require(sceNpWebApi2PushEventCreateHandle(0x7FFFFFFF) == LibContextNotFound);
    Require(sceNpWebApi2PushEventDeleteHandle(context, handle) == 0);
    Require(sceNpWebApi2PushEventAbortHandle(context, handle) == HandleNotFound);
    Require(sceNpWebApi2PushEventDeleteHandle(context, handle) == HandleNotFound);
    Require(sceNpWebApi2Terminate(context) == 0);
    Require(sceNpWebApi2PushEventAbortHandle(context, kept) == LibContextNotFound);
    Require(sceNpWebApi2Terminate(other) == 0);
}

static void TestInitializeVariants() {
    constexpr int invalidArgument = static_cast<int>(0x80553402);
    NpWebApi2MemoryPoolStats stats{};
    const int presence = sceNpWebApi2InitializeForPresence(1, 0x4000);
    Require(presence > 0);
    Require(sceNpWebApi2GetMemoryPoolStats(presence, &stats) == 0 && stats.pool_size == 0x4000 && stats.current_inuse_size == 0);
    Require(sceNpWebApi2PushEventCreateHandle(presence) > 0);

    Require(sceNpWebApi2IntInitialize(nullptr) == invalidArgument);
    NpWebApi2IntInitializeArgs args{1, 0, 0x6000, "internal", sizeof(NpWebApi2IntInitializeArgs) + 8};
    Require(sceNpWebApi2IntInitialize(&args) == invalidArgument);
    args.struct_size = sizeof(NpWebApi2IntInitializeArgs);
    const int internal = sceNpWebApi2IntInitialize(&args);
    Require(internal > 0 && internal != presence);
    Require(sceNpWebApi2GetMemoryPoolStats(internal, &stats) == 0 && stats.pool_size == 0x6000);
    args.name = nullptr;
    const int unnamed = sceNpWebApi2IntInitialize(&args);
    Require(unnamed > 0 && unnamed != internal);

    Require(sceNpWebApi2IntInitialize2(nullptr) == invalidArgument);
    NpWebApi2IntInitialize2Args args2{1, 0, 0x7000, "internal2", 3, 0, sizeof(NpWebApi2IntInitializeArgs)};
    Require(sceNpWebApi2IntInitialize2(&args2) == invalidArgument);
    args2.struct_size = sizeof(NpWebApi2IntInitialize2Args);
    const int internal2 = sceNpWebApi2IntInitialize2(&args2);
    Require(internal2 > 0 && internal2 != unnamed);
    Require(sceNpWebApi2GetMemoryPoolStats(internal2, &stats) == 0 && stats.pool_size == 0x7000);

    for (const int context : {presence, internal, unnamed, internal2}) {
        Require(sceNpWebApi2Terminate(context) == 0);
        Require(Throws([&] { sceNpWebApi2GetMemoryPoolStats(context, &stats); }));
    }
}

static void TestMultipartRequest() {
    constexpr int invalidArgument = static_cast<int>(0x80553402);
    constexpr int unavailable = static_cast<int>(0x80553406);
    const int context = sceNpWebApi2Initialize(1, 0x8000);
    const int user = sceNpWebApi2CreateUserContext(context, 1);
    std::int64_t request = 0;
    Require(sceNpWebApi2CreateMultipartRequest(user, nullptr, "/v1/x", "POST", &request) == invalidArgument);
    Require(sceNpWebApi2CreateMultipartRequest(user, "ugc", nullptr, "POST", &request) == invalidArgument);
    Require(sceNpWebApi2CreateMultipartRequest(user, "ugc", "/v1/x", nullptr, &request) == invalidArgument);
    Require(sceNpWebApi2CreateMultipartRequest(user, "ugc", "/v1/x", "POST", nullptr) == invalidArgument);
    Require(request == 0);
    Require(sceNpWebApi2CreateMultipartRequest(user, "ugc", "/v1/x", "POST", &request) == 0 && request > 0);
    Require(sceNpWebApi2AddWebTraceTag(request, "multipart") == 0);

    char part[4] = {'d', 'a', 't', 'a'};
    NpWebApi2ResponseInformationOption response{};
    Require(sceNpWebApi2SendMultipartRequest(request, 0, part, sizeof(part), &response) == invalidArgument);
    Require(sceNpWebApi2SendMultipartRequest(request, -1, part, sizeof(part), &response) == invalidArgument);
    Require(sceNpWebApi2SendMultipartRequest(request, 1, nullptr, sizeof(part), &response) == invalidArgument);
    Require(sceNpWebApi2SendMultipartRequest(request, 1, part, 0, &response) == invalidArgument);
    Require(sceNpWebApi2SendMultipartRequest(request, 1, part, sizeof(part), &response) == unavailable);
    Require(sceNpWebApi2SendMultipartRequest(request, 2, part, sizeof(part), nullptr) == unavailable);
    Require(sceNpWebApi2DeleteRequest(request) == 0);
    Require(sceNpWebApi2AddWebTraceTag(request, "multipart") == unavailable);
    Require(sceNpWebApi2Terminate(context) == 0);
}

int main() {
    TestPushEventAbortHandle();
    TestInitializeVariants();
    TestMultipartRequest();
    const int context = sceNpWebApi2Initialize(1, 0x20000);
    const int other = sceNpWebApi2Initialize(1, 0x8000);
    Require(context > 0 && other > 0 && context != other);
    NpWebApi2MemoryPoolStats stats{1, 1, 1, 1};
    Require(sceNpWebApi2GetMemoryPoolStats(context, &stats) == 0);
    Require(stats.pool_size == 0x20000 && stats.max_inuse_size == 0 && stats.current_inuse_size == 0 && stats.reserved == 0);
    Require(sceNpWebApi2GetMemoryPoolStats(other, &stats) == 0 && stats.pool_size == 0x8000);
    Require(Throws([&] { sceNpWebApi2GetMemoryPoolStats(context, nullptr); }));
    const int handle = sceNpWebApi2PushEventCreateHandle(context);
    Require(handle > 0);
    Require(sceNpWebApi2PushEventAbortHandle(context, handle) == 0);
    Require(sceNpWebApi2PushEventAbortHandle(context, handle + 1000) == HandleNotFound);
    Require(sceNpWebApi2PushEventSetHandleTimeout(context, handle, 30000) == 0);
    constexpr int invalidArgument = static_cast<int>(0x80553402);
    constexpr int requestNotFound = static_cast<int>(0x80553406);
    const int user = sceNpWebApi2CreateUserContext(context, 1);
    std::int64_t request = 0;
    Require(user > 0 && sceNpWebApi2CreateRequest(user, "userProfile", "/v1/users/me/profile", "GET", nullptr, &request) == 0 && request > 0);
    Require(sceNpWebApi2AddWebTraceTag(request, nullptr) == invalidArgument);
    Require(sceNpWebApi2AddWebTraceTag(request, "trace-1") == 0);
    Require(sceNpWebApi2AddWebTraceTag(request, "trace-2") == 0);
    Require(sceNpWebApi2AddWebTraceTag(request + 1000, "trace") == requestNotFound);
    Require(sceNpWebApi2AddWebTraceTag(0, "trace") == requestNotFound);
    Require(sceNpWebApi2SetRequestTimeout(request, 30000000) == 0);
    Require(sceNpWebApi2DeleteRequest(request) == 0);
    Require(sceNpWebApi2AddWebTraceTag(request, "trace") == requestNotFound);
    Require(sceNpWebApi2Terminate(context) == 0);
    Require(Throws([&] { sceNpWebApi2GetMemoryPoolStats(context, &stats); }));
    Require(sceNpWebApi2PushEventAbortHandle(context, handle) == LibContextNotFound);
    Require(Throws([&] { sceNpWebApi2PushEventSetHandleTimeout(context, handle, 30000); }));
    Require(sceNpWebApi2Terminate(other) == 0);
    return 0;
}
