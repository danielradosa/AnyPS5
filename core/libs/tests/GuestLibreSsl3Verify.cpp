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

struct GuestAlgor {
    GuestAsn1Object* algorithm;
    void* parameter;
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

struct GuestCertInfo {
    void* version;
    void* serialNumber;
    GuestAlgor* signature;
    void* issuer;
    void* validity;
    void* subject;
    GuestPubkey* key;
    void* issuerUID;
    void* subjectUID;
    GuestStack* extensions;
};

struct GuestX509 {
    GuestCertInfo* cert_info;
    GuestAlgor* sig_alg;
    GuestAsn1String* signature;
    int valid;
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

struct GuestStoreCtx;

struct GuestStore {
    int cache;
    GuestStack* objs;
    GuestStack* get_cert_methods;
    GuestVerifyParam* param;
    int (APS5_VABI* verify)(GuestStoreCtx*);
    int (APS5_VABI* verify_cb)(int, GuestStoreCtx*);
    void* reserved[9];
    void* ex_data;
    int references;
};

struct GuestStoreCtx {
    GuestStore* ctx;
    int current_method;
    GuestX509* cert;
    GuestStack* untrusted;
    GuestStack* crls;
    GuestVerifyParam* param;
    void* other_ctx;
    void* verify;
    void* verify_cb;
    void* callbacks[10];
    int valid;
    int last_untrusted;
    GuestStack* chain;
    void* tree;
    int explicit_policy;
    int error_depth;
    int error;
    GuestX509* current_cert;
    GuestX509* current_issuer;
    void* current_crl;
    int current_crl_score;
    unsigned current_reasons;
    GuestStoreCtx* parent;
    void* ex_data;
};

static_assert(sizeof(GuestStoreCtx) == 240 && offsetof(GuestStoreCtx, error) == 184);

struct GuestInfo {
    GuestX509* x509;
    void* crl;
};

struct GuestBio;

extern "C" {
GuestStoreCtx* APS5_VABI X509_STORE_CTX_new_nid_postfix();
void APS5_VABI X509_STORE_CTX_free_nid_postfix(GuestStoreCtx*);
int APS5_VABI X509_STORE_CTX_init_nid_postfix(GuestStoreCtx*, GuestStore*, GuestX509*, GuestStack*);
int APS5_VABI X509_STORE_up_ref_nid_postfix(GuestStore*);
GuestExtension* APS5_VABI X509_get_ext_nid_postfix(const GuestX509*, int);
const GuestStack* APS5_VABI X509_get0_extensions_nid_postfix(const GuestX509*);
void APS5_VABI X509_get0_signature_nid_postfix(const GuestAsn1String**, const GuestAlgor**, const GuestX509*);
int APS5_VABI X509_PUBKEY_get0_param_nid_postfix(const GuestAsn1Object**, const unsigned char**, int*, GuestAlgor**, const GuestPubkey*);
int APS5_VABI X509_get_ext_count_nid_postfix(const GuestX509*);
int APS5_VABI X509_up_ref_nid_postfix(GuestX509*);

int APS5_VABI X509_verify_cert_nid_postfix(GuestStoreCtx*);
int APS5_VABI X509_STORE_CTX_get_error_nid_postfix(const GuestStoreCtx*);
void APS5_VABI X509_STORE_CTX_set_error_nid_postfix(GuestStoreCtx*, int);
int APS5_VABI X509_STORE_CTX_get_error_depth_nid_postfix(const GuestStoreCtx*);
GuestX509* APS5_VABI X509_STORE_CTX_get_current_cert_nid_postfix(const GuestStoreCtx*);
GuestStack* APS5_VABI X509_STORE_CTX_get_chain_nid_postfix(const GuestStoreCtx*);
GuestStack* APS5_VABI X509_STORE_CTX_get1_chain_nid_postfix(const GuestStoreCtx*);
void* APS5_VABI X509_STORE_CTX_get_ex_data_nid_postfix(const GuestStoreCtx*, int);
GuestStore* APS5_VABI X509_STORE_new_nid_postfix();
void APS5_VABI X509_STORE_free_nid_postfix(GuestStore*);
int APS5_VABI X509_STORE_add_cert_nid_postfix(GuestStore*, GuestX509*);
int APS5_VABI X509_STORE_add_crl_nid_postfix(GuestStore*, void*);
int APS5_VABI X509_STORE_set_flags_nid_postfix(GuestStore*, std::uint64_t);
int APS5_VABI X509_VERIFY_PARAM_set1_host_nid_postfix(GuestVerifyParam*, const char*, std::size_t);
void APS5_VABI X509_VERIFY_PARAM_set_hostflags_nid_postfix(GuestVerifyParam*, unsigned int);
int APS5_VABI X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(GuestVerifyParam*, const char*);
GuestX509* APS5_VABI PEM_read_bio_X509_nid_postfix(GuestBio*, GuestX509**, void*, void*);
GuestStack* APS5_VABI PEM_X509_INFO_read_bio_nid_postfix(GuestBio*, GuestStack*, void*, void*);
void APS5_VABI X509_INFO_free_nid_postfix(void*);
void APS5_VABI X509_free_nid_postfix(GuestX509*);
void APS5_VABI sk_pop_free_nid_postfix(GuestStack*, void (APS5_VABI*)(void*));
int APS5_VABI sk_num_nid_postfix(const GuestStack*);
void* APS5_VABI sk_value_nid_postfix(const GuestStack*, int);
GuestBio* APS5_VABI BIO_new_mem_buf_nid_postfix(void*, int);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
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

static const char RevokedPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB/TCCAWagAwIBAgIBBDANBgkqhkiG9w0BAQsFADA/MQswCQYDVQQGEwJDWjEU\n"
    "MBIGA1UECgwLQW55UFM1IFRlc3QxGjAYBgNVBAMMEVRlc3QgSW50ZXJtZWRpYXRl\n"
    "MB4XDTI2MTAyMTE4MjA0M1oXDTQ1MTIyMDE4MjA0M1owQjELMAkGA1UEBhMCQ1ox\n"
    "FDASBgNVBAoMC0FueVBTNSBUZXN0MR0wGwYDVQQDDBRyZXZva2VkLmV4YW1wbGUu\n"
    "dGVzdDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABE/8GhZBxFDBxs3MQlULOipk\n"
    "K2whicZHVnWb5xbCl/ti7FxcejWGv6Jf/pkwZ7cjaoDX3L9vE78yolB5DFmd6vij\n"
    "TTBLMAkGA1UdEwQCMAAwHQYDVR0OBBYEFIm8Asp3I9KhWC3Xmti8drLjYygYMB8G\n"
    "A1UdIwQYMBaAFLcVkYu6I3JpjzYUROl500Omr/wkMA0GCSqGSIb3DQEBCwUAA4GB\n"
    "AAuCHD0HwYYr194IIahAprpyQ7BDNkPD4KYvF2oEv1s8hbewkBtzyMnxfO5pnUxQ\n"
    "EiAwOZTRPYRjhhvfj8ctRa372Z/HyRrwT5iFaJztyh2YWRjV0VYfY4kh/rdffz2e\n"
    "mWAqa6IvblnnBUR6RFCrp8dHP4x02JcohvHIiFdvyXuL\n"
    "-----END CERTIFICATE-----\n";

static const char ExpiredPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB/TCCAWagAwIBAgIBBTANBgkqhkiG9w0BAQsFADA/MQswCQYDVQQGEwJDWjEU\n"
    "MBIGA1UECgwLQW55UFM1IFRlc3QxGjAYBgNVBAMMEVRlc3QgSW50ZXJtZWRpYXRl\n"
    "MB4XDTIwMDEwMTAwMDAwMFoXDTIxMDEwMTAwMDAwMFowQjELMAkGA1UEBhMCQ1ox\n"
    "FDASBgNVBAoMC0FueVBTNSBUZXN0MR0wGwYDVQQDDBRyZXZva2VkLmV4YW1wbGUu\n"
    "dGVzdDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABE/8GhZBxFDBxs3MQlULOipk\n"
    "K2whicZHVnWb5xbCl/ti7FxcejWGv6Jf/pkwZ7cjaoDX3L9vE78yolB5DFmd6vij\n"
    "TTBLMAkGA1UdEwQCMAAwHQYDVR0OBBYEFIm8Asp3I9KhWC3Xmti8drLjYygYMB8G\n"
    "A1UdIwQYMBaAFLcVkYu6I3JpjzYUROl500Omr/wkMA0GCSqGSIb3DQEBCwUAA4GB\n"
    "AG23G+MhnIwPoP1xVIXhnWg+mkdAFpBlg3Yg4K/kvNMUm6DmRJRP0CHs5mMbzTET\n"
    "WG5Un6ewpaBSDp0qYhCRHJFZ1lct5mEeC7OnxmLaR397MJ4oZJ9Te7vNyEFa7hy5\n"
    "jBo7CCMoe7C3LAxymmPB/M8ERfID97H4DQoZd1xBjyy1\n"
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

static const char BadSigPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB/TCCAWagAwIBAgIBBDANBgkqhkiG9w0BAQsFADA/MQswCQYDVQQGEwJDWjEU\n"
    "MBIGA1UECgwLQW55UFM1IFRlc3QxGjAYBgNVBAMMEVRlc3QgSW50ZXJtZWRpYXRl\n"
    "MB4XDTI2MTAyMTE4MjA0M1oXDTQ1MTIyMDE4MjA0M1owQjELMAkGA1UEBhMCQ1ox\n"
    "FDASBgNVBAoMC0FueVBTNSBUZXN0MR0wGwYDVQQDDBRyZXZva2VkLmV4YW1wbGUu\n"
    "dGVzdDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABE/8GhZBxFDBxs3MQlULOipk\n"
    "K2whicZHVnWb5xbCl/ti7FxcejWGv6Jf/pkwZ7cjaoDX3L9vE78yolB5DFmd6vij\n"
    "TTBLMAkGA1UdEwQCMAAwHQYDVR0OBBYEFIm8Asp3I9KhWC3Xmti8drLjYygYMB8G\n"
    "A1UdIwQYMBaAFLcVkYu6I3JpjzYUROl500Omr/wkMA0GCSqGSIb3DQEBCwUAA4GB\n"
    "AAuCHD0HwYYr194IIahAprpyQ7BDNkPD4KYvF2oEv1s8hbewkBtzyMnxfO5pnUxQ\n"
    "EiAwOZTRPYRjhhvfj8ctRa372Z/HyRrwT5iFaJztyh2YWRjV0VYfY4kh/rdffz2e\n"
    "mWAqa6IvblnnBUR6RFCrp8dHP4x02JcohvHIiFdvyXuK\n"
    "-----END CERTIFICATE-----\n";

static const char SubCaPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICCjCCAXOgAwIBAgIBCDANBgkqhkiG9w0BAQsFADA/MQswCQYDVQQGEwJDWjEU\n"
    "MBIGA1UECgwLQW55UFM1IFRlc3QxGjAYBgNVBAMMEVRlc3QgSW50ZXJtZWRpYXRl\n"
    "MB4XDTI2MTAyMTE4MjA0M1oXDTQ1MTIyMDE4MjA0M1owOTELMAkGA1UEBhMCQ1ox\n"
    "FDASBgNVBAoMC0FueVBTNSBUZXN0MRQwEgYDVQQDDAtUZXN0IFN1YiBDQTBZMBMG\n"
    "ByqGSM49AgEGCCqGSM49AwEHA0IABBXHCHk8dNEAYd4hUOSgL+8h5ZoPxhJBWygy\n"
    "vLhqAB506M0CLhWvIZDIO1xpfDtbjYGleGnT5+HM2dAdzisj6XijYzBhMA8GA1Ud\n"
    "EwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMB0GA1UdDgQWBBS58YbM0B4bgRz7\n"
    "dl9G5980Ff6NQzAfBgNVHSMEGDAWgBS3FZGLuiNyaY82FETpedNDpq/8JDANBgkq\n"
    "hkiG9w0BAQsFAAOBgQBiY/U2iUXTqINmwvIMPd5T/G8mU3LZQiV3IHIB0lK9c0Zf\n"
    "SBu8BpCOO1oz0EmF3S54rvKrLPFdq12fGUGjIcWar03R6E90ahbKOq7PrnEKYvW8\n"
    "PgfC7BUqjTMecQuVZuRLFDUIXX9FLbS2jHj5yqNTyZLz8m+CPlOFVahVi4F/Pw==\n"
    "-----END CERTIFICATE-----\n";

static const char DeepPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBuDCCAV2gAwIBAgIBCTAKBggqhkjOPQQDAjA5MQswCQYDVQQGEwJDWjEUMBIG\n"
    "A1UECgwLQW55UFM1IFRlc3QxFDASBgNVBAMMC1Rlc3QgU3ViIENBMB4XDTI2MTAy\n"
    "MTE4MjA0NFoXDTQ1MTIyMDE4MjA0NFowQjELMAkGA1UEBhMCQ1oxFDASBgNVBAoM\n"
    "C0FueVBTNSBUZXN0MR0wGwYDVQQDDBRyZXZva2VkLmV4YW1wbGUudGVzdDBZMBMG\n"
    "ByqGSM49AgEGCCqGSM49AwEHA0IABE/8GhZBxFDBxs3MQlULOipkK2whicZHVnWb\n"
    "5xbCl/ti7FxcejWGv6Jf/pkwZ7cjaoDX3L9vE78yolB5DFmd6vijTTBLMAkGA1Ud\n"
    "EwQCMAAwHQYDVR0OBBYEFIm8Asp3I9KhWC3Xmti8drLjYygYMB8GA1UdIwQYMBaA\n"
    "FLnxhszQHhuBHPt2X0bn3zQV/o1DMAoGCCqGSM49BAMCA0kAMEYCIQCrpkc1mfaS\n"
    "NNkHoUHRrVwP0pJl9hWFl2KOsNp27r0pvQIhAOrs+uWeeelMtC4HTIc5T4MQFOsi\n"
    "movaZ1mPKN88beA9\n"
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

static const char UnderNotCaPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBnDCCAUGgAwIBAgIBCzAKBggqhkjOPQQDAjBCMQswCQYDVQQGEwJDWjEUMBIG\n"
    "A1UECgwLQW55UFM1IFRlc3QxHTAbBgNVBAMMFHJldm9rZWQuZXhhbXBsZS50ZXN0\n"
    "MB4XDTI2MTAyMTE4MjExMFoXDTQ1MTIyMDE4MjExMFowHTEbMBkGA1UEAwwSdW5k\n"
    "ZXIuZXhhbXBsZS50ZXN0MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEBRzFLvNU\n"
    "GS7TArV/nRsBPiD/vysNJqU2hd8blwviNq7Ls/TAx+TA1uVEEjVu2gvbspmmsVeR\n"
    "Fbmk0brZ5qq7laNNMEswCQYDVR0TBAIwADAdBgNVHQ4EFgQUf0FoB9S89W3Us7qb\n"
    "kcyQMBxunS8wHwYDVR0jBBgwFoAUibwCyncj0qFYLdea2Lx2suNjKBgwCgYIKoZI\n"
    "zj0EAwIDSQAwRgIhANOCCkt7hKC/DGWq+UXgOtYs/N/btup8DH4aoP+YvVnvAiEA\n"
    "wgbjUagokiWlRFnrXaRs+uvvmKypa5BYRnqud/0hyFY=\n"
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

constexpr std::int64_t Year2031 = 1924992000;
constexpr std::int64_t Year2025 = 1735689600;

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

static GuestX509* Load(const char* pem) {
    std::string text(pem);
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(text.data(), static_cast<int>(text.size()));
    GuestX509* x = PEM_read_bio_X509_nid_postfix(bio, nullptr, nullptr, nullptr);
    BIO_free_nid_postfix(bio);
    Require(x != nullptr);
    return x;
}

struct Certs {
    GuestX509* root = Load(RootPem);
    GuestX509* inter = Load(InterPem);
    GuestX509* leaf = Load(LeafPem);
    GuestX509* revoked = Load(RevokedPem);
    GuestX509* expired = Load(ExpiredPem);
    GuestX509* self = Load(SelfPem);
    GuestX509* badsig = Load(BadSigPem);
    GuestX509* subca = Load(SubCaPem);
    GuestX509* deep = Load(DeepPem);
    GuestX509* underleaf = Load(UnderLeafPem);
    GuestX509* undernotca = Load(UnderNotCaPem);

    ~Certs() {
        for (GuestX509* x : {root, inter, leaf, revoked, expired, self, badsig, subca, deep, underleaf, undernotca}) X509_free_nid_postfix(x);
    }
};

struct List {
    std::vector<char*> items;
    GuestStack stack{};

    List(std::initializer_list<GuestX509*> certs) {
        for (auto* cert : certs) items.push_back(reinterpret_cast<char*>(cert));
        stack = {static_cast<int>(items.size()), items.data(), 0, static_cast<int>(items.size()), nullptr};
    }
};

struct Outcome {
    int result;
    int error;
    int depth;
    GuestX509* current;
    int chain;
};

static Outcome Verify(GuestStore* store, GuestX509* cert, GuestStack* untrusted, std::int64_t time = Year2031, std::uint64_t flags = 0) {
    GuestStoreCtx* ctx = X509_STORE_CTX_new_nid_postfix();
    Require(ctx && X509_STORE_CTX_init_nid_postfix(ctx, store, cert, untrusted) == 1);
    Require(ctx->param && ctx->cert == cert && ctx->untrusted == untrusted && ctx->ctx == store);
    ctx->param->check_time = time;
    ctx->param->flags |= 0x2 | flags;
    Outcome outcome{};
    outcome.result = X509_verify_cert_nid_postfix(ctx);
    outcome.error = X509_STORE_CTX_get_error_nid_postfix(ctx);
    outcome.depth = X509_STORE_CTX_get_error_depth_nid_postfix(ctx);
    outcome.current = X509_STORE_CTX_get_current_cert_nid_postfix(ctx);
    outcome.chain = sk_num_nid_postfix(X509_STORE_CTX_get_chain_nid_postfix(ctx));
    Require(ctx->error == outcome.error && ctx->error_depth == outcome.depth);
    X509_STORE_CTX_free_nid_postfix(ctx);
    return outcome;
}

static bool Fails(const Outcome& outcome, int error, int depth, GuestX509* current) {
    return outcome.result == 0 && outcome.error == error && outcome.depth == depth && outcome.current == current;
}

static std::vector<int> calls;

static int APS5_VABI Accepting(int ok, GuestStoreCtx* ctx) {
    calls.push_back(ok);
    calls.push_back(X509_STORE_CTX_get_error_nid_postfix(ctx));
    calls.push_back(X509_STORE_CTX_get_error_depth_nid_postfix(ctx));
    return 1;
}

static int APS5_VABI Rejecting(int ok, GuestStoreCtx* ctx) {
    calls.push_back(ok);
    X509_STORE_CTX_set_error_nid_postfix(ctx, 50);
    return 0;
}

static void TestChains(Certs& c) {
    GuestStore* store = X509_STORE_new_nid_postfix();
    Require(X509_STORE_add_cert_nid_postfix(store, c.root) == 1);
    List inter{c.inter};
    GuestStoreCtx* ctx = X509_STORE_CTX_new_nid_postfix();
    Require(X509_STORE_CTX_init_nid_postfix(ctx, store, c.leaf, &inter.stack) == 1);
    ctx->param->check_time = Year2031;
    ctx->param->flags |= 0x2;
    Require(X509_STORE_CTX_get_ex_data_nid_postfix(ctx, 0) == nullptr);
    Require(X509_verify_cert_nid_postfix(ctx) == 1 && X509_STORE_CTX_get_error_nid_postfix(ctx) == 0 && ctx->valid == 1);
    GuestStack* chain = X509_STORE_CTX_get_chain_nid_postfix(ctx);
    Require(sk_num_nid_postfix(chain) == 3 && sk_value_nid_postfix(chain, 0) == c.leaf && sk_value_nid_postfix(chain, 1) == c.inter);
    Require(sk_value_nid_postfix(chain, 2) == c.root && ctx->last_untrusted == 2 && X509_STORE_CTX_get_current_cert_nid_postfix(ctx) == c.leaf);
    const int leafRefs = c.leaf->references;
    GuestStack* copy = X509_STORE_CTX_get1_chain_nid_postfix(ctx);
    Require(copy && copy != chain && sk_num_nid_postfix(copy) == 3 && c.leaf->references == leafRefs + 1);
    Require(Throws([&] { sk_pop_free_nid_postfix(&inter.stack, reinterpret_cast<void (APS5_VABI*)(void*)>(X509_free_nid_postfix)); }));
    Require(X509_STORE_CTX_init_nid_postfix(ctx, store, c.leaf, nullptr) == 1 && ctx->chain == nullptr && c.leaf->references == leafRefs);
    sk_pop_free_nid_postfix(copy, reinterpret_cast<void (APS5_VABI*)(void*)>(X509_free_nid_postfix));
    Require(c.leaf->references == leafRefs - 1);
    X509_STORE_CTX_free_nid_postfix(ctx);
    X509_STORE_CTX_free_nid_postfix(nullptr);

    Require(Fails(Verify(store, c.leaf, nullptr), 20, 0, c.leaf));
    List leafIssuer{c.leaf, c.inter};
    Require(Fails(Verify(store, c.underleaf, &leafIssuer.stack), 20, 0, c.underleaf));
    List full{c.inter, c.root};
    GuestStore* empty = X509_STORE_new_nid_postfix();
    Require(Fails(Verify(empty, c.leaf, &full.stack), 19, 2, c.root));
    Require(Fails(Verify(empty, c.self, nullptr), 18, 0, c.self));
    Require(Fails(Verify(nullptr, c.self, nullptr), 18, 0, c.self));
    GuestStore* partial = X509_STORE_new_nid_postfix();
    Require(X509_STORE_add_cert_nid_postfix(partial, c.inter) == 1);
    Require(Fails(Verify(partial, c.leaf, nullptr), 2, 1, c.inter));
    const auto ok = Verify(partial, c.leaf, nullptr, Year2031, 0x80000);
    Require(ok.result == 1 && ok.error == 0 && ok.chain == 2);
    Require(Fails(Verify(store, c.expired, &inter.stack), 10, 0, c.expired));
    Require(Fails(Verify(store, c.leaf, &inter.stack, Year2025), 9, 2, c.root));
    Require(Verify(store, c.expired, &inter.stack, Year2031, 0x200000).result == 1);
    Require(Fails(Verify(store, c.badsig, &inter.stack), 7, 0, c.badsig));
    List deep{c.subca, c.inter};
    Require(Fails(Verify(store, c.deep, &deep.stack), 25, 2, c.inter));
    List notCa{c.revoked, c.inter};
    Require(Fails(Verify(store, c.undernotca, &notCa.stack), 24, 1, c.revoked));
    GuestStore* selfStore = X509_STORE_new_nid_postfix();
    Require(X509_STORE_add_cert_nid_postfix(selfStore, c.self) == 1);
    const auto trustedSelf = Verify(selfStore, c.self, nullptr);
    Require(trustedSelf.result == 1 && trustedSelf.chain == 1);
    Require(Verify(store, c.leaf, &inter.stack, Year2031, 0x20).result == 1);

    GuestStoreCtx* noCert = X509_STORE_CTX_new_nid_postfix();
    Require(X509_STORE_CTX_init_nid_postfix(noCert, store, nullptr, nullptr) == 1);
    ERR_clear_error_nid_postfix();
    Require(X509_verify_cert_nid_postfix(noCert) == -1 && ERR_get_error_nid_postfix() == ((11ull << 24) | (127ull << 12) | 105));
    X509_STORE_CTX_free_nid_postfix(noCert);

    GuestStoreCtx local;
    std::memset(&local, 0x5a, sizeof(local));
    Require(X509_STORE_CTX_init_nid_postfix(&local, store, c.leaf, &inter.stack) == 1 && local.error == 0 && local.chain == nullptr);
    local.param->flags |= 0x2;
    local.param->check_time = Year2031;
    Require(X509_verify_cert_nid_postfix(&local) == 1 && sk_num_nid_postfix(local.chain) == 3);
    Require(X509_STORE_CTX_init_nid_postfix(&local, store, c.leaf, &inter.stack) == 1);

    Require(X509_STORE_up_ref_nid_postfix(store) == 1 && store->references == 2);
    X509_STORE_free_nid_postfix(store);
    Require(store->references == 1);
    X509_STORE_free_nid_postfix(selfStore);
    X509_STORE_free_nid_postfix(partial);
    X509_STORE_free_nid_postfix(empty);
    X509_STORE_free_nid_postfix(store);
}

static void TestCallbacks(Certs& c) {
    GuestStore* store = X509_STORE_new_nid_postfix();
    store->verify_cb = Accepting;
    calls.clear();
    const auto outcome = Verify(store, c.leaf, nullptr);
    Require(outcome.result == 1 && outcome.error == 21);
    Require(calls == std::vector<int>({0, 20, 0, 0, 21, 0, 1, 21, 0}));
    Require(X509_STORE_add_cert_nid_postfix(store, c.root) == 1);
    calls.clear();
    List inter{c.inter};
    Require(Verify(store, c.expired, &inter.stack).result == 1);
    Require(calls == std::vector<int>({1, 0, 2, 1, 0, 1, 0, 10, 0, 1, 10, 0}));
    store->verify_cb = Rejecting;
    calls.clear();
    const auto rejected = Verify(store, c.leaf, &inter.stack);
    Require(rejected.result == 0 && rejected.error == 50 && calls == std::vector<int>({1}));
    X509_STORE_free_nid_postfix(store);
}

static void TestRevocation(Certs& c) {
    std::string text(CrlPem);
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(text.data(), static_cast<int>(text.size()));
    GuestStack* infos = PEM_X509_INFO_read_bio_nid_postfix(bio, nullptr, nullptr, nullptr);
    BIO_free_nid_postfix(bio);
    Require(infos && sk_num_nid_postfix(infos) == 1);
    void* crl = static_cast<GuestInfo*>(sk_value_nid_postfix(infos, 0))->crl;
    GuestStore* store = X509_STORE_new_nid_postfix();
    Require(X509_STORE_add_cert_nid_postfix(store, c.root) == 1);
    List inter{c.inter};
    Require(Fails(Verify(store, c.leaf, &inter.stack, Year2031, 0x4), 3, 0, c.leaf));
    Require(X509_STORE_add_crl_nid_postfix(store, crl) == 1);
    Require(Verify(store, c.leaf, &inter.stack, Year2031, 0x4).result == 1);
    Require(Fails(Verify(store, c.revoked, &inter.stack, Year2031, 0x4), 23, 0, c.revoked));
    Require(Fails(Verify(store, c.leaf, &inter.stack, Year2031, 0x8), 3, 1, c.inter));
    Require(Fails(Verify(store, c.leaf, &inter.stack, Year2025, 0x4), 11, 0, c.leaf));
    Require(Verify(store, c.revoked, &inter.stack).result == 1);
    sk_pop_free_nid_postfix(infos, X509_INFO_free_nid_postfix);
    X509_STORE_free_nid_postfix(store);
}

static void TestNames(Certs& c) {
    GuestStore* store = X509_STORE_new_nid_postfix();
    Require(X509_STORE_add_cert_nid_postfix(store, c.root) == 1);
    List inter{c.inter};
    const auto host = [&](const char* name, unsigned flags) {
        Require(X509_VERIFY_PARAM_set1_host_nid_postfix(store->param, name, 0) == 1);
        X509_VERIFY_PARAM_set_hostflags_nid_postfix(store->param, flags);
        return Verify(store, c.leaf, &inter.stack);
    };
    Require(host("host.example.test", 0).result == 1);
    Require(host("HOST.Example.Test", 0).result == 1);
    Require(host("other.example.test", 0).result == 1);
    Require(Fails(host("other.example.test", 0x2), 62, 0, c.leaf));
    Require(Fails(host("a.b.example.test", 0), 62, 0, c.leaf));
    Require(host("a.b.example.test", 0x8).result == 1);
    Require(Fails(host("leaf.other.test", 0), 62, 0, c.leaf));
    Require(Fails(host("w.leaf.example.test", 0), 62, 0, c.leaf));
    Require(host(".example.test", 0).result == 1);
    Require(Fails(host("example.test", 0), 62, 0, c.leaf));
    Require(X509_VERIFY_PARAM_set1_host_nid_postfix(store->param, nullptr, 0) == 1);
    Require(X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(store->param, "192.0.2.7") == 1);
    Require(Verify(store, c.leaf, &inter.stack).result == 1);
    Require(X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(store->param, "192.0.2.8") == 1);
    Require(Fails(Verify(store, c.leaf, &inter.stack), 64, 0, c.leaf));
    X509_STORE_free_nid_postfix(store);
    GuestStore* selfStore = X509_STORE_new_nid_postfix();
    Require(X509_STORE_add_cert_nid_postfix(selfStore, c.self) == 1);
    Require(X509_VERIFY_PARAM_set1_host_nid_postfix(selfStore->param, "Self.Example.Test", 0) == 1);
    Require(Verify(selfStore, c.self, nullptr).result == 1);
    Require(X509_VERIFY_PARAM_set1_host_nid_postfix(selfStore->param, "x.example.test", 14) == 1);
    Require(Fails(Verify(selfStore, c.self, nullptr), 62, 0, c.self));
    X509_STORE_free_nid_postfix(selfStore);
}

static void TestAccessors(Certs& c) {
    Require(X509_get_ext_count_nid_postfix(c.leaf) == 5);
    GuestExtension* san = X509_get_ext_nid_postfix(c.leaf, 4);
    Require(san && san->object->nid == 85 && X509_get_ext_nid_postfix(c.leaf, 5) == nullptr && X509_get_ext_nid_postfix(c.leaf, -1) == nullptr);
    Require(X509_get0_extensions_nid_postfix(c.leaf) == c.leaf->cert_info->extensions);
    const GuestAsn1String* sig = nullptr;
    const GuestAlgor* alg = nullptr;
    X509_get0_signature_nid_postfix(&sig, &alg, c.leaf);
    Require(sig == c.leaf->signature && alg == c.leaf->sig_alg && alg->algorithm->nid == 668);
    X509_get0_signature_nid_postfix(nullptr, nullptr, c.leaf);
    const GuestAsn1Object* algorithm = nullptr;
    const unsigned char* pk = nullptr;
    int length = 0;
    GuestAlgor* pa = nullptr;
    Require(X509_PUBKEY_get0_param_nid_postfix(&algorithm, &pk, &length, &pa, c.leaf->cert_info->key) == 1);
    Require(algorithm->nid == 408 && length == 65 && pk[0] == 4 && pa == c.leaf->cert_info->key->algor);
    Require(X509_PUBKEY_get0_param_nid_postfix(nullptr, nullptr, nullptr, nullptr, c.inter->cert_info->key) == 1);
    Require(Throws([] { X509_PUBKEY_get0_param_nid_postfix(nullptr, nullptr, nullptr, nullptr, nullptr); }));
}

int main() {
    void* const heapApi[10] {};
    ApplicationHeapRegister_nid_no_patch(heapApi);
    Certs certs;
    TestChains(certs);
    TestCallbacks(certs);
    TestRevocation(certs);
    TestNames(certs);
    TestAccessors(certs);
    return 0;
}
