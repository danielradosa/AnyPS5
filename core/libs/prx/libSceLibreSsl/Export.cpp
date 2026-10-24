#include <cinttypes>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "mbedtls/des.h"

namespace {

constexpr std::size_t DesScheduleBytes = 128;

}

extern "C" {

APS5_DUMMY_FUN
int APS5_VABI BN_print_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI CRYPTO_THREADID_set_callback_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

void APS5_VABI DES_ecb_encrypt_nid_postfix(const unsigned char* input, unsigned char* output, const unsigned char* schedule, int enc) {
    if (input == nullptr || output == nullptr || schedule == nullptr) APS5_INVALID_ARG_EX;
    mbedtls_des_context context;
    mbedtls_des_init(&context);
    unsigned char block[MBEDTLS_DES_KEY_SIZE];
    std::memcpy(block, input, sizeof(block));
    int status = enc ? mbedtls_des_setkey_enc(&context, schedule) : mbedtls_des_setkey_dec(&context, schedule);
    if (status == 0) status = mbedtls_des_crypt_ecb(&context, block, output);
    mbedtls_des_free(&context);
    if (status != 0) throw std::runtime_error("DES_ecb_encrypt: mbedtls failed");
}

int APS5_VABI DES_set_key_nid_postfix(const unsigned char* key, unsigned char* schedule) {
    if (key == nullptr || schedule == nullptr) APS5_INVALID_ARG_EX;
    std::memset(schedule, 0, DesScheduleBytes);
    std::memcpy(schedule, key, MBEDTLS_DES_KEY_SIZE);
    return 0;
}

void APS5_VABI DES_set_odd_parity_nid_postfix(unsigned char* key) {
    if (key == nullptr) APS5_INVALID_ARG_EX;
    mbedtls_des_key_set_parity(key);
}

int APS5_VABI DTLSv1_client_method_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI DTLSv1_server_method_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ENGINE_ctrl_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ENGINE_get_first_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ENGINE_get_next_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_BASICRESP_free_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_RESPONSE_free_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_basic_add1_cert_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_basic_verify_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_check_validity_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_resp_count_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_resp_get0_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_response_get1_basic_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_CTX_set_msg_callback_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI UI_OpenSSL_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI UI_set_result_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI X509V3_EXT_print_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI X509_NAME_oneline_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI X509_print_ex_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

const char* APS5_VABI X509_verify_cert_error_string_nid_postfix(std::int64_t error) {
    static constexpr const char* messages[] {
        "ok",
        nullptr,
        "unable to get issuer certificate",
        "unable to get certificate CRL",
        "unable to decrypt certificate's signature",
        "unable to decrypt CRL's signature",
        "unable to decode issuer public key",
        "certificate signature failure",
        "CRL signature failure",
        "certificate is not yet valid",
        "certificate has expired",
        "CRL is not yet valid",
        "CRL has expired",
        "format error in certificate's notBefore field",
        "format error in certificate's notAfter field",
        "format error in CRL's lastUpdate field",
        "format error in CRL's nextUpdate field",
        "out of memory",
        "self signed certificate",
        "self signed certificate in certificate chain",
        "unable to get local issuer certificate",
        "unable to verify the first certificate",
        "certificate chain too long",
        "certificate revoked",
        "invalid CA certificate",
        "path length constraint exceeded",
        "unsupported certificate purpose",
        "certificate not trusted",
        "certificate rejected",
        "subject issuer mismatch",
        "authority and subject key identifier mismatch",
        "authority and issuer serial number mismatch",
        "key usage does not include certificate signing",
        "unable to get CRL issuer certificate",
        "unhandled critical extension",
        "key usage does not include CRL signing",
        "unhandled critical CRL extension",
        "invalid non-CA certificate (has CA markings)",
        "proxy path length constraint exceeded",
        "key usage does not include digital signature",
        "proxy certificates not allowed, please set the appropriate flag",
        "invalid or inconsistent certificate extension",
        "invalid or inconsistent certificate policy extension",
        "no explicit policy",
        "Different CRL scope",
        "Unsupported extension feature",
        "RFC 3779 resource not subset of parent's resources",
        "permitted subtree violation",
        "excluded subtree violation",
        "name constraints minimum and maximum not supported",
        "application verification failure",
        "unsupported name constraint type",
        "unsupported or invalid name constraint syntax",
        "unsupported or invalid name syntax",
        "CRL path validation error",
    };
    const auto code = static_cast<std::uint32_t>(error);
    if (code < sizeof(messages) / sizeof(messages[0]) && messages[code]) return messages[code];
    static char unknown[100];
    std::snprintf(unknown, sizeof(unknown), "error number %" PRId64, error);
    return unknown;
}

int APS5_VABI d2i_OCSP_RESPONSE_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI i2t_ASN1_OBJECT_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceLibreSslGetMemoryPoolStats(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceLibreSslInit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceLibreSslInitEx(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceLibreSslInitEx2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceLibreSslTerm(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
