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

struct GuestCtxHead {
    const GuestMethod* method;
    void* cipherList;
    void* certStore;
    std::int64_t sessionTimeout;
    int references;
};

struct GuestSessionHead {
    unsigned char fields[160];
    int references;
};

extern "C" {
int APS5_VABI SSL_CTX_up_ref_nid_postfix(GuestCtx*);
int APS5_VABI SSL_CTX_set_min_proto_version_nid_postfix(GuestCtx*, std::uint16_t);
int APS5_VABI SSL_CTX_set_max_proto_version_nid_postfix(GuestCtx*, std::uint16_t);
int APS5_VABI SSL_CTX_add0_chain_cert_nid_postfix(GuestCtx*, GuestX509*);
int APS5_VABI SSL_CTX_clear_chain_certs_nid_postfix(GuestCtx*);
int APS5_VABI SSL_up_ref_nid_postfix(GuestSsl*);
int APS5_VABI SSL_SESSION_up_ref_nid_postfix(GuestSessionHead*);
std::size_t APS5_VABI SSL_get_client_random_nid_postfix(const GuestSsl*, unsigned char*, std::size_t);

const GuestMethod* APS5_VABI SSLv23_server_method_nid_postfix();
const GuestMethod* APS5_VABI TLS_client_method_nid_postfix();
GuestCtx* APS5_VABI SSL_CTX_new_nid_postfix(const GuestMethod*);
void APS5_VABI SSL_CTX_free_nid_postfix(GuestCtx*);
int APS5_VABI SSL_CTX_use_certificate_ASN1_nid_postfix(GuestCtx*, int, const unsigned char*);
int APS5_VABI SSL_CTX_use_PrivateKey_ASN1_nid_postfix(int, GuestCtx*, const unsigned char*, std::int64_t);
GuestSsl* APS5_VABI SSL_new_nid_postfix(GuestCtx*);
void APS5_VABI SSL_free_nid_postfix(GuestSsl*);
void APS5_VABI SSL_set_bio_nid_postfix(GuestSsl*, GuestBio*, GuestBio*);
int APS5_VABI SSL_connect_nid_postfix(GuestSsl*);
int APS5_VABI SSL_accept_nid_postfix(GuestSsl*);
int APS5_VABI SSL_get_error_nid_postfix(const GuestSsl*, int);
GuestSessionHead* APS5_VABI SSL_get_session_nid_postfix(const GuestSsl*);
void APS5_VABI SSL_SESSION_free_nid_postfix(GuestSessionHead*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
const GuestBioMethod* APS5_VABI BIO_s_mem_nid_postfix();
GuestBio* APS5_VABI BIO_new_nid_postfix(const GuestBioMethod*);
int APS5_VABI BIO_write_nid_postfix(GuestBio*, const void*, int);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
GuestX509* APS5_VABI d2i_X509_nid_postfix(GuestX509**, const unsigned char**, std::int64_t);
}

namespace {

constexpr int ErrorSsl = 1, ErrorWantRead = 2;
constexpr int PkeyEc = 408;

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


void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "LibreSSL 3 SSL check failed at line %u\n", location.line());
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

void Move(GuestBio* from, GuestBio* to) {
    char* data = nullptr;
    const auto length = BIO_ctrl_nid_postfix(from, 3, 0, &data);
    if (length <= 0) return;
    const std::string copy(data, static_cast<std::size_t>(length));
    BIO_ctrl_nid_postfix(from, 1, 0, nullptr);
    Require(BIO_write_nid_postfix(to, copy.data(), static_cast<int>(copy.size())) == static_cast<int>(copy.size()));
}

bool Handshake(GuestSsl* client, GuestSsl* server, GuestBio* clientOut, GuestBio* serverIn, GuestBio* serverOut, GuestBio* clientIn) {
    int clientDone = 0;
    int serverDone = 0;
    for (int round = 0; round < 100 && (clientDone != 1 || serverDone != 1); ++round) {
        if (clientDone != 1) {
            clientDone = SSL_connect_nid_postfix(client);
            if (clientDone != 1 && SSL_get_error_nid_postfix(client, clientDone) != ErrorWantRead) return false;
        }
        Move(clientOut, serverIn);
        if (serverDone != 1) {
            serverDone = SSL_accept_nid_postfix(server);
            if (serverDone != 1 && SSL_get_error_nid_postfix(server, serverDone) != ErrorWantRead) return false;
        }
        Move(serverOut, clientIn);
    }
    return clientDone == 1 && serverDone == 1;
}

}

int main() {
    void* const api[10] {};
    ApplicationHeapRegister_nid_no_patch(api);

    auto* server = SSL_CTX_new_nid_postfix(SSLv23_server_method_nid_postfix());
    auto* client = SSL_CTX_new_nid_postfix(TLS_client_method_nid_postfix());
    Require(server && client);
    Require(SSL_CTX_use_certificate_ASN1_nid_postfix(server, sizeof(CertificateDer), CertificateDer) == 1);
    Require(SSL_CTX_use_PrivateKey_ASN1_nid_postfix(PkeyEc, server, KeyDer, sizeof(KeyDer)) == 1);

    auto* head = reinterpret_cast<GuestCtxHead*>(client);
    Require(SSL_CTX_up_ref_nid_postfix(client) == 1 && head->references == 2);
    SSL_CTX_free_nid_postfix(client);
    Require(head->references == 1);
    Require(Throws([] { SSL_CTX_up_ref_nid_postfix(nullptr); }));

    Require(SSL_CTX_set_min_proto_version_nid_postfix(client, 0x0303) == 1);
    Require(SSL_CTX_set_max_proto_version_nid_postfix(client, 0x0304) == 1);
    Require(SSL_CTX_set_max_proto_version_nid_postfix(client, 0x0300) == 0);
    Require(SSL_CTX_set_min_proto_version_nid_postfix(client, 0) == 1 && SSL_CTX_set_max_proto_version_nid_postfix(client, 0) == 1);

    const unsigned char* cursor = CertificateDer;
    auto* extra = d2i_X509_nid_postfix(nullptr, &cursor, sizeof(CertificateDer));
    Require(extra != nullptr && SSL_CTX_add0_chain_cert_nid_postfix(server, extra) == 1);
    Require(SSL_CTX_clear_chain_certs_nid_postfix(server) == 1);

    auto* c = SSL_new_nid_postfix(client);
    auto* s = SSL_new_nid_postfix(server);
    Require(SSL_up_ref_nid_postfix(c) == 1);
    SSL_free_nid_postfix(c);
    Require(Throws([&] { SSL_get_client_random_nid_postfix(c, nullptr, 0); }));
    auto* clientIn = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    auto* clientOut = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    auto* serverIn = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    auto* serverOut = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    SSL_set_bio_nid_postfix(c, clientIn, clientOut);
    SSL_set_bio_nid_postfix(s, serverIn, serverOut);
    Require(Handshake(c, s, clientOut, serverIn, serverOut, clientIn));

    unsigned char clientRandom[32]{};
    unsigned char serverView[40]{};
    Require(SSL_get_client_random_nid_postfix(c, nullptr, 0) == 32);
    Require(SSL_get_client_random_nid_postfix(c, clientRandom, sizeof(clientRandom)) == 32);
    Require(SSL_get_client_random_nid_postfix(s, serverView, sizeof(serverView)) == 32 && std::memcmp(clientRandom, serverView, 32) == 0);
    unsigned char prefix[8]{};
    Require(SSL_get_client_random_nid_postfix(c, prefix, sizeof(prefix)) == 8 && std::memcmp(prefix, clientRandom, 8) == 0);

    auto* session = SSL_get_session_nid_postfix(c);
    Require(session && session->references == 1);
    Require(SSL_SESSION_up_ref_nid_postfix(session) == 1 && session->references == 2);
    SSL_free_nid_postfix(c);
    Require(session->references == 1);
    SSL_SESSION_free_nid_postfix(session);
    Require(Throws([] { SSL_SESSION_up_ref_nid_postfix(nullptr); }));
    SSL_free_nid_postfix(s);

    Require(SSL_CTX_set_min_proto_version_nid_postfix(client, 0x0304) == 1);
    auto* tooNew = SSL_new_nid_postfix(client);
    auto* in = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    auto* out = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    SSL_set_bio_nid_postfix(tooNew, in, out);
    Require(SSL_connect_nid_postfix(tooNew) == -1 && SSL_get_error_nid_postfix(tooNew, -1) == ErrorSsl && ERR_get_error_nid_postfix() == 0x140840BF);
    SSL_free_nid_postfix(tooNew);

    SSL_CTX_free_nid_postfix(client);
    SSL_CTX_free_nid_postfix(server);
    return 0;
}
