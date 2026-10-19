#pragma once

#include <cstddef>
#include <cstdint>
#include "prx/libc/include/general/VabiMacros.hpp"

namespace NpCppWebApi {

using Deleter = void (APS5_VABI*)(void*);

struct IntrusivePtr {
    void* object;
    Deleter deleter;
    void* libContext;
};
static_assert(sizeof(IntrusivePtr) == 0x18);

inline constexpr std::size_t ReferenceCountOffset = 0x00;
inline constexpr std::size_t VectorReferenceCountOffset = 0x30;

struct String {
    std::int32_t referenceCount;
    char* buffer;
    std::uint64_t bufferSize;
    void* libContext;
};
static_assert(sizeof(String) == 0x20 && offsetof(String, buffer) == 0x08);
static_assert(offsetof(String, bufferSize) == 0x10 && offsetof(String, libContext) == 0x18);

inline constexpr std::uint64_t VectorBodyMagic = 0x434e507056656342ULL;

enum class VectorElement : std::int32_t {
    IntrusivePtr = 1,
    String = 2,
};

struct VectorBody {
    std::uint64_t magic;
    std::uint8_t reserved[0x28];
    std::int32_t referenceCount;
    VectorElement elementKind;
    void* elements;
    std::uint64_t size;
    std::uint64_t capacity;
};
static_assert(sizeof(VectorBody) == 0x50 && offsetof(VectorBody, referenceCount) == VectorReferenceCountOffset);

struct VectorObject {
    IntrusivePtr handle;
    std::uint8_t reserved[0x18];
    std::int32_t referenceCount;
};
static_assert(offsetof(VectorObject, referenceCount) == VectorReferenceCountOffset);

struct VectorIterator {
    IntrusivePtr* element;
};
static_assert(sizeof(VectorIterator) == 0x08);

struct Binary {
    std::int32_t referenceCount;
    void* data;
    std::uint64_t size;
    void* libContext;
};

namespace Leaderboards {

struct Entry {
    std::int32_t referenceCount;
    std::int32_t rank;
    std::int32_t serialRank;
    std::int32_t highestRank;
    std::int32_t highestSerialRank;
    std::int32_t pcId;
    std::int64_t score;
    std::uint64_t accountId;
    IntrusivePtr smallData;
    bool commentSet;
    bool objectIdSet;
    bool smallDataSet;
};

struct GetRankingResponseBody {
    std::int32_t referenceCount;
    IntrusivePtr entries;
};

struct RecordScoreResponseBody {
    std::int32_t referenceCount;
    std::int32_t tmpRank;
    std::int32_t tmpSerialRank;
};

struct GetBoardDefinitionResponseBody {
    std::int32_t referenceCount;
    std::int32_t sortMode;
    std::int32_t entryLimit;
    std::int32_t updateMode;
    std::int32_t largeDataNumLimit;
    std::int64_t maxScoreLimit;
    std::int64_t minScoreLimit;
    std::int64_t largeDataSizeLimit;
    bool sortModeSet;
    bool entryLimitSet;
    bool updateModeSet;
    bool maxScoreLimitSet;
    bool minScoreLimitSet;
    bool largeDataNumLimitSet;
    bool largeDataSizeLimitSet;
};

}

namespace SessionManager {

struct PostPlayerSessionsResponseBody {
    std::int32_t referenceCount;
    IntrusivePtr playerSessions;
};

struct GetUsersAccountIdPlayerSessionsInvitationsResponseBody {
    std::int32_t referenceCount;
    IntrusivePtr invitations;
    bool invitationsSet;
};

struct UsersPlayerSessionsInvitationForRead {
    std::int32_t referenceCount;
    IntrusivePtr from;
    bool fromSet;
    bool sessionIdSet;
    bool invitationIdSet;
    bool receivedTimestampSet;
};

}

}
