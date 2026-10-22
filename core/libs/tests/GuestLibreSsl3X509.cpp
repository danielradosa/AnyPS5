#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

struct GuestAsn1String {
    int length;
    int type;
    unsigned char* data;
    std::int64_t flags;
};

struct GuestAsn1Object {
    const char* sn;
    const char* ln;
    int nid;
    int length;
    const unsigned char* data;
    int flags;
};

struct GuestNameEntry {
    GuestAsn1Object* object;
    GuestAsn1String* value;
    int set;
    int size;
};

struct GuestX509;
struct GuestName;
struct GuestBio;
struct GuestBioMethod;

extern "C" {
int APS5_VABI X509_up_ref_nid_postfix(GuestX509*);
GuestX509* APS5_VABI d2i_X509_bio_nid_postfix(GuestBio*, GuestX509**);
std::int64_t APS5_VABI X509_get_version_nid_postfix(const GuestX509*);
const GuestAsn1String* APS5_VABI X509_get0_notBefore_nid_postfix(const GuestX509*);
const GuestAsn1String* APS5_VABI X509_get0_notAfter_nid_postfix(const GuestX509*);
GuestAsn1String* APS5_VABI X509_getm_notBefore_nid_postfix(const GuestX509*);
GuestAsn1String* APS5_VABI X509_getm_notAfter_nid_postfix(const GuestX509*);
int APS5_VABI X509_get_ext_count_nid_postfix(const GuestX509*);
const unsigned char* APS5_VABI ASN1_STRING_get0_data_nid_postfix(const GuestAsn1String*);
GuestAsn1String* APS5_VABI d2i_ASN1_INTEGER_nid_postfix(GuestAsn1String**, const unsigned char**, std::int64_t);
void APS5_VABI ASN1_INTEGER_free_nid_postfix(GuestAsn1String*);
void APS5_VABI ASN1_OBJECT_free_nid_postfix(GuestAsn1Object*);

void APS5_VABI X509_free_nid_postfix(GuestX509*);
GuestName* APS5_VABI X509_get_subject_name_nid_postfix(GuestX509*);
GuestNameEntry* APS5_VABI X509_NAME_get_entry_nid_postfix(GuestName*, int);
GuestAsn1String* APS5_VABI X509_get_serialNumber_nid_postfix(GuestX509*);
std::int64_t APS5_VABI ASN1_INTEGER_get_nid_postfix(const GuestAsn1String*);
GuestBio* APS5_VABI BIO_new_mem_buf_nid_postfix(void*, int);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
}

static const char certificate[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICfjCCAiSgAwIBAgIFAPECAwQwCgYIKoZIzj0EAwIwgY4xCzAJBgNVBAYTAkNa\n"
    "MQ4wDAYDVQQIDAVQcmFoYTEUMBIGA1UECgwLQW55UFM1IFRlc3QxDzANBgNVBAsM\n"
    "BlVuaXQgQTEPMA0GA1UECwwGVW5pdCBCMRYwFAYDVQQDDA1saWJyZXNzbC50ZXN0\n"
    "MR8wHQYJKoZIhvcNAQkBFhB0ZXN0QGV4YW1wbGUuY29tMB4XDTI0MDEwMTAwMDAw\n"
    "MFoXDTQ5MTIzMTIzNTk1OVowgY4xCzAJBgNVBAYTAkNaMQ4wDAYDVQQIDAVQcmFo\n"
    "YTEUMBIGA1UECgwLQW55UFM1IFRlc3QxDzANBgNVBAsMBlVuaXQgQTEPMA0GA1UE\n"
    "CwwGVW5pdCBCMRYwFAYDVQQDDA1saWJyZXNzbC50ZXN0MR8wHQYJKoZIhvcNAQkB\n"
    "FhB0ZXN0QGV4YW1wbGUuY29tMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEbgQg\n"
    "mTFAnaedLqP/ezD9wt+oZsFphS8kxoJ1psd/q7LOm4yftb7Z9rZBVIMxOzY9n+rZ\n"
    "aTyOwm6La3yjLYqQ8aNtMGswHQYDVR0OBBYEFNyTpGP4BtKV2b14aZ5PX/3pKv30\n"
    "MB8GA1UdIwQYMBaAFNyTpGP4BtKV2b14aZ5PX/3pKv30MA8GA1UdEwEB/wQFMAMB\n"
    "Af8wGAYDVR0RBBEwD4INbGlicmVzc2wudGVzdDAKBggqhkjOPQQDAgNIADBFAiEA\n"
    "+6x/q2Vjz4mi2NmQaOopcDpXuXWo6IW0h8nvtG/VIVMCIGIcu4kdNgzoqe4AX+yf\n"
    "UveV6abNQUakRsAZjb8YmoHS\n"
    "-----END CERTIFICATE-----\n";

static void Require(bool value) {
    if (!value) std::abort();
}

template <typename TCall>
static bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

static std::string Der() {
    const std::string text(certificate);
    const auto begin = text.find('\n') + 1;
    const auto end = text.find("-----END");
    std::string der;
    unsigned value = 0;
    int bits = 0;
    for (auto index = begin; index < end; ++index) {
        const char character = text[index];
        int digit = -1;
        if (character >= 'A' && character <= 'Z') digit = character - 'A';
        else if (character >= 'a' && character <= 'z') digit = character - 'a' + 26;
        else if (character >= '0' && character <= '9') digit = character - '0' + 52;
        else if (character == '+') digit = 62;
        else if (character == '/') digit = 63;
        if (digit < 0) continue;
        value = value << 6 | static_cast<unsigned>(digit);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            der += static_cast<char>((value >> bits) & 0xff);
        }
    }
    return der;
}

static std::string Text(const GuestAsn1String* string) {
    return std::string(reinterpret_cast<const char*>(string->data), static_cast<std::size_t>(string->length));
}

static void TestCertificate() {
    std::string der = Der() + "next";
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(der.data(), static_cast<int>(der.size()));
    GuestX509* out = nullptr;
    GuestX509* x = d2i_X509_bio_nid_postfix(bio, &out);
    Require(x && out == x && BIO_ctrl_nid_postfix(bio, 10, 0, nullptr) == 4);
    Require(d2i_X509_bio_nid_postfix(bio, nullptr) == nullptr && BIO_ctrl_nid_postfix(bio, 10, 0, nullptr) == 4);
    Require(Throws([&] { d2i_X509_bio_nid_postfix(bio, &out); }));
    BIO_free_nid_postfix(bio);

    Require(X509_get_version_nid_postfix(x) == 2);
    const GuestAsn1String* notBefore = X509_get0_notBefore_nid_postfix(x);
    Require(notBefore && notBefore == X509_getm_notBefore_nid_postfix(x) && notBefore->type == 23 && Text(notBefore) == "240101000000Z");
    const GuestAsn1String* notAfter = X509_get0_notAfter_nid_postfix(x);
    Require(notAfter && notAfter == X509_getm_notAfter_nid_postfix(x) && Text(notAfter) == "491231235959Z");
    Require(X509_get_ext_count_nid_postfix(x) == 4);
    Require(ASN1_STRING_get0_data_nid_postfix(notAfter) == notAfter->data);
    GuestNameEntry* country = X509_NAME_get_entry_nid_postfix(X509_get_subject_name_nid_postfix(x), 0);
    ASN1_OBJECT_free_nid_postfix(country->object);
    ASN1_OBJECT_free_nid_postfix(nullptr);
    Require(country->object->nid == 14);
    Require(Throws([&] { ASN1_INTEGER_free_nid_postfix(X509_get_serialNumber_nid_postfix(x)); }));

    Require(X509_up_ref_nid_postfix(x) == 1);
    X509_free_nid_postfix(x);
    Require(X509_get_version_nid_postfix(x) == 2);
    X509_free_nid_postfix(x);
}

static void TestInteger() {
    const unsigned char encoded[] = {0x02, 0x02, 0xff, 0x7f, 0x02, 0x01, 0x05};
    const unsigned char* cursor = encoded;
    GuestAsn1String* out = nullptr;
    GuestAsn1String* negative = d2i_ASN1_INTEGER_nid_postfix(&out, &cursor, sizeof(encoded));
    Require(negative && out == negative && cursor == encoded + 4);
    Require(negative->type == (2 | 0x100) && negative->length == 1 && negative->data[0] == 0x81);
    Require(ASN1_INTEGER_get_nid_postfix(negative) == -129);
    GuestAsn1String* five = d2i_ASN1_INTEGER_nid_postfix(nullptr, &cursor, 3);
    Require(five && cursor == encoded + 7 && ASN1_INTEGER_get_nid_postfix(five) == 5);
    Require(d2i_ASN1_INTEGER_nid_postfix(nullptr, &cursor, 0) == nullptr);
    const unsigned char wrong[] = {0x04, 0x01, 0x00};
    cursor = wrong;
    Require(d2i_ASN1_INTEGER_nid_postfix(nullptr, &cursor, sizeof(wrong)) == nullptr && cursor == wrong);
    ASN1_INTEGER_free_nid_postfix(negative);
    ASN1_INTEGER_free_nid_postfix(five);
    ASN1_INTEGER_free_nid_postfix(nullptr);
}

int main() {
    TestCertificate();
    TestInteger();
    return 0;
}
