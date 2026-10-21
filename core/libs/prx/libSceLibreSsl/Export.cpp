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

int APS5_VABI ERR_error_string_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ERR_error_string_n_nid_postfix(void) {
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

int APS5_VABI OCSP_cert_status_str_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_check_validity_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_crl_reason_str_nid_postfix(void) {
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

int APS5_VABI OCSP_response_status_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_response_status_str_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_single_get0_status_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI RSA_flags_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_CTX_get_cert_store_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_CTX_set_alpn_select_cb_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_CTX_set_cert_store_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_CTX_set_cert_verify_callback_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_CTX_set_client_cert_cb_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_CTX_set_info_callback_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_CTX_set_msg_callback_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_CTX_set_next_proto_select_cb_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_CTX_use_PrivateKey_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_get0_param_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_get_ex_data_X509_STORE_CTX_idx_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI SSL_get_privatekey_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI UI_OpenSSL_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI UI_get0_user_data_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI UI_get_input_flags_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI UI_get_string_type_nid_postfix(void) {
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
