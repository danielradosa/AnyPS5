#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <source_location>
#include <string>

struct GuestBio;
struct GuestBioMethod;
struct GuestSsl;
struct GuestCtx;
struct GuestMethod;
struct GuestX509;
struct GuestStore;
struct GuestStoreCtx;
struct GuestVerifyParam;

struct GuestCtxHead {
    const GuestMethod* method;
    void* cipherList;
    GuestStore* certStore;
};

struct GuestSslHead {
    int version;
    const GuestMethod* method;
    GuestBio* rbio;
    GuestBio* wbio;
    GuestBio* bbio;
    int server;
    void* s3;
    void* d1;
    GuestVerifyParam* param;
};

using GuestVerifyCallback = int (APS5_VABI*)(int, GuestStoreCtx*);
using GuestCertVerifyCallback = int (APS5_VABI*)(GuestStoreCtx*, void*);

extern "C" {
const GuestMethod* APS5_VABI SSLv23_server_method_nid_postfix();
const GuestMethod* APS5_VABI TLS_client_method_nid_postfix();
GuestCtx* APS5_VABI SSL_CTX_new_nid_postfix(const GuestMethod*);
void APS5_VABI SSL_CTX_free_nid_postfix(GuestCtx*);
void APS5_VABI SSL_CTX_set_verify_nid_postfix(GuestCtx*, int, GuestVerifyCallback);
int APS5_VABI SSL_CTX_use_certificate_ASN1_nid_postfix(GuestCtx*, int, const unsigned char*);
int APS5_VABI SSL_CTX_use_PrivateKey_ASN1_nid_postfix(int, GuestCtx*, const unsigned char*, std::int64_t);
int APS5_VABI SSL_CTX_check_private_key_nid_postfix(const GuestCtx*);
int APS5_VABI SSL_CTX_load_verify_mem_nid_postfix(GuestCtx*, void*, int);
GuestStore* APS5_VABI SSL_CTX_get_cert_store_nid_postfix(const GuestCtx*);
void APS5_VABI SSL_CTX_set_cert_store_nid_postfix(GuestCtx*, GuestStore*);
void APS5_VABI SSL_CTX_set_cert_verify_callback_nid_postfix(GuestCtx*, GuestCertVerifyCallback, void*);
GuestSsl* APS5_VABI SSL_new_nid_postfix(GuestCtx*);
void APS5_VABI SSL_free_nid_postfix(GuestSsl*);
void APS5_VABI SSL_set_bio_nid_postfix(GuestSsl*, GuestBio*, GuestBio*);
int APS5_VABI SSL_connect_nid_postfix(GuestSsl*);
int APS5_VABI SSL_accept_nid_postfix(GuestSsl*);
int APS5_VABI SSL_get_error_nid_postfix(const GuestSsl*, int);
std::int64_t APS5_VABI SSL_get_verify_result_nid_postfix(const GuestSsl*);
GuestX509* APS5_VABI SSL_get_peer_certificate_nid_postfix(const GuestSsl*);
GuestVerifyParam* APS5_VABI SSL_get0_param_nid_postfix(GuestSsl*);
int APS5_VABI SSL_get_ex_data_X509_STORE_CTX_idx_nid_postfix();
const GuestBioMethod* APS5_VABI BIO_s_mem_nid_postfix();
GuestBio* APS5_VABI BIO_new_nid_postfix(const GuestBioMethod*);
int APS5_VABI BIO_write_nid_postfix(GuestBio*, const void*, int);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
GuestX509* APS5_VABI d2i_X509_nid_postfix(GuestX509**, const unsigned char**, std::int64_t);
void APS5_VABI X509_free_nid_postfix(GuestX509*);
GuestStore* APS5_VABI X509_STORE_new_nid_postfix();
int APS5_VABI X509_STORE_add_cert_nid_postfix(GuestStore*, GuestX509*);
void* APS5_VABI X509_STORE_CTX_get_ex_data_nid_postfix(const GuestStoreCtx*, int);
int APS5_VABI X509_STORE_CTX_get_error_nid_postfix(const GuestStoreCtx*);
void APS5_VABI X509_STORE_CTX_set_error_nid_postfix(GuestStoreCtx*, int);
int APS5_VABI X509_STORE_CTX_get_error_depth_nid_postfix(const GuestStoreCtx*);
GuestX509* APS5_VABI X509_STORE_CTX_get_current_cert_nid_postfix(const GuestStoreCtx*);
int APS5_VABI X509_verify_cert_nid_postfix(GuestStoreCtx*);
int APS5_VABI X509_VERIFY_PARAM_set1_host_nid_postfix(GuestVerifyParam*, const char*, std::size_t);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
}

namespace {

constexpr int ErrorWantRead = 2;
constexpr int VerifyPeer = 1;
constexpr int PkeyEc = 408;
constexpr int BioCtrlReset = 1, BioCtrlInfo = 3;
constexpr std::uint64_t ClientVerifyFailed = 0x14084086, ServerUnknownCa = 0x14080418, ServerBadCertificate = 0x14080412;
constexpr std::uint64_t ServerAccessDenied = 0x14080419;

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

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "LibreSSL TLS verification check failed at line %u\n", location.line());
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

GuestX509* Certificate() {
    const unsigned char* cursor = CertificateDer;
    auto* x509 = d2i_X509_nid_postfix(nullptr, &cursor, sizeof(CertificateDer));
    Require(x509 != nullptr);
    return x509;
}

GuestCtx* ServerContext() {
    auto* ctx = SSL_CTX_new_nid_postfix(SSLv23_server_method_nid_postfix());
    Require(ctx != nullptr && SSL_CTX_use_certificate_ASN1_nid_postfix(ctx, sizeof(CertificateDer), CertificateDer) == 1);
    Require(SSL_CTX_use_PrivateKey_ASN1_nid_postfix(PkeyEc, ctx, KeyDer, sizeof(KeyDer)) == 1 && SSL_CTX_check_private_key_nid_postfix(ctx) == 1);
    return ctx;
}

GuestCtx* ClientContext(bool trust, int mode) {
    auto* ctx = SSL_CTX_new_nid_postfix(TLS_client_method_nid_postfix());
    Require(ctx != nullptr);
    if (trust) Require(SSL_CTX_load_verify_mem_nid_postfix(ctx, const_cast<char*>(CertificatePem), sizeof(CertificatePem) - 1) == 1);
    SSL_CTX_set_verify_nid_postfix(ctx, mode, nullptr);
    return ctx;
}

void Shuttle(GuestBio* from, GuestBio* to) {
    char* data = nullptr;
    const auto length = BIO_ctrl_nid_postfix(from, BioCtrlInfo, 0, &data);
    if (length <= 0) return;
    const std::string bytes(data, static_cast<std::size_t>(length));
    BIO_ctrl_nid_postfix(from, BioCtrlReset, 0, nullptr);
    Require(BIO_write_nid_postfix(to, bytes.data(), static_cast<int>(bytes.size())) == static_cast<int>(bytes.size()));
}

struct Link {
    GuestSsl* client;
    GuestSsl* server;
    GuestBio* clientIn = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    GuestBio* clientOut = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    GuestBio* serverIn = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    GuestBio* serverOut = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    int clientResult = 0;
    int serverResult = 0;
    std::uint64_t clientCode = 0;
    std::uint64_t serverCode = 0;

    Link(GuestCtx* clientCtx, GuestCtx* serverCtx) : client(SSL_new_nid_postfix(clientCtx)), server(SSL_new_nid_postfix(serverCtx)) {
        Require(client && server);
        SSL_set_bio_nid_postfix(client, clientIn, clientOut);
        SSL_set_bio_nid_postfix(server, serverIn, serverOut);
    }

    ~Link() {
        SSL_free_nid_postfix(client);
        SSL_free_nid_postfix(server);
    }

    bool Run() {
        bool clientDone = false;
        bool serverDone = false;
        for (int round = 0; round < 200 && !(clientDone && serverDone); ++round) {
            if (!clientDone) {
                clientResult = SSL_connect_nid_postfix(client);
                const int error = clientResult == 1 ? 0 : SSL_get_error_nid_postfix(client, clientResult);
                if (clientResult == 1 || error != ErrorWantRead) {
                    clientDone = true;
                    clientCode = ERR_get_error_nid_postfix();
                    ERR_clear_error_nid_postfix();
                }
            }
            Shuttle(clientOut, serverIn);
            if (!serverDone) {
                serverResult = SSL_accept_nid_postfix(server);
                if (serverResult == 1 || SSL_get_error_nid_postfix(server, serverResult) != ErrorWantRead) {
                    serverDone = true;
                    serverCode = ERR_get_error_nid_postfix();
                    ERR_clear_error_nid_postfix();
                }
            }
            Shuttle(serverOut, clientIn);
        }
        return clientResult == 1 && serverResult == 1;
    }
};

struct Seen {
    int ok;
    int error;
    int depth;
    bool current;
    GuestSsl* ssl;
};

Seen seen[8];
int seenCount = 0;
int acceptResult = 1;

int APS5_VABI RecordVerify(int ok, GuestStoreCtx* ctx) {
    Require(seenCount < 8);
    auto* ssl = static_cast<GuestSsl*>(X509_STORE_CTX_get_ex_data_nid_postfix(ctx, SSL_get_ex_data_X509_STORE_CTX_idx_nid_postfix()));
    seen[seenCount++] = {ok, X509_STORE_CTX_get_error_nid_postfix(ctx), X509_STORE_CTX_get_error_depth_nid_postfix(ctx),
        X509_STORE_CTX_get_current_cert_nid_postfix(ctx) != nullptr, ssl};
    return ok ? ok : acceptResult;
}

int appCalls = 0;
int appVerdict = 1;
int marker = 0;
GuestSsl* appSsl = nullptr;

int APS5_VABI AppVerify(GuestStoreCtx* ctx, void* arg) {
    ++appCalls;
    Require(arg == &marker);
    appSsl = static_cast<GuestSsl*>(X509_STORE_CTX_get_ex_data_nid_postfix(ctx, SSL_get_ex_data_X509_STORE_CTX_idx_nid_postfix()));
    Require(X509_verify_cert_nid_postfix(ctx) == 1 && X509_STORE_CTX_get_error_nid_postfix(ctx) == 0);
    if (!appVerdict) X509_STORE_CTX_set_error_nid_postfix(ctx, 50);
    return appVerdict;
}

}

int main() {
    void* const api[10]{};
    ApplicationHeapRegister_nid_no_patch(api);
    ERR_clear_error_nid_postfix();
    Require(SSL_get_ex_data_X509_STORE_CTX_idx_nid_postfix() == 0);

    auto* server = ServerContext();
    {
        auto* client = ClientContext(false, VerifyPeer);
        SSL_CTX_set_verify_nid_postfix(client, VerifyPeer, RecordVerify);
        Link link(client, server);
        Require(link.Run() && SSL_get_verify_result_nid_postfix(link.client) == 18);
        Require(seenCount == 2 && seen[0].ok == 0 && seen[0].error == 18 && seen[0].depth == 0 && seen[0].current && seen[0].ssl == link.client);
        Require(seen[1].ok == 1 && seen[1].error == 18 && seen[1].depth == 0 && seen[1].ssl == link.client);
        auto* peer = SSL_get_peer_certificate_nid_postfix(link.client);
        Require(peer != nullptr);
        X509_free_nid_postfix(peer);

        seenCount = 0;
        acceptResult = 0;
        Link refused(client, server);
        Require(!refused.Run() && refused.clientResult == -1 && refused.clientCode == ClientVerifyFailed && refused.serverCode == ServerUnknownCa);
        Require(seenCount == 1 && seen[0].ok == 0 && SSL_get_verify_result_nid_postfix(refused.client) == 18);
        SSL_CTX_free_nid_postfix(client);
    }

    {
        auto* client = ClientContext(true, 0);
        Link mismatch(client, server);
        auto* param = SSL_get0_param_nid_postfix(mismatch.client);
        Require(param && param == reinterpret_cast<GuestSslHead*>(mismatch.client)->param && SSL_get0_param_nid_postfix(mismatch.client) == param);
        Require(X509_VERIFY_PARAM_set1_host_nid_postfix(param, "other.example", 0) == 1);
        Require(mismatch.Run() && SSL_get_verify_result_nid_postfix(mismatch.client) == 62);

        SSL_CTX_set_verify_nid_postfix(client, VerifyPeer, nullptr);
        Link matching(client, server);
        Require(X509_VERIFY_PARAM_set1_host_nid_postfix(SSL_get0_param_nid_postfix(matching.client), "localhost", 0) == 1);
        Require(matching.Run() && SSL_get_verify_result_nid_postfix(matching.client) == 0);

        Link refused(client, server);
        Require(X509_VERIFY_PARAM_set1_host_nid_postfix(SSL_get0_param_nid_postfix(refused.client), "other.example", 0) == 1);
        Require(!refused.Run() && refused.clientCode == ClientVerifyFailed && refused.serverCode == ServerBadCertificate);
        Require(SSL_get_verify_result_nid_postfix(refused.client) == 62);
        SSL_CTX_free_nid_postfix(client);
    }

    {
        auto* client = ClientContext(true, VerifyPeer);
        Link before(client, server);
        SSL_CTX_set_cert_verify_callback_nid_postfix(client, AppVerify, &marker);
        Require(before.Run() && appCalls == 0);
        Link accepted(client, server);
        Require(accepted.Run() && appCalls == 1 && appSsl == accepted.client && SSL_get_verify_result_nid_postfix(accepted.client) == 0);
        appVerdict = 0;
        Link refused(client, server);
        Require(!refused.Run() && appCalls == 2 && refused.clientCode == ClientVerifyFailed && refused.serverCode == ServerAccessDenied);
        Require(SSL_get_verify_result_nid_postfix(refused.client) == 50);
        SSL_CTX_set_cert_verify_callback_nid_postfix(client, nullptr, nullptr);
        Link restored(client, server);
        Require(restored.Run() && appCalls == 2);
        SSL_CTX_free_nid_postfix(client);
    }

    {
        auto* client = ClientContext(true, VerifyPeer);
        auto* store = SSL_CTX_get_cert_store_nid_postfix(client);
        Require(store && store == reinterpret_cast<GuestCtxHead*>(client)->certStore);
        std::string twice = std::string(CertificatePem) + CertificatePem;
        Require(SSL_CTX_load_verify_mem_nid_postfix(client, twice.data(), static_cast<int>(twice.size())) == 1 && ERR_get_error_nid_postfix() == 0);
        auto* certificate = Certificate();
        Require(X509_STORE_add_cert_nid_postfix(store, certificate) == 0 && ERR_get_error_nid_postfix() == 0x0B07C065);
        auto* fresh = X509_STORE_new_nid_postfix();
        SSL_CTX_set_cert_store_nid_postfix(client, fresh);
        SSL_CTX_set_cert_store_nid_postfix(client, fresh);
        Require(SSL_CTX_get_cert_store_nid_postfix(client) == fresh && reinterpret_cast<GuestCtxHead*>(client)->certStore == fresh);
        Link untrusted(client, server);
        Require(!untrusted.Run() && untrusted.clientCode == ClientVerifyFailed && SSL_get_verify_result_nid_postfix(untrusted.client) == 18);
        Require(X509_STORE_add_cert_nid_postfix(fresh, certificate) == 1);
        Link trusted(client, server);
        Require(trusted.Run() && SSL_get_verify_result_nid_postfix(trusted.client) == 0);
        X509_free_nid_postfix(certificate);
        SSL_CTX_set_cert_store_nid_postfix(client, nullptr);
        Require(SSL_CTX_get_cert_store_nid_postfix(client) == nullptr);
        Require(Throws([&] { SSL_CTX_load_verify_mem_nid_postfix(client, const_cast<char*>(CertificatePem), sizeof(CertificatePem) - 1); }));
        SSL_CTX_free_nid_postfix(client);
    }

    SSL_CTX_free_nid_postfix(server);
    return 0;
}
