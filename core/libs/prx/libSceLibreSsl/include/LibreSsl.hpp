#ifndef CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_LIBRESSL_HPP
#define CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_LIBRESSL_HPP

#include <cstddef>
#include <cstdint>

#include "prx/libc/include/general/VabiMacros.hpp"

namespace LibreSsl {

struct EvpMd;
struct EvpCipher;

struct Bignum {
    std::uint64_t* d;
    int top;
    int dmax;
    int neg;
    int flags;
};

struct EvpMdCtx {
    const EvpMd* digest;
    void* engine;
    std::uint64_t flags;
    void* mdData;
    void* pctx;
    void* update;
};

struct HmacCtx {
    const EvpMd* md;
    EvpMdCtx mdCtx;
    EvpMdCtx iCtx;
    EvpMdCtx oCtx;
    std::uint32_t keyLength;
    unsigned char key[128];
};

struct EvpCipherCtx {
    const EvpCipher* cipher;
    void* engine;
    int encrypt;
    int bufLen;
    unsigned char oiv[16];
    unsigned char iv[16];
    unsigned char buf[32];
    int num;
    void* appData;
    int keyLen;
    std::uint64_t flags;
    void* cipherData;
    int finalUsed;
    int blockMask;
    unsigned char finalBlock[32];
};

static_assert(sizeof(Bignum) == 24 && offsetof(Bignum, top) == 8 && offsetof(Bignum, flags) == 20);
static_assert(sizeof(EvpMdCtx) == 48 && alignof(EvpMdCtx) == 8);
static_assert(sizeof(HmacCtx) == 288 && alignof(HmacCtx) == 8);
static_assert(offsetof(HmacCtx, keyLength) == 152 && offsetof(HmacCtx, key) == 156);
static_assert(sizeof(EvpCipherCtx) == 168 && alignof(EvpCipherCtx) == 8);
static_assert(offsetof(EvpCipherCtx, num) == 88 && offsetof(EvpCipherCtx, appData) == 96);
static_assert(offsetof(EvpCipherCtx, flags) == 112 && offsetof(EvpCipherCtx, cipherData) == 120);
static_assert(offsetof(EvpCipherCtx, finalBlock) == 136);

constexpr std::uint64_t CipherCtxFlagWrapAllow = 0x1;

struct Bio;
struct X509Object;
struct Asn1String;
struct Asn1Object;
struct SslCtxObject;
struct SslObject;
struct SslSessionObject;
struct Stack;
struct Rsa;
struct EvpPkey;
struct EvpPkeyCtx;
struct EcKey;
struct Store;
struct StoreCtx;
struct Pkcs8;
struct Pubkey;
struct Algor;
struct Extension;

}

extern "C" {
const LibreSsl::EvpMd* LibreSslSha1_nid_no_patch();
const LibreSsl::EvpMd* LibreSslSha224_nid_no_patch();
const LibreSsl::EvpCipher* LibreSslAesWrap_nid_no_patch(int bits);
std::uint64_t LibreSslPeekLastError_nid_no_patch();
int LibreSslBioUpRef_nid_no_patch(LibreSsl::Bio* bio);
LibreSsl::X509Object* LibreSslD2iX509Bio_nid_no_patch(LibreSsl::Bio* bp, LibreSsl::X509Object** x509);
int LibreSslX509UpRef_nid_no_patch(LibreSsl::X509Object* x);
std::int64_t LibreSslX509Version_nid_no_patch(const LibreSsl::X509Object* x);
LibreSsl::Asn1String* LibreSslX509Time_nid_no_patch(const LibreSsl::X509Object* x, int which);
int LibreSslX509ExtensionCount_nid_no_patch(const LibreSsl::X509Object* x);
LibreSsl::Asn1String* LibreSslD2iAsn1Integer_nid_no_patch(LibreSsl::Asn1String** a, const unsigned char** pp, std::int64_t length);
void LibreSslAsn1IntegerFree_nid_no_patch(LibreSsl::Asn1String* a);
void LibreSslAsn1ObjectFree_nid_no_patch(LibreSsl::Asn1Object* a);
int LibreSslObjCmp_nid_no_patch(const LibreSsl::Asn1Object* a, const LibreSsl::Asn1Object* b);
LibreSsl::Asn1Object* LibreSslObjTxt2Obj_nid_no_patch(const char* s, int noName);
int LibreSslSslCtxUpRef_nid_no_patch(LibreSsl::SslCtxObject* ctx);
int LibreSslSslCtxSetProtoVersion_nid_no_patch(LibreSsl::SslCtxObject* ctx, std::uint16_t version, int maximum);
int LibreSslSslCtxAdd0ChainCert_nid_no_patch(LibreSsl::SslCtxObject* ctx, LibreSsl::X509Object* x);
int LibreSslSslCtxClearChainCerts_nid_no_patch(LibreSsl::SslCtxObject* ctx);
int LibreSslSslUpRef_nid_no_patch(LibreSsl::SslObject* ssl);
int LibreSslSslSessionUpRef_nid_no_patch(LibreSsl::SslSessionObject* session);
std::size_t LibreSslSslGetClientRandom_nid_no_patch(const LibreSsl::SslObject* ssl, unsigned char* out, std::size_t outlen);
int LibreSslRsaUpRef_nid_no_patch(LibreSsl::Rsa* r);
void LibreSslRsaGet0Key_nid_no_patch(const LibreSsl::Rsa* r, const LibreSsl::Bignum** n, const LibreSsl::Bignum** e, const LibreSsl::Bignum** d);
int LibreSslRsaPaddingAddPssMgf1_nid_no_patch(LibreSsl::Rsa* rsa, unsigned char* EM, const unsigned char* mHash, const LibreSsl::EvpMd* Hash,
    const LibreSsl::EvpMd* mgf1Hash, int sLen);
int LibreSslRsaCheckKey_nid_no_patch(const LibreSsl::Rsa* rsa);
int LibreSslEcKeyUpRef_nid_no_patch(LibreSsl::EcKey* key);
LibreSsl::EcKey* LibreSslD2iEcParameters_nid_no_patch(LibreSsl::EcKey** key, const unsigned char** in, std::int64_t len);
LibreSsl::EcKey* LibreSslO2iEcPublicKey_nid_no_patch(LibreSsl::EcKey** key, const unsigned char** in, std::int64_t len);
int LibreSslDigestSignInit_nid_no_patch(LibreSsl::EvpMdCtx* ctx, LibreSsl::EvpPkeyCtx** pctx, const LibreSsl::EvpMd* type, void* e, LibreSsl::EvpPkey* pkey);
int LibreSslDigestSignFinal_nid_no_patch(LibreSsl::EvpMdCtx* ctx, unsigned char* sigret, std::size_t* siglen);
int LibreSslSignFinal_nid_no_patch(LibreSsl::EvpMdCtx* ctx, unsigned char* md, unsigned int* s, LibreSsl::EvpPkey* pkey);
int LibreSslVerifyFinal_nid_no_patch(LibreSsl::EvpMdCtx* ctx, const unsigned char* sigbuf, unsigned int siglen, LibreSsl::EvpPkey* pkey);
LibreSsl::EvpPkeyCtx* LibreSslPkeyCtxNew_nid_no_patch(LibreSsl::EvpPkey* pkey, void* e);
int LibreSslPkeyCtxCtrl_nid_no_patch(LibreSsl::EvpPkeyCtx* ctx, int keytype, int optype, int cmd, int p1, void* p2);
int LibreSslPkeyEncryptInit_nid_no_patch(LibreSsl::EvpPkeyCtx* ctx);
int LibreSslPkeyDecryptInit_nid_no_patch(LibreSsl::EvpPkeyCtx* ctx);
int LibreSslPkeyEncrypt_nid_no_patch(LibreSsl::EvpPkeyCtx* ctx, unsigned char* out, std::size_t* outlen, const unsigned char* in, std::size_t inlen);
int LibreSslPkeyDecrypt_nid_no_patch(LibreSsl::EvpPkeyCtx* ctx, unsigned char* out, std::size_t* outlen, const unsigned char* in, std::size_t inlen);
LibreSsl::EvpPkey* LibreSslPkeyNewMacKey_nid_no_patch(int type, void* e, const unsigned char* key, int keylen);
int LibreSslPkeySize_nid_no_patch(const LibreSsl::EvpPkey* pkey);
int LibreSslPkeyUpRef_nid_no_patch(LibreSsl::EvpPkey* pkey);
LibreSsl::Rsa* LibreSslPkeyGet0Rsa_nid_no_patch(LibreSsl::EvpPkey* pkey);
void* LibreSslPkeyGet0Other_nid_no_patch(LibreSsl::EvpPkey* pkey, int dsa);
int LibreSslPkeySet1EcKey_nid_no_patch(LibreSsl::EvpPkey* pkey, LibreSsl::EcKey* key);
LibreSsl::EvpPkey* LibreSslD2iPrivateKeyBio_nid_no_patch(LibreSsl::Bio* bp, LibreSsl::EvpPkey** a);
LibreSsl::Rsa* LibreSslD2iRsaPublicKeyBio_nid_no_patch(LibreSsl::Bio* bp, LibreSsl::Rsa** a);
LibreSsl::Pkcs8* LibreSslD2iPkcs8Bio_nid_no_patch(LibreSsl::Bio* bp, LibreSsl::Pkcs8** p8);
void LibreSslPkcs8Free_nid_no_patch(LibreSsl::Pkcs8* a);
LibreSsl::EvpPkey* LibreSslPkcs82Pkey_nid_no_patch(const LibreSsl::Pkcs8* p8);
LibreSsl::Extension* LibreSslX509GetExt_nid_no_patch(const LibreSsl::X509Object* x, int loc);
const LibreSsl::Stack* LibreSslX509Get0Extensions_nid_no_patch(const LibreSsl::X509Object* x);
void LibreSslX509Get0Signature_nid_no_patch(const LibreSsl::Asn1String** psig, const LibreSsl::Algor** palg, const LibreSsl::X509Object* x);
int LibreSslX509PubkeyGet0Param_nid_no_patch(const LibreSsl::Asn1Object** ppkalg, const unsigned char** pk, int* ppklen, LibreSsl::Algor** pa,
    const LibreSsl::Pubkey* pub);
int LibreSslX509StoreUpRef_nid_no_patch(LibreSsl::Store* store);
LibreSsl::StoreCtx* LibreSslX509StoreCtxNew_nid_no_patch();
void LibreSslX509StoreCtxFree_nid_no_patch(LibreSsl::StoreCtx* ctx);
int LibreSslX509StoreCtxInit_nid_no_patch(LibreSsl::StoreCtx* ctx, LibreSsl::Store* store, LibreSsl::X509Object* x509, LibreSsl::Stack* chain);
void LibreSslX509StoreCtxSetExData_nid_no_patch(LibreSsl::StoreCtx* ctx, int idx, void* data);
}

#endif
