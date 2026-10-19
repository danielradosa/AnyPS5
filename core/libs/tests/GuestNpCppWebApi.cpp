#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceNpCppWebApi/include/CppWebApiObjects.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

using NpCppWebApi::Binary;
using NpCppWebApi::IntrusivePtr;
using NpCppWebApi::ParameterObject;
using NpCppWebApi::VectorIterator;
using NpCppWebApi::VectorObject;
namespace Leaderboards = NpCppWebApi::Leaderboards;
namespace SessionManager = NpCppWebApi::SessionManager;
namespace TitleCloudStorage = NpCppWebApi::TitleCloudStorage;

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
bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEneERKS9_(const VectorIterator*, const VectorIterator*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEC1Ev(IntrusivePtr*);
int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_(const void*, IntrusivePtr*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_17TitleCloudStorage2V110DataStatusEEEEEEdeEv(const IntrusivePtr*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEED1Ev(IntrusivePtr*);
IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V132GetMultiDataStatusesResponseBody17getDataStatusListEv(IntrusivePtr*, TitleCloudStorage::GetMultiDataStatusesResponseBody*);
std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus11getDataSizeEv(const TitleCloudStorage::DataStatus*);
VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEE5beginEv(VectorIterator*, IntrusivePtr*);
VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEppEv(VectorIterator*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEptEv(const IntrusivePtr*);
int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEED2Ev(VectorIterator*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_17TitleCloudStorage2V110DataStatusEEEEEED1Ev(IntrusivePtr*);
IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEdeEv(const VectorIterator*);
IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus8getOwnerEv(IntrusivePtr*, const TitleCloudStorage::DataStatus*);
void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V15OwnerEED1Ev(IntrusivePtr*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V15OwnerEEptEv(const IntrusivePtr*);
VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEE3endEv(VectorIterator*, IntrusivePtr*);
int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE(void*, void*);
void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEptEv(const IntrusivePtr*);
int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv(void*);
int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void*);
int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V132GetMultiDataStatusesResponseBody23getTotalDataStatusCountEv(const TitleCloudStorage::GetMultiDataStatusesResponseBody*);
std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus9getSlotIdEv(const TitleCloudStorage::DataStatus*);
int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi26getMultiDataStatusesByUserEiRKNS4_37ParameterToGetMultiDataStatusesByUserERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_32GetMultiDataStatusesResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE(int, const void*, void*);
void APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadDataC1Ev(ParameterObject*);
void APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadDataD1Ev(ParameterObject*);
int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData10initializeEPNS1_6Common10LibContextEPKci(ParameterObject*, void*, const char*, std::int32_t);
int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData11setobjectIdEPKc(ParameterObject*, const char*);
int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData9terminateEv(ParameterObject*);
void APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUserC1Ev(ParameterObject*);
void APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUserD1Ev(ParameterObject*);
int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser10initializeEPNS1_6Common10LibContextEPKcSA_(ParameterObject*, void*, const char*, const char*);
int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser9setfieldsEPKc(ParameterObject*, const char*);
int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser9terminateEv(ParameterObject*);
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

constexpr int NotSignedIn = static_cast<int>(0x80552907);

struct ParameterStorage {
    alignas(8) unsigned char bytes[0x20];
};

ParameterObject* FreshParameter(ParameterStorage& storage) {
    std::memset(storage.bytes, 0xa5, sizeof(storage.bytes));
    return reinterpret_cast<ParameterObject*>(storage.bytes);
}

bool ParameterTailIntact(const ParameterStorage& storage) {
    for (std::size_t index = sizeof(ParameterObject); index < sizeof(storage.bytes); ++index) {
        if (storage.bytes[index] != 0xa5) return false;
    }
    return true;
}

bool SameText(const char* stored, const char* expected) {
    return stored != nullptr && std::strcmp(stored, expected) == 0;
}

void TestDownloadDataParameterKeepsValues() {
    int context = 0;
    ParameterStorage storage;
    ParameterObject* parameter = FreshParameter(storage);
    _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadDataC1Ev(parameter);
    Require(parameter->state == nullptr && ParameterTailIntact(storage));

    char account[] = "me";
    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData10initializeEPNS1_6Common10LibContextEPKci(parameter, &context, account, 3) == 0);
    account[0] = 'x';
    const auto* state = static_cast<const TitleCloudStorage::DownloadDataParameter*>(parameter->state);
    Require(state != nullptr && state->initialized && state->libContext == &context);
    Require(SameText(state->requiredText, "me") && state->requiredNumber == 3 && state->objectId == nullptr);
    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData11setobjectIdEPKc(parameter, "save0") == 0);
    Require(SameText(state->objectId, "save0"));
    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData11setobjectIdEPKc(parameter, "save1") == 0);
    Require(SameText(state->objectId, "save1"));
    Require(ParameterTailIntact(storage));

    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData9terminateEv(parameter) == 0);
    Require(parameter->state == nullptr);
    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData9terminateEv(parameter) == 0);

    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData11setobjectIdEPKc(parameter, "early") == 0);
    state = static_cast<const TitleCloudStorage::DownloadDataParameter*>(parameter->state);
    Require(state != nullptr && !state->initialized && SameText(state->objectId, "early"));
    _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadDataD1Ev(parameter);
    Require(parameter->state == nullptr && ParameterTailIntact(storage));
}

void TestMultiDataStatusesParameterKeepsValues() {
    int context = 0;
    ParameterStorage storage;
    ParameterObject* parameter = FreshParameter(storage);
    _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUserC1Ev(parameter);
    Require(parameter->state == nullptr);

    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser10initializeEPNS1_6Common10LibContextEPKcSA_(parameter, &context, "me", "slot1,slot2") == 0);
    const auto* state = static_cast<const TitleCloudStorage::GetMultiDataStatusesByUserParameter*>(parameter->state);
    Require(state != nullptr && state->initialized && state->libContext == &context);
    Require(SameText(state->requiredText[0], "me") && SameText(state->requiredText[1], "slot1,slot2"));
    Require(state->fields == nullptr);
    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser9setfieldsEPKc(parameter, "objectId,dataSize") == 0);
    Require(SameText(state->fields, "objectId,dataSize"));
    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser10initializeEPNS1_6Common10LibContextEPKcSA_(parameter, &context, "other", nullptr) == 0);
    Require(SameText(state->requiredText[0], "other") && state->requiredText[1] == nullptr);
    Require(SameText(state->fields, "objectId,dataSize"));
    Require(ParameterTailIntact(storage));

    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser9terminateEv(parameter) == 0);
    Require(parameter->state == nullptr);
    _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUserD1Ev(parameter);
    Require(parameter->state == nullptr && ParameterTailIntact(storage));
}

void TestMultiDataStatusesRequestFailsOffline() {
    int context = 0;
    alignas(8) unsigned char transaction[0x40] = {};
    Require(_ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(transaction) == 0);
    ParameterStorage storage;
    ParameterObject* parameter = FreshParameter(storage);
    _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUserC1Ev(parameter);
    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser10initializeEPNS1_6Common10LibContextEPKcSA_(parameter, &context, "me", "slot1") == 0);
    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi26getMultiDataStatusesByUserEiRKNS4_37ParameterToGetMultiDataStatusesByUserERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_32GetMultiDataStatusesResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE(1, parameter, transaction) == NotSignedIn);
    Require(_ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE(transaction, &context) == NotSignedIn);
    IntrusivePtr response;
    _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEC1Ev(&response);
    Require(response.object == nullptr && response.deleter == nullptr && response.libContext == nullptr);
    Require(_ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_(transaction, &response) == NotSignedIn);
    Require(response.object == nullptr);
    Require(_ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv(transaction) == 0);
    _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEED1Ev(&response);
    Require(_ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(transaction) == 0);
    _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUserD1Ev(parameter);
}

void TestDataStatusListIterate() {
    int context = 0;
    deleterCalls = 0;
    alignas(8) unsigned char owner[PlainObjectSize];
    PlainObject(owner);
    TitleCloudStorage::DataStatus statuses[2] = {};
    statuses[0] = {1, 0, 4096, {owner, CountDelete, &context}};
    statuses[1] = {1, 1, 0x100000000LL, {nullptr, nullptr, &context}};
    VectorObject vector{};
    vector.referenceCount = 1;
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEEC1EPNS2_10LibContextE(&vector.handle, &context);
    for (auto& status : statuses) {
        const IntrusivePtr pointer{&status, CountDelete, &context};
        Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEE8pushBackERKS8_(&vector.handle, &pointer) == 0);
    }

    TitleCloudStorage::GetMultiDataStatusesResponseBody body{1, 2, {&vector, CountDelete, &context}};
    const IntrusivePtr bodyPointer{&body, CountDelete, &context};
    auto* bodyObject = static_cast<TitleCloudStorage::GetMultiDataStatusesResponseBody*>(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEptEv(&bodyPointer));
    Require(bodyObject == &body);
    Require(_ZNK3sce2Np9CppWebApi17TitleCloudStorage2V132GetMultiDataStatusesResponseBody23getTotalDataStatusCountEv(bodyObject) == 2);
    IntrusivePtr list;
    Require(_ZN3sce2Np9CppWebApi17TitleCloudStorage2V132GetMultiDataStatusesResponseBody17getDataStatusListEv(&list, bodyObject) == &list);
    Require(list.object == &vector && vector.referenceCount == 2);

    auto* handle = static_cast<IntrusivePtr*>(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_17TitleCloudStorage2V110DataStatusEEEEEEdeEv(&list));
    VectorIterator iterator{};
    VectorIterator end{};
    Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEE5beginEv(&iterator, handle) == &iterator);
    Require(_ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEE3endEv(&end, handle) == &end);
    int seen = 0;
    for (; _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEneERKS9_(&iterator, &end); _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEppEv(&iterator)) {
        const auto* status = static_cast<const TitleCloudStorage::DataStatus*>(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEptEv(_ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEdeEv(&iterator)));
        Require(status == &statuses[seen]);
        Require(_ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus9getSlotIdEv(status) == seen);
        ++seen;
    }
    Require(seen == 2);
    Require(_ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus11getDataSizeEv(&statuses[0]) == 4096);
    Require(_ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus11getDataSizeEv(&statuses[1]) == 0x100000000LL);
    Require(_ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEED2Ev(&iterator) == 0);
    Require(_ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEED2Ev(&end) == 0);

    IntrusivePtr ownerPointer;
    Require(_ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus8getOwnerEv(&ownerPointer, &statuses[0]) == &ownerPointer);
    Require(_ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V15OwnerEEptEv(&ownerPointer) == owner && PlainCount(owner) == 2);
    _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V15OwnerEED1Ev(&ownerPointer);
    Require(PlainCount(owner) == 1 && deleterCalls == 0);
    Require(_ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus8getOwnerEv(&ownerPointer, &statuses[1]) == &ownerPointer);
    Require(ownerPointer.object == nullptr);

    _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_17TitleCloudStorage2V110DataStatusEEEEEED1Ev(&list);
    Require(vector.referenceCount == 1);
    _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEED1Ev(&vector.handle);
    for (const auto& status : statuses) Require(status.referenceCount == 1);
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
    TestDownloadDataParameterKeepsValues();
    TestMultiDataStatusesParameterKeepsValues();
    TestMultiDataStatusesRequestFailsOffline();
    TestDataStatusListIterate();
    return 0;
}
