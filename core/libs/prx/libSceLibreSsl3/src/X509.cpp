#include "prx/libSceLibreSsl/include/LibreSsl.hpp"
#include "prx/libc/include/General.hpp"

using namespace LibreSsl;

extern "C" {
unsigned char* APS5_VABI ASN1_STRING_data_nid_postfix(Asn1String* x);

int APS5_VABI X509_up_ref_nid_postfix(X509Object* x) {
    return LibreSslX509UpRef_nid_no_patch(x);
}

X509Object* APS5_VABI d2i_X509_bio_nid_postfix(Bio* bp, X509Object** x509) {
    return LibreSslD2iX509Bio_nid_no_patch(bp, x509);
}

std::int64_t APS5_VABI X509_get_version_nid_postfix(const X509Object* x) {
    return LibreSslX509Version_nid_no_patch(x);
}

const Asn1String* APS5_VABI X509_get0_notBefore_nid_postfix(const X509Object* x) {
    return LibreSslX509Time_nid_no_patch(x, 0);
}

const Asn1String* APS5_VABI X509_get0_notAfter_nid_postfix(const X509Object* x) {
    return LibreSslX509Time_nid_no_patch(x, 1);
}

Asn1String* APS5_VABI X509_getm_notBefore_nid_postfix(const X509Object* x) {
    return LibreSslX509Time_nid_no_patch(x, 0);
}

Asn1String* APS5_VABI X509_getm_notAfter_nid_postfix(const X509Object* x) {
    return LibreSslX509Time_nid_no_patch(x, 1);
}

int APS5_VABI X509_get_ext_count_nid_postfix(const X509Object* x) {
    return LibreSslX509ExtensionCount_nid_no_patch(x);
}

const unsigned char* APS5_VABI ASN1_STRING_get0_data_nid_postfix(const Asn1String* x) {
    return ASN1_STRING_data_nid_postfix(const_cast<Asn1String*>(x));
}

Asn1String* APS5_VABI d2i_ASN1_INTEGER_nid_postfix(Asn1String** a, const unsigned char** pp, std::int64_t length) {
    return LibreSslD2iAsn1Integer_nid_no_patch(a, pp, length);
}

void APS5_VABI ASN1_INTEGER_free_nid_postfix(Asn1String* a) {
    LibreSslAsn1IntegerFree_nid_no_patch(a);
}

void APS5_VABI ASN1_OBJECT_free_nid_postfix(Asn1Object* a) {
    LibreSslAsn1ObjectFree_nid_no_patch(a);
}

}
