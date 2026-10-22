#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceNpCppWebApi/include/CppWebApiObjects.hpp"

static constexpr int SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY = static_cast<int>(0x80552901);
static constexpr int SCE_NP_WEBAPI_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80552902);
static constexpr int SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN = static_cast<int>(0x80552907);

using NpCppWebApi::Binary;
using NpCppWebApi::IntrusivePtr;
using NpCppWebApi::ParameterObject;
using NpCppWebApi::ReferenceCountOffset;
using NpCppWebApi::VectorBody;
using NpCppWebApi::VectorBodyMagic;
using NpCppWebApi::VectorElement;
using NpCppWebApi::VectorIterator;
using NpCppWebApi::VectorReferenceCountOffset;
using CommonString = NpCppWebApi::String;
namespace Leaderboards = NpCppWebApi::Leaderboards;
namespace SessionManager = NpCppWebApi::SessionManager;
namespace TitleCloudStorage = NpCppWebApi::TitleCloudStorage;
namespace InGameCatalog = NpCppWebApi::InGameCatalog;

extern "C" {

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V111UserFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_4UserEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V121GetRankingRequestBody18setStartSerialRankERKi() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V121GetRankingRequestBody8setGroupERKNS3_5GroupE() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V121GetRankingRequestBody8setLimitERKi() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V121GetRankingRequestBody8setUsersERKNS1_6Common6VectorINS5_12IntrusivePtrINS3_4UserEEEEE() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V121GetRankingRequestBody9setOffsetERKi() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122RecordScoreRequestBody10setCommentEPKc() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122RecordScoreRequestBody12setSmallDataEPKvm() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122RecordScoreRequestBody15setNeedsTmpRankERKb() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122RecordScoreRequestBody19setComparedDateTimeERK10SceRtcTick() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122RecordScoreRequestBody7setPcIdERKi() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V128GetRankingRequestBodyFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_21GetRankingRequestBodyEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V129RecordScoreRequestBodyFactory6createEPNS1_6Common10LibContextElPNS5_12IntrusivePtrINS3_22RecordScoreRequestBodyEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V14User12setAccountIdERKm() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V14User7setPcIdERKi() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi10getRankingEiRKNS4_21ParameterToGetRankingERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_22GetRankingResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi21ParameterToGetRanking10initializeEPNS1_6Common10LibContextEi() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi21ParameterToGetRanking24setgetRankingRequestBodyENS1_6Common12IntrusivePtrINS3_21GetRankingRequestBodyEEE() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi21ParameterToGetRanking9terminateEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi21ParameterToGetRankingC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi21ParameterToGetRankingD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi22getLargeDataByObjectIdEiRKNS4_33ParameterToGetLargeDataByObjectIdERNS1_6Common21DownStreamTransactionINS8_12IntrusivePtrINS4_37GetLargeDataByObjectIdResponseHeadersEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi33ParameterToGetLargeDataByObjectId10initializeEPNS1_6Common10LibContextEPKc() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi33ParameterToGetLargeDataByObjectId9terminateEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi33ParameterToGetLargeDataByObjectIdC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi33ParameterToGetLargeDataByObjectIdD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19BoardsApi18getBoardDefinitionEiRKNS4_29ParameterToGetBoardDefinitionERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_30GetBoardDefinitionResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19BoardsApi29ParameterToGetBoardDefinition10initializeEPNS1_6Common10LibContextEi() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19BoardsApi29ParameterToGetBoardDefinition9terminateEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19BoardsApi29ParameterToGetBoardDefinitionC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19BoardsApi29ParameterToGetBoardDefinitionD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi11recordScoreEiRKNS4_22ParameterToRecordScoreERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_23RecordScoreResponseBodyEEENSA_INS4_26RecordScoreResponseHeadersEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi15recordLargeDataEiRKNS4_26ParameterToRecordLargeDataERNS1_6Common19UpStreamTransactionINS8_12IntrusivePtrINS3_27RecordLargeDataResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi22ParameterToRecordScore10initializeEPNS1_6Common10LibContextEiNS6_12IntrusivePtrINS3_22RecordScoreRequestBodyEEE() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi22ParameterToRecordScore22setxPsnAtomicOperationENS5_19XPsnAtomicOperationE() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi22ParameterToRecordScore9terminateEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi22ParameterToRecordScoreC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi22ParameterToRecordScoreD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi26ParameterToRecordLargeData10initializeEPNS1_6Common10LibContextEiNS5_19XPsnAtomicOperationEPKc() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi26ParameterToRecordLargeData9terminateEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi26ParameterToRecordLargeDataC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi26ParameterToRecordLargeDataD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi12downloadDataEiRKNS4_23ParameterToDownloadDataERNS1_6Common21DownStreamTransactionINS8_12IntrusivePtrINS4_27DownloadDataResponseHeadersEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common10InitParamsC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common10InitParamsD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common10LibContextC1Ev(uint64_t* self) {
    *self = 0;
    return 0;
}

// Initialization succeeds as on an offline console (see libSceJson2); individual requests fail.
int APS5_VABI _ZN3sce2Np9CppWebApi6Common10initializeERKNS2_10InitParamsERNS2_10LibContextE(const void* params, uint64_t* context) {
    (void)params;
    if (context) *context = 1;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

static std::atomic_ref<std::int32_t> ReferenceCount(void* object, const std::size_t offset) {
    return std::atomic_ref<std::int32_t>(*reinterpret_cast<std::int32_t*>(static_cast<std::uint8_t*>(object) + offset));
}

static void CopyIntrusivePtr(IntrusivePtr* self, const IntrusivePtr* source, const std::size_t referenceCountOffset) {
    *self = *source;
    if (self->object) ReferenceCount(self->object, referenceCountOffset).fetch_add(1);
}

static void ReleaseIntrusivePtr(IntrusivePtr* self, const std::size_t referenceCountOffset) {
    if (!self->object) return;
    if (ReferenceCount(self->object, referenceCountOffset).fetch_sub(1) != 1) return;
    if (self->deleter) self->deleter(self->object);
    self->object = nullptr;
    self->deleter = nullptr;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V121GetRankingRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    CopyIntrusivePtr(self, source, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V121GetRankingRequestBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V121GetRankingRequestBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122RecordScoreRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    CopyIntrusivePtr(self, source, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122RecordScoreRequestBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122RecordScoreRequestBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V14UserEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V14UserEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V19RecordApi26RecordScoreResponseHeadersEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V19RecordApi26RecordScoreResponseHeadersEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_12Leaderboards2V15EntryEEEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, VectorReferenceCountOffset);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEED2Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEE5startEPNS2_10LibContextE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V127RecordLargeDataResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common19UpStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V127RecordLargeDataResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextEm() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common19UpStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V127RecordLargeDataResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE8sendDataEPKvm() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common19UpStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V127RecordLargeDataResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common19UpStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V127RecordLargeDataResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V17ViewApi37GetLargeDataByObjectIdResponseHeadersEEEE5startEPNS2_10LibContextE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V17ViewApi37GetLargeDataByObjectIdResponseHeadersEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V17ViewApi37GetLargeDataByObjectIdResponseHeadersEEEE8readDataEPcm() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V17ViewApi37GetLargeDataByObjectIdResponseHeadersEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V17ViewApi37GetLargeDataByObjectIdResponseHeadersEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V17DataApi27DownloadDataResponseHeadersEEEE5startEPNS2_10LibContextE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V17DataApi27DownloadDataResponseHeadersEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V17DataApi27DownloadDataResponseHeadersEEEE8readDataEPcm() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V17DataApi27DownloadDataResponseHeadersEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V17DataApi27DownloadDataResponseHeadersEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6StringC1EPNS2_10LibContextE(CommonString* self, void* libContext) {
    self->referenceCount = 0;
    self->buffer = nullptr;
    self->bufferSize = 0;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6StringD1Ev(CommonString* self) {
    std::free(self->buffer);
    self->buffer = nullptr;
    self->bufferSize = 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common9terminateERNS2_10LibContextE(uint64_t* context) {
    *context = 0;
    return 0;
}

const void* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V122GetRankingResponseBody22getLastUpdatedDateTimeEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry10getCommentEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry11getObjectIdEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry11getOnlineIdEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V19RecordApi26RecordScoreResponseHeaders24getXPsnAtomicOperationIdEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEE11getResponseERS8_() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V121GetRankingRequestBodyEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122RecordScoreRequestBodyEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V14UserEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V15EntryEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V19RecordApi26RecordScoreResponseHeadersEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6BinaryEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_12Leaderboards2V15EntryEEEEEEdeEv(const IntrusivePtr* self) {
    return self->object;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEE18getResponseHeadersERSB_() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common6Binary4sizeEv(const Binary* self) {
    return self->size;
}

const void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common6Binary9getBinaryEv(const Binary* self) {
    return self->data;
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi6Common6String5c_strEv(const CommonString* self) {
    return self->buffer != nullptr ? self->buffer : "";
}

static void APS5_VABI DestroyVectorBody(void* object) {
    auto* body = static_cast<VectorBody*>(object);
    if (body->elementKind == VectorElement::IntrusivePtr) {
        auto* elements = static_cast<IntrusivePtr*>(body->elements);
        for (std::uint64_t index = 0; index < body->size; ++index) {
            ReleaseIntrusivePtr(&elements[index], ReferenceCountOffset);
        }
    } else if (body->elementKind == VectorElement::String) {
        auto* elements = static_cast<CommonString*>(body->elements);
        for (std::uint64_t index = 0; index < body->size; ++index) {
            std::free(elements[index].buffer);
        }
    }
    std::free(body->elements);
    std::free(body);
}

static int VectorBodyFor(IntrusivePtr* self, const VectorElement elementKind, VectorBody** result) {
    auto* body = static_cast<VectorBody*>(self->object);
    if (body == nullptr) {
        body = static_cast<VectorBody*>(std::calloc(1, sizeof(VectorBody)));
        if (body == nullptr) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
        body->magic = VectorBodyMagic;
        body->referenceCount = 1;
        body->elementKind = elementKind;
        self->object = body;
        self->deleter = &DestroyVectorBody;
    } else if (body->magic != VectorBodyMagic || body->elementKind != elementKind) {
        return SCE_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    *result = body;
    return 0;
}

static bool VectorGrow(VectorBody* body, const std::size_t elementSize) {
    if (body->size < body->capacity) return true;
    const std::uint64_t capacity = body->capacity != 0 ? body->capacity * 2 : 4;
    void* elements = std::realloc(body->elements, static_cast<std::size_t>(capacity) * elementSize);
    if (elements == nullptr) return false;
    body->elements = elements;
    body->capacity = capacity;
    return true;
}

static int VectorPushBackIntrusivePtr(IntrusivePtr* self, const IntrusivePtr* element) {
    VectorBody* body = nullptr;
    const int result = VectorBodyFor(self, VectorElement::IntrusivePtr, &body);
    if (result != 0) return result;
    if (!VectorGrow(body, sizeof(IntrusivePtr))) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
    CopyIntrusivePtr(&static_cast<IntrusivePtr*>(body->elements)[body->size], element, ReferenceCountOffset);
    body->size += 1;
    return 0;
}

static int VectorPushBackString(IntrusivePtr* self, const CommonString* element) {
    VectorBody* body = nullptr;
    const int result = VectorBodyFor(self, VectorElement::String, &body);
    if (result != 0) return result;
    if (!VectorGrow(body, sizeof(CommonString))) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
    CommonString copy = *element;
    copy.referenceCount = 0;
    copy.buffer = nullptr;
    copy.bufferSize = 0;
    if (element->buffer != nullptr) {
        const std::size_t length = std::strlen(element->buffer) + 1;
        copy.buffer = static_cast<char*>(std::malloc(length));
        if (copy.buffer == nullptr) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
        std::memcpy(copy.buffer, element->buffer, length);
        copy.bufferSize = length;
    }
    static_cast<CommonString*>(body->elements)[body->size] = copy;
    body->size += 1;
    return 0;
}

static const VectorBody* PointerVectorBody(const IntrusivePtr* self) {
    const auto* body = static_cast<const VectorBody*>(self->object);
    if (body == nullptr || body->magic != VectorBodyMagic || body->elementKind != VectorElement::IntrusivePtr) return nullptr;
    return body;
}

static VectorIterator* VectorBegin(VectorIterator* result, const IntrusivePtr* self) {
    const VectorBody* body = PointerVectorBody(self);
    result->element = body != nullptr ? static_cast<IntrusivePtr*>(body->elements) : nullptr;
    return result;
}

static VectorIterator* VectorEnd(VectorIterator* result, const IntrusivePtr* self) {
    const VectorBody* body = PointerVectorBody(self);
    result->element = body != nullptr ? static_cast<IntrusivePtr*>(body->elements) + body->size : nullptr;
    return result;
}

static IntrusivePtr* VectorElementAt(const IntrusivePtr* self, const std::uint64_t index) {
    const VectorBody* body = PointerVectorBody(self);
    if (body == nullptr || index >= body->size) return nullptr;
    return static_cast<IntrusivePtr*>(body->elements) + index;
}

static std::uint64_t VectorSize(const IntrusivePtr* self) {
    const VectorBody* body = PointerVectorBody(self);
    return body != nullptr ? body->size : 0;
}

static VectorIterator* IteratorAdvance(VectorIterator* self) {
    self->element += 1;
    return self;
}

// --------------------------------------------------------------------------
// SessionManager2V1 and extra Common symbols, from a title dump whose modules lack
// libSceNpCppWebApi.prx.
// --------------------------------------------------------------------------


void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V115LocalizedStringEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    CopyIntrusivePtr(self, source, 0x00);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEptEv(const IntrusivePtr* self) {
    return self->object;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120RequestPlayerSession16setMaxSpectatorsERKi() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V139RequestPlayerSessionMemberPlayerFactory6createEPNS1_6Common10LibContextERKNS5_6VectorINS5_12IntrusivePtrINS3_32RequestCreatePlayerSessionPlayerEEEEEPNS9_INS3_32RequestPlayerSessionMemberPlayerEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToJoinPlayerSessionAsPlayerD1Ev() {
    return 0;
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V110FromMember11getOnlineIdEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38ParameterToGetPlayerSessionInvitations26setinvitationInvalidFilterEPKc() {
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V151PostPlayerSessionsSessionIdMemberPlayersRequestBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi29ParameterToLeavePlayerSessionD1Ev() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V122LocalizedStringFactory6createEPNS1_6Common10LibContextEPKcNS_4Json6ObjectEPNS5_12IntrusivePtrINS3_15LocalizedStringEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V151PostPlayerSessionsSessionIdMemberPlayersRequestBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V132RequestCreatePlayerSessionPlayerEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x30);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEED2Ev(void* self) {
    (void)self;
    return 0;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEptEv(const IntrusivePtr* self) {
    return self->object;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120RequestPlayerSession20setInvitableUserTypeERKNS3_17InvitableUserTypeE() {
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V132RequestCreatePlayerSessionPlayerEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V132RequestCreatePlayerSessionPlayerEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_6StringEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    return VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element));
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_6StringEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x30);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V129PostPlayerSessionsRequestBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x30);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEEEdeEv(const IntrusivePtr* self) {
    return self->object;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V115LocalizedStringEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V151PostPlayerSessionsSessionIdMemberPlayersRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    CopyIntrusivePtr(self, source, 0x00);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi25joinPlayerSessionAsPlayerEiRKNS4_36ParameterToJoinPlayerSessionAsPlayerERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_52PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6String6appendEPKc(CommonString* self, const char* text) {
    if (text == nullptr) return 0;
    const std::size_t textLength = std::strlen(text);
    const std::size_t oldLength = self->buffer != nullptr ? std::strlen(self->buffer) : 0;
    const std::size_t needed = oldLength + textLength + 1;
    if (needed > self->bufferSize) {
        std::size_t capacity = self->bufferSize != 0 ? static_cast<std::size_t>(self->bufferSize) : 16;
        while (capacity < needed) capacity *= 2;
        char* buffer = static_cast<char*>(std::malloc(capacity));
        if (buffer == nullptr) return -1;
        if (oldLength != 0) std::memcpy(buffer, self->buffer, oldLength);
        std::free(self->buffer);
        self->buffer = buffer;
        self->bufferSize = capacity;
    }
    std::memcpy(self->buffer + oldLength, text, textLength + 1);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V132RequestCreatePlayerSessionPlayerEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    return VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element));
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15DefaultResponseC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToJoinPlayerSessionAsPlayer9terminateEv() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi31ParameterToCreatePlayerSessions10initializeEPNS1_6Common10LibContextENS6_12IntrusivePtrINS3_29PostPlayerSessionsRequestBodyEEE() {
    return 0;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V110FromMemberEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V132RequestPlayerSessionMemberPlayerEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x30);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi29ParameterToLeavePlayerSessionC1Ev() {
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead12getSessionIdEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V129PostPlayerSessionsRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    CopyIntrusivePtr(self, source, 0x00);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi18leavePlayerSessionEiRKNS4_29ParameterToLeavePlayerSessionERNS1_6Common11TransactionINS8_15DefaultResponseENS8_12IntrusivePtrINS8_18ResponseHeaderBaseEEEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V124PlayerSessionPushContextEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    return VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element));
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi31ParameterToCreatePlayerSessionsD1Ev() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi27getPlayerSessionInvitationsEiRKNS4_38ParameterToGetPlayerSessionInvitationsERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_54GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V132RequestPlayerSessionMemberPlayerEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEEdeEv(const IntrusivePtr* self) {
    return self->object;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V124PlayerSessionPushContextEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V110FromMemberEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEptEv(const IntrusivePtr* self) {
    return self->object;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38ParameterToGetPlayerSessionInvitationsC1Ev() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    return VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element));
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120RequestPlayerSession19setJoinableUserTypeERKNS3_16JoinableUserTypeE() {
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38ParameterToGetPlayerSessionInvitationsD1Ev() {
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x30);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V132RequestPlayerSessionMemberPlayerEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    CopyIntrusivePtr(self, source, 0x00);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi31ParameterToCreatePlayerSessionsC1Ev() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToJoinPlayerSessionAsPlayer10initializeEPNS1_6Common10LibContextEPKcNS6_12IntrusivePtrINS3_51PostPlayerSessionsSessionIdMemberPlayersRequestBodyEEE() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi29ParameterToLeavePlayerSession10initializeEPNS1_6Common10LibContextEPKcSA_() {
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V115LocalizedStringEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS2_6StringEEEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    CopyIntrusivePtr(self, source, 0x30);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38ParameterToGetPlayerSessionInvitations9setfieldsENS1_6Common12IntrusivePtrINS6_6VectorINS6_6StringEEEEE() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15DefaultResponseD1Ev(void* self) {
    (void)self;
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS2_6StringEEEEC1EPS6_PFvS8_EPNS2_10LibContextE(IntrusivePtr* self, void* body, void (APS5_VABI* deleter)(void*), void* libContext) {
    self->object = body;
    self->deleter = deleter;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V133RequestPlayerSessionPlayerFactory6createEPNS1_6Common10LibContextEPKcS9_PNS5_12IntrusivePtrINS3_26RequestPlayerSessionPlayerEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi20createPlayerSessionsEiRKNS4_31ParameterToCreatePlayerSessionsERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_30PostPlayerSessionsResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToJoinPlayerSessionAsPlayerC1Ev() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V126RequestPlayerSessionPlayer15setPushContextsERKNS1_6Common6VectorINS5_12IntrusivePtrINS3_24PlayerSessionPushContextEEEEE() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V127RequestPlayerSessionFactory6createEPNS1_6Common10LibContextEiNS5_12IntrusivePtrINS3_32RequestPlayerSessionMemberPlayerEEERKNS5_6VectorINS5_6StringEEENS8_INS3_15LocalizedStringEEEPNS8_INS3_20RequestPlayerSessionEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V124PlayerSessionPushContextEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V131PlayerSessionPushContextFactory6createEPNS1_6Common10LibContextEPKcPNS5_12IntrusivePtrINS3_24PlayerSessionPushContextEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V129PostPlayerSessionsRequestBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_6StringEE8pushBackERKS4_(IntrusivePtr* self, const void* element) {
    return VectorPushBackString(self, static_cast<const CommonString*>(element));
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38ParameterToGetPlayerSessionInvitations10initializeEPNS1_6Common10LibContextEPKc() {
    return 0;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS2_6StringEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x30);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V132RequestCreatePlayerSessionPlayerEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEE6finishEv() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x30);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEE11getResponseERS4_() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V139RequestCreatePlayerSessionPlayerFactory6createEPNS1_6Common10LibContextEPKcS9_RKNS5_6VectorINS5_12IntrusivePtrINS3_24PlayerSessionPushContextEEEEEPNSB_INS3_32RequestCreatePlayerSessionPlayerEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V124PlayerSessionPushContextEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V124PlayerSessionPushContextEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, 0x30);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V113PlayerSession12getSessionIdEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V158PostPlayerSessionsSessionIdMemberPlayersRequestBodyFactory6createEPNS1_6Common10LibContextERKNS5_6VectorINS5_12IntrusivePtrINS3_26RequestPlayerSessionPlayerEEEEEPNS9_INS3_51PostPlayerSessionsSessionIdMemberPlayersRequestBodyEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V136PostPlayerSessionsRequestBodyFactory6createEPNS1_6Common10LibContextERKNS5_6VectorINS5_12IntrusivePtrINS3_20RequestPlayerSessionEEEEEPNS9_INS3_29PostPlayerSessionsRequestBodyEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120RequestPlayerSession28setExclusiveLeaderPrivilegesERKNS1_6Common6VectorINS5_6StringEEE() {
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V123RecordScoreResponseBody10getTmpRankEv(const Leaderboards::RecordScoreResponseBody* self) {
    return self->tmpRank;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V123RecordScoreResponseBody16getTmpSerialRankEv(const Leaderboards::RecordScoreResponseBody* self) {
    return self->tmpSerialRank;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody11getSortModeEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->sortMode;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13getEntryLimitEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->entryLimit;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13getUpdateModeEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->updateMode;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13sortModeIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->sortModeSet;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody15entryLimitIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->entryLimitSet;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody15updateModeIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->updateModeSet;
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody16getMaxScoreLimitEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->maxScoreLimit;
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody16getMinScoreLimitEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->minScoreLimit;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody18maxScoreLimitIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->maxScoreLimitSet;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody18minScoreLimitIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->minScoreLimitSet;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody20getLargeDataNumLimitEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->largeDataNumLimit;
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody21getLargeDataSizeLimitEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->largeDataSizeLimit;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody22largeDataNumLimitIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->largeDataNumLimitSet;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody23largeDataSizeLimitIsSetEv(const Leaderboards::GetBoardDefinitionResponseBody* self) {
    return self->largeDataSizeLimitSet;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12commentIsSetEv(const Leaderboards::Entry* self) {
    return self->commentSet;
}

std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12getAccountIdEv(const Leaderboards::Entry* self) {
    return self->accountId;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry13getSerialRankEv(const Leaderboards::Entry* self) {
    return self->serialRank;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry13objectIdIsSetEv(const Leaderboards::Entry* self) {
    return self->objectIdSet;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry14getHighestRankEv(const Leaderboards::Entry* self) {
    return self->highestRank;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry14smallDataIsSetEv(const Leaderboards::Entry* self) {
    return self->smallDataSet;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry20getHighestSerialRankEv(const Leaderboards::Entry* self) {
    return self->highestSerialRank;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry7getPcIdEv(const Leaderboards::Entry* self) {
    return self->pcId;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry7getRankEv(const Leaderboards::Entry* self) {
    return self->rank;
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry8getScoreEv(const Leaderboards::Entry* self) {
    return self->score;
}

const IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12getSmallDataEv(const Leaderboards::Entry* self) {
    return &self->smallData;
}

IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122GetRankingResponseBody10getEntriesEv(IntrusivePtr* result, Leaderboards::GetRankingResponseBody* self) {
    CopyIntrusivePtr(result, &self->entries, VectorReferenceCountOffset);
    return result;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, VectorReferenceCountOffset);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    return VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element));
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEE5beginEv(VectorIterator* result, IntrusivePtr* self) {
    return VectorBegin(result, self);
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEE3endEv(VectorIterator* result, IntrusivePtr* self) {
    return VectorEnd(result, self);
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEppEv(VectorIterator* self) {
    return IteratorAdvance(self);
}

IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEdeEv(const VectorIterator* self) {
    return self->element;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEneERKS9_(const VectorIterator* self, const VectorIterator* other) {
    return self->element != other->element;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBody16invitationsIsSetEv(const SessionManager::GetUsersAccountIdPlayerSessionsInvitationsResponseBody* self) {
    return self->invitationsSet;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead14sessionIdIsSetEv(const SessionManager::UsersPlayerSessionsInvitationForRead* self) {
    return self->sessionIdSet;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead17invitationIdIsSetEv(const SessionManager::UsersPlayerSessionsInvitationForRead* self) {
    return self->invitationIdSet;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead22receivedTimestampIsSetEv(const SessionManager::UsersPlayerSessionsInvitationForRead* self) {
    return self->receivedTimestampSet;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead9fromIsSetEv(const SessionManager::UsersPlayerSessionsInvitationForRead* self) {
    return self->fromSet;
}

IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead7getFromEv(IntrusivePtr* result, const SessionManager::UsersPlayerSessionsInvitationForRead* self) {
    CopyIntrusivePtr(result, &self->from, ReferenceCountOffset);
    return result;
}

IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V130PostPlayerSessionsResponseBody17getPlayerSessionsEv(IntrusivePtr* result, SessionManager::PostPlayerSessionsResponseBody* self) {
    CopyIntrusivePtr(result, &self->playerSessions, VectorReferenceCountOffset);
    return result;
}

IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBody14getInvitationsEv(IntrusivePtr* result, SessionManager::GetUsersAccountIdPlayerSessionsInvitationsResponseBody* self) {
    CopyIntrusivePtr(result, &self->invitations, VectorReferenceCountOffset);
    return result;
}

IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEEEixEm(IntrusivePtr* self, std::uint64_t index) {
    return VectorElementAt(self, index);
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEE5beginEv(VectorIterator* result, IntrusivePtr* self) {
    return VectorBegin(result, self);
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEE3endEv(VectorIterator* result, IntrusivePtr* self) {
    return VectorEnd(result, self);
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEppEv(VectorIterator* self) {
    return IteratorAdvance(self);
}

IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEdeEv(const VectorIterator* self) {
    return self->element;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEneERKS9_(const VectorIterator* self, const VectorIterator* other) {
    return self->element != other->element;
}

static int CopyParameterText(char** slot, const char* text) {
    char* copy = nullptr;
    if (text != nullptr) {
        const std::size_t length = std::strlen(text) + 1;
        copy = static_cast<char*>(std::malloc(length));
        if (copy == nullptr) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
        std::memcpy(copy, text, length);
    }
    std::free(*slot);
    *slot = copy;
    return 0;
}

static TitleCloudStorage::DownloadDataParameter* DownloadDataState(ParameterObject* self) {
    if (self->state == nullptr) self->state = std::calloc(1, sizeof(TitleCloudStorage::DownloadDataParameter));
    return static_cast<TitleCloudStorage::DownloadDataParameter*>(self->state);
}

static void ReleaseDownloadDataState(ParameterObject* self) {
    auto* state = static_cast<TitleCloudStorage::DownloadDataParameter*>(self->state);
    if (state != nullptr) {
        std::free(state->requiredText);
        std::free(state->objectId);
        std::free(state);
    }
    self->state = nullptr;
}

void APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadDataC1Ev(ParameterObject* self) {
    self->state = nullptr;
}

void APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadDataD1Ev(ParameterObject* self) {
    ReleaseDownloadDataState(self);
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData10initializeEPNS1_6Common10LibContextEPKci(ParameterObject* self, void* libContext, const char* text, std::int32_t number) {
    auto* state = DownloadDataState(self);
    if (state == nullptr) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
    const int result = CopyParameterText(&state->requiredText, text);
    if (result != 0) return result;
    state->libContext = libContext;
    state->requiredNumber = number;
    state->initialized = true;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData11setobjectIdEPKc(ParameterObject* self, const char* objectId) {
    auto* state = DownloadDataState(self);
    if (state == nullptr) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
    return CopyParameterText(&state->objectId, objectId);
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData9terminateEv(ParameterObject* self) {
    ReleaseDownloadDataState(self);
    return 0;
}

static TitleCloudStorage::GetMultiDataStatusesByUserParameter* MultiDataStatusesState(ParameterObject* self) {
    if (self->state == nullptr) self->state = std::calloc(1, sizeof(TitleCloudStorage::GetMultiDataStatusesByUserParameter));
    return static_cast<TitleCloudStorage::GetMultiDataStatusesByUserParameter*>(self->state);
}

static void ReleaseMultiDataStatusesState(ParameterObject* self) {
    auto* state = static_cast<TitleCloudStorage::GetMultiDataStatusesByUserParameter*>(self->state);
    if (state != nullptr) {
        std::free(state->requiredText[0]);
        std::free(state->requiredText[1]);
        std::free(state->fields);
        std::free(state);
    }
    self->state = nullptr;
}

void APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUserC1Ev(ParameterObject* self) {
    self->state = nullptr;
}

void APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUserD1Ev(ParameterObject* self) {
    ReleaseMultiDataStatusesState(self);
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser10initializeEPNS1_6Common10LibContextEPKcSA_(ParameterObject* self, void* libContext, const char* first, const char* second) {
    auto* state = MultiDataStatusesState(self);
    if (state == nullptr) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
    int result = CopyParameterText(&state->requiredText[0], first);
    if (result == 0) result = CopyParameterText(&state->requiredText[1], second);
    if (result != 0) return result;
    state->libContext = libContext;
    state->initialized = true;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser9setfieldsEPKc(ParameterObject* self, const char* fields) {
    auto* state = MultiDataStatusesState(self);
    if (state == nullptr) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
    return CopyParameterText(&state->fields, fields);
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser9terminateEv(ParameterObject* self) {
    ReleaseMultiDataStatusesState(self);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi26getMultiDataStatusesByUserEiRKNS4_37ParameterToGetMultiDataStatusesByUserERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_32GetMultiDataStatusesResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_17TitleCloudStorage2V110DataStatusEEEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, VectorReferenceCountOffset);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_17TitleCloudStorage2V110DataStatusEEEEEEdeEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V15OwnerEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V15OwnerEEptEv(const IntrusivePtr* self) {
    return self->object;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V132GetMultiDataStatusesResponseBody23getTotalDataStatusCountEv(const TitleCloudStorage::GetMultiDataStatusesResponseBody* self) {
    return self->totalDataStatusCount;
}

IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V132GetMultiDataStatusesResponseBody17getDataStatusListEv(IntrusivePtr* result, TitleCloudStorage::GetMultiDataStatusesResponseBody* self) {
    CopyIntrusivePtr(result, &self->dataStatusList, VectorReferenceCountOffset);
    return result;
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus11getDataSizeEv(const TitleCloudStorage::DataStatus* self) {
    return self->dataSize;
}

std::int32_t APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus9getSlotIdEv(const TitleCloudStorage::DataStatus* self) {
    return self->slotId;
}

IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus8getOwnerEv(IntrusivePtr* result, const TitleCloudStorage::DataStatus* self) {
    CopyIntrusivePtr(result, &self->owner, ReferenceCountOffset);
    return result;
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus11getObjectIdEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V15Owner12getAccountIdEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEE5beginEv(VectorIterator* result, IntrusivePtr* self) {
    return VectorBegin(result, self);
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEE3endEv(VectorIterator* result, IntrusivePtr* self) {
    return VectorEnd(result, self);
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEppEv(VectorIterator* self) {
    return IteratorAdvance(self);
}

IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEdeEv(const VectorIterator* self) {
    return self->element;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEneERKS9_(const VectorIterator* self, const VectorIterator* other) {
    return self->element != other->element;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEED2Ev(void* self) {
    (void)self;
    return 0;
}

static InGameCatalog::GetContainerParameter* GetContainerState(ParameterObject* self) {
    if (self->state == nullptr) self->state = std::calloc(1, sizeof(InGameCatalog::GetContainerParameter));
    return static_cast<InGameCatalog::GetContainerParameter*>(self->state);
}

void APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainerC1Ev(ParameterObject* self) {
    self->state = nullptr;
}

void APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainerD1Ev(ParameterObject* self) {
    std::free(self->state);
    self->state = nullptr;
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer10initializeEPNS1_6Common10LibContextE(ParameterObject* self, void* libContext) {
    auto* state = GetContainerState(self);
    if (state == nullptr) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
    state->libContext = libContext;
    state->initialized = true;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer15setserviceLabelEi(ParameterObject* self, std::int32_t serviceLabel) {
    auto* state = GetContainerState(self);
    if (state == nullptr) return SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY;
    state->serviceLabel = serviceLabel;
    state->serviceLabelSet = true;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer9terminateEv(ParameterObject* self) {
    std::free(self->state);
    self->state = nullptr;
    return 0;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer13isInitializedEv(const ParameterObject* self) {
    const auto* state = static_cast<const InGameCatalog::GetContainerParameter*>(self->state);
    return state != nullptr && state->initialized;
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi12getContainerEiRKNS4_23ParameterToGetContainerERNS1_6Common11TransactionINS8_12IntrusivePtrINS8_6VectorINSA_INS3_9ContainerEEEEEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERSB_() {
    return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V59ContainerEEEEEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V59ContainerEEEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, VectorReferenceCountOffset);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V59ContainerEEEEEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V59ContainerEEEEEEdeEv(const IntrusivePtr* self) {
    return self->object;
}

std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEE4sizeEv(const IntrusivePtr* self) {
    return VectorSize(self);
}

IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEEixEm(IntrusivePtr* self, std::uint64_t index) {
    return VectorElementAt(self, index);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    CopyIntrusivePtr(self, source, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V59ContainerEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEptEv(const IntrusivePtr* self) {
    return self->object;
}

IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V59Container11getChildrenEv(IntrusivePtr* result, InGameCatalog::Container* self) {
    CopyIntrusivePtr(result, &self->children, VectorReferenceCountOffset);
    return result;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V57ProductEEEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, VectorReferenceCountOffset);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V57ProductEEEEEEptEv(const IntrusivePtr* self) {
    return self->object;
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEE5beginEv(VectorIterator* result, IntrusivePtr* self) {
    return VectorBegin(result, self);
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEE3endEv(VectorIterator* result, IntrusivePtr* self) {
    return VectorEnd(result, self);
}

VectorIterator* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEEppEv(VectorIterator* self) {
    return IteratorAdvance(self);
}

IntrusivePtr* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEEdeEv(const VectorIterator* self) {
    return self->element;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEEneERKS9_(const VectorIterator* self, const VectorIterator* other) {
    return self->element != other->element;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEED2Ev(void* self) {
    (void)self;
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V57ProductEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    CopyIntrusivePtr(self, source, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V57ProductEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V57ProductEEptEv(const IntrusivePtr* self) {
    return self->object;
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product9skusIsSetEv(const InGameCatalog::Product* self) {
    return self->skusSet;
}

IntrusivePtr* APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V57Product7getSkusEv(IntrusivePtr* result, InGameCatalog::Product* self) {
    CopyIntrusivePtr(result, &self->skus, VectorReferenceCountOffset);
    return result;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V53SkuEEEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, VectorReferenceCountOffset);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V53SkuEEEEEEptEv(const IntrusivePtr* self) {
    return self->object;
}

std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V53SkuEEEE4sizeEv(const IntrusivePtr* self) {
    return VectorSize(self);
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product8getLabelEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return nullptr;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product7getTypeEv(const void* self) {
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi10getFriendsEiRKNS4_21ParameterToGetFriendsERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_18GetFriendsResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi21ParameterToGetFriends10initializeEPNS1_6Common10LibContextEPKc() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi21ParameterToGetFriends9terminateEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi21ParameterToGetFriendsC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi21ParameterToGetFriendsD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V111PresenceApi17getBasicPresencesEiRKNS4_28ParameterToGetBasicPresencesERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_25GetBasicPresencesResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V111PresenceApi28ParameterToGetBasicPresences10initializeEPNS1_6Common10LibContextEPKc() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V111PresenceApi28ParameterToGetBasicPresences9terminateEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V111PresenceApi28ParameterToGetBasicPresencesC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V111PresenceApi28ParameterToGetBasicPresencesD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V118GetFriendsResponse10getFriendsEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V124GetBlockingUsersResponse9getBlocksEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V125GetBasicPresencesResponse17getBasicPresencesEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi16getBlockingUsersEiRKNS4_27ParameterToGetBlockingUsersERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_24GetBlockingUsersResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi27ParameterToGetBlockingUsers10initializeEPNS1_6Common10LibContextE() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi27ParameterToGetBlockingUsers9terminateEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi27ParameterToGetBlockingUsersC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi27ParameterToGetBlockingUsersD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V212ProfanityApi15filterProfanityEiRKNS4_26ParameterToFilterProfanityERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_23FilterProfanityResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V212ProfanityApi26ParameterToFilterProfanity10initializeEPNS1_6Common10LibContextEPKcSA_NS6_12IntrusivePtrINS3_19WebApiFilterRequestEEE() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V212ProfanityApi26ParameterToFilterProfanity9terminateEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V212ProfanityApi26ParameterToFilterProfanityC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V212ProfanityApi26ParameterToFilterProfanityD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V226WebApiFilterRequestFactory6createEPNS1_6Common10LibContextEPKcPNS5_12IntrusivePtrINS3_19WebApiFilterRequestEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi30CommunicationRestrictionStatus2V333CommunicationRestrictionStatusApi33getCommunicationRestrictionStatusEiRKNS4_44ParameterToGetCommunicationRestrictionStatusERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_38CommunicationRestrictionStatusResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi30CommunicationRestrictionStatus2V333CommunicationRestrictionStatusApi44ParameterToGetCommunicationRestrictionStatus10initializeEPNS1_6Common10LibContextEm() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi30CommunicationRestrictionStatus2V333CommunicationRestrictionStatusApi44ParameterToGetCommunicationRestrictionStatus9terminateEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi30CommunicationRestrictionStatus2V333CommunicationRestrictionStatusApi44ParameterToGetCommunicationRestrictionStatusC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi30CommunicationRestrictionStatus2V333CommunicationRestrictionStatusApi44ParameterToGetCommunicationRestrictionStatusD1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE21setOnFinishedCallbackEPFvliS8_PvEPFvliSC_ESC_() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE21setOnFinishedCallbackEPFvliS8_PvEPFvliSC_ESC_() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE21setOnFinishedCallbackEPFvliS8_PvEPFvliSC_ESC_() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEE21setOnFinishedCallbackEPFvliS8_PvEPFvliSC_ESC_() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE21setOnFinishedCallbackEPFvliS8_PvEPFvliSC_ESC_() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    (void)self;
    return 0;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_15ProfanityFilter2V219WebApiFilterRequestEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    CopyIntrusivePtr(self, source, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_15ProfanityFilter2V219WebApiFilterRequestEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_15ProfanityFilter2V219WebApiFilterRequestEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_18ResponseHeaderBaseEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_18ResponseHeaderBaseEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6StringEEC1Ev(IntrusivePtr* self) {
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6StringEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V113BasicPresenceEEEEEED1Ev(IntrusivePtr* self) {
    ReleaseIntrusivePtr(self, VectorReferenceCountOffset);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEED2Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorImED2Ev(void* self) {
    (void)self;
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE28setResponseInformationOptionEP37SceNpWebApi2ResponseInformationOption() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE28setResponseInformationOptionEP37SceNpWebApi2ResponseInformationOption() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE28setResponseInformationOptionEP37SceNpWebApi2ResponseInformationOption() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEE28setResponseInformationOptionEP37SceNpWebApi2ResponseInformationOption() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE28setResponseInformationOptionEP37SceNpWebApi2ResponseInformationOption() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
 return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common18ResponseHeaderBase14getHeaderValueEPKcRNS2_12IntrusivePtrINS2_6StringEEE() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEE3endEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEE5beginEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorImE3endEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorImE5beginEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEEppEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorImEppEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V113BasicPresence12getAccountIdEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V113BasicPresence12getInContextEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V113BasicPresence15getOnlineStatusEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi15ProfanityFilter2V223FilterProfanityResponse10getMessageEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponse13getRestrictedEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_18ResponseHeaderBaseEE3getEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_18ResponseHeaderBaseEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6StringEE3getEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6StringEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V113BasicPresenceEEEEEEdeEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V113BasicPresenceEEEEEEptEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorImEEEdeEv(const IntrusivePtr* self) {
    return self->object;
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorImEEEptEv(const IntrusivePtr* self) {
    return self->object;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE18getResponseHeadersERSA_() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE18getResponseHeadersERSA_() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE18getResponseHeadersERSA_() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEE18getResponseHeadersERSA_() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE18getResponseHeadersERSA_() {
 return SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEE4sizeEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorImE4sizeEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEEdeEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEEneERKS9_() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorImEdeEv() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorImEneERKS4_() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
}
