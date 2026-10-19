#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceNpCppWebApi/include/CppWebApiObjects.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

using NpCppWebApi::Binary;
using NpCppWebApi::IntrusivePtr;
using NpCppWebApi::VectorIterator;
using NpCppWebApi::VectorObject;
namespace Leaderboards = NpCppWebApi::Leaderboards;
namespace SessionManager = NpCppWebApi::SessionManager;

extern "C" {
std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common6Binary4sizeEv(const Binary*);
const void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common6Binary9getBinaryEv(const Binary*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V123RecordScoreResponseBody10getTmpRankEv(const Leaderboards::RecordScoreResponseBody*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V123RecordScoreResponseBody16getTmpSerialRankEv(const Leaderboards::RecordScoreResponseBody*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody11getSortModeEv(const Leaderboards::GetBoardDefinitionResponseBody*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13getEntryLimitEv(const Leaderboards::GetBoardDefinitionResponseBody*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13getUpdateModeEv(const Leaderboards::GetBoardDefinitionResponseBody*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13sortModeIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody15entryLimitIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody15updateModeIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody*);
std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody16getMaxScoreLimitEv(const Leaderboards::GetBoardDefinitionResponseBody*);
std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody16getMinScoreLimitEv(const Leaderboards::GetBoardDefinitionResponseBody*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody18maxScoreLimitIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody18minScoreLimitIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody20getLargeDataNumLimitEv(const Leaderboards::GetBoardDefinitionResponseBody*);
std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody21getLargeDataSizeLimitEv(const Leaderboards::GetBoardDefinitionResponseBody*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody22largeDataNumLimitIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody23largeDataSizeLimitIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12commentIsSetEv(const Leaderboards::Entry*);
std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12getAccountIdEv(const Leaderboards::Entry*);
const IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12getSmallDataEv(const Leaderboards::Entry*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry13getSerialRankEv(const Leaderboards::Entry*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry13objectIdIsSetEv(const Leaderboards::Entry*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry14getHighestRankEv(const Leaderboards::Entry*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry14smallDataIsSetEv(const Leaderboards::Entry*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry20getHighestSerialRankEv(const Leaderboards::Entry*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry7getPcIdEv(const Leaderboards::Entry*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry7getRankEv(const Leaderboards::Entry*);
std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry8getScoreEv(const Leaderboards::Entry*);
IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122GetRankingResponseBody10getEntriesEv(IntrusivePtr*, Leaderboards::GetRankingResponseBody*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEEC1EPNS2_10LibContextE(IntrusivePtr*, void*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEED1Ev(IntrusivePtr*);
int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEE8pushBackERKS8_(IntrusivePtr*, const IntrusivePtr*);
VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEE5beginEv(VectorIterator*, IntrusivePtr*);
VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEE3endEv(VectorIterator*, IntrusivePtr*);
VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEppEv(VectorIterator*);
IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEdeEv(const VectorIterator*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEneERKS9_(const VectorIterator*, const VectorIterator*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_12Leaderboards2V15EntryEEEEEEdeEv(const IntrusivePtr*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_12Leaderboards2V15EntryEEEEEED1Ev(IntrusivePtr*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V15EntryEEptEv(const IntrusivePtr*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6BinaryEEptEv(const IntrusivePtr*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBody16invitationsIsSetEv(const SessionManager::GetUsersAccountIdPlayerSessionsInvitationsResponseBody*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead14sessionIdIsSetEv(const SessionManager::UsersPlayerSessionsInvitationForRead*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead17invitationIdIsSetEv(const SessionManager::UsersPlayerSessionsInvitationForRead*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead22receivedTimestampIsSetEv(const SessionManager::UsersPlayerSessionsInvitationForRead*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead9fromIsSetEv(const SessionManager::UsersPlayerSessionsInvitationForRead*);
IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead7getFromEv(IntrusivePtr*, const SessionManager::UsersPlayerSessionsInvitationForRead*);
IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V130PostPlayerSessionsResponseBody17getPlayerSessionsEv(IntrusivePtr*, SessionManager::PostPlayerSessionsResponseBody*);
IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBody14getInvitationsEv(IntrusivePtr*, SessionManager::GetUsersAccountIdPlayerSessionsInvitationsResponseBody*);
IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEEEixEm(IntrusivePtr*, std::uint64_t);
VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEE5beginEv(VectorIterator*, IntrusivePtr*);
VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEE3endEv(VectorIterator*, IntrusivePtr*);
VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEppEv(VectorIterator*);
IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEdeEv(const VectorIterator*);
bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEneERKS9_(const VectorIterator*, const VectorIterator*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEEC1EPNS2_10LibContextE(IntrusivePtr*, void*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEED1Ev(IntrusivePtr*);
int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEE8pushBackERKS8_(IntrusivePtr*, const IntrusivePtr*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V110FromMemberEEptEv(const IntrusivePtr*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V110FromMemberEED1Ev(IntrusivePtr*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEptEv(const IntrusivePtr*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEptEv(const IntrusivePtr*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEEEdeEv(const IntrusivePtr*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEEED1Ev(IntrusivePtr*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEEdeEv(const IntrusivePtr*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEED1Ev(IntrusivePtr*);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

void TestBinaryReportsItsBytes() {
    std::uint8_t bytes[] = {1, 2, 3, 4, 5};
    int context = 0;
    const Binary binary{1, bytes, sizeof(bytes), &context};
    Require(_ZNK3sce2Np9CppWebApi6Common6Binary9getBinaryEv(&binary) == bytes);
    Require(_ZNK3sce2Np9CppWebApi6Common6Binary4sizeEv(&binary) == sizeof(bytes));

    const Binary empty{1, nullptr, 0, &context};
    Require(_ZNK3sce2Np9CppWebApi6Common6Binary9getBinaryEv(&empty) == nullptr);
    Require(_ZNK3sce2Np9CppWebApi6Common6Binary4sizeEv(&empty) == 0);
}

int deleterCalls = 0;

void APS5_VABI CountDelete(void*) {
    ++deleterCalls;
}

void TestEntryGetters() {
    std::uint8_t bytes[] = {9, 8, 7};
    int context = 0;
    Binary smallData{1, bytes, sizeof(bytes), &context};
    Leaderboards::Entry entry{};
    entry.referenceCount = 1;
    entry.rank = 3;
    entry.serialRank = 4;
    entry.highestRank = 2;
    entry.highestSerialRank = 5;
    entry.pcId = 7;
    entry.score = -1234567890123LL;
    entry.accountId = 0x1122334455667788ULL;
    entry.smallData = {&smallData, CountDelete, &context};
    entry.commentSet = true;
    entry.objectIdSet = false;
    entry.smallDataSet = true;

    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry7getRankEv(&entry) == 3);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry13getSerialRankEv(&entry) == 4);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry14getHighestRankEv(&entry) == 2);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry20getHighestSerialRankEv(&entry) == 5);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry7getPcIdEv(&entry) == 7);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry8getScoreEv(&entry) == -1234567890123LL);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12getAccountIdEv(&entry) == 0x1122334455667788ULL);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12commentIsSetEv(&entry));
    Require(!_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry13objectIdIsSetEv(&entry));
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry14smallDataIsSetEv(&entry));

    const IntrusivePtr* pointer = _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12getSmallDataEv(&entry);
    Require(pointer == &entry.smallData);
    const auto* binary = static_cast<const Binary*>(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6BinaryEEptEv(pointer));
    Require(binary == &smallData);
    Require(_ZNK3sce2Np9CppWebApi6Common6Binary9getBinaryEv(binary) == bytes);
    Require(_ZNK3sce2Np9CppWebApi6Common6Binary4sizeEv(binary) == sizeof(bytes));
    Require(smallData.referenceCount == 1);
}

void TestScoreAndBoardDefinitionGetters() {
    const Leaderboards::RecordScoreResponseBody score{1, 12, 13};
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V123RecordScoreResponseBody10getTmpRankEv(&score) == 12);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V123RecordScoreResponseBody16getTmpSerialRankEv(&score) == 13);

    Leaderboards::GetBoardDefinitionResponseBody board{};
    board.referenceCount = 1;
    board.sortMode = 1;
    board.entryLimit = 1000;
    board.updateMode = 2;
    board.largeDataNumLimit = 10;
    board.maxScoreLimit = 9000000000LL;
    board.minScoreLimit = -9000000000LL;
    board.largeDataSizeLimit = 1048576;
    board.sortModeSet = true;
    board.entryLimitSet = true;
    board.updateModeSet = false;
    board.maxScoreLimitSet = true;
    board.minScoreLimitSet = false;
    board.largeDataNumLimitSet = true;
    board.largeDataSizeLimitSet = false;
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody11getSortModeEv(&board) == 1);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13getEntryLimitEv(&board) == 1000);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13getUpdateModeEv(&board) == 2);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody20getLargeDataNumLimitEv(&board) == 10);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody16getMaxScoreLimitEv(&board) == 9000000000LL);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody16getMinScoreLimitEv(&board) == -9000000000LL);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody21getLargeDataSizeLimitEv(&board) == 1048576);
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13sortModeIsSetEv(&board));
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody15entryLimitIsSetEv(&board));
    Require(!_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody15updateModeIsSetEv(&board));
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody18maxScoreLimitIsSetEv(&board));
    Require(!_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody18minScoreLimitIsSetEv(&board));
    Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody22largeDataNumLimitIsSetEv(&board));
    Require(!_ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody23largeDataSizeLimitIsSetEv(&board));
}

void TestRankingEntriesIterate() {
    int context = 0;
    deleterCalls = 0;
    Leaderboards::Entry entries[3] = {};
    VectorObject vector{};
    vector.referenceCount = 1;
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEEC1EPNS2_10LibContextE(&vector.handle, &context);
    for (int index = 0; index < 3; ++index) {
        entries[index].referenceCount = 1;
        entries[index].rank = index + 1;
        const IntrusivePtr pointer{&entries[index], CountDelete, &context};
        Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEE8pushBackERKS8_(&vector.handle, &pointer) == 0);
        Require(entries[index].referenceCount == 2);
    }

    Leaderboards::GetRankingResponseBody body{1, {&vector, CountDelete, &context}};
    IntrusivePtr result;
    std::memset(&result, 0xa5, sizeof(result));
    Require(_ZN3sce2Np9CppWebApi12Leaderboards2V122GetRankingResponseBody10getEntriesEv(&result, &body) == &result);
    Require(result.object == &vector && result.deleter == CountDelete && result.libContext == &context);
    Require(vector.referenceCount == 2);

    auto* list = static_cast<IntrusivePtr*>(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_12Leaderboards2V15EntryEEEEEEdeEv(&result));
    Require(list == &vector.handle);
    VectorIterator iterator{};
    VectorIterator end{};
    Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEE5beginEv(&iterator, list) == &iterator);
    Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEE3endEv(&end, list) == &end);
    int seen = 0;
    while (_ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEneERKS9_(&iterator, &end)) {
        const IntrusivePtr* element = _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEdeEv(&iterator);
        const auto* entry = static_cast<const Leaderboards::Entry*>(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V15EntryEEptEv(element));
        Require(entry == &entries[seen]);
        Require(_ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry7getRankEv(entry) == seen + 1);
        Require(_ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEppEv(&iterator) == &iterator);
        ++seen;
    }
    Require(seen == 3);

    _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_12Leaderboards2V15EntryEEEEEED1Ev(&result);
    Require(vector.referenceCount == 1 && deleterCalls == 0);
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEED1Ev(&vector.handle);
    for (const auto& entry : entries) Require(entry.referenceCount == 1);
    Require(deleterCalls == 0);

    Leaderboards::GetRankingResponseBody missing{1, {nullptr, nullptr, &context}};
    Require(_ZN3sce2Np9CppWebApi12Leaderboards2V122GetRankingResponseBody10getEntriesEv(&result, &missing) == &result);
    Require(result.object == nullptr && result.libContext == &context);
}

void TestEmptyEntriesVector() {
    int context = 0;
    IntrusivePtr vector;
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEEC1EPNS2_10LibContextE(&vector, &context);
    Require(vector.object == nullptr && vector.deleter == nullptr && vector.libContext == &context);
    VectorIterator begin{};
    VectorIterator end{};
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEE5beginEv(&begin, &vector);
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEE3endEv(&end, &vector);
    Require(!_ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEneERKS9_(&begin, &end));
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEED1Ev(&vector);
    Require(vector.object == nullptr);
}

constexpr std::size_t PlainObjectSize = 0x40;

std::int32_t PlainCount(const unsigned char* object) {
    std::int32_t value;
    std::memcpy(&value, object, sizeof(value));
    return value;
}

void PlainObject(unsigned char* object) {
    std::memset(object, 0x5a, PlainObjectSize);
    const std::int32_t count = 1;
    std::memcpy(object, &count, sizeof(count));
}

void TestInvitationAccessors() {
    int context = 0;
    deleterCalls = 0;
    alignas(8) unsigned char from[PlainObjectSize];
    PlainObject(from);
    SessionManager::UsersPlayerSessionsInvitationForRead invitation{};
    invitation.referenceCount = 1;
    invitation.from = {from, CountDelete, &context};
    invitation.fromSet = true;
    invitation.sessionIdSet = true;
    invitation.invitationIdSet = false;
    invitation.receivedTimestampSet = true;
    Require(_ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead9fromIsSetEv(&invitation));
    Require(_ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead14sessionIdIsSetEv(&invitation));
    Require(!_ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead17invitationIdIsSetEv(&invitation));
    Require(_ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead22receivedTimestampIsSetEv(&invitation));

    IntrusivePtr result;
    std::memset(&result, 0xa5, sizeof(result));
    Require(_ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead7getFromEv(&result, &invitation) == &result);
    Require(result.object == from && result.deleter == CountDelete && result.libContext == &context);
    Require(PlainCount(from) == 2);
    Require(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V110FromMemberEEptEv(&result) == from);
    _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V110FromMemberEED1Ev(&result);
    Require(PlainCount(from) == 1 && deleterCalls == 0);

    SessionManager::UsersPlayerSessionsInvitationForRead anonymous{};
    anonymous.from = {nullptr, nullptr, &context};
    Require(!_ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead9fromIsSetEv(&anonymous));
    Require(_ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead7getFromEv(&result, &anonymous) == &result);
    Require(result.object == nullptr && result.libContext == &context);
}

void TestInvitationsIterate() {
    int context = 0;
    deleterCalls = 0;
    SessionManager::UsersPlayerSessionsInvitationForRead invitations[2] = {};
    VectorObject vector{};
    vector.referenceCount = 1;
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEEC1EPNS2_10LibContextE(&vector.handle, &context);
    for (auto& invitation : invitations) {
        invitation.referenceCount = 1;
        const IntrusivePtr pointer{&invitation, CountDelete, &context};
        Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEE8pushBackERKS8_(&vector.handle, &pointer) == 0);
    }
    invitations[1].sessionIdSet = true;

    SessionManager::GetUsersAccountIdPlayerSessionsInvitationsResponseBody body{1, {&vector, CountDelete, &context}, true};
    Require(_ZNK3sce2Np9CppWebApi14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBody16invitationsIsSetEv(&body));
    IntrusivePtr result;
    Require(_ZN3sce2Np9CppWebApi14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBody14getInvitationsEv(&result, &body) == &result);
    Require(result.object == &vector && vector.referenceCount == 2);

    auto* list = static_cast<IntrusivePtr*>(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEEEdeEv(&result));
    Require(list == &vector.handle);
    VectorIterator iterator{};
    VectorIterator end{};
    Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEE5beginEv(&iterator, list) == &iterator);
    Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEE3endEv(&end, list) == &end);
    int seen = 0;
    for (; _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEneERKS9_(&iterator, &end); _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEppEv(&iterator)) {
        const auto* invitation = static_cast<const SessionManager::UsersPlayerSessionsInvitationForRead*>(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEptEv(_ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEdeEv(&iterator)));
        Require(invitation == &invitations[seen]);
        Require(_ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead14sessionIdIsSetEv(invitation) == (seen == 1));
        ++seen;
    }
    Require(seen == 2);

    _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEEED1Ev(&result);
    Require(vector.referenceCount == 1);
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEED1Ev(&vector.handle);
    for (const auto& invitation : invitations) Require(invitation.referenceCount == 1);
    Require(deleterCalls == 0);

    SessionManager::GetUsersAccountIdPlayerSessionsInvitationsResponseBody none{1, {nullptr, nullptr, &context}, false};
    Require(!_ZNK3sce2Np9CppWebApi14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBody16invitationsIsSetEv(&none));
}

void TestPlayerSessionsIndex() {
    int context = 0;
    deleterCalls = 0;
    alignas(8) unsigned char sessions[3][PlainObjectSize];
    VectorObject vector{};
    vector.referenceCount = 1;
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEEC1EPNS2_10LibContextE(&vector.handle, &context);
    Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEEEixEm(&vector.handle, 0) == nullptr);
    for (auto& session : sessions) {
        PlainObject(session);
        const IntrusivePtr pointer{session, CountDelete, &context};
        Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEE8pushBackERKS8_(&vector.handle, &pointer) == 0);
    }

    SessionManager::PostPlayerSessionsResponseBody body{1, {&vector, CountDelete, &context}};
    IntrusivePtr result;
    Require(_ZN3sce2Np9CppWebApi14SessionManager2V130PostPlayerSessionsResponseBody17getPlayerSessionsEv(&result, &body) == &result);
    Require(result.object == &vector && vector.referenceCount == 2);
    auto* list = static_cast<IntrusivePtr*>(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEEdeEv(&result));
    for (std::uint64_t index = 0; index < 3; ++index) {
        IntrusivePtr* element = _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEEEixEm(list, index);
        Require(element != nullptr);
        Require(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEptEv(element) == sessions[index]);
        Require(PlainCount(sessions[index]) == 2);
    }
    Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEEEixEm(list, 3) == nullptr);

    _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEED1Ev(&result);
    Require(vector.referenceCount == 1);
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEED1Ev(&vector.handle);
    for (const auto& session : sessions) Require(PlainCount(session) == 1);
    Require(deleterCalls == 0);
}

}

int main() {
    TestBinaryReportsItsBytes();
    TestEntryGetters();
    TestScoreAndBoardDefinitionGetters();
    TestRankingEntriesIterate();
    TestEmptyEntriesVector();
    TestInvitationAccessors();
    TestInvitationsIterate();
    TestPlayerSessionsIndex();
    return 0;
}
