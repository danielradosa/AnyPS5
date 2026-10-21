#include "prx/libSceLibreSsl/include/LibreSsl.hpp"
#include "prx/libc/include/General.hpp"

using namespace LibreSsl;

extern "C" {

Extension* APS5_VABI X509_get_ext_nid_postfix(const X509Object* x, int loc) {
    return LibreSslX509GetExt_nid_no_patch(x, loc);
}

const Stack* APS5_VABI X509_get0_extensions_nid_postfix(const X509Object* x) {
    return LibreSslX509Get0Extensions_nid_no_patch(x);
}

void APS5_VABI X509_get0_signature_nid_postfix(const Asn1String** psig, const Algor** palg, const X509Object* x) {
    LibreSslX509Get0Signature_nid_no_patch(psig, palg, x);
}

int APS5_VABI X509_PUBKEY_get0_param_nid_postfix(const Asn1Object** ppkalg, const unsigned char** pk, int* ppklen, Algor** pa, const Pubkey* pub) {
    return LibreSslX509PubkeyGet0Param_nid_no_patch(ppkalg, pk, ppklen, pa, pub);
}

}
