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
}

#endif
