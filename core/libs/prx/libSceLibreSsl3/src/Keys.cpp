#include "prx/libSceLibreSsl/include/LibreSsl.hpp"
#include "prx/libc/include/General.hpp"

using namespace LibreSsl;

extern "C" {

int APS5_VABI RSA_up_ref_nid_postfix(Rsa* r) {
    return LibreSslRsaUpRef_nid_no_patch(r);
}

void APS5_VABI RSA_get0_key_nid_postfix(const Rsa* r, const Bignum** n, const Bignum** e, const Bignum** d) {
    LibreSslRsaGet0Key_nid_no_patch(r, n, e, d);
}

int APS5_VABI RSA_check_key_nid_postfix(const Rsa* rsa) {
    return LibreSslRsaCheckKey_nid_no_patch(rsa);
}

int APS5_VABI RSA_padding_add_PKCS1_PSS_mgf1_nid_postfix(Rsa* rsa, unsigned char* EM, const unsigned char* mHash, const EvpMd* Hash, const EvpMd* mgf1Hash, int sLen) {
    return LibreSslRsaPaddingAddPssMgf1_nid_no_patch(rsa, EM, mHash, Hash, mgf1Hash, sLen);
}

int APS5_VABI EC_KEY_up_ref_nid_postfix(EcKey* key) {
    return LibreSslEcKeyUpRef_nid_no_patch(key);
}

EcKey* APS5_VABI d2i_ECParameters_nid_postfix(EcKey** key, const unsigned char** in, std::int64_t len) {
    return LibreSslD2iEcParameters_nid_no_patch(key, in, len);
}

EcKey* APS5_VABI o2i_ECPublicKey_nid_postfix(EcKey** key, const unsigned char** in, std::int64_t len) {
    return LibreSslO2iEcPublicKey_nid_no_patch(key, in, len);
}

int APS5_VABI EVP_DigestSignInit_nid_postfix(EvpMdCtx* ctx, EvpPkeyCtx** pctx, const EvpMd* type, void* e, EvpPkey* pkey) {
    return LibreSslDigestSignInit_nid_no_patch(ctx, pctx, type, e, pkey);
}

int APS5_VABI EVP_DigestSignFinal_nid_postfix(EvpMdCtx* ctx, unsigned char* sigret, std::size_t* siglen) {
    return LibreSslDigestSignFinal_nid_no_patch(ctx, sigret, siglen);
}

int APS5_VABI EVP_SignFinal_nid_postfix(EvpMdCtx* ctx, unsigned char* md, unsigned int* s, EvpPkey* pkey) {
    return LibreSslSignFinal_nid_no_patch(ctx, md, s, pkey);
}

int APS5_VABI EVP_VerifyFinal_nid_postfix(EvpMdCtx* ctx, const unsigned char* sigbuf, unsigned int siglen, EvpPkey* pkey) {
    return LibreSslVerifyFinal_nid_no_patch(ctx, sigbuf, siglen, pkey);
}

EvpPkeyCtx* APS5_VABI EVP_PKEY_CTX_new_nid_postfix(EvpPkey* pkey, void* e) {
    return LibreSslPkeyCtxNew_nid_no_patch(pkey, e);
}

int APS5_VABI EVP_PKEY_CTX_ctrl_nid_postfix(EvpPkeyCtx* ctx, int keytype, int optype, int cmd, int p1, void* p2) {
    return LibreSslPkeyCtxCtrl_nid_no_patch(ctx, keytype, optype, cmd, p1, p2);
}

int APS5_VABI EVP_PKEY_encrypt_init_nid_postfix(EvpPkeyCtx* ctx) {
    return LibreSslPkeyEncryptInit_nid_no_patch(ctx);
}

int APS5_VABI EVP_PKEY_decrypt_init_nid_postfix(EvpPkeyCtx* ctx) {
    return LibreSslPkeyDecryptInit_nid_no_patch(ctx);
}

int APS5_VABI EVP_PKEY_encrypt_nid_postfix(EvpPkeyCtx* ctx, unsigned char* out, std::size_t* outlen, const unsigned char* in, std::size_t inlen) {
    return LibreSslPkeyEncrypt_nid_no_patch(ctx, out, outlen, in, inlen);
}

int APS5_VABI EVP_PKEY_decrypt_nid_postfix(EvpPkeyCtx* ctx, unsigned char* out, std::size_t* outlen, const unsigned char* in, std::size_t inlen) {
    return LibreSslPkeyDecrypt_nid_no_patch(ctx, out, outlen, in, inlen);
}

EvpPkey* APS5_VABI EVP_PKEY_new_mac_key_nid_postfix(int type, void* e, const unsigned char* key, int keylen) {
    return LibreSslPkeyNewMacKey_nid_no_patch(type, e, key, keylen);
}

int APS5_VABI EVP_PKEY_size_nid_postfix(const EvpPkey* pkey) {
    return LibreSslPkeySize_nid_no_patch(pkey);
}

int APS5_VABI EVP_PKEY_up_ref_nid_postfix(EvpPkey* pkey) {
    return LibreSslPkeyUpRef_nid_no_patch(pkey);
}

Rsa* APS5_VABI EVP_PKEY_get0_RSA_nid_postfix(EvpPkey* pkey) {
    return LibreSslPkeyGet0Rsa_nid_no_patch(pkey);
}

void* APS5_VABI EVP_PKEY_get0_DSA_nid_postfix(EvpPkey* pkey) {
    return LibreSslPkeyGet0Other_nid_no_patch(pkey, 1);
}

void* APS5_VABI EVP_PKEY_get0_DH_nid_postfix(EvpPkey* pkey) {
    return LibreSslPkeyGet0Other_nid_no_patch(pkey, 0);
}

int APS5_VABI EVP_PKEY_set1_EC_KEY_nid_postfix(EvpPkey* pkey, EcKey* key) {
    return LibreSslPkeySet1EcKey_nid_no_patch(pkey, key);
}

EvpPkey* APS5_VABI d2i_PrivateKey_bio_nid_postfix(Bio* bp, EvpPkey** a) {
    return LibreSslD2iPrivateKeyBio_nid_no_patch(bp, a);
}

Rsa* APS5_VABI d2i_RSAPublicKey_bio_nid_postfix(Bio* bp, Rsa** a) {
    return LibreSslD2iRsaPublicKeyBio_nid_no_patch(bp, a);
}

Pkcs8* APS5_VABI d2i_PKCS8_PRIV_KEY_INFO_bio_nid_postfix(Bio* bp, Pkcs8** p8) {
    return LibreSslD2iPkcs8Bio_nid_no_patch(bp, p8);
}

void APS5_VABI PKCS8_PRIV_KEY_INFO_free_nid_postfix(Pkcs8* a) {
    LibreSslPkcs8Free_nid_no_patch(a);
}

EvpPkey* APS5_VABI EVP_PKCS82PKEY_nid_postfix(const Pkcs8* p8) {
    return LibreSslPkcs82Pkey_nid_no_patch(p8);
}

}
