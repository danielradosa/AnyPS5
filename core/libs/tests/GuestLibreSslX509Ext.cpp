#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

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

struct GuestAsn1Type {
    int type;
    void* value;
};

struct GuestAlgor {
    GuestAsn1Object* algorithm;
    GuestAsn1Type* parameter;
};

struct GuestPubkey {
    GuestAlgor* algor;
    GuestAsn1String* public_key;
    void* pkey;
};

struct GuestExtension {
    GuestAsn1Object* object;
    int critical;
    GuestAsn1String* value;
};

struct GuestGeneralName {
    int type;
    void* d;
};

struct GuestOtherName {
    GuestAsn1Object* type_id;
    GuestAsn1Type* value;
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

struct GuestCertInfo {
    GuestAsn1String* version;
    GuestAsn1String* serialNumber;
    GuestAlgor* signature;
    GuestName* issuer;
    void* validity;
    GuestName* subject;
    GuestPubkey* key;
    GuestAsn1String* issuerUID;
    GuestAsn1String* subjectUID;
    GuestStack* extensions;
};

struct GuestX509 {
    GuestCertInfo* cert_info;
    GuestAlgor* sig_alg;
    GuestAsn1String* signature;
    int valid;
    int references;
};

struct GuestEvpPkey {
    int type;
    int save_type;
    int references;
    const void* ameth;
    void* engine;
    void* pkey;
    int save_parameters;
    void* attributes;
};

struct GuestCrl {
    void* crl;
    void* sig_alg;
    void* signature;
    int references;
    int flags;
};

struct GuestXPkey {
    int version;
    void* enc_algor;
    void* enc_pkey;
    GuestEvpPkey* dec_pkey;
    int key_length;
    char* key_data;
    int key_free;
    const void* cipher;
    unsigned char iv[16];
    int references;
};

struct GuestInfo {
    GuestX509* x509;
    GuestCrl* crl;
    GuestXPkey* x_pkey;
    const void* cipher;
    unsigned char iv[16];
    int enc_len;
    char* enc_data;
    int references;
};

struct GuestBio;
struct GuestBioMethod;

extern "C" {
GuestX509* APS5_VABI PEM_read_bio_X509_nid_postfix(GuestBio*, GuestX509**, void*, void*);
GuestX509* APS5_VABI PEM_read_bio_X509_AUX_nid_postfix(GuestBio*, GuestX509**, void*, void*);
int APS5_VABI PEM_write_bio_X509_nid_postfix(GuestBio*, GuestX509*);
GuestStack* APS5_VABI PEM_X509_INFO_read_bio_nid_postfix(GuestBio*, GuestStack*, void*, void*);
void APS5_VABI X509_INFO_free_nid_postfix(GuestInfo*);
void APS5_VABI X509_free_nid_postfix(GuestX509*);
GuestEvpPkey* APS5_VABI X509_get_pubkey_nid_postfix(GuestX509*);
GuestAsn1Object* APS5_VABI X509_EXTENSION_get_object_nid_postfix(GuestExtension*);
int APS5_VABI X509_EXTENSION_get_critical_nid_postfix(const GuestExtension*);
GuestAsn1String* APS5_VABI X509_EXTENSION_get_data_nid_postfix(GuestExtension*);
void* APS5_VABI X509_get_ext_d2i_nid_postfix(GuestX509*, int, int*, int*);
void APS5_VABI GENERAL_NAME_free_nid_postfix(GuestGeneralName*);
void APS5_VABI GENERAL_NAMES_free_nid_postfix(GuestStack*);
void APS5_VABI sk_pop_free_nid_postfix(GuestStack*, void (APS5_VABI*)(void*));
int APS5_VABI sk_num_nid_postfix(const GuestStack*);
void* APS5_VABI sk_value_nid_postfix(const GuestStack*, int);
void* APS5_VABI sk_pop_nid_postfix(GuestStack*);
int APS5_VABI i2d_X509_PUBKEY_nid_postfix(const GuestPubkey*, unsigned char**);
int APS5_VABI EVP_PKEY_id_nid_postfix(const GuestEvpPkey*);
void APS5_VABI EVP_PKEY_free_nid_postfix(GuestEvpPkey*);
const GuestBioMethod* APS5_VABI BIO_s_mem_nid_postfix();
GuestBio* APS5_VABI BIO_new_nid_postfix(const GuestBioMethod*);
GuestBio* APS5_VABI BIO_new_mem_buf_nid_postfix(void*, int);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
void APS5_VABI free_nid_postfix(void*);
}

static const char RootPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBoDCCAUegAwIBAgIBATAKBggqhkjOPQQDAjA3MQswCQYDVQQGEwJDWjEUMBIG\n"
    "A1UECgwLQW55UFM1IFRlc3QxEjAQBgNVBAMMCVRlc3QgUm9vdDAgFw0yNjEwMjEx\n"
    "ODIwNDNaGA8yMDUxMDYxMjE4MjA0M1owNzELMAkGA1UEBhMCQ1oxFDASBgNVBAoM\n"
    "C0FueVBTNSBUZXN0MRIwEAYDVQQDDAlUZXN0IFJvb3QwWTATBgcqhkjOPQIBBggq\n"
    "hkjOPQMBBwNCAASDNbbsOsv1IuS46MlJLMueQqhULAKPkJ1SlC3v9mHRl2BIkoNO\n"
    "7/GRJxZ/TCTIRvU/lC7fWgp8FUkPzQ/q5ol6o0IwQDAPBgNVHRMBAf8EBTADAQH/\n"
    "MA4GA1UdDwEB/wQEAwIBBjAdBgNVHQ4EFgQUyCwHEEftOMunZuj6/M9dTbAALd8w\n"
    "CgYIKoZIzj0EAwIDRwAwRAIgZ9mpZOxraLV7ZubCZ/sIRu8+DdSdRom7zoEAC43Y\n"
    "iN8CIACPMDCtI3D/lomq9MWs6nv7st41rqCuLqiUm8ADA9SR\n"
    "-----END CERTIFICATE-----\n";

static const char InterPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICEzCCAbigAwIBAgIBAjAKBggqhkjOPQQDAjA3MQswCQYDVQQGEwJDWjEUMBIG\n"
    "A1UECgwLQW55UFM1IFRlc3QxEjAQBgNVBAMMCVRlc3QgUm9vdDAeFw0yNjEwMjEx\n"
    "ODIwNDNaFw00ODA5MTUxODIwNDNaMD8xCzAJBgNVBAYTAkNaMRQwEgYDVQQKDAtB\n"
    "bnlQUzUgVGVzdDEaMBgGA1UEAwwRVGVzdCBJbnRlcm1lZGlhdGUwgZ8wDQYJKoZI\n"
    "hvcNAQEBBQADgY0AMIGJAoGBAMVxFsl+MsjNeokIdQDS2CsawKJEEJ3EcaLbuo7n\n"
    "Xt+Ua9PUe7LrDHePdD5EZQR/BfZOVLjrvTX+QyFCYbuYgsrzmQ6cOv+5j7mBK3lf\n"
    "j7BbWs5JKxm95AX8ckyLl3MmfXSJigcG0SkpFsO6BGAnGkXqKF/t6jXlWWyCsHcv\n"
    "aZYlAgMBAAGjZjBkMBIGA1UdEwEB/wQIMAYBAf8CAQAwDgYDVR0PAQH/BAQDAgEG\n"
    "MB0GA1UdDgQWBBS3FZGLuiNyaY82FETpedNDpq/8JDAfBgNVHSMEGDAWgBTILAcQ\n"
    "R+04y6dm6Pr8z11NsAAt3zAKBggqhkjOPQQDAgNJADBGAiEA0LUtIjRrMF42Eqeb\n"
    "4MSvcoJY0wHzbwAREedvBTToRNQCIQCLKMtjgCj8hR9F27GuoRvZyvnEdZ0csWJA\n"
    "B1n3alz46A==\n"
    "-----END CERTIFICATE-----\n";

static const char LeafPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIC1jCCAj+gAwIBAgIBAzANBgkqhkiG9w0BAQsFADA/MQswCQYDVQQGEwJDWjEU\n"
    "MBIGA1UECgwLQW55UFM1IFRlc3QxGjAYBgNVBAMMEVRlc3QgSW50ZXJtZWRpYXRl\n"
    "MB4XDTI2MTAyMTE4MjA0M1oXDTQ1MTIyMDE4MjA0M1owPzELMAkGA1UEBhMCQ1ox\n"
    "FDASBgNVBAoMC0FueVBTNSBUZXN0MRowGAYDVQQDDBFsZWFmLmV4YW1wbGUudGVz\n"
    "dDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABE/8GhZBxFDBxs3MQlULOipkK2wh\n"
    "icZHVnWb5xbCl/ti7FxcejWGv6Jf/pkwZ7cjaoDX3L9vE78yolB5DFmd6vijggEn\n"
    "MIIBIzAJBgNVHRMEAjAAMA4GA1UdDwEB/wQEAwIHgDAdBgNVHQ4EFgQUibwCyncj\n"
    "0qFYLdea2Lx2suNjKBgwHwYDVR0jBBgwFoAUtxWRi7ojcmmPNhRE6XnTQ6av/CQw\n"
    "gcUGA1UdEQSBvTCBuoIOKi5leGFtcGxlLnRlc3SCEWhvc3QuZXhhbXBsZS50ZXN0\n"
    "hwTAAAIHgRJhZG1pbkBleGFtcGxlLnRlc3SGFWh0dHBzOi8vZXhhbXBsZS50ZXN0\n"
    "L4gDKgMEpDwwOjELMAkGA1UEBhMCQ1oxEDAOBgNVBAoMB0FsdCBEaXIxGTAXBgNV\n"
    "BAMMEGFsdC5leGFtcGxlLnRlc3SgIQYKKwYBBAGCNxQCA6ATDBF1c2VyQGV4YW1w\n"
    "bGUudGVzdDANBgkqhkiG9w0BAQsFAAOBgQCQaDZOcefL3EcyZWk4WIO9E55IJdYY\n"
    "icKgS7tyDIxLXaymINR26F3JO9x9Z73zi9YhuaS04JH9Mk+l4WA3HYaPY5Kz62qf\n"
    "d9L85f4UoCFIwxlwnrqnKqIq8hEum9ak0bpFmWOjJCg1R90wxU4g2dWjGkBBuzHy\n"
    "PQUWqDI1e77H0g==\n"
    "-----END CERTIFICATE-----\n";

static const char LeafTrustedPem[] =
    "-----BEGIN TRUSTED CERTIFICATE-----\n"
    "MIIC1jCCAj+gAwIBAgIBAzANBgkqhkiG9w0BAQsFADA/MQswCQYDVQQGEwJDWjEU\n"
    "MBIGA1UECgwLQW55UFM1IFRlc3QxGjAYBgNVBAMMEVRlc3QgSW50ZXJtZWRpYXRl\n"
    "MB4XDTI2MTAyMTE4MjA0M1oXDTQ1MTIyMDE4MjA0M1owPzELMAkGA1UEBhMCQ1ox\n"
    "FDASBgNVBAoMC0FueVBTNSBUZXN0MRowGAYDVQQDDBFsZWFmLmV4YW1wbGUudGVz\n"
    "dDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABE/8GhZBxFDBxs3MQlULOipkK2wh\n"
    "icZHVnWb5xbCl/ti7FxcejWGv6Jf/pkwZ7cjaoDX3L9vE78yolB5DFmd6vijggEn\n"
    "MIIBIzAJBgNVHRMEAjAAMA4GA1UdDwEB/wQEAwIHgDAdBgNVHQ4EFgQUibwCyncj\n"
    "0qFYLdea2Lx2suNjKBgwHwYDVR0jBBgwFoAUtxWRi7ojcmmPNhRE6XnTQ6av/CQw\n"
    "gcUGA1UdEQSBvTCBuoIOKi5leGFtcGxlLnRlc3SCEWhvc3QuZXhhbXBsZS50ZXN0\n"
    "hwTAAAIHgRJhZG1pbkBleGFtcGxlLnRlc3SGFWh0dHBzOi8vZXhhbXBsZS50ZXN0\n"
    "L4gDKgMEpDwwOjELMAkGA1UEBhMCQ1oxEDAOBgNVBAoMB0FsdCBEaXIxGTAXBgNV\n"
    "BAMMEGFsdC5leGFtcGxlLnRlc3SgIQYKKwYBBAGCNxQCA6ATDBF1c2VyQGV4YW1w\n"
    "bGUudGVzdDANBgkqhkiG9w0BAQsFAAOBgQCQaDZOcefL3EcyZWk4WIO9E55IJdYY\n"
    "icKgS7tyDIxLXaymINR26F3JO9x9Z73zi9YhuaS04JH9Mk+l4WA3HYaPY5Kz62qf\n"
    "d9L85f4UoCFIwxlwnrqnKqIq8hEum9ak0bpFmWOjJCg1R90wxU4g2dWjGkBBuzHy\n"
    "PQUWqDI1e77H0jAMMAoGCCsGAQUFBwMB\n"
    "-----END TRUSTED CERTIFICATE-----\n";

static const char CrlPem[] =
    "-----BEGIN X509 CRL-----\n"
    "MIIBLjCBmAIBATANBgkqhkiG9w0BAQsFADA/MQswCQYDVQQGEwJDWjEUMBIGA1UE\n"
    "CgwLQW55UFM1IFRlc3QxGjAYBgNVBAMMEVRlc3QgSW50ZXJtZWRpYXRlFw0yNjEw\n"
    "MjExODIwNTZaFw00NTEyMjAxODIwNTZaMBQwEgIBBBcNMjYxMDIxMTgyMDU1WqAP\n"
    "MA0wCwYDVR0UBAQCAhAAMA0GCSqGSIb3DQEBCwUAA4GBAHXWEgXXX3CNxNQ4YnsM\n"
    "ehF/KMEkjDkuJ/4MVNvjNACAAzbTLliIClBhLIYbsOy4bt6SEKnRpRNelt48Ah6G\n"
    "MGB8o6WnR2OPjhSgqvRr3fBzaEgldaiRQs/mZoVedXHMzQ89GV4PhRKbxAWP3y/4\n"
    "Pymo+Ukkg84L/vF7AT9GorGQ\n"
    "-----END X509 CRL-----\n";

static const char EcKeyPem[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIN59H0jvfHQ6H7PSNAAmi6lw/aE2tke2uMSbluI/ESDWoAoGCCqGSM49\n"
    "AwEHoUQDQgAET/waFkHEUMHGzcxCVQs6KmQrbCGJxkdWdZvnFsKX+2LsXFx6NYa/\n"
    "ol/+mTBntyNqgNfcv28TvzKiUHkMWZ3q+A==\n"
    "-----END EC PRIVATE KEY-----\n";

static const char RsaKeyMarker[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDAGAJNFKocAP/WpV/BxTkx0dVi\n"
    "x9/FGEtmDZDNPXroWM2aVonyVrJVSFwi3OrXdFFUeoPf55B3qArv0xcLwze6h8GO\n"
    "C2tin4Xy3ugnCVz4kq3jWTPbSmBJ3Sn3X9DHs5RxabrJqZMBojKQMeFlClotGRyi\n"
    "gQncjlS0IqYGYtEJiwIDAQAB\n"
    "-----END PUBLIC KEY-----\n";

static const char LeafSpkiHex[] =
    "3059301306072a8648ce3d020106082a8648ce3d030107034200044ffc1a1641"
    "c450c1c6cdcc42550b3a2a642b6c2189c64756759be716c297fb62ec5c5c7a35"
    "86bfa25ffe993067b7236a80d7dcbf6f13bf32a250790c599deaf8";

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

static std::vector<unsigned char> Hex(const char* text) {
    std::vector<unsigned char> out;
    for (std::size_t index = 0; text[index] && text[index + 1]; index += 2) {
        const auto nibble = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
        out.push_back(static_cast<unsigned char>(nibble(text[index]) << 4 | nibble(text[index + 1])));
    }
    return out;
}

static std::uint64_t Code(std::uint64_t library, std::uint64_t function, std::uint64_t reason) {
    return library << 24 | function << 12 | reason;
}

static bool LastError(std::uint64_t expected) {
    std::uint64_t last = 0;
    while (const auto code = ERR_get_error_nid_postfix()) last = code;
    return last == expected;
}

static GuestX509* Load(const char* pem) {
    std::string text(pem);
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(text.data(), static_cast<int>(text.size()));
    GuestX509* x = PEM_read_bio_X509_nid_postfix(bio, nullptr, nullptr, nullptr);
    BIO_free_nid_postfix(bio);
    Require(x != nullptr);
    return x;
}

static std::string Text(const GuestAsn1String* string) {
    return std::string(reinterpret_cast<const char*>(string->data), static_cast<std::size_t>(string->length));
}

static int freedNames = 0;

static void APS5_VABI CountingFree(void* name) {
    ++freedNames;
    GENERAL_NAME_free_nid_postfix(static_cast<GuestGeneralName*>(name));
}

static void TestStructure() {
    GuestX509* leaf = Load(LeafPem);
    GuestCertInfo& info = *leaf->cert_info;
    Require(info.key && info.key->algor->algorithm->nid == 408 && std::strcmp(info.key->algor->algorithm->sn, "id-ecPublicKey") == 0);
    Require(info.key->algor->parameter && info.key->algor->parameter->type == 6);
    Require(static_cast<GuestAsn1Object*>(info.key->algor->parameter->value)->nid == 415);
    Require(info.key->public_key->type == 3 && info.key->public_key->length == 65 && info.key->public_key->data[0] == 4 && info.key->public_key->flags == 0x08);
    Require(info.signature && info.signature->algorithm->nid == 668 && info.signature->parameter->type == 5);
    Require(leaf->sig_alg && leaf->sig_alg->algorithm->nid == 668 && std::strcmp(leaf->sig_alg->algorithm->ln, "sha256WithRSAEncryption") == 0);
    Require(leaf->signature && leaf->signature->type == 3 && leaf->signature->length == 128);
    Require(!info.issuerUID && !info.subjectUID && info.extensions && sk_num_nid_postfix(info.extensions) == 5);
    const int nids[] = {87, 83, 82, 90, 85};
    for (int index = 0; index < 5; ++index) {
        auto* extension = static_cast<GuestExtension*>(sk_value_nid_postfix(info.extensions, index));
        Require(X509_EXTENSION_get_object_nid_postfix(extension)->nid == nids[index]);
        Require(X509_EXTENSION_get_critical_nid_postfix(extension) == (index == 1 ? 1 : 0));
        Require(X509_EXTENSION_get_data_nid_postfix(extension) == extension->value && extension->value->type == 4);
    }
    Require(Throws([&] { sk_pop_free_nid_postfix(info.extensions, CountingFree); }));
    auto* ski = static_cast<GuestExtension*>(sk_value_nid_postfix(info.extensions, 2));
    Require(ski->value->length == 22 && ski->value->data[0] == 4 && ski->value->data[2] == 0x89 && ski->value->data[21] == 0x18);
    Require(std::strcmp(ski->object->ln, "X509v3 Subject Key Identifier") == 0);

    const int length = i2d_X509_PUBKEY_nid_postfix(info.key, nullptr);
    const auto spki = Hex(LeafSpkiHex);
    Require(length == static_cast<int>(spki.size()));
    std::vector<unsigned char> buffer(static_cast<std::size_t>(length) + 4);
    unsigned char* cursor = buffer.data();
    Require(i2d_X509_PUBKEY_nid_postfix(info.key, &cursor) == length && cursor == buffer.data() + length);
    Require(std::vector<unsigned char>(buffer.begin(), buffer.begin() + length) == spki);
    unsigned char* allocated = nullptr;
    Require(i2d_X509_PUBKEY_nid_postfix(info.key, &allocated) == length && allocated && std::memcmp(allocated, spki.data(), spki.size()) == 0);
    free_nid_postfix(allocated);

    Require(info.key->pkey == nullptr);
    GuestEvpPkey* pkey = X509_get_pubkey_nid_postfix(leaf);
    Require(pkey && EVP_PKEY_id_nid_postfix(pkey) == 408 && pkey->references == 2 && info.key->pkey == pkey);
    GuestEvpPkey* again = X509_get_pubkey_nid_postfix(leaf);
    Require(again == pkey && pkey->references == 3);
    EVP_PKEY_free_nid_postfix(again);
    GuestX509* inter = Load(InterPem);
    GuestEvpPkey* rsa = X509_get_pubkey_nid_postfix(inter);
    Require(rsa && rsa->type == 6);
    X509_free_nid_postfix(inter);
    Require(rsa->references == 1);
    EVP_PKEY_free_nid_postfix(rsa);
    X509_free_nid_postfix(leaf);
    Require(pkey->references == 1 && pkey->type == 408);
    EVP_PKEY_free_nid_postfix(pkey);
}

static void TestAltNames() {
    GuestX509* leaf = Load(LeafPem);
    int crit = 7;
    auto* names = static_cast<GuestStack*>(X509_get_ext_d2i_nid_postfix(leaf, 85, &crit, nullptr));
    Require(names && crit == 0 && sk_num_nid_postfix(names) == 8);
    const auto name = [&](int index) { return static_cast<GuestGeneralName*>(sk_value_nid_postfix(names, index)); };
    Require(name(0)->type == 2 && Text(static_cast<GuestAsn1String*>(name(0)->d)) == "*.example.test" && static_cast<GuestAsn1String*>(name(0)->d)->type == 22);
    Require(name(1)->type == 2 && Text(static_cast<GuestAsn1String*>(name(1)->d)) == "host.example.test");
    auto* ip = static_cast<GuestAsn1String*>(name(2)->d);
    Require(name(2)->type == 7 && ip->type == 4 && ip->length == 4 && ip->data[0] == 192 && ip->data[3] == 7);
    Require(name(3)->type == 1 && Text(static_cast<GuestAsn1String*>(name(3)->d)) == "admin@example.test");
    Require(name(4)->type == 6 && Text(static_cast<GuestAsn1String*>(name(4)->d)) == "https://example.test/");
    auto* rid = static_cast<GuestAsn1Object*>(name(5)->d);
    Require(name(5)->type == 8 && rid->nid == 0 && rid->length == 3 && rid->data[0] == 0x2a);
    auto* dir = static_cast<GuestName*>(name(6)->d);
    Require(name(6)->type == 4 && sk_num_nid_postfix(dir->entries) == 3);
    auto* cn = static_cast<GuestNameEntry*>(sk_value_nid_postfix(dir->entries, 2));
    Require(cn->object->nid == 13 && Text(cn->value) == "alt.example.test");
    auto* other = static_cast<GuestOtherName*>(name(7)->d);
    Require(name(7)->type == 0 && other->type_id->length == 10 && other->value->type == 12);
    Require(Text(static_cast<GuestAsn1String*>(other->value->value)) == "user@example.test");
    GuestGeneralName* last = name(7);
    GuestGeneralName* popped = static_cast<GuestGeneralName*>(sk_pop_nid_postfix(names));
    Require(popped == last && sk_num_nid_postfix(names) == 7);
    GENERAL_NAME_free_nid_postfix(popped);
    Require(Throws([&] { GENERAL_NAME_free_nid_postfix(popped); }));
    GENERAL_NAMES_free_nid_postfix(names);
    GENERAL_NAMES_free_nid_postfix(nullptr);
    GENERAL_NAME_free_nid_postfix(nullptr);

    freedNames = 0;
    names = static_cast<GuestStack*>(X509_get_ext_d2i_nid_postfix(leaf, 85, nullptr, nullptr));
    sk_pop_free_nid_postfix(names, CountingFree);
    Require(freedNames == 8);
    sk_pop_free_nid_postfix(nullptr, CountingFree);

    int idx = -1;
    names = static_cast<GuestStack*>(X509_get_ext_d2i_nid_postfix(leaf, 85, &crit, &idx));
    Require(names && idx == 4 && crit == 0);
    GENERAL_NAMES_free_nid_postfix(names);
    Require(X509_get_ext_d2i_nid_postfix(leaf, 85, &crit, &idx) == nullptr && idx == -1 && crit == -1);
    Require(X509_get_ext_d2i_nid_postfix(leaf, 86, &crit, nullptr) == nullptr && crit == -1);
    Require(Throws([&] { X509_get_ext_d2i_nid_postfix(leaf, 83, &crit, nullptr); }) && crit == 1);
    GuestX509* root = Load(RootPem);
    Require(X509_get_ext_d2i_nid_postfix(root, 85, &crit, nullptr) == nullptr && crit == -1);
    X509_free_nid_postfix(root);
    X509_free_nid_postfix(leaf);
}

static void TestPem() {
    GuestX509* leaf = Load(LeafPem);
    GuestBio* bio = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    Require(PEM_write_bio_X509_nid_postfix(bio, leaf) == 1);
    char* data = nullptr;
    const auto size = BIO_ctrl_nid_postfix(bio, 3, 0, &data);
    Require(std::string(data, static_cast<std::size_t>(size)) == LeafPem);
    BIO_free_nid_postfix(bio);
    char readOnly[] = "x";
    GuestBio* fixed = BIO_new_mem_buf_nid_postfix(readOnly, 1);
    ERR_clear_error_nid_postfix();
    Require(PEM_write_bio_X509_nid_postfix(fixed, leaf) == 0 && LastError(Code(9, 114, 7)));
    BIO_free_nid_postfix(fixed);

    std::string trusted = std::string(RsaKeyMarker) + LeafTrustedPem + LeafPem;
    GuestBio* both = BIO_new_mem_buf_nid_postfix(trusted.data(), static_cast<int>(trusted.size()));
    GuestX509* out = nullptr;
    GuestX509* aux = PEM_read_bio_X509_AUX_nid_postfix(both, &out, nullptr, nullptr);
    Require(aux && out == aux && aux->cert_info->key->public_key->length == 65);
    GuestX509* plain = PEM_read_bio_X509_AUX_nid_postfix(both, nullptr, nullptr, nullptr);
    Require(plain && plain != aux && plain->references == 1);
    Require(PEM_read_bio_X509_AUX_nid_postfix(both, nullptr, nullptr, nullptr) == nullptr && LastError(Code(9, 109, 108)));
    Require(Throws([&] { PEM_read_bio_X509_AUX_nid_postfix(both, &out, nullptr, nullptr); }));
    BIO_free_nid_postfix(both);
    X509_free_nid_postfix(plain);
    X509_free_nid_postfix(aux);
    X509_free_nid_postfix(leaf);
}

static void TestInfo() {
    std::string bundle = std::string("junk\n") + RootPem + InterPem + CrlPem + EcKeyPem + RsaKeyMarker + LeafPem;
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(bundle.data(), static_cast<int>(bundle.size()));
    GuestStack* infos = PEM_X509_INFO_read_bio_nid_postfix(bio, nullptr, nullptr, nullptr);
    BIO_free_nid_postfix(bio);
    Require(infos && sk_num_nid_postfix(infos) == 3);
    auto* first = static_cast<GuestInfo*>(sk_value_nid_postfix(infos, 0));
    auto* second = static_cast<GuestInfo*>(sk_value_nid_postfix(infos, 1));
    auto* third = static_cast<GuestInfo*>(sk_value_nid_postfix(infos, 2));
    Require(first->x509 && !first->crl && !first->x_pkey && first->references == 1);
    Require(second->x509 && second->crl && second->x_pkey && second->x_pkey->dec_pkey && second->x_pkey->dec_pkey->type == 408);
    Require(second->crl->references == 1 && second->x_pkey->references == 1);
    Require(third->x509 && !third->crl && !third->x_pkey);
    Require(first->x509->references == 1 && second->x509->references == 1 && third->x509->references == 1);
    sk_pop_free_nid_postfix(infos, reinterpret_cast<void (APS5_VABI*)(void*)>(X509_INFO_free_nid_postfix));

    char empty[] = "nothing here\n";
    GuestBio* none = BIO_new_mem_buf_nid_postfix(empty, -1);
    GuestStack* nothing = PEM_X509_INFO_read_bio_nid_postfix(none, nullptr, nullptr, nullptr);
    Require(nothing && sk_num_nid_postfix(nothing) == 0);
    sk_pop_free_nid_postfix(nothing, reinterpret_cast<void (APS5_VABI*)(void*)>(X509_INFO_free_nid_postfix));
    BIO_free_nid_postfix(none);
    std::string broken = std::string(RootPem) + "-----BEGIN X509 CRL-----\nMAMCAQA=\n-----END X509 CRL-----\n";
    GuestBio* bad = BIO_new_mem_buf_nid_postfix(broken.data(), static_cast<int>(broken.size()));
    ERR_clear_error_nid_postfix();
    Require(PEM_X509_INFO_read_bio_nid_postfix(bad, nullptr, nullptr, nullptr) == nullptr && LastError(Code(9, 116, 13)));
    BIO_free_nid_postfix(bad);
}

int main() {
    void* const heapApi[10] {};
    ApplicationHeapRegister_nid_no_patch(heapApi);
    TestStructure();
    TestAltNames();
    TestPem();
    TestInfo();
    return 0;
}
