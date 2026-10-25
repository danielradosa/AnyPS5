#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <source_location>
#include <string>
#include <vector>

struct GuestBio;
struct GuestBioMethod;
struct GuestSsl;
struct GuestCtx;
struct GuestMethod;
struct GuestCipher;
struct GuestX509;
struct GuestName;

struct GuestStack {
    int num;
    char** data;
    int sorted;
    int numAlloc;
    void* comp;
};

struct GuestSession {
    int sslVersion;
    int masterKeyLength;
    unsigned char masterKey[48];
    unsigned int sessionIdLength;
    unsigned char sessionId[32];
    unsigned int sidCtxLength;
    unsigned char sidCtx[32];
    GuestX509* peer;
    std::int64_t verifyResult;
    std::int64_t timeout;
    std::int64_t time;
    int references;
    const GuestCipher* cipher;
    std::uint64_t cipherId;
    void* ciphers;
    char* tlsextHostname;
    unsigned char* tlsextTick;
    std::size_t tlsextTicklen;
    std::int64_t tlsextTickLifetimeHint;
    void* internal;
};

struct GuestCtxHead {
    const GuestMethod* method;
    void* cipherList;
    void* certStore;
    std::int64_t sessionTimeout;
    int references;
    void* extraCerts;
    int verifyMode;
};

struct GuestSslHead {
    int version;
    const GuestMethod* method;
    GuestBio* rbio;
    GuestBio* wbio;
    GuestBio* bbio;
    int server;
};

extern "C" {
const GuestMethod* APS5_VABI SSLv23_client_method_nid_postfix();
const GuestMethod* APS5_VABI SSLv23_server_method_nid_postfix();
const GuestMethod* APS5_VABI TLS_client_method_nid_postfix();
GuestCtx* APS5_VABI SSL_CTX_new_nid_postfix(const GuestMethod*);
void APS5_VABI SSL_CTX_free_nid_postfix(GuestCtx*);
std::int64_t APS5_VABI SSL_CTX_ctrl_nid_postfix(GuestCtx*, int, std::int64_t, void*);
void APS5_VABI SSL_CTX_set_verify_nid_postfix(GuestCtx*, int, void*);
int APS5_VABI SSL_CTX_get_verify_mode_nid_postfix(const GuestCtx*);
int APS5_VABI SSL_CTX_use_certificate_ASN1_nid_postfix(GuestCtx*, int, const unsigned char*);
int APS5_VABI SSL_CTX_use_certificate_nid_postfix(GuestCtx*, GuestX509*);
int APS5_VABI SSL_CTX_use_certificate_file_nid_postfix(GuestCtx*, const char*, int);
int APS5_VABI SSL_CTX_use_certificate_chain_file_nid_postfix(GuestCtx*, const char*);
int APS5_VABI SSL_CTX_use_PrivateKey_ASN1_nid_postfix(int, GuestCtx*, const unsigned char*, std::int64_t);
int APS5_VABI SSL_CTX_use_PrivateKey_file_nid_postfix(GuestCtx*, const char*, int);
int APS5_VABI SSL_CTX_check_private_key_nid_postfix(const GuestCtx*);
int APS5_VABI SSL_CTX_load_verify_mem_nid_postfix(GuestCtx*, void*, int);
int APS5_VABI SSL_CTX_load_verify_locations_nid_postfix(GuestCtx*, const char*, const char*);
int APS5_VABI SSL_CTX_set_cipher_list_nid_postfix(GuestCtx*, const char*);
int APS5_VABI SSL_CTX_set1_groups_list_nid_postfix(GuestCtx*, const char*);
int APS5_VABI SSL_CTX_set_alpn_protos_nid_postfix(GuestCtx*, const unsigned char*, unsigned int);
int APS5_VABI SSL_CTX_set_session_id_context_nid_postfix(GuestCtx*, const unsigned char*, unsigned int);
void APS5_VABI SSL_CTX_set_quiet_shutdown_nid_postfix(GuestCtx*, int);
int APS5_VABI SSL_CTX_get_ex_new_index_nid_postfix(std::int64_t, void*, void*, void*, void*);
int APS5_VABI SSL_CTX_set_ex_data_nid_postfix(GuestCtx*, int, void*);
void* APS5_VABI SSL_CTX_get_ex_data_nid_postfix(const GuestCtx*, int);
void APS5_VABI SSL_CTX_sess_set_new_cb_nid_postfix(GuestCtx*, int (APS5_VABI*)(GuestSsl*, GuestSession*));
void APS5_VABI SSL_CTX_set_info_callback_nid_postfix(GuestCtx*, void (APS5_VABI*)(const GuestSsl*, int, int));
void APS5_VABI SSL_CTX_set_next_proto_select_cb_nid_postfix(GuestCtx*,
    int (APS5_VABI*)(GuestSsl*, unsigned char**, unsigned char*, const unsigned char*, unsigned int, void*), void*);
void APS5_VABI SSL_CTX_set_alpn_select_cb_nid_postfix(GuestCtx*,
    int (APS5_VABI*)(GuestSsl*, const unsigned char**, unsigned char*, const unsigned char*, unsigned int, void*), void*);
std::int64_t APS5_VABI SSL_CTX_callback_ctrl_nid_postfix(GuestCtx*, int, void (*)());
void APS5_VABI SSL_CTX_set_default_passwd_cb_userdata_nid_postfix(GuestCtx*, void*);
GuestSsl* APS5_VABI SSL_new_nid_postfix(GuestCtx*);
void APS5_VABI SSL_free_nid_postfix(GuestSsl*);
int APS5_VABI SSL_set_fd_nid_postfix(GuestSsl*, int);
int APS5_VABI SSL_get_fd_nid_postfix(const GuestSsl*);
void APS5_VABI SSL_set_bio_nid_postfix(GuestSsl*, GuestBio*, GuestBio*);
GuestBio* APS5_VABI SSL_get_rbio_nid_postfix(const GuestSsl*);
GuestBio* APS5_VABI SSL_get_wbio_nid_postfix(const GuestSsl*);
int APS5_VABI SSL_connect_nid_postfix(GuestSsl*);
int APS5_VABI SSL_accept_nid_postfix(GuestSsl*);
void APS5_VABI SSL_set_connect_state_nid_postfix(GuestSsl*);
int APS5_VABI SSL_read_nid_postfix(GuestSsl*, void*, int);
int APS5_VABI SSL_peek_nid_postfix(GuestSsl*, void*, int);
int APS5_VABI SSL_write_nid_postfix(GuestSsl*, const void*, int);
int APS5_VABI SSL_pending_nid_postfix(const GuestSsl*);
int APS5_VABI SSL_shutdown_nid_postfix(GuestSsl*);
int APS5_VABI SSL_get_shutdown_nid_postfix(const GuestSsl*);
int APS5_VABI SSL_get_error_nid_postfix(const GuestSsl*, int);
int APS5_VABI SSL_want_nid_postfix(const GuestSsl*);
std::int64_t APS5_VABI SSL_ctrl_nid_postfix(GuestSsl*, int, std::int64_t, void*);
const char* APS5_VABI SSL_get_servername_nid_postfix(const GuestSsl*, int);
std::int64_t APS5_VABI SSL_get_verify_result_nid_postfix(const GuestSsl*);
void APS5_VABI SSL_set_verify_nid_postfix(GuestSsl*, int, void*);
GuestX509* APS5_VABI SSL_get_peer_certificate_nid_postfix(const GuestSsl*);
GuestStack* APS5_VABI SSL_get_peer_cert_chain_nid_postfix(const GuestSsl*);
GuestX509* APS5_VABI SSL_get_certificate_nid_postfix(const GuestSsl*);
int APS5_VABI SSL_check_private_key_nid_postfix(const GuestSsl*);
int APS5_VABI SSL_version_nid_postfix(const GuestSsl*);
const GuestCipher* APS5_VABI SSL_get_current_cipher_nid_postfix(const GuestSsl*);
const char* APS5_VABI SSL_CIPHER_get_name_nid_postfix(const GuestCipher*);
int APS5_VABI SSL_CIPHER_get_bits_nid_postfix(const GuestCipher*, int*);
const char* APS5_VABI SSL_CIPHER_get_version_nid_postfix(const GuestCipher*);
void APS5_VABI SSL_get0_alpn_selected_nid_postfix(const GuestSsl*, const unsigned char**, unsigned int*);
int APS5_VABI SSL_select_next_proto_nid_postfix(unsigned char**, unsigned char*, const unsigned char*, unsigned int, const unsigned char*, unsigned int);
GuestSession* APS5_VABI SSL_get_session_nid_postfix(const GuestSsl*);
GuestSession* APS5_VABI SSL_get1_session_nid_postfix(GuestSsl*);
int APS5_VABI SSL_set_session_nid_postfix(GuestSsl*, GuestSession*);
void APS5_VABI SSL_SESSION_free_nid_postfix(GuestSession*);
GuestCtx* APS5_VABI SSL_get_SSL_CTX_nid_postfix(const GuestSsl*);
GuestCtx* APS5_VABI SSL_set_SSL_CTX_nid_postfix(GuestSsl*, GuestCtx*);
int APS5_VABI SSL_get_ex_new_index_nid_postfix(std::int64_t, void*, void*, void*, void*);
int APS5_VABI SSL_set_ex_data_nid_postfix(GuestSsl*, int, void*);
void* APS5_VABI SSL_get_ex_data_nid_postfix(const GuestSsl*, int);
const char* APS5_VABI SSL_alert_desc_string_long_nid_postfix(int);
GuestBio* APS5_VABI BIO_new_socket_nid_postfix(int, int);
const GuestBioMethod* APS5_VABI BIO_f_ssl_nid_postfix();
const GuestBioMethod* APS5_VABI BIO_s_mem_nid_postfix();
GuestBio* APS5_VABI BIO_new_nid_postfix(const GuestBioMethod*);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
int APS5_VABI BIO_write_nid_postfix(GuestBio*, const void*, int);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
GuestX509* APS5_VABI d2i_X509_nid_postfix(GuestX509**, const unsigned char**, std::int64_t);
void APS5_VABI X509_free_nid_postfix(GuestX509*);
GuestName* APS5_VABI X509_get_subject_name_nid_postfix(GuestX509*);
int APS5_VABI X509_NAME_get_text_by_NID_nid_postfix(GuestName*, int, char*, int);
int APS5_VABI sk_num_nid_postfix(const GuestStack*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
int APS5_VABI socketpair_nid_postfix(int, int, int, int*);
int APS5_VABI close_nid_postfix(int);
}

namespace {

constexpr int ErrorNone = 0, ErrorSsl = 1, ErrorWantRead = 2, ErrorSyscall = 5, ErrorZeroReturn = 6;
constexpr int VerifyPeer = 1;
constexpr int FiletypePem = 1, FiletypeAsn1 = 2;
constexpr int PkeyRsa = 6, PkeyEc = 408;
constexpr int CtrlGetSessionReused = 8, CtrlOptions = 32, CtrlMode = 33, CtrlSetSessCacheMode = 44, CtrlGetSessCacheMode = 45;
constexpr int CtrlSetTlsextServernameCb = 53, CtrlSetTlsextServernameArg = 54, CtrlSetTlsextHostname = 55, CtrlClearOptions = 77, CtrlSetEcdhAuto = 94;
constexpr int CtrlSetMinProtoVersion = 123, CtrlSetMaxProtoVersion = 124;
constexpr int BioCtrlReset = 1, BioCtrlInfo = 3, BioCtrlPending = 10, BioGetFd = 105, BioSetSsl = 109, BioGetSsl = 110;
constexpr std::uint64_t OptionNoTicket = 0x4000, OptionNoTlsv1_2 = 0x08000000;
constexpr int GuestUnix = 1, GuestStream = 1, GuestNonBlocking = 0x20000000;

const unsigned char CertificateDer[] = {
    0x30, 0x82, 0x01, 0xb4, 0x30, 0x82, 0x01, 0x59, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x02, 0x10,
    0x21, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x30, 0x37, 0x31,
    0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x14, 0x30, 0x12,
    0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x0b, 0x41, 0x6e, 0x79, 0x50, 0x53, 0x35, 0x20, 0x54, 0x65,
    0x73, 0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x09, 0x6c, 0x6f, 0x63,
    0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x30, 0x20, 0x17, 0x0d, 0x32, 0x36, 0x31, 0x30, 0x32, 0x31,
    0x31, 0x38, 0x30, 0x37, 0x34, 0x33, 0x5a, 0x18, 0x0f, 0x32, 0x31, 0x32, 0x36, 0x30, 0x39, 0x32,
    0x37, 0x31, 0x38, 0x30, 0x37, 0x34, 0x33, 0x5a, 0x30, 0x37, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03,
    0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x04, 0x0a,
    0x0c, 0x0b, 0x41, 0x6e, 0x79, 0x50, 0x53, 0x35, 0x20, 0x54, 0x65, 0x73, 0x74, 0x31, 0x12, 0x30,
    0x10, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73,
    0x74, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08,
    0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0xe4, 0x69, 0xf7, 0xb5,
    0x03, 0x82, 0xe1, 0xce, 0x56, 0x84, 0xef, 0x9b, 0xca, 0x1e, 0x18, 0x70, 0xdb, 0x0d, 0xc7, 0xb6,
    0xec, 0xca, 0x8b, 0x82, 0x77, 0x71, 0xe2, 0xad, 0xfe, 0x6a, 0x79, 0xaf, 0xbf, 0x34, 0xde, 0x4c,
    0xa3, 0xb3, 0xc5, 0xed, 0xba, 0xa4, 0xe9, 0x02, 0xf2, 0x80, 0xfd, 0xcc, 0x88, 0xeb, 0x4b, 0x85,
    0xa6, 0xbd, 0x22, 0x0b, 0xc2, 0x9d, 0xbd, 0x50, 0xb5, 0xec, 0xed, 0xe2, 0xa3, 0x53, 0x30, 0x51,
    0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0xa1, 0xf3, 0x7e, 0x1e, 0x45,
    0xf3, 0x91, 0x94, 0x87, 0xe1, 0x76, 0xf8, 0x91, 0x3f, 0xc4, 0x3a, 0x98, 0xae, 0x40, 0x7d, 0x30,
    0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0xa1, 0xf3, 0x7e, 0x1e,
    0x45, 0xf3, 0x91, 0x94, 0x87, 0xe1, 0x76, 0xf8, 0x91, 0x3f, 0xc4, 0x3a, 0x98, 0xae, 0x40, 0x7d,
    0x30, 0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01,
    0xff, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x03, 0x49, 0x00,
    0x30, 0x46, 0x02, 0x21, 0x00, 0xe3, 0x99, 0x76, 0x1e, 0x12, 0xb4, 0xe8, 0x62, 0x02, 0x70, 0xcf,
    0x31, 0xd1, 0xc2, 0xf5, 0x88, 0x05, 0xfd, 0xfc, 0x43, 0x28, 0x51, 0x01, 0x62, 0xaa, 0x0a, 0x9f,
    0x8a, 0xa5, 0xc2, 0xef, 0xab, 0x02, 0x21, 0x00, 0x8f, 0x3a, 0xad, 0x90, 0x03, 0x56, 0x19, 0xb9,
    0x58, 0x62, 0xd4, 0x9d, 0xb9, 0x91, 0xce, 0xa3, 0x61, 0x03, 0xfc, 0xda, 0x56, 0xd6, 0xc6, 0xdc,
    0x2f, 0xd3, 0x7b, 0x27, 0x87, 0x35, 0xb2, 0x99,
};

const unsigned char KeyDer[] = {
    0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0x2e, 0x55, 0x4b, 0x8a, 0x03, 0xe8, 0xa3, 0x67, 0xb1,
    0x41, 0x2c, 0x71, 0x1e, 0x80, 0xd1, 0xf6, 0xba, 0x9c, 0x6b, 0xac, 0xfc, 0x52, 0x32, 0xb4, 0x79,
    0x5b, 0x2d, 0x3c, 0xcd, 0xd4, 0x58, 0x1d, 0xa0, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d,
    0x03, 0x01, 0x07, 0xa1, 0x44, 0x03, 0x42, 0x00, 0x04, 0xe4, 0x69, 0xf7, 0xb5, 0x03, 0x82, 0xe1,
    0xce, 0x56, 0x84, 0xef, 0x9b, 0xca, 0x1e, 0x18, 0x70, 0xdb, 0x0d, 0xc7, 0xb6, 0xec, 0xca, 0x8b,
    0x82, 0x77, 0x71, 0xe2, 0xad, 0xfe, 0x6a, 0x79, 0xaf, 0xbf, 0x34, 0xde, 0x4c, 0xa3, 0xb3, 0xc5,
    0xed, 0xba, 0xa4, 0xe9, 0x02, 0xf2, 0x80, 0xfd, 0xcc, 0x88, 0xeb, 0x4b, 0x85, 0xa6, 0xbd, 0x22,
    0x0b, 0xc2, 0x9d, 0xbd, 0x50, 0xb5, 0xec, 0xed, 0xe2,
};

const char CertificatePem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBtDCCAVmgAwIBAgICECEwCgYIKoZIzj0EAwIwNzELMAkGA1UEBhMCVVMxFDAS\n"
    "BgNVBAoMC0FueVBTNSBUZXN0MRIwEAYDVQQDDAlsb2NhbGhvc3QwIBcNMjYxMDIx\n"
    "MTgwNzQzWhgPMjEyNjA5MjcxODA3NDNaMDcxCzAJBgNVBAYTAlVTMRQwEgYDVQQK\n"
    "DAtBbnlQUzUgVGVzdDESMBAGA1UEAwwJbG9jYWxob3N0MFkwEwYHKoZIzj0CAQYI\n"
    "KoZIzj0DAQcDQgAE5Gn3tQOC4c5WhO+byh4YcNsNx7bsyouCd3Hirf5qea+/NN5M\n"
    "o7PF7bqk6QLygP3MiOtLhaa9IgvCnb1Qtezt4qNTMFEwHQYDVR0OBBYEFKHzfh5F\n"
    "85GUh+F2+JE/xDqYrkB9MB8GA1UdIwQYMBaAFKHzfh5F85GUh+F2+JE/xDqYrkB9\n"
    "MA8GA1UdEwEB/wQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAOOZdh4StOhiAnDP\n"
    "MdHC9YgF/fxDKFEBYqoKn4qlwu+rAiEAjzqtkANWGblYYtSduZHOo2ED/NpW1sbc\n"
    "L9N7J4c1spk=\n"
    "-----END CERTIFICATE-----\n";

const char KeyPem[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgLlVLigPoo2exQSxx\n"
    "HoDR9rqca6z8UjK0eVstPM3UWB2hRANCAATkafe1A4LhzlaE75vKHhhw2w3HtuzK\n"
    "i4J3ceKt/mp5r7803kyjs8XtuqTpAvKA/cyI60uFpr0iC8KdvVC17O3i\n"
    "-----END PRIVATE KEY-----\n";


void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "LibreSSL TLS check failed at line %u\n", location.line());
    std::abort();
}

template <typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

struct Pair {
    int fds[2]{-1, -1};

    Pair() {
        Require(socketpair_nid_postfix(GuestUnix, GuestStream | GuestNonBlocking, 0, fds) == 0);
    }

    ~Pair() {
        close_nid_postfix(fds[0]);
        close_nid_postfix(fds[1]);
    }
};

bool Handshake(GuestSsl* client, GuestSsl* server) {
    int clientResult = 0;
    int serverResult = 0;
    bool sawWant = false;
    for (int round = 0; round < 2000 && (clientResult != 1 || serverResult != 1); ++round) {
        if (clientResult != 1) {
            clientResult = SSL_connect_nid_postfix(client);
            if (clientResult != 1) {
                const int error = SSL_get_error_nid_postfix(client, clientResult);
                if (error != ErrorWantRead) return false;
                sawWant = true;
            }
        }
        if (serverResult != 1) {
            serverResult = SSL_accept_nid_postfix(server);
            if (serverResult != 1 && SSL_get_error_nid_postfix(server, serverResult) != ErrorWantRead) return false;
        }
    }
    return clientResult == 1 && serverResult == 1 && sawWant;
}

int ReadAll(GuestSsl* reader, GuestSsl* writer, std::vector<unsigned char>& out, std::size_t wanted) {
    unsigned char chunk[4096];
    for (int round = 0; round < 20000 && out.size() < wanted; ++round) {
        const int got = SSL_read_nid_postfix(reader, chunk, sizeof(chunk));
        if (got > 0) {
            out.insert(out.end(), chunk, chunk + got);
            continue;
        }
        if (SSL_get_error_nid_postfix(reader, got) != ErrorWantRead) return got;
        if (writer) SSL_pending_nid_postfix(writer);
    }
    return static_cast<int>(out.size());
}

std::string BioData(GuestBio* bio) {
    char* data = nullptr;
    const auto length = BIO_ctrl_nid_postfix(bio, BioCtrlInfo, 0, &data);
    std::string text = length > 0 ? std::string(data, static_cast<std::size_t>(length)) : std::string();
    BIO_ctrl_nid_postfix(bio, BioCtrlReset, 0, nullptr);
    return text;
}

bool Shuttle(GuestBio* from, GuestBio* to) {
    const auto data = BioData(from);
    if (data.empty()) return false;
    Require(BIO_write_nid_postfix(to, data.data(), static_cast<int>(data.size())) == static_cast<int>(data.size()));
    return true;
}

int newSessions = 0;
int servernameCalls = 0;
GuestCtx* switchTarget = nullptr;

int APS5_VABI CountSession(GuestSsl*, GuestSession* session) {
    Require(session != nullptr && session->references >= 2);
    ++newSessions;
    return 0;
}

void APS5_VABI Info(const GuestSsl*, int, int) {
}

int APS5_VABI SelectNextProto(GuestSsl*, unsigned char**, unsigned char*, const unsigned char*, unsigned int, void*) {
    return 0;
}

int APS5_VABI SelectAlpn(GuestSsl*, const unsigned char**, unsigned char*, const unsigned char*, unsigned int, void*) {
    return 3;
}

int APS5_VABI Servername(GuestSsl* ssl, int* alert, void* argument) {
    Require(alert != nullptr && argument == &servernameCalls);
    const char* name = SSL_get_servername_nid_postfix(ssl, 0);
    Require(name && std::strcmp(name, "localhost") == 0);
    ++servernameCalls;
    if (switchTarget) Require(SSL_set_SSL_CTX_nid_postfix(ssl, switchTarget) == switchTarget);
    return 0;
}

GuestCtx* ServerContext() {
    auto* ctx = SSL_CTX_new_nid_postfix(SSLv23_server_method_nid_postfix());
    Require(ctx != nullptr);
    Require(SSL_CTX_use_certificate_ASN1_nid_postfix(ctx, sizeof(CertificateDer), CertificateDer) == 1);
    Require(SSL_CTX_use_PrivateKey_ASN1_nid_postfix(PkeyEc, ctx, KeyDer, sizeof(KeyDer)) == 1);
    Require(SSL_CTX_check_private_key_nid_postfix(ctx) == 1);
    return ctx;
}

GuestCtx* ClientContext(bool trust) {
    auto* ctx = SSL_CTX_new_nid_postfix(TLS_client_method_nid_postfix());
    Require(ctx != nullptr);
    if (trust) Require(SSL_CTX_load_verify_mem_nid_postfix(ctx, const_cast<char*>(CertificatePem), sizeof(CertificatePem) - 1) == 1);
    return ctx;
}

}

int main(int argc, char** argv) {
    void* const api[10] {};
    ApplicationHeapRegister_nid_no_patch(api);
    ERR_clear_error_nid_postfix();

    Require(SSL_CTX_new_nid_postfix(nullptr) == nullptr && ERR_get_error_nid_postfix() == 0x140A90C4);
    Require(std::strcmp(SSL_alert_desc_string_long_nid_postfix(0), "close notify") == 0);
    Require(std::strcmp(SSL_alert_desc_string_long_nid_postfix(42), "bad certificate") == 0);
    Require(std::strcmp(SSL_alert_desc_string_long_nid_postfix(0x0230), "unknown CA") == 0);
    Require(std::strcmp(SSL_alert_desc_string_long_nid_postfix(10), "unexpected message") == 0);
    Require(std::strcmp(SSL_alert_desc_string_long_nid_postfix(99), "unknown") == 0);
    Require(Throws([] { SSL_alert_desc_string_long_nid_postfix(112); }));
    Require(std::strcmp(SSL_CIPHER_get_name_nid_postfix(nullptr), "(NONE)") == 0 && SSL_CIPHER_get_bits_nid_postfix(nullptr, nullptr) == 0);
    Require(std::strcmp(SSL_CIPHER_get_version_nid_postfix(nullptr), "(NONE)") == 0);

    static const unsigned char serverProtocols[] = {6, 's', 'p', 'd', 'y', '/', '1', 8, 'h', 't', 't', 'p', '/', '1', '.', '1'};
    static const unsigned char clientProtocols[] = {2, 'h', '2', 8, 'h', 't', 't', 'p', '/', '1', '.', '1'};
    static const unsigned char otherProtocols[] = {3, 'f', 'o', 'o'};
    unsigned char* selected = nullptr;
    unsigned char selectedLength = 0;
    Require(SSL_select_next_proto_nid_postfix(&selected, &selectedLength, serverProtocols, sizeof(serverProtocols), clientProtocols, sizeof(clientProtocols)) == 1);
    Require(selected == serverProtocols + 8 && selectedLength == 8);
    Require(SSL_select_next_proto_nid_postfix(&selected, &selectedLength, serverProtocols, sizeof(serverProtocols), otherProtocols, sizeof(otherProtocols)) == 2);
    Require(selected == otherProtocols + 1 && selectedLength == 3);
    static const unsigned char truncated[] = {5, 'a', 'b'};
    Require(Throws([&] { SSL_select_next_proto_nid_postfix(&selected, &selectedLength, truncated, sizeof(truncated), otherProtocols, sizeof(otherProtocols)); }));

    auto* server = ServerContext();
    auto* client = ClientContext(true);
    auto* clientHead = reinterpret_cast<GuestCtxHead*>(client);
    Require(clientHead->method == TLS_client_method_nid_postfix() && clientHead->references == 1 && clientHead->sessionTimeout == 300);
    Require(SSL_CTX_use_PrivateKey_ASN1_nid_postfix(PkeyRsa, server, KeyDer, sizeof(KeyDer)) == 0 && ERR_get_error_nid_postfix() == 0x140AF00D);
    static const unsigned char wrongKey[] = {0x30, 0x03, 0x02, 0x01, 0x00};
    Require(SSL_CTX_use_PrivateKey_ASN1_nid_postfix(PkeyEc, server, wrongKey, sizeof(wrongKey)) == 0 && ERR_get_error_nid_postfix() == 0x140AF00D);
    Require(Throws([&] { SSL_CTX_use_PrivateKey_ASN1_nid_postfix(116, server, KeyDer, sizeof(KeyDer)); }));
    Require(SSL_CTX_check_private_key_nid_postfix(client) == 0 && ERR_get_error_nid_postfix() == 0x140A80B1);
    Require(SSL_CTX_load_verify_mem_nid_postfix(client, const_cast<char*>("no certificate here"), 19) == 0);

    Require(SSL_CTX_set_cipher_list_nid_postfix(client, "nonsense:!ALL") == 0 && ERR_get_error_nid_postfix() == 0x1410D0B9);
    Require(Throws([&] { SSL_CTX_set_cipher_list_nid_postfix(client, "HIGH:!aNULL"); }));
    Require(Throws([&] { SSL_CTX_set_cipher_list_nid_postfix(client, "AES:DEFAULT"); }));
    Require(SSL_CTX_set_cipher_list_nid_postfix(client, "DEFAULT:!AESGCM:!CHACHA20:!CAMELLIA:!SHA1:!SHA256:!SHA384") == 0);
    ERR_clear_error_nid_postfix();
    Require(SSL_CTX_set_cipher_list_nid_postfix(client, "ECDHE-ECDSA-AES128-GCM-SHA256:unknown-name") == 1);
    Require(SSL_CTX_set1_groups_list_nid_postfix(client, "P-256:prime256v1:X25519") == 1);
    Require(SSL_CTX_set1_groups_list_nid_postfix(client, "P-256:brainpool") == 0);
    static const unsigned char alpn[] = {8, 'h', 't', 't', 'p', '/', '1', '.', '1'};
    Require(SSL_CTX_set_alpn_protos_nid_postfix(client, alpn, sizeof(alpn)) == 0);
    Require(SSL_CTX_set_alpn_protos_nid_postfix(client, truncated, sizeof(truncated)) == 1);
    Require(SSL_CTX_ctrl_nid_postfix(client, CtrlOptions, OptionNoTicket, nullptr) == static_cast<std::int64_t>(OptionNoTicket));
    Require(SSL_CTX_ctrl_nid_postfix(client, CtrlOptions, 0, nullptr) == static_cast<std::int64_t>(OptionNoTicket));
    Require(SSL_CTX_ctrl_nid_postfix(client, CtrlClearOptions, OptionNoTicket, nullptr) == 0);
    Require(SSL_CTX_ctrl_nid_postfix(client, CtrlMode, 0x4, nullptr) == 0x4 && SSL_CTX_ctrl_nid_postfix(client, CtrlSetEcdhAuto, 1, nullptr) == 1);
    Require(SSL_CTX_ctrl_nid_postfix(server, CtrlGetSessCacheMode, 0, nullptr) == 2);
    Require(SSL_CTX_ctrl_nid_postfix(client, CtrlSetSessCacheMode, 1, nullptr) == 2 && SSL_CTX_ctrl_nid_postfix(client, CtrlGetSessCacheMode, 0, nullptr) == 1);
    Require(SSL_CTX_ctrl_nid_postfix(client, CtrlSetMinProtoVersion, 0x0301, nullptr) == 1 && SSL_CTX_ctrl_nid_postfix(client, CtrlSetMaxProtoVersion, 0x0999, nullptr) == 0);
    Require(Throws([&] { SSL_CTX_ctrl_nid_postfix(client, 999, 0, nullptr); }));
    static const unsigned char longContext[33] {};
    Require(SSL_CTX_set_session_id_context_nid_postfix(server, longContext, 33) == 0 && ERR_get_error_nid_postfix() == 0x140DB111);
    Require(SSL_CTX_set_session_id_context_nid_postfix(server, reinterpret_cast<const unsigned char*>("test"), 4) == 1);
    const int ctxIndex = SSL_CTX_get_ex_new_index_nid_postfix(0, nullptr, nullptr, nullptr, nullptr);
    Require(ctxIndex > 0 && SSL_CTX_set_ex_data_nid_postfix(client, ctxIndex, &newSessions) == 1);
    Require(SSL_CTX_get_ex_data_nid_postfix(client, ctxIndex) == &newSessions && SSL_CTX_get_ex_data_nid_postfix(client, ctxIndex + 1) == nullptr);
    SSL_CTX_set_default_passwd_cb_userdata_nid_postfix(client, &newSessions);
    SSL_CTX_sess_set_new_cb_nid_postfix(client, CountSession);
    SSL_CTX_set_info_callback_nid_postfix(client, Info);
    SSL_CTX_set_next_proto_select_cb_nid_postfix(client, SelectNextProto, &newSessions);
    SSL_CTX_set_alpn_select_cb_nid_postfix(server, SelectAlpn, &servernameCalls);
    Require(Throws([] { SSL_CTX_set_info_callback_nid_postfix(nullptr, Info); }));
    Require(Throws([] { SSL_CTX_set_next_proto_select_cb_nid_postfix(nullptr, SelectNextProto, nullptr); }));
    Require(Throws([] { SSL_CTX_set_alpn_select_cb_nid_postfix(nullptr, SelectAlpn, nullptr); }));
    Require(SSL_CTX_ctrl_nid_postfix(server, CtrlSetTlsextServernameArg, 0, &servernameCalls) == 1);
    Require(SSL_CTX_callback_ctrl_nid_postfix(server, CtrlSetTlsextServernameCb, reinterpret_cast<void (*)()>(Servername)) == 1);
    Require(SSL_CTX_get_verify_mode_nid_postfix(client) == 0);
    SSL_CTX_set_verify_nid_postfix(client, VerifyPeer, nullptr);
    Require(SSL_CTX_get_verify_mode_nid_postfix(client) == VerifyPeer);

    {
        Pair pair;
        auto* c = SSL_new_nid_postfix(client);
        auto* s = SSL_new_nid_postfix(server);
        Require(c && s && clientHead->references == 2);
        Require(reinterpret_cast<GuestSslHead*>(s)->server == 1 && reinterpret_cast<GuestSslHead*>(c)->server == 0);
        Require(SSL_set_fd_nid_postfix(c, pair.fds[0]) == 1 && SSL_set_fd_nid_postfix(s, pair.fds[1]) == 1);
        Require(SSL_get_fd_nid_postfix(c) == pair.fds[0] && SSL_get_rbio_nid_postfix(c) == SSL_get_wbio_nid_postfix(c));
        Require(BIO_ctrl_nid_postfix(SSL_get_rbio_nid_postfix(s), BioGetFd, 0, nullptr) == pair.fds[1]);
        Require(SSL_ctrl_nid_postfix(c, CtrlSetTlsextHostname, 0, const_cast<char*>("localhost")) == 1);
        Require(std::strcmp(SSL_get_servername_nid_postfix(c, 0), "localhost") == 0);
        Require(SSL_get_current_cipher_nid_postfix(c) == nullptr && Throws([&] { SSL_version_nid_postfix(c); }));
        const int sslIndex = SSL_get_ex_new_index_nid_postfix(0, nullptr, nullptr, nullptr, nullptr);
        Require(sslIndex > 0 && SSL_set_ex_data_nid_postfix(c, sslIndex, &pair) == 1 && SSL_get_ex_data_nid_postfix(c, sslIndex) == &pair);
        Require(SSL_get_SSL_CTX_nid_postfix(c) == client);
        Require(Handshake(c, s));
        Require(servernameCalls == 1 && newSessions == 1);
        Require(SSL_version_nid_postfix(c) == 0x0303 && SSL_version_nid_postfix(s) == 0x0303);
        Require(SSL_get_verify_result_nid_postfix(c) == 0 && SSL_get_verify_result_nid_postfix(s) == 0);
        const auto* cipher = SSL_get_current_cipher_nid_postfix(c);
        Require(cipher && std::strcmp(SSL_CIPHER_get_name_nid_postfix(cipher), "ECDHE-ECDSA-AES128-GCM-SHA256") == 0);
        int algorithmBits = 0;
        Require(SSL_CIPHER_get_bits_nid_postfix(cipher, &algorithmBits) == 128 && algorithmBits == 128);
        Require(std::strcmp(SSL_CIPHER_get_version_nid_postfix(cipher), "TLSv1/SSLv3") == 0);
        Require(SSL_get_current_cipher_nid_postfix(s) == cipher);
        Require(std::strcmp(SSL_get_servername_nid_postfix(s, 0), "localhost") == 0 && SSL_get_servername_nid_postfix(s, 1) == nullptr);
        const unsigned char* protocol = nullptr;
        unsigned int protocolLength = 7;
        SSL_get0_alpn_selected_nid_postfix(c, &protocol, &protocolLength);
        Require(protocol == nullptr && protocolLength == 0);
        auto* peer = SSL_get_peer_certificate_nid_postfix(c);
        Require(peer != nullptr);
        char commonName[32]{};
        Require(X509_NAME_get_text_by_NID_nid_postfix(X509_get_subject_name_nid_postfix(peer), 13, commonName, sizeof(commonName)) == 9);
        Require(std::strcmp(commonName, "localhost") == 0);
        X509_free_nid_postfix(peer);
        Require(SSL_get_peer_certificate_nid_postfix(s) == nullptr && SSL_get_peer_cert_chain_nid_postfix(s) == nullptr);
        Require(sk_num_nid_postfix(SSL_get_peer_cert_chain_nid_postfix(c)) == 1);
        Require(SSL_get_certificate_nid_postfix(s) != nullptr && SSL_get_certificate_nid_postfix(c) == nullptr);
        Require(SSL_check_private_key_nid_postfix(s) == 1);
        auto* session = SSL_get_session_nid_postfix(c);
        Require(session && session->sessionIdLength == 32 && session->masterKeyLength == 48 && session->sslVersion == 0x0303);
        Require(session->cipher == cipher && session->cipherId == 0x0300C02B && session->peer != nullptr);
        Require(std::strcmp(session->tlsextHostname, "localhost") == 0);
        Require(SSL_ctrl_nid_postfix(c, CtrlGetSessionReused, 0, nullptr) == 0);

        Require(SSL_write_nid_postfix(c, "ping-pong", 9) == 9);
        char buffer[64]{};
        int got = -1;
        for (int round = 0; round < 2000 && got < 0; ++round) {
            got = SSL_peek_nid_postfix(s, buffer, 4);
            if (got < 0) Require(SSL_get_error_nid_postfix(s, got) == ErrorWantRead && SSL_want_nid_postfix(s) == 3);
        }
        Require(got == 4 && std::memcmp(buffer, "ping", 4) == 0 && SSL_pending_nid_postfix(s) == 9);
        Require(SSL_read_nid_postfix(s, buffer, sizeof(buffer)) == 9 && std::memcmp(buffer, "ping-pong", 9) == 0);
        Require(SSL_pending_nid_postfix(s) == 0);
        got = SSL_read_nid_postfix(s, buffer, sizeof(buffer));
        Require(got == -1 && SSL_get_error_nid_postfix(s, got) == ErrorWantRead);

        std::vector<unsigned char> big(50000);
        for (std::size_t index = 0; index < big.size(); ++index) big[index] = static_cast<unsigned char>(index * 7);
        std::vector<unsigned char> received;
        int sent = -1;
        unsigned char chunk[4096];
        for (int round = 0; round < 20000 && sent < 0; ++round) {
            sent = SSL_write_nid_postfix(s, big.data(), static_cast<int>(big.size()));
            if (sent > 0) break;
            const int error = SSL_get_error_nid_postfix(s, sent);
            Require(error == 3 || error == ErrorWantRead);
            const int got = SSL_read_nid_postfix(c, chunk, sizeof(chunk));
            if (got > 0) received.insert(received.end(), chunk, chunk + got);
        }
        Require(sent == static_cast<int>(big.size()));
        Require(ReadAll(c, nullptr, received, big.size()) == static_cast<int>(big.size()) && received == big);

        auto* filter = BIO_new_nid_postfix(BIO_f_ssl_nid_postfix());
        Require(filter && BIO_ctrl_nid_postfix(filter, BioSetSsl, 0, c) == 1);
        GuestSsl* inner = nullptr;
        Require(BIO_ctrl_nid_postfix(filter, BioGetSsl, 0, &inner) == 1 && inner == c);
        Require(BIO_write_nid_postfix(filter, "via bio", 7) == 7 && BIO_ctrl_nid_postfix(filter, BioCtrlPending, 0, nullptr) == 0);
        Require(Throws([&] { BIO_ctrl_nid_postfix(filter, 999, 0, nullptr); }));
        Require(BIO_free_nid_postfix(filter) == 1);
        received.clear();
        Require(ReadAll(s, nullptr, received, 7) == 7 && std::memcmp(received.data(), "via bio", 7) == 0);

        Require(SSL_shutdown_nid_postfix(c) == 0 && SSL_get_shutdown_nid_postfix(c) == 1);
        got = -1;
        for (int round = 0; round < 2000 && got < 0; ++round) {
            got = SSL_read_nid_postfix(s, buffer, sizeof(buffer));
            if (got < 0) Require(SSL_get_error_nid_postfix(s, got) == ErrorWantRead);
        }
        Require(got == 0 && SSL_get_error_nid_postfix(s, got) == ErrorZeroReturn && SSL_get_shutdown_nid_postfix(s) == 2);
        Require(SSL_shutdown_nid_postfix(s) == 1 && SSL_get_shutdown_nid_postfix(s) == 3);
        int closed = -1;
        for (int round = 0; round < 2000 && closed < 0; ++round) closed = SSL_shutdown_nid_postfix(c);
        Require(closed == 1 && SSL_get_shutdown_nid_postfix(c) == 3);
        Require(SSL_write_nid_postfix(c, "x", 1) == -1 && SSL_get_error_nid_postfix(c, -1) == ErrorSsl && ERR_get_error_nid_postfix() == 0x1409E0CF);

        auto* saved = SSL_get1_session_nid_postfix(c);
        Require(saved == session && saved->references == 2);
        SSL_free_nid_postfix(c);
        SSL_free_nid_postfix(s);
        Require(saved->references == 1 && clientHead->references == 1);

        Pair second;
        auto* c2 = SSL_new_nid_postfix(client);
        auto* s2 = SSL_new_nid_postfix(server);
        Require(SSL_set_session_nid_postfix(c2, saved) == 1 && saved->references == 2);
        Require(SSL_set_fd_nid_postfix(c2, second.fds[0]) == 1 && SSL_set_fd_nid_postfix(s2, second.fds[1]) == 1);
        Require(SSL_ctrl_nid_postfix(c2, CtrlSetTlsextHostname, 0, const_cast<char*>("localhost")) == 1);
        Require(Handshake(c2, s2));
        Require(SSL_ctrl_nid_postfix(c2, CtrlGetSessionReused, 0, nullptr) == 1 && newSessions == 1 && servernameCalls == 2);
        Require(Throws([&] { SSL_ctrl_nid_postfix(s2, CtrlGetSessionReused, 0, nullptr); }));
        Require(SSL_get_session_nid_postfix(c2) != saved && saved->references == 1);
        SSL_SESSION_free_nid_postfix(saved);
        SSL_free_nid_postfix(c2);
        SSL_free_nid_postfix(s2);
    }

    {
        auto* untrusting = ClientContext(false);
        SSL_CTX_set_verify_nid_postfix(untrusting, VerifyPeer, nullptr);
        Pair pair;
        auto* c = SSL_new_nid_postfix(untrusting);
        auto* s = SSL_new_nid_postfix(server);
        Require(SSL_set_fd_nid_postfix(c, pair.fds[0]) == 1 && SSL_set_fd_nid_postfix(s, pair.fds[1]) == 1);
        int clientResult = -1;
        for (int round = 0; round < 2000; ++round) {
            clientResult = SSL_connect_nid_postfix(c);
            if (clientResult != -1 || SSL_get_error_nid_postfix(c, clientResult) != ErrorWantRead) break;
            const int serverResult = SSL_accept_nid_postfix(s);
            Require(serverResult == -1 && SSL_get_error_nid_postfix(s, serverResult) == ErrorWantRead);
        }
        Require(clientResult == -1 && SSL_get_error_nid_postfix(c, clientResult) == ErrorSsl && ERR_get_error_nid_postfix() == 0x14084086);
        Require(SSL_get_verify_result_nid_postfix(c) == 18);
        int serverResult = -1;
        for (int round = 0; round < 2000; ++round) {
            serverResult = SSL_accept_nid_postfix(s);
            if (serverResult != -1 || SSL_get_error_nid_postfix(s, serverResult) != ErrorWantRead) break;
        }
        Require(serverResult == -1 && SSL_get_error_nid_postfix(s, serverResult) == ErrorSsl && ERR_get_error_nid_postfix() == 0x14080418);
        SSL_free_nid_postfix(c);
        SSL_free_nid_postfix(s);

        SSL_CTX_set_verify_nid_postfix(untrusting, 0, nullptr);
        auto* bioClient = SSL_new_nid_postfix(untrusting);
        auto* bioServer = SSL_new_nid_postfix(server);
        auto* clientIn = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
        auto* clientOut = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
        auto* serverIn = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
        auto* serverOut = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
        SSL_set_bio_nid_postfix(bioClient, clientIn, clientOut);
        SSL_set_bio_nid_postfix(bioServer, serverIn, serverOut);
        Require(SSL_get_fd_nid_postfix(bioClient) == -1);
        SSL_set_connect_state_nid_postfix(bioClient);
        int clientDone = 0;
        int serverDone = 0;
        for (int round = 0; round < 100 && (clientDone != 1 || serverDone != 1); ++round) {
            if (clientDone != 1) {
                clientDone = SSL_connect_nid_postfix(bioClient);
                Require(clientDone == 1 || SSL_get_error_nid_postfix(bioClient, clientDone) == ErrorWantRead);
            }
            Shuttle(clientOut, serverIn);
            if (serverDone != 1) {
                serverDone = SSL_accept_nid_postfix(bioServer);
                Require(serverDone == 1 || SSL_get_error_nid_postfix(bioServer, serverDone) == ErrorWantRead);
            }
            Shuttle(serverOut, clientIn);
        }
        Require(clientDone == 1 && serverDone == 1 && SSL_get_verify_result_nid_postfix(bioClient) == 18);
        Require(SSL_write_nid_postfix(bioServer, "memory", 6) == 6 && Shuttle(serverOut, clientIn));
        char text[16]{};
        Require(SSL_read_nid_postfix(bioClient, text, sizeof(text)) == 6 && std::memcmp(text, "memory", 6) == 0);
        SSL_CTX_set_quiet_shutdown_nid_postfix(untrusting, 1);
        auto* quiet = SSL_new_nid_postfix(untrusting);
        Require(SSL_shutdown_nid_postfix(quiet) == 1 && SSL_get_shutdown_nid_postfix(quiet) == 3);
        SSL_free_nid_postfix(quiet);
        SSL_free_nid_postfix(bioClient);
        SSL_free_nid_postfix(bioServer);
        SSL_CTX_free_nid_postfix(untrusting);
    }

    {
        auto* noTls12 = ClientContext(true);
        Require(SSL_CTX_ctrl_nid_postfix(noTls12, CtrlOptions, OptionNoTlsv1_2, nullptr) == static_cast<std::int64_t>(OptionNoTlsv1_2));
        Pair pair;
        auto* c = SSL_new_nid_postfix(noTls12);
        Require(SSL_set_fd_nid_postfix(c, pair.fds[0]) == 1);
        Require(SSL_connect_nid_postfix(c) == -1 && SSL_get_error_nid_postfix(c, -1) == ErrorSsl && ERR_get_error_nid_postfix() == 0x140840BF);
        SSL_free_nid_postfix(c);
        SSL_CTX_free_nid_postfix(noTls12);
        auto* unset = SSL_new_nid_postfix(client);
        char text[4];
        Require(Throws([&] { SSL_read_nid_postfix(unset, text, sizeof(text)); }));
        Require(Throws([&] { SSL_accept_nid_postfix(unset); }));
        SSL_free_nid_postfix(unset);
    }

    {
        auto* swapped = ServerContext();
        switchTarget = swapped;
        Pair pair;
        auto* c = SSL_new_nid_postfix(client);
        auto* s = SSL_new_nid_postfix(server);
        Require(SSL_set_fd_nid_postfix(c, pair.fds[0]) == 1 && SSL_set_fd_nid_postfix(s, pair.fds[1]) == 1);
        Require(SSL_ctrl_nid_postfix(c, CtrlSetTlsextHostname, 0, const_cast<char*>("localhost")) == 1);
        Require(Handshake(c, s) && SSL_get_SSL_CTX_nid_postfix(s) == swapped && servernameCalls == 3);
        SSL_free_nid_postfix(c);
        SSL_free_nid_postfix(s);
        switchTarget = nullptr;
        SSL_CTX_free_nid_postfix(swapped);
    }

    if (argc > 1) {
        const std::string base = argv[1];
        const std::string certificatePath = base + ".crt";
        const std::string keyPath = base + ".key";
        const std::string derPath = base + ".der";
        std::ofstream(certificatePath, std::ios::binary) << "leading text\n" << CertificatePem << CertificatePem;
        std::ofstream(keyPath, std::ios::binary) << KeyPem;
        std::ofstream(derPath, std::ios::binary).write(reinterpret_cast<const char*>(CertificateDer), sizeof(CertificateDer));
        auto* files = SSL_CTX_new_nid_postfix(SSLv23_server_method_nid_postfix());
        Require(SSL_CTX_use_certificate_file_nid_postfix(files, certificatePath.c_str(), FiletypePem) == 1);
        Require(SSL_CTX_use_certificate_file_nid_postfix(files, derPath.c_str(), FiletypeAsn1) == 1);
        Require(SSL_CTX_use_PrivateKey_file_nid_postfix(files, keyPath.c_str(), FiletypePem) == 1);
        Require(SSL_CTX_check_private_key_nid_postfix(files) == 1);
        Require(SSL_CTX_use_certificate_chain_file_nid_postfix(files, certificatePath.c_str()) == 1);
        Require(Throws([&] { SSL_CTX_use_certificate_chain_file_nid_postfix(files, certificatePath.c_str()); }));
        Require(SSL_CTX_use_certificate_file_nid_postfix(files, (base + ".missing").c_str(), FiletypePem) == 0 && ERR_get_error_nid_postfix() == 0x140AD002);
        Require(SSL_CTX_use_PrivateKey_file_nid_postfix(files, derPath.c_str(), FiletypePem) == 0 && ERR_get_error_nid_postfix() == 0x140B0009);
        Require(SSL_CTX_load_verify_locations_nid_postfix(files, certificatePath.c_str(), nullptr) == 1);
        Require(SSL_CTX_load_verify_locations_nid_postfix(files, nullptr, nullptr) == 0);
        Require(Throws([&] { SSL_CTX_load_verify_locations_nid_postfix(files, nullptr, "certs"); }));
        auto* sockets = BIO_new_socket_nid_postfix(7, 0);
        int descriptor = -1;
        Require(sockets && BIO_ctrl_nid_postfix(sockets, BioGetFd, 0, &descriptor) == 7 && descriptor == 7);
        Require(BIO_free_nid_postfix(sockets) == 1);
        SSL_CTX_free_nid_postfix(files);
        std::remove(certificatePath.c_str());
        std::remove(keyPath.c_str());
        std::remove(derPath.c_str());
    }

    SSL_CTX_free_nid_postfix(client);
    SSL_CTX_free_nid_postfix(server);
    return 0;
}
