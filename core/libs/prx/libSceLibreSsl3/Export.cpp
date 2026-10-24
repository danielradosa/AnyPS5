#include <cstddef>
#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

APS5_DUMMY_FUN
int APS5_VABI CRYPTO_memcmp_nid_postfix(const void* first, const void* second, std::size_t length) {
    const auto* firstBytes = static_cast<const volatile unsigned char*>(first);
    const auto* secondBytes = static_cast<const volatile unsigned char*>(second);
    unsigned int difference = 0;
    for (std::size_t index = 0; index < length; ++index)
        difference |= firstBytes[index] ^ secondBytes[index];
    return static_cast<int>(difference);
}

int APS5_VABI ERR_print_errors_cb_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_CERTID_free_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_cert_to_id_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI OCSP_resp_find_status_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI PKCS7_free_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI d2i_PKCS7_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
