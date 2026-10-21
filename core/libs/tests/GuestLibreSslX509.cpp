#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
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

struct GuestStack {
    int num;
    char** data;
    int sorted;
    int num_alloc;
    void* comp;
};

struct GuestNameEntry {
    GuestAsn1Object* object;
    GuestAsn1String* value;
    int set;
    int size;
};

struct GuestName {
    GuestStack* entries;
};

struct GuestValidity {
    GuestAsn1String* notBefore;
    GuestAsn1String* notAfter;
};

struct GuestCertInfo {
    GuestAsn1String* version;
    GuestAsn1String* serialNumber;
    void* signature;
    GuestName* issuer;
    GuestValidity* validity;
    GuestName* subject;
};

struct GuestX509 {
    GuestCertInfo* cert_info;
    void* sig_alg;
    GuestAsn1String* signature;
    int valid;
    int references;
};

struct GuestBio;
struct GuestBioMethod;
struct FileStream;

extern "C" {
GuestX509* APS5_VABI d2i_X509_nid_postfix(GuestX509**, const unsigned char**, std::int64_t);
GuestX509* APS5_VABI PEM_read_bio_X509_nid_postfix(GuestBio*, GuestX509**, void*, void*);
GuestX509* APS5_VABI PEM_read_X509_nid_postfix(FileStream*, GuestX509**, void*, void*);
void APS5_VABI X509_free_nid_postfix(GuestX509*);
GuestName* APS5_VABI X509_get_subject_name_nid_postfix(GuestX509*);
GuestName* APS5_VABI X509_get_issuer_name_nid_postfix(GuestX509*);
GuestAsn1String* APS5_VABI X509_get_serialNumber_nid_postfix(GuestX509*);
int APS5_VABI X509_NAME_get_index_by_NID_nid_postfix(GuestName*, int, int);
GuestNameEntry* APS5_VABI X509_NAME_get_entry_nid_postfix(GuestName*, int);
GuestAsn1String* APS5_VABI X509_NAME_ENTRY_get_data_nid_postfix(GuestNameEntry*);
int APS5_VABI X509_NAME_get_text_by_NID_nid_postfix(GuestName*, int, char*, int);
unsigned char* APS5_VABI ASN1_STRING_data_nid_postfix(GuestAsn1String*);
int APS5_VABI ASN1_STRING_length_nid_postfix(const GuestAsn1String*);
int APS5_VABI ASN1_STRING_type_nid_postfix(GuestAsn1String*);
int APS5_VABI ASN1_STRING_to_UTF8_nid_postfix(unsigned char**, GuestAsn1String*);
std::int64_t APS5_VABI ASN1_INTEGER_get_nid_postfix(const GuestAsn1String*);
int APS5_VABI sk_num_nid_postfix(const GuestStack*);
void* APS5_VABI sk_value_nid_postfix(const GuestStack*, int);
void* APS5_VABI sk_pop_nid_postfix(GuestStack*);
const GuestBioMethod* APS5_VABI BIO_s_mem_nid_postfix();
GuestBio* APS5_VABI BIO_new_nid_postfix(const GuestBioMethod*);
GuestBio* APS5_VABI BIO_new_mem_buf_nid_postfix(void*, int);
int APS5_VABI BIO_write_nid_postfix(GuestBio*, const void*, int);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
FileStream* APS5_VABI fopen_nid_postfix(const char*, const char*);
int APS5_VABI fclose_nid_postfix(FileStream*);
std::size_t APS5_VABI fwrite_nid_postfix(const void*, std::size_t, std::size_t, FileStream*);
void APS5_VABI free_nid_postfix(void*);
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

static void CheckCertificate(GuestX509* x) {
    Require(x && x->references == 1);
    GuestName* subject = X509_get_subject_name_nid_postfix(x);
    GuestName* issuer = X509_get_issuer_name_nid_postfix(x);
    Require(subject && issuer && subject != issuer);
    Require(x->cert_info->subject == subject && x->cert_info->issuer == issuer);
    Require(sk_num_nid_postfix(subject->entries) == 7 && subject->entries->num == 7);
    Require(X509_NAME_get_index_by_NID_nid_postfix(subject, 18, -1) == 3);
    Require(X509_NAME_get_index_by_NID_nid_postfix(subject, 18, 3) == 4);
    Require(X509_NAME_get_index_by_NID_nid_postfix(subject, 18, 4) == -1);
    Require(X509_NAME_get_index_by_NID_nid_postfix(subject, 15, -1) == -1);
    Require(Throws([&] { X509_NAME_get_index_by_NID_nid_postfix(subject, 999999, -1); }));
    GuestNameEntry* country = X509_NAME_get_entry_nid_postfix(subject, 0);
    Require(country && country->object->nid == 14 && std::strcmp(country->object->sn, "C") == 0 && std::strcmp(country->object->ln, "countryName") == 0);
    Require(country->set == 0 && country->value->type == 19 && Text(country->value) == "CZ");
    Require(sk_value_nid_postfix(subject->entries, 0) == country && sk_value_nid_postfix(subject->entries, 7) == nullptr && sk_value_nid_postfix(nullptr, 0) == nullptr);
    Require(sk_num_nid_postfix(nullptr) == -1);
    GuestNameEntry* unitB = X509_NAME_get_entry_nid_postfix(subject, 4);
    Require(unitB->set == 4 && Text(X509_NAME_ENTRY_get_data_nid_postfix(unitB)) == "Unit B");
    GuestNameEntry* common = X509_NAME_get_entry_nid_postfix(subject, 5);
    GuestAsn1String* commonData = X509_NAME_ENTRY_get_data_nid_postfix(common);
    Require(ASN1_STRING_type_nid_postfix(commonData) == 12 && ASN1_STRING_length_nid_postfix(commonData) == 13);
    Require(std::memcmp(ASN1_STRING_data_nid_postfix(commonData), "libressl.test", 13) == 0);
    GuestNameEntry* email = X509_NAME_get_entry_nid_postfix(subject, 6);
    Require(email->object->nid == 48 && email->value->type == 22);
    Require(X509_NAME_get_entry_nid_postfix(subject, 7) == nullptr && X509_NAME_get_entry_nid_postfix(subject, -1) == nullptr);

    char buffer[64];
    Require(X509_NAME_get_text_by_NID_nid_postfix(subject, 13, buffer, sizeof(buffer)) == 13 && std::strcmp(buffer, "libressl.test") == 0);
    Require(X509_NAME_get_text_by_NID_nid_postfix(subject, 13, nullptr, 0) == 13);
    Require(X509_NAME_get_text_by_NID_nid_postfix(issuer, 17, buffer, sizeof(buffer)) == 11 && std::strcmp(buffer, "AnyPS5 Test") == 0);
    Require(X509_NAME_get_text_by_NID_nid_postfix(subject, 15, buffer, sizeof(buffer)) == -1);
    Require(Throws([&] { X509_NAME_get_text_by_NID_nid_postfix(subject, 13, buffer, 13); }));

    GuestAsn1String* serial = X509_get_serialNumber_nid_postfix(x);
    Require(serial == x->cert_info->serialNumber && serial->type == 2 && serial->length == 4);
    Require(ASN1_INTEGER_get_nid_postfix(serial) == 0xf1020304ll);
    Require(ASN1_INTEGER_get_nid_postfix(x->cert_info->version) == 2);
    Require(ASN1_INTEGER_get_nid_postfix(nullptr) == 0);
    Require(x->cert_info->validity->notBefore->type == 23 && Text(x->cert_info->validity->notBefore) == "240101000000Z");
    Require(x->cert_info->validity->notAfter->type == 23 && Text(x->cert_info->validity->notAfter) == "491231235959Z");

    unsigned char* utf8 = nullptr;
    Require(ASN1_STRING_to_UTF8_nid_postfix(&utf8, commonData) == 13 && std::strcmp(reinterpret_cast<char*>(utf8), "libressl.test") == 0);
    free_nid_postfix(utf8);
}

static void TestDer() {
    std::string der = Der();
    Require(der.size() == 642);
    der += "trailing";
    const auto* cursor = reinterpret_cast<const unsigned char*>(der.data());
    GuestX509* out = nullptr;
    GuestX509* x = d2i_X509_nid_postfix(&out, &cursor, static_cast<std::int64_t>(der.size()));
    Require(x && out == x && cursor == reinterpret_cast<const unsigned char*>(der.data()) + 642);
    CheckCertificate(x);
    Require(Throws([&] { d2i_X509_nid_postfix(&out, &cursor, 8); }));
    X509_free_nid_postfix(x);
    X509_free_nid_postfix(nullptr);
    cursor = reinterpret_cast<const unsigned char*>(der.data());
    Require(d2i_X509_nid_postfix(nullptr, &cursor, 100) == nullptr && cursor == reinterpret_cast<const unsigned char*>(der.data()));
    std::string broken = Der();
    broken[300] ^= 0x5a;
    cursor = reinterpret_cast<const unsigned char*>(broken.data());
    GuestX509* bad = d2i_X509_nid_postfix(nullptr, &cursor, static_cast<std::int64_t>(broken.size()));
    if (bad) X509_free_nid_postfix(bad);
}

static void TestPem(const char* path) {
    std::string bundle = std::string("comment line\n") + certificate + certificate;
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(bundle.data(), static_cast<int>(bundle.size()));
    GuestX509* first = nullptr;
    Require(PEM_read_bio_X509_nid_postfix(bio, &first, nullptr, nullptr) == first && first);
    CheckCertificate(first);
    GuestX509* second = PEM_read_bio_X509_nid_postfix(bio, nullptr, nullptr, nullptr);
    Require(second && second != first);
    ERR_clear_error_nid_postfix();
    Require(PEM_read_bio_X509_nid_postfix(bio, nullptr, nullptr, nullptr) == nullptr);
    Require(ERR_get_error_nid_postfix() == ((9ull << 24) | (109ull << 12) | 108));
    X509_free_nid_postfix(first);
    X509_free_nid_postfix(second);
    BIO_free_nid_postfix(bio);

    GuestBio* writable = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    Require(BIO_write_nid_postfix(writable, certificate, static_cast<int>(std::strlen(certificate))) > 0);
    Require(BIO_write_nid_postfix(writable, "tail", 4) == 4);
    GuestX509* fromWritable = PEM_read_bio_X509_nid_postfix(writable, nullptr, nullptr, nullptr);
    Require(fromWritable && BIO_ctrl_nid_postfix(writable, 10, 0, nullptr) == 4);
    X509_free_nid_postfix(fromWritable);
    BIO_free_nid_postfix(writable);

    char key[] = "-----BEGIN PRIVATE KEY-----\nAAAA\n-----END PRIVATE KEY-----\n";
    GuestBio* other = BIO_new_mem_buf_nid_postfix(key, -1);
    Require(Throws([&] { PEM_read_bio_X509_nid_postfix(other, nullptr, nullptr, nullptr); }));
    BIO_free_nid_postfix(other);

    FileStream* stream = fopen_nid_postfix(path, "wb");
    Require(stream && fwrite_nid_postfix(certificate, 1, std::strlen(certificate), stream) == std::strlen(certificate));
    Require(fclose_nid_postfix(stream) == 0);
    stream = fopen_nid_postfix(path, "rb");
    GuestX509* fromFile = PEM_read_X509_nid_postfix(stream, nullptr, nullptr, nullptr);
    CheckCertificate(fromFile);
    Require(PEM_read_X509_nid_postfix(stream, nullptr, nullptr, nullptr) == nullptr);
    Require(fclose_nid_postfix(stream) == 0);
    X509_free_nid_postfix(fromFile);
    std::remove(path);
}

static void TestStrings() {
    unsigned char bmp[] = {0x00, 'A', 0x01, 0x0d};
    GuestAsn1String text{4, 30, bmp, 0};
    unsigned char* utf8 = nullptr;
    Require(ASN1_STRING_to_UTF8_nid_postfix(&utf8, &text) == 3 && std::memcmp(utf8, "A\xc4\x8d", 4) == 0);
    free_nid_postfix(utf8);
    unsigned char universal[] = {0x00, 0x01, 0xf6, 0x00};
    GuestAsn1String wide{4, 28, universal, 0};
    Require(ASN1_STRING_to_UTF8_nid_postfix(&utf8, &wide) == 4 && std::memcmp(utf8, "\xf0\x9f\x98\x80", 5) == 0);
    free_nid_postfix(utf8);
    GuestAsn1String odd{3, 30, bmp, 0};
    Require(ASN1_STRING_to_UTF8_nid_postfix(&utf8, &odd) == -1);
    GuestAsn1String octets{4, 4, bmp, 0};
    Require(Throws([&] { ASN1_STRING_to_UTF8_nid_postfix(&utf8, &octets); }));
    unsigned char big[] = {0x80, 0, 0, 0, 0, 0, 0, 0};
    GuestAsn1String large{8, 2, big, 0};
    Require(ASN1_INTEGER_get_nid_postfix(&large) == -1);
    unsigned char small[] = {0x01, 0x00};
    GuestAsn1String negative{2, 2 | 0x100, small, 0};
    Require(ASN1_INTEGER_get_nid_postfix(&negative) == -256);
    GuestAsn1String notInteger{2, 4, small, 0};
    Require(ASN1_INTEGER_get_nid_postfix(&notInteger) == -1);
}

int main(int argc, char** argv) {
    Require(argc == 2);
    void* const heapApi[10] {};
    ApplicationHeapRegister_nid_no_patch(heapApi);
    TestDer();
    TestPem(argv[1]);
    TestStrings();
    return 0;
}
