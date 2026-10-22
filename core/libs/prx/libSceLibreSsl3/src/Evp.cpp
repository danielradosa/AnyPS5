#include "prx/libSceLibreSsl/include/LibreSsl.hpp"
#include "prx/libc/include/General.hpp"

#include <cstring>
#include <stdexcept>

using namespace LibreSsl;

extern "C" {
EvpMdCtx* APS5_VABI EVP_MD_CTX_create_nid_postfix();
void APS5_VABI EVP_MD_CTX_destroy_nid_postfix(EvpMdCtx* ctx);
void APS5_VABI EVP_MD_CTX_init_nid_postfix(EvpMdCtx* ctx);
int APS5_VABI EVP_DigestInit_ex_nid_postfix(EvpMdCtx* ctx, const EvpMd* type, void* impl);
void APS5_VABI EVP_CIPHER_CTX_init_nid_postfix(EvpCipherCtx* ctx);
int APS5_VABI EVP_EncryptInit_ex_nid_postfix(EvpCipherCtx* ctx, const EvpCipher* cipher, void* impl, const unsigned char* key, const unsigned char* iv);

const EvpMd* APS5_VABI EVP_sha1_nid_postfix() {
    return LibreSslSha1_nid_no_patch();
}

const EvpMd* APS5_VABI EVP_sha224_nid_postfix() {
    return LibreSslSha224_nid_no_patch();
}

EvpMdCtx* APS5_VABI EVP_MD_CTX_new_nid_postfix() {
    return EVP_MD_CTX_create_nid_postfix();
}

void APS5_VABI EVP_MD_CTX_free_nid_postfix(EvpMdCtx* ctx) {
    EVP_MD_CTX_destroy_nid_postfix(ctx);
}

int APS5_VABI EVP_DigestInit_nid_postfix(EvpMdCtx* ctx, const EvpMd* type) {
    EVP_MD_CTX_init_nid_postfix(ctx);
    return EVP_DigestInit_ex_nid_postfix(ctx, type, nullptr);
}

int APS5_VABI EVP_EncryptInit_nid_postfix(EvpCipherCtx* ctx, const EvpCipher* cipher, const unsigned char* key, const unsigned char* iv) {
    if (cipher) EVP_CIPHER_CTX_init_nid_postfix(ctx);
    return EVP_EncryptInit_ex_nid_postfix(ctx, cipher, nullptr, key, iv);
}

void APS5_VABI EVP_CIPHER_CTX_set_flags_nid_postfix(EvpCipherCtx* ctx, int flags) {
    if (!ctx) throw std::invalid_argument("EVP_CIPHER_CTX_set_flags: null context");
    ctx->flags |= static_cast<std::uint64_t>(static_cast<std::int64_t>(flags));
}

const EvpCipher* APS5_VABI EVP_aes_128_wrap_nid_postfix() {
    return LibreSslAesWrap_nid_no_patch(128);
}

const EvpCipher* APS5_VABI EVP_aes_192_wrap_nid_postfix() {
    return LibreSslAesWrap_nid_no_patch(192);
}

const EvpCipher* APS5_VABI EVP_aes_256_wrap_nid_postfix() {
    return LibreSslAesWrap_nid_no_patch(256);
}

}
