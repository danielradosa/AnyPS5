#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
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

struct GuestX509 {
    void* cert_info;
    void* sig_alg;
    void* signature;
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

struct GuestVerifyParam {
    char* name;
    std::int64_t check_time;
    std::uint64_t inh_flags;
    std::uint64_t flags;
    int purpose;
    int trust;
    int depth;
    void* policies;
    void* id;
};

struct GuestStore {
    int cache;
    GuestStack* objs;
    GuestStack* get_cert_methods;
    GuestVerifyParam* param;
    void* verify;
    void* verify_cb;
    void* reserved[9];
    void* ex_data;
    int references;
};

struct GuestLookup {
    int init;
    int skip;
    const void* method;
    char* method_data;
    GuestStore* store_ctx;
};

struct GuestBio;
struct FileStream;

extern "C" {
GuestX509* APS5_VABI PEM_read_bio_X509_nid_postfix(GuestBio*, GuestX509**, void*, void*);
GuestStack* APS5_VABI PEM_X509_INFO_read_bio_nid_postfix(GuestBio*, GuestStack*, void*, void*);
void APS5_VABI X509_INFO_free_nid_postfix(GuestInfo*);
void APS5_VABI X509_free_nid_postfix(GuestX509*);
void APS5_VABI sk_pop_free_nid_postfix(GuestStack*, void (APS5_VABI*)(void*));
int APS5_VABI sk_num_nid_postfix(const GuestStack*);
void* APS5_VABI sk_value_nid_postfix(const GuestStack*, int);
int APS5_VABI X509_check_issued_nid_postfix(GuestX509*, GuestX509*);
const char* APS5_VABI X509_verify_cert_error_string_nid_postfix(std::int64_t);
GuestStore* APS5_VABI X509_STORE_new_nid_postfix();
void APS5_VABI X509_STORE_free_nid_postfix(GuestStore*);
int APS5_VABI X509_STORE_add_cert_nid_postfix(GuestStore*, GuestX509*);
int APS5_VABI X509_STORE_add_crl_nid_postfix(GuestStore*, GuestCrl*);
int APS5_VABI X509_STORE_set_flags_nid_postfix(GuestStore*, std::uint64_t);
const void* APS5_VABI X509_LOOKUP_file_nid_postfix();
GuestLookup* APS5_VABI X509_STORE_add_lookup_nid_postfix(GuestStore*, const void*);
int APS5_VABI X509_load_crl_file_nid_postfix(GuestLookup*, const char*, int);
int APS5_VABI X509_VERIFY_PARAM_set1_host_nid_postfix(GuestVerifyParam*, const char*, std::size_t);
void APS5_VABI X509_VERIFY_PARAM_set_hostflags_nid_postfix(GuestVerifyParam*, unsigned int);
int APS5_VABI X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(GuestVerifyParam*, const char*);
GuestBio* APS5_VABI BIO_new_mem_buf_nid_postfix(void*, int);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
FileStream* APS5_VABI fopen_nid_postfix(const char*, const char*);
int APS5_VABI fclose_nid_postfix(FileStream*);
std::size_t APS5_VABI fwrite_nid_postfix(const void*, std::size_t, std::size_t, FileStream*);
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

static const char SelfPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBezCCASCgAwIBAgIBBjAKBggqhkjOPQQDAjAcMRowGAYDVQQDDBFzZWxmLmV4\n"
    "YW1wbGUudGVzdDAeFw0yNjEwMjExODIwNDNaFw00NTEyMjAxODIwNDNaMBwxGjAY\n"
    "BgNVBAMMEXNlbGYuZXhhbXBsZS50ZXN0MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcD\n"
    "QgAET/waFkHEUMHGzcxCVQs6KmQrbCGJxkdWdZvnFsKX+2LsXFx6NYa/ol/+mTBn\n"
    "tyNqgNfcv28TvzKiUHkMWZ3q+KNTMFEwHQYDVR0OBBYEFIm8Asp3I9KhWC3Xmti8\n"
    "drLjYygYMB8GA1UdIwQYMBaAFIm8Asp3I9KhWC3Xmti8drLjYygYMA8GA1UdEwEB\n"
    "/wQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAI7Cki8hWLITGwhDTSlCrotr/84Y\n"
    "K8efmxV9z2Gg8vjPAiEA5jM9tm2/sRZYgjC8YGk8640uz+CGo/cRpyg1CFD4dZU=\n"
    "-----END CERTIFICATE-----\n";

static const char UnderLeafPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBvTCCAWOgAwIBAgIBCjAKBggqhkjOPQQDAjA/MQswCQYDVQQGEwJDWjEUMBIG\n"
    "A1UECgwLQW55UFM1IFRlc3QxGjAYBgNVBAMMEWxlYWYuZXhhbXBsZS50ZXN0MB4X\n"
    "DTI2MTAyMTE4MjA0NFoXDTQ1MTIyMDE4MjA0NFowQjELMAkGA1UEBhMCQ1oxFDAS\n"
    "BgNVBAoMC0FueVBTNSBUZXN0MR0wGwYDVQQDDBRyZXZva2VkLmV4YW1wbGUudGVz\n"
    "dDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABE/8GhZBxFDBxs3MQlULOipkK2wh\n"
    "icZHVnWb5xbCl/ti7FxcejWGv6Jf/pkwZ7cjaoDX3L9vE78yolB5DFmd6vijTTBL\n"
    "MAkGA1UdEwQCMAAwHQYDVR0OBBYEFIm8Asp3I9KhWC3Xmti8drLjYygYMB8GA1Ud\n"
    "IwQYMBaAFIm8Asp3I9KhWC3Xmti8drLjYygYMAoGCCqGSM49BAMCA0gAMEUCIHhf\n"
    "eAwoUYQcVT+MirLY7Q5xPOaftAKWkCK8emxyzTicAiEAkV/osb5Y7f7T8vzIJPfB\n"
    "jb9B6NR3/Nt0q857sN6HzJw=\n"
    "-----END CERTIFICATE-----\n";

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

static void TestStore() {
    std::string bundle = std::string("junk\n") + RootPem + InterPem + CrlPem + EcKeyPem + RsaKeyMarker + LeafPem;
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(bundle.data(), static_cast<int>(bundle.size()));
    GuestStack* infos = PEM_X509_INFO_read_bio_nid_postfix(bio, nullptr, nullptr, nullptr);
    BIO_free_nid_postfix(bio);
    Require(infos && sk_num_nid_postfix(infos) == 3);
    auto* first = static_cast<GuestInfo*>(sk_value_nid_postfix(infos, 0));
    auto* second = static_cast<GuestInfo*>(sk_value_nid_postfix(infos, 1));
    auto* third = static_cast<GuestInfo*>(sk_value_nid_postfix(infos, 2));
    Require(X509_check_issued_nid_postfix(first->x509, second->x509) == 0);
    Require(X509_check_issued_nid_postfix(second->x509, third->x509) == 0);

    GuestStore* store = X509_STORE_new_nid_postfix();
    Require(store && store->references == 1 && store->param && store->param->depth == -1 && store->objs && sk_num_nid_postfix(store->objs) == 0);
    Require(X509_STORE_add_cert_nid_postfix(store, first->x509) == 1 && first->x509->references == 2 && sk_num_nid_postfix(store->objs) == 1);
    ERR_clear_error_nid_postfix();
    Require(X509_STORE_add_cert_nid_postfix(store, first->x509) == 0 && LastError(Code(11, 124, 101)));
    Require(X509_STORE_add_cert_nid_postfix(store, nullptr) == 0);
    Require(X509_STORE_add_crl_nid_postfix(store, second->crl) == 1 && second->crl->references == 2);
    Require(X509_STORE_add_crl_nid_postfix(store, second->crl) == 0 && LastError(Code(11, 125, 101)));
    Require(X509_STORE_set_flags_nid_postfix(store, 0x4) == 1 && store->param->flags == 0x4);
    Require(X509_STORE_set_flags_nid_postfix(store, 0x8000) == 1 && store->param->flags == 0x8004);
    Require(X509_VERIFY_PARAM_set1_host_nid_postfix(store->param, "host.example.test", 0) == 1);
    Require(X509_VERIFY_PARAM_set1_host_nid_postfix(store->param, "bad\0name", 8) == 0);
    Require(X509_VERIFY_PARAM_set1_host_nid_postfix(store->param, nullptr, 0) == 1);
    X509_VERIFY_PARAM_set_hostflags_nid_postfix(store->param, 0x4);
    Require(X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(store->param, "192.0.2.7") == 1);
    Require(X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(store->param, "2001:db8::7") == 1);
    Require(X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(store->param, "::ffff:192.0.2.7") == 1);
    Require(X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(store->param, "192.0.2") == 0);
    Require(X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(store->param, "1::2::3") == 0);
    Require(X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(store->param, "256.0.0.1") == 0);
    sk_pop_free_nid_postfix(infos, reinterpret_cast<void (APS5_VABI*)(void*)>(X509_INFO_free_nid_postfix));
    Require(sk_num_nid_postfix(store->objs) == 2);
    X509_STORE_free_nid_postfix(store);
    X509_STORE_free_nid_postfix(nullptr);
}

static void TestLookup(const char* path) {
    GuestStore* store = X509_STORE_new_nid_postfix();
    const void* method = X509_LOOKUP_file_nid_postfix();
    Require(method && method == X509_LOOKUP_file_nid_postfix());
    GuestLookup* lookup = X509_STORE_add_lookup_nid_postfix(store, method);
    Require(lookup && lookup->store_ctx == store && lookup->method == method && X509_STORE_add_lookup_nid_postfix(store, method) == lookup);
    FileStream* stream = fopen_nid_postfix(path, "wb");
    const std::string text = std::string("leading text\n") + RootPem + CrlPem;
    Require(stream && fwrite_nid_postfix(text.data(), 1, text.size(), stream) == text.size() && fclose_nid_postfix(stream) == 0);
    Require(X509_load_crl_file_nid_postfix(lookup, path, 1) == 1 && sk_num_nid_postfix(store->objs) == 1);
    ERR_clear_error_nid_postfix();
    Require(X509_load_crl_file_nid_postfix(lookup, path, 1) == 0 && LastError(Code(11, 125, 101)));
    Require(X509_load_crl_file_nid_postfix(lookup, path, 2) == 0 && LastError(Code(11, 112, 13)));
    Require(X509_load_crl_file_nid_postfix(lookup, path, 3) == 0 && LastError(Code(11, 112, 100)));
    std::remove(path);
    Require(X509_load_crl_file_nid_postfix(lookup, path, 1) == 0 && LastError(Code(11, 112, 2)));
    X509_STORE_free_nid_postfix(store);
}

static void TestIssued() {
    GuestX509* root = Load(RootPem);
    GuestX509* inter = Load(InterPem);
    GuestX509* leaf = Load(LeafPem);
    GuestX509* self = Load(SelfPem);
    GuestX509* under = Load(UnderLeafPem);
    Require(X509_check_issued_nid_postfix(root, inter) == 0 && X509_check_issued_nid_postfix(inter, leaf) == 0);
    Require(X509_check_issued_nid_postfix(root, root) == 0 && X509_check_issued_nid_postfix(self, self) == 0);
    Require(X509_check_issued_nid_postfix(root, leaf) == 29 && X509_check_issued_nid_postfix(leaf, leaf) == 29);
    Require(X509_check_issued_nid_postfix(leaf, under) == 32);
    Require(Throws([&] { X509_check_issued_nid_postfix(nullptr, leaf); }));
    Require(std::strcmp(X509_verify_cert_error_string_nid_postfix(0), "ok") == 0);
    Require(std::strcmp(X509_verify_cert_error_string_nid_postfix(20), "unable to get local issuer certificate") == 0);
    Require(std::strcmp(X509_verify_cert_error_string_nid_postfix(10), "certificate has expired") == 0);
    Require(Throws([] { X509_verify_cert_error_string_nid_postfix(62); }));
    for (GuestX509* x : {root, inter, leaf, self, under}) X509_free_nid_postfix(x);
}

int main(int argc, char** argv) {
    Require(argc == 2);
    void* const heapApi[10] {};
    ApplicationHeapRegister_nid_no_patch(heapApi);
    TestStore();
    TestLookup(argv[1]);
    TestIssued();
    return 0;
}
