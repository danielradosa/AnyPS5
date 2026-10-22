#include "prx/libSceLibreSsl/include/LibreSsl.hpp"
#include "prx/libc/include/General.hpp"

using namespace LibreSsl;

extern "C" {
int APS5_VABI BIO_free_nid_postfix(Bio* a);

int APS5_VABI BIO_up_ref_nid_postfix(Bio* bio) {
    return LibreSslBioUpRef_nid_no_patch(bio);
}

void APS5_VABI BIO_vfree_nid_postfix(Bio* a) {
    BIO_free_nid_postfix(a);
}

std::uint64_t APS5_VABI ERR_peek_last_error_nid_postfix() {
    return LibreSslPeekLastError_nid_no_patch();
}

void APS5_VABI ERR_load_crypto_strings_nid_postfix() {
}

}
