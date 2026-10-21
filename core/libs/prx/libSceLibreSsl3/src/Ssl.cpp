#include "prx/libSceLibreSsl/include/LibreSsl.hpp"
#include "prx/libc/include/General.hpp"

using namespace LibreSsl;

extern "C" {

int APS5_VABI SSL_CTX_up_ref_nid_postfix(SslCtxObject* ctx) {
    return LibreSslSslCtxUpRef_nid_no_patch(ctx);
}

int APS5_VABI SSL_CTX_set_min_proto_version_nid_postfix(SslCtxObject* ctx, std::uint16_t version) {
    return LibreSslSslCtxSetProtoVersion_nid_no_patch(ctx, version, 0);
}

int APS5_VABI SSL_CTX_set_max_proto_version_nid_postfix(SslCtxObject* ctx, std::uint16_t version) {
    return LibreSslSslCtxSetProtoVersion_nid_no_patch(ctx, version, 1);
}

int APS5_VABI SSL_CTX_add0_chain_cert_nid_postfix(SslCtxObject* ctx, X509Object* x509) {
    return LibreSslSslCtxAdd0ChainCert_nid_no_patch(ctx, x509);
}

int APS5_VABI SSL_CTX_clear_chain_certs_nid_postfix(SslCtxObject* ctx) {
    return LibreSslSslCtxClearChainCerts_nid_no_patch(ctx);
}

int APS5_VABI SSL_up_ref_nid_postfix(SslObject* ssl) {
    return LibreSslSslUpRef_nid_no_patch(ssl);
}

int APS5_VABI SSL_SESSION_up_ref_nid_postfix(SslSessionObject* session) {
    return LibreSslSslSessionUpRef_nid_no_patch(session);
}

std::size_t APS5_VABI SSL_get_client_random_nid_postfix(const SslObject* ssl, unsigned char* out, std::size_t out_len) {
    return LibreSslSslGetClientRandom_nid_no_patch(ssl, out, out_len);
}

}
