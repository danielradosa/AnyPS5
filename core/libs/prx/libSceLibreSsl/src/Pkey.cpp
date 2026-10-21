#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/include/Keys.hpp"
#include "prx/libSceLibreSsl/include/X509Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/platform_util.h>

#include <atomic>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

extern "C" {
int APS5_VABI EVP_DigestInit_ex_nid_postfix(LibreSsl::EvpMdCtx* ctx, const LibreSsl::EvpMd* type, void* impl);
int APS5_VABI EVP_DigestUpdate_nid_postfix(LibreSsl::EvpMdCtx* ctx, const void* d, std::size_t cnt);
int APS5_VABI RSA_private_encrypt_nid_postfix(int flen, const unsigned char* from, unsigned char* to, LibreSsl::Rsa* rsa, int padding);
int APS5_VABI RSA_public_decrypt_nid_postfix(int flen, const unsigned char* from, unsigned char* to, LibreSsl::Rsa* rsa, int padding);
}

namespace LibreSsl {

struct EvpPkeyCtx {
    EvpPkey* pkey = nullptr;
    int operation = 0;
    int padding = RsaPkcs1Padding;
    int saltLength = -2;
    const EvpMd* md = nullptr;
    const EvpMd* mgf1 = nullptr;
    const EvpMd* oaep = nullptr;
    std::vector<unsigned char> hmacKey;
};

namespace {

constexpr int NidCmac = 894;
constexpr int OperationSign = 1 << 3;
constexpr int OperationVerify = 1 << 4;
constexpr int OperationSignCtx = 1 << 6;
constexpr int OperationVerifyCtx = 1 << 7;
constexpr int OperationEncrypt = 1 << 8;
constexpr int OperationDecrypt = 1 << 9;
constexpr int MaxMdSize = 64;

constexpr int CtrlMd = 1;
constexpr int CtrlGetMd = 13;
constexpr int CtrlRsaPadding = 0x1001;
constexpr int CtrlRsaPssSaltlen = 0x1002;
constexpr int CtrlRsaMgf1Md = 0x1005;
constexpr int CtrlGetRsaPadding = 0x1006;
constexpr int CtrlGetRsaPssSaltlen = 0x1007;
constexpr int CtrlGetRsaMgf1Md = 0x1008;
constexpr int CtrlRsaOaepMd = 0x1009;
constexpr int CtrlGetRsaOaepMd = 0x100b;

constexpr int EvpFunctionPkeyCopyParameters = 103;
constexpr int EvpFunctionPkeyDecrypt = 104;
constexpr int EvpFunctionPkeyEncrypt = 105;
constexpr int EvpFunctionPkeyNew = 106;
constexpr int EvpFunctionSignFinal = 107;
constexpr int EvpFunctionVerifyFinal = 108;
constexpr int EvpFunctionPkeyGet1Dh = 119;
constexpr int EvpFunctionPkeyGet1Dsa = 120;
constexpr int EvpFunctionPkeyCtxCtrl = 137;
constexpr int EvpFunctionPkeyDecryptInit = 138;
constexpr int EvpFunctionPkeyEncryptInit = 139;
constexpr int EvpFunctionPkeySign = 140;
constexpr int EvpFunctionDoSigverInit = 161;

constexpr int EvpReasonDifferentKeyTypes = 101;
constexpr int EvpReasonMissingParameters = 103;
constexpr int EvpReasonExpectingAnRsaKey = 127;
constexpr int EvpReasonExpectingADhKey = 128;
constexpr int EvpReasonExpectingADsaKey = 129;
constexpr int EvpReasonExpectingAnEcKey = 142;
constexpr int EvpReasonCommandNotSupported = 147;
constexpr int EvpReasonInvalidOperation = 148;
constexpr int EvpReasonNoOperationSet = 149;
constexpr int EvpReasonOperationNotSupported = 150;
constexpr int EvpReasonOperationNotInitialized = 151;
constexpr int EvpReasonBufferTooSmall = 155;

constexpr int RsaFunctionVerify = 119;
constexpr int RsaReasonWrongSignatureLength = 119;
constexpr int RsaFunctionPkeyRsaCtrl = 143;
constexpr int RsaReasonIllegalOrUnsupportedPaddingMode = 144;
constexpr int RsaReasonInvalidPssSaltlen = 146;
constexpr int RsaReasonInvalidMgf1Md = 156;

std::mutex& ContextMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_set<const EvpPkeyCtx*>& Contexts() {
    static std::unordered_set<const EvpPkeyCtx*> contexts;
    return contexts;
}

EvpPkey& RequirePkey(EvpPkey* pkey, const char* function) {
    if (!pkey) throw std::invalid_argument(std::string(function) + ": null EVP_PKEY");
    return *pkey;
}

const EvpPkey& RequirePkey(const EvpPkey* pkey, const char* function) {
    if (!pkey) throw std::invalid_argument(std::string(function) + ": null EVP_PKEY");
    return *pkey;
}

EvpPkeyCtx& RequireCtx(EvpPkeyCtx* ctx, const char* function) {
    if (!ctx) throw std::invalid_argument(std::string(function) + ": null EVP_PKEY_CTX");
    std::lock_guard lock(ContextMutex());
    if (!Contexts().count(ctx)) throw std::invalid_argument(std::string(function) + ": EVP_PKEY_CTX not created by this library");
    return *ctx;
}

void ReleaseContent(EvpPkey& pkey) {
    switch (pkey.type) {
    case NidRsaEncryption:
        FreeRsa(static_cast<Rsa*>(pkey.pkey));
        break;
    case NidEcPublicKey:
        FreeEcKey(static_cast<EcKey*>(pkey.pkey));
        break;
    case NidHmac:
        if (auto* key = static_cast<Asn1String*>(pkey.pkey)) {
            if (key->data && key->length > 0) mbedtls_platform_zeroize(key->data, static_cast<std::size_t>(key->length));
            delete[] key->data;
            delete key;
        }
        break;
    default:
        break;
    }
    pkey.pkey = nullptr;
}

EvpPkeyCtx* NewCtx(EvpPkey* pkey, const char* function) {
    auto& key = RequirePkey(pkey, function);
    if (key.type != NidRsaEncryption && key.type != NidEcPublicKey && key.type != NidHmac)
        throw std::invalid_argument(std::string(function) + ": the key has no algorithm");
    auto* ctx = new (std::nothrow) EvpPkeyCtx;
    if (!ctx) return nullptr;
    PkeyUpRef(pkey);
    ctx->pkey = pkey;
    if (key.type == NidHmac) {
        const auto* octets = static_cast<const Asn1String*>(key.pkey);
        ctx->hmacKey.assign(octets->data, octets->data + octets->length);
    }
    std::lock_guard lock(ContextMutex());
    Contexts().insert(ctx);
    return ctx;
}

std::vector<unsigned char> HmacBlockKey(const EvpPkeyCtx& ctx, const EvpMd& md, const char* function) {
    std::vector<unsigned char> block(static_cast<std::size_t>(md.blockSize));
    if (ctx.hmacKey.size() > block.size()) {
        if (mbedtls_md(mbedtls_md_info_from_type(md.md), ctx.hmacKey.data(), ctx.hmacKey.size(), block.data()) != 0)
            throw std::runtime_error(std::string(function) + ": Mbed TLS failed");
    } else if (!ctx.hmacKey.empty()) {
        std::memcpy(block.data(), ctx.hmacKey.data(), ctx.hmacKey.size());
    }
    return block;
}

EvpPkeyCtx& DigestCtxPkey(EvpMdCtx* ctx, int operation, const char* function) {
    if (!ctx) throw std::invalid_argument(std::string(function) + ": null context");
    auto* pctx = static_cast<EvpPkeyCtx*>(ctx->pctx);
    if (!pctx) throw std::logic_error(std::string(function) + ": the context was not initialized for signing or verifying");
    auto& context = RequireCtx(pctx, function);
    if (context.operation != operation) throw std::logic_error(std::string(function) + ": the context was initialized for another operation");
    if (!ctx->digest || ctx->digest != context.md) throw std::logic_error(std::string(function) + ": the context's digest changed since the initialization");
    return context;
}

int SigverInit(EvpMdCtx* ctx, EvpPkeyCtx** pctx, const EvpMd* type, void* e, EvpPkey* pkey, int operation, const char* function) {
    if (!ctx) throw std::invalid_argument(std::string(function) + ": null context");
    auto& key = RequirePkey(pkey, function);
    if (e) throw std::invalid_argument(std::string(function) + ": engines are not supported");
    if (!type) throw std::invalid_argument(std::string(function) + ": a default digest is not supported");
    if (key.type == NidHmac && operation == OperationVerifyCtx) {
        PushError(ErrLibEvp, EvpFunctionDoSigverInit, EvpReasonOperationNotSupported);
        return -2;
    }
    if (!EVP_DigestInit_ex_nid_postfix(ctx, type, nullptr)) return 0;
    auto* context = NewCtx(pkey, function);
    if (!context) return 0;
    context->operation = operation;
    context->md = type;
    FreePkeyCtx(ctx->pctx);
    ctx->pctx = context;
    if (pctx) *pctx = context;
    if (key.type == NidHmac) {
        auto pad = HmacBlockKey(*context, *type, function);
        for (auto& byte : pad) byte ^= 0x36;
        if (!EVP_DigestUpdate_nid_postfix(ctx, pad.data(), pad.size())) return 0;
    }
    return 1;
}

int SignDigest(const EvpPkeyCtx& context, const EvpMd& md, const unsigned char* digest, unsigned char* sig, std::size_t* siglen, int function) {
    auto& key = *context.pkey;
    const int needed = PkeySize(key, "EVP_PKEY sign");
    if (*siglen < static_cast<std::size_t>(needed)) {
        PushError(ErrLibEvp, function, EvpReasonBufferTooSmall);
        return 0;
    }
    unsigned int length = 0;
    if (key.type == NidEcPublicKey) {
        if (!EcdsaSign(digest, md.mdSize, sig, &length, *static_cast<EcKey*>(key.pkey), "EVP_PKEY sign")) return 0;
        *siglen = length;
        return 1;
    }
    auto& rsa = *static_cast<Rsa*>(key.pkey);
    if (context.padding == RsaPkcs1PssPadding) {
        std::vector<unsigned char> em(static_cast<std::size_t>(needed));
        if (!RsaPssEncode(rsa, em.data(), digest, &md, context.mgf1 ? context.mgf1 : &md, context.saltLength)) return 0;
        const int written = RSA_private_encrypt_nid_postfix(needed, em.data(), sig, &rsa, RsaNoPadding);
        if (written < 0) return 0;
        *siglen = static_cast<std::size_t>(written);
        return 1;
    }
    if (context.padding != RsaPkcs1Padding) throw std::invalid_argument("EVP_PKEY sign: only PKCS #1 and PSS padding are supported for signatures");
    if (!RsaSignDigest(md.type, digest, static_cast<unsigned int>(md.mdSize), sig, &length, rsa, "EVP_PKEY sign")) return 0;
    *siglen = length;
    return 1;
}

int VerifyDigest(const EvpPkeyCtx& context, const EvpMd& md, const unsigned char* digest, const unsigned char* sig, std::size_t siglen) {
    auto& key = *context.pkey;
    if (siglen > static_cast<std::size_t>(std::numeric_limits<int>::max())) return 0;
    if (key.type == NidEcPublicKey) return EcdsaVerify(digest, md.mdSize, sig, static_cast<int>(siglen), *static_cast<EcKey*>(key.pkey), "EVP_PKEY verify");
    auto& rsa = *static_cast<Rsa*>(key.pkey);
    if (context.padding == RsaPkcs1PssPadding) {
        const int size = RsaSize(rsa, "EVP_PKEY verify");
        if (siglen != static_cast<std::size_t>(size)) {
            PushError(ErrLibRsa, RsaFunctionVerify, RsaReasonWrongSignatureLength);
            return 0;
        }
        std::vector<unsigned char> em(static_cast<std::size_t>(size));
        if (RSA_public_decrypt_nid_postfix(size, sig, em.data(), &rsa, RsaNoPadding) != size) return 0;
        return RsaPssVerify(rsa, digest, &md, context.mgf1 ? context.mgf1 : &md, em.data(), context.saltLength);
    }
    if (context.padding != RsaPkcs1Padding) throw std::invalid_argument("EVP_PKEY verify: only PKCS #1 and PSS padding are supported for signatures");
    return RsaVerifyDigest(md.type, digest, static_cast<unsigned int>(md.mdSize), sig, static_cast<unsigned int>(siglen), rsa, "EVP_PKEY verify");
}

int RsaCtrl(EvpPkeyCtx& ctx, int cmd, int p1, void* p2) {
    const bool signature = ctx.operation & (OperationSign | OperationVerify | OperationSignCtx | OperationVerifyCtx);
    const bool crypt = ctx.operation & (OperationEncrypt | OperationDecrypt);
    switch (cmd) {
    case CtrlRsaPadding:
        if (p1 == 5 || p1 == 2) throw std::invalid_argument("EVP_PKEY_CTX_ctrl: SSLv23 and X9.31 padding are not supported");
        if (p1 == RsaPkcs1Padding || p1 == RsaNoPadding || (p1 == RsaPkcs1OaepPadding && crypt) || (p1 == RsaPkcs1PssPadding && signature)) {
            ctx.padding = p1;
            return 1;
        }
        PushError(ErrLibRsa, RsaFunctionPkeyRsaCtrl, RsaReasonIllegalOrUnsupportedPaddingMode);
        return -2;
    case CtrlGetRsaPadding:
        if (!p2) throw std::invalid_argument("EVP_PKEY_CTX_ctrl: null output");
        *static_cast<int*>(p2) = ctx.padding;
        return 1;
    case CtrlRsaPssSaltlen:
    case CtrlGetRsaPssSaltlen:
        if (ctx.padding != RsaPkcs1PssPadding) {
            PushError(ErrLibRsa, RsaFunctionPkeyRsaCtrl, RsaReasonInvalidPssSaltlen);
            return -2;
        }
        if (cmd == CtrlGetRsaPssSaltlen) {
            if (!p2) throw std::invalid_argument("EVP_PKEY_CTX_ctrl: null output");
            *static_cast<int*>(p2) = ctx.saltLength;
            return 1;
        }
        if (p1 < -2) return -2;
        ctx.saltLength = p1;
        return 1;
    case CtrlRsaMgf1Md:
    case CtrlGetRsaMgf1Md:
        if (ctx.padding != RsaPkcs1PssPadding) {
            if (ctx.padding == RsaPkcs1OaepPadding) throw std::invalid_argument("EVP_PKEY_CTX_ctrl: an OAEP MGF1 digest is not supported");
            PushError(ErrLibRsa, RsaFunctionPkeyRsaCtrl, RsaReasonInvalidMgf1Md);
            return -2;
        }
        if (!p2) throw std::invalid_argument("EVP_PKEY_CTX_ctrl: null digest");
        if (cmd == CtrlGetRsaMgf1Md) *static_cast<const EvpMd**>(p2) = ctx.mgf1 ? ctx.mgf1 : ctx.md;
        else ctx.mgf1 = static_cast<const EvpMd*>(p2);
        return 1;
    case CtrlRsaOaepMd:
    case CtrlGetRsaOaepMd:
        if (ctx.padding != RsaPkcs1OaepPadding) {
            PushError(ErrLibRsa, RsaFunctionPkeyRsaCtrl, RsaReasonIllegalOrUnsupportedPaddingMode);
            return -2;
        }
        if (!p2) throw std::invalid_argument("EVP_PKEY_CTX_ctrl: null digest");
        if (cmd == CtrlGetRsaOaepMd) *static_cast<const EvpMd**>(p2) = ctx.oaep ? ctx.oaep : LibreSslSha1_nid_no_patch();
        else ctx.oaep = static_cast<const EvpMd*>(p2);
        return 1;
    default:
        return 0;
    }
}

int EncryptCommon(EvpPkeyCtx& ctx, unsigned char* out, std::size_t* outlen, const unsigned char* in, std::size_t inlen, bool encrypt) {
    const char* function = encrypt ? "EVP_PKEY_encrypt" : "EVP_PKEY_decrypt";
    const int functionCode = encrypt ? EvpFunctionPkeyEncrypt : EvpFunctionPkeyDecrypt;
    if (ctx.operation != (encrypt ? OperationEncrypt : OperationDecrypt)) {
        PushError(ErrLibEvp, functionCode, EvpReasonOperationNotInitialized);
        return -1;
    }
    if (!outlen) throw std::invalid_argument(std::string(function) + ": null output length");
    auto& rsa = *static_cast<Rsa*>(ctx.pkey->pkey);
    const int size = RsaSize(rsa, function);
    if (!out) {
        *outlen = static_cast<std::size_t>(size);
        return 1;
    }
    if (*outlen < static_cast<std::size_t>(size)) {
        PushError(ErrLibEvp, functionCode, EvpReasonBufferTooSmall);
        return 0;
    }
    if (inlen > static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::invalid_argument(std::string(function) + ": input too long");
    if (ctx.padding == RsaPkcs1OaepPadding && ctx.oaep && ctx.oaep->type != NidSha1)
        throw std::invalid_argument(std::string(function) + ": OAEP digests other than SHA-1 are not supported");
    if (ctx.padding == RsaPkcs1PssPadding) throw std::invalid_argument(std::string(function) + ": PSS padding is for signatures");
    const int result = encrypt ? RsaPublicEncrypt(static_cast<int>(inlen), in, out, rsa, ctx.padding) : RsaPrivateDecrypt(static_cast<int>(inlen), in, out, rsa, ctx.padding);
    if (result < 0) return result;
    *outlen = static_cast<std::size_t>(result);
    return 1;
}

int CryptInit(EvpPkeyCtx* ctx, int operation, int function) {
    auto& context = RequireCtx(ctx, operation == OperationEncrypt ? "EVP_PKEY_encrypt_init" : "EVP_PKEY_decrypt_init");
    if (context.pkey->type != NidRsaEncryption) {
        PushError(ErrLibEvp, function, EvpReasonOperationNotSupported);
        return -2;
    }
    context.operation = operation;
    context.padding = RsaPkcs1Padding;
    return 1;
}

int FinalDigest(EvpMdCtx* ctx, unsigned char* digest, const char* function) {
    if (!ctx) throw std::invalid_argument(std::string(function) + ": null context");
    if (!ctx->digest) throw std::logic_error(std::string(function) + ": the context has no digest set up");
    return DigestCopyFinal(*ctx, digest, function) ? 1 : 0;
}

}

EvpPkey* NewPkey() {
    auto* pkey = new (std::nothrow) EvpPkey{};
    if (!pkey) return nullptr;
    pkey->references = 1;
    pkey->saveParameters = 1;
    return pkey;
}

void PkeyUpRef(EvpPkey* pkey) {
    std::atomic_ref<int>(RequirePkey(pkey, "EVP_PKEY_up_ref").references).fetch_add(1);
}

void FreePkey(EvpPkey* pkey) {
    if (!pkey) return;
    if (std::atomic_ref<int>(pkey->references).fetch_sub(1) > 1) return;
    ReleaseContent(*pkey);
    delete pkey;
}

void AssignRsa(EvpPkey& pkey, Rsa* rsa) {
    ReleaseContent(pkey);
    pkey.type = NidRsaEncryption;
    pkey.saveType = NidRsaEncryption;
    pkey.pkey = rsa;
}

void AssignEcKey(EvpPkey& pkey, EcKey* key) {
    ReleaseContent(pkey);
    pkey.type = NidEcPublicKey;
    pkey.saveType = NidEcPublicKey;
    pkey.pkey = key;
}

EvpPkey* PkeyFromMbed(const mbedtls_pk_context& pk, const char* function) {
    const auto type = mbedtls_pk_get_type(&pk);
    auto* pkey = NewPkey();
    if (!pkey) throw std::bad_alloc();
    try {
        if (type == MBEDTLS_PK_RSA) {
            auto* rsa = NewRsa();
            if (!rsa) throw std::bad_alloc();
            AssignRsa(*pkey, rsa);
            FillRsa(*rsa, *mbedtls_pk_rsa(pk), function);
        } else if (type == MBEDTLS_PK_ECKEY || type == MBEDTLS_PK_ECKEY_DH || type == MBEDTLS_PK_ECDSA) {
            AssignEcKey(*pkey, EcKeyFromKeypair(*mbedtls_pk_ec(pk), function));
        } else {
            throw std::invalid_argument(std::string(function) + ": the key algorithm is not supported");
        }
    } catch (...) {
        FreePkey(pkey);
        throw;
    }
    return pkey;
}

bool PkeyToMbed(const EvpPkey& pkey, mbedtls_pk_context& pk, const char* function) {
    if (pkey.type == NidRsaEncryption) {
        if (mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) != 0) throw std::bad_alloc();
        return RsaContext(*static_cast<const Rsa*>(pkey.pkey), *mbedtls_pk_rsa(pk), false, function);
    }
    if (pkey.type == NidEcPublicKey) {
        if (mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) != 0) throw std::bad_alloc();
        return EcKeypair(*static_cast<const EcKey*>(pkey.pkey), *mbedtls_pk_ec(pk), false, function);
    }
    throw std::invalid_argument(std::string(function) + ": the key algorithm is not supported");
}

int PkeySize(const EvpPkey& pkey, const char* function) {
    switch (pkey.type) {
    case NidRsaEncryption:
        return RsaSize(*static_cast<const Rsa*>(pkey.pkey), function);
    case NidEcPublicKey:
        return EcdsaSize(*static_cast<const EcKey*>(pkey.pkey));
    case NidHmac:
        return MaxMdSize;
    default:
        return 0;
    }
}

Rsa* PkeyRsa(const EvpPkey* pkey, int function) {
    const auto& key = RequirePkey(pkey, "EVP_PKEY RSA getter");
    if (key.type != NidRsaEncryption) {
        PushError(ErrLibEvp, function, EvpReasonExpectingAnRsaKey);
        return nullptr;
    }
    return static_cast<Rsa*>(key.pkey);
}

EcKey* PkeyEcKey(const EvpPkey* pkey, int function) {
    const auto& key = RequirePkey(pkey, "EVP_PKEY EC getter");
    if (key.type != NidEcPublicKey) {
        PushError(ErrLibEvp, function, EvpReasonExpectingAnEcKey);
        return nullptr;
    }
    return static_cast<EcKey*>(key.pkey);
}

void FreePkeyCtx(void* ctx) {
    if (!ctx) return;
    auto* context = static_cast<EvpPkeyCtx*>(ctx);
    {
        std::lock_guard lock(ContextMutex());
        if (!Contexts().erase(context)) throw std::invalid_argument("EVP_PKEY_CTX free: context not created by this library");
    }
    FreePkey(context->pkey);
    if (!context->hmacKey.empty()) mbedtls_platform_zeroize(context->hmacKey.data(), context->hmacKey.size());
    delete context;
}

std::vector<unsigned char> EncodeSubjectPublicKey(const EvpPkey& pkey, const char* function) {
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    std::vector<unsigned char> buffer(4096);
    int length = -1;
    try {
        if (PkeyToMbed(pkey, pk, function)) length = mbedtls_pk_write_pubkey_der(&pk, buffer.data(), buffer.size());
    } catch (...) {
        mbedtls_pk_free(&pk);
        throw;
    }
    mbedtls_pk_free(&pk);
    if (length < 0) throw std::invalid_argument(std::string(function) + ": the public key cannot be encoded");
    return std::vector<unsigned char>(buffer.end() - length, buffer.end());
}

}

using namespace LibreSsl;

extern "C" {

EvpPkey* APS5_VABI EVP_PKEY_new_nid_postfix() {
    auto* pkey = NewPkey();
    if (!pkey) PushError(ErrLibEvp, EvpFunctionPkeyNew, ErrReasonMallocFailure);
    return pkey;
}

void APS5_VABI EVP_PKEY_free_nid_postfix(EvpPkey* pkey) {
    FreePkey(pkey);
}

int APS5_VABI EVP_PKEY_id_nid_postfix(const EvpPkey* pkey) {
    return RequirePkey(pkey, __func__).type;
}

int APS5_VABI EVP_PKEY_type_nid_postfix(int type) {
    switch (type) {
    case NidRsaEncryption:
    case NidRsa:
        return NidRsaEncryption;
    case NidDsa:
    case 66:
    case 67:
    case 70:
    case 113:
        return NidDsa;
    case NidDhKeyAgreement:
    case NidEcPublicKey:
    case NidHmac:
    case NidCmac:
        return type;
    case 811:
    case 812:
    case 979:
    case 980:
        throw std::invalid_argument("EVP_PKEY_type: GOST key types are not supported");
    default:
        return NidUndef;
    }
}

int APS5_VABI EVP_PKEY_set1_RSA_nid_postfix(EvpPkey* pkey, Rsa* key) {
    auto& target = RequirePkey(pkey, __func__);
    if (!key) throw std::invalid_argument("EVP_PKEY_set1_RSA: null RSA");
    RsaUpRef(key);
    AssignRsa(target, key);
    return 1;
}

Rsa* APS5_VABI EVP_PKEY_get1_RSA_nid_postfix(EvpPkey* pkey) {
    auto* rsa = PkeyRsa(pkey, 121);
    if (rsa) RsaUpRef(rsa);
    return rsa;
}

EcKey* APS5_VABI EVP_PKEY_get1_EC_KEY_nid_postfix(EvpPkey* pkey) {
    auto* key = PkeyEcKey(pkey, 131);
    if (key) EcKeyUpRef(key);
    return key;
}

int APS5_VABI EVP_PKEY_copy_parameters_nid_postfix(EvpPkey* to, const EvpPkey* from) {
    auto& target = RequirePkey(to, __func__);
    const auto& source = RequirePkey(from, __func__);
    if (target.type == NidUndef) throw std::invalid_argument("EVP_PKEY_copy_parameters: copying into an empty EVP_PKEY is not supported");
    if (target.type != source.type) {
        PushError(ErrLibEvp, EvpFunctionPkeyCopyParameters, EvpReasonDifferentKeyTypes);
        return 0;
    }
    if (source.type != NidEcPublicKey) throw std::invalid_argument("EVP_PKEY_copy_parameters: only EC keys have parameters in this build");
    const auto& sourceKey = *static_cast<const EcKey*>(source.pkey);
    if (!sourceKey.group) {
        PushError(ErrLibEvp, EvpFunctionPkeyCopyParameters, EvpReasonMissingParameters);
        return 0;
    }
    auto& targetKey = *static_cast<EcKey*>(target.pkey);
    auto* copy = CopyGroup(*sourceKey.group);
    delete targetKey.group;
    targetKey.group = copy;
    return 1;
}

int APS5_VABI EVP_DigestVerifyInit_nid_postfix(EvpMdCtx* ctx, EvpPkeyCtx** pctx, const EvpMd* type, void* e, EvpPkey* pkey) {
    return SigverInit(ctx, pctx, type, e, pkey, OperationVerifyCtx, __func__);
}

int APS5_VABI EVP_DigestVerifyFinal_nid_postfix(EvpMdCtx* ctx, const unsigned char* sig, std::size_t siglen) {
    auto& context = DigestCtxPkey(ctx, OperationVerifyCtx, __func__);
    if (!sig && siglen) throw std::invalid_argument("EVP_DigestVerifyFinal: null signature");
    unsigned char digest[MaxMdSize];
    if (!FinalDigest(ctx, digest, __func__)) return 0;
    return VerifyDigest(context, *context.md, digest, sig, siglen);
}

int LibreSslDigestSignInit_nid_no_patch(EvpMdCtx* ctx, EvpPkeyCtx** pctx, const EvpMd* type, void* e, EvpPkey* pkey) {
    return SigverInit(ctx, pctx, type, e, pkey, OperationSignCtx, "EVP_DigestSignInit");
}

int LibreSslDigestSignFinal_nid_no_patch(EvpMdCtx* ctx, unsigned char* sigret, std::size_t* siglen) {
    constexpr const char* function = "EVP_DigestSignFinal";
    auto& context = DigestCtxPkey(ctx, OperationSignCtx, function);
    if (!siglen) throw std::invalid_argument("EVP_DigestSignFinal: null signature length");
    const auto& md = *context.md;
    if (context.pkey->type == NidHmac) {
        if (!sigret) {
            *siglen = static_cast<std::size_t>(md.mdSize);
            return 1;
        }
        if (*siglen < static_cast<std::size_t>(md.mdSize)) {
            PushError(ErrLibEvp, EvpFunctionPkeySign, EvpReasonBufferTooSmall);
            return 0;
        }
        unsigned char inner[MaxMdSize];
        if (!FinalDigest(ctx, inner, function)) return 0;
        auto pad = HmacBlockKey(context, md, function);
        for (auto& byte : pad) byte ^= 0x5c;
        pad.insert(pad.end(), inner, inner + md.mdSize);
        if (mbedtls_md(mbedtls_md_info_from_type(md.md), pad.data(), pad.size(), sigret) != 0) return 0;
        *siglen = static_cast<std::size_t>(md.mdSize);
        return 1;
    }
    if (!sigret) {
        *siglen = static_cast<std::size_t>(PkeySize(*context.pkey, function));
        return 1;
    }
    unsigned char digest[MaxMdSize];
    if (!FinalDigest(ctx, digest, function)) return 0;
    return SignDigest(context, md, digest, sigret, siglen, EvpFunctionPkeySign);
}

int LibreSslSignFinal_nid_no_patch(EvpMdCtx* ctx, unsigned char* md, unsigned int* s, EvpPkey* pkey) {
    constexpr const char* function = "EVP_SignFinal";
    auto& key = RequirePkey(pkey, function);
    if (!md || !s) throw std::invalid_argument("EVP_SignFinal: null signature output");
    *s = 0;
    unsigned char digest[MaxMdSize];
    if (!FinalDigest(ctx, digest, function)) return 0;
    if (key.type != NidRsaEncryption && key.type != NidEcPublicKey) {
        PushError(ErrLibEvp, EvpFunctionSignFinal, EvpReasonOperationNotSupported);
        return 0;
    }
    EvpPkeyCtx context;
    context.pkey = pkey;
    context.md = ctx->digest;
    std::size_t length = static_cast<std::size_t>(PkeySize(key, function));
    if (!SignDigest(context, *ctx->digest, digest, md, &length, EvpFunctionSignFinal)) return 0;
    *s = static_cast<unsigned int>(length);
    return 1;
}

int LibreSslVerifyFinal_nid_no_patch(EvpMdCtx* ctx, const unsigned char* sigbuf, unsigned int siglen, EvpPkey* pkey) {
    constexpr const char* function = "EVP_VerifyFinal";
    auto& key = RequirePkey(pkey, function);
    if (!sigbuf && siglen) throw std::invalid_argument("EVP_VerifyFinal: null signature");
    unsigned char digest[MaxMdSize];
    if (!FinalDigest(ctx, digest, function)) return -1;
    if (key.type != NidRsaEncryption && key.type != NidEcPublicKey) {
        PushError(ErrLibEvp, EvpFunctionVerifyFinal, EvpReasonOperationNotSupported);
        return -1;
    }
    EvpPkeyCtx context;
    context.pkey = pkey;
    context.md = ctx->digest;
    return VerifyDigest(context, *ctx->digest, digest, sigbuf, siglen);
}

EvpPkeyCtx* LibreSslPkeyCtxNew_nid_no_patch(EvpPkey* pkey, void* e) {
    if (e) throw std::invalid_argument("EVP_PKEY_CTX_new: engines are not supported");
    return NewCtx(pkey, "EVP_PKEY_CTX_new");
}

int LibreSslPkeyCtxCtrl_nid_no_patch(EvpPkeyCtx* ctx, int keytype, int optype, int cmd, int p1, void* p2) {
    auto& context = RequireCtx(ctx, "EVP_PKEY_CTX_ctrl");
    if (keytype != -1 && keytype != context.pkey->type) return -1;
    if (!context.operation) {
        PushError(ErrLibEvp, EvpFunctionPkeyCtxCtrl, EvpReasonNoOperationSet);
        return -1;
    }
    if (optype != -1 && !(context.operation & optype)) {
        PushError(ErrLibEvp, EvpFunctionPkeyCtxCtrl, EvpReasonInvalidOperation);
        return -1;
    }
    int result = 0;
    if (cmd == CtrlMd) {
        if (!p2) throw std::invalid_argument("EVP_PKEY_CTX_ctrl: null digest");
        context.md = static_cast<const EvpMd*>(p2);
        result = 1;
    } else if (cmd == CtrlGetMd) {
        if (!p2) throw std::invalid_argument("EVP_PKEY_CTX_ctrl: null output");
        *static_cast<const EvpMd**>(p2) = context.md;
        result = 1;
    } else if (context.pkey->type == NidRsaEncryption) {
        result = RsaCtrl(context, cmd, p1, p2);
    }
    if (result == 0) throw std::invalid_argument("EVP_PKEY_CTX_ctrl: control " + std::to_string(cmd) + " is not supported");
    if (result == -2) PushError(ErrLibEvp, EvpFunctionPkeyCtxCtrl, EvpReasonCommandNotSupported);
    return result;
}

int LibreSslPkeyEncryptInit_nid_no_patch(EvpPkeyCtx* ctx) {
    return CryptInit(ctx, OperationEncrypt, EvpFunctionPkeyEncryptInit);
}

int LibreSslPkeyDecryptInit_nid_no_patch(EvpPkeyCtx* ctx) {
    return CryptInit(ctx, OperationDecrypt, EvpFunctionPkeyDecryptInit);
}

int LibreSslPkeyEncrypt_nid_no_patch(EvpPkeyCtx* ctx, unsigned char* out, std::size_t* outlen, const unsigned char* in, std::size_t inlen) {
    return EncryptCommon(RequireCtx(ctx, "EVP_PKEY_encrypt"), out, outlen, in, inlen, true);
}

int LibreSslPkeyDecrypt_nid_no_patch(EvpPkeyCtx* ctx, unsigned char* out, std::size_t* outlen, const unsigned char* in, std::size_t inlen) {
    return EncryptCommon(RequireCtx(ctx, "EVP_PKEY_decrypt"), out, outlen, in, inlen, false);
}

EvpPkey* LibreSslPkeyNewMacKey_nid_no_patch(int type, void* e, const unsigned char* key, int keylen) {
    if (e) throw std::invalid_argument("EVP_PKEY_new_mac_key: engines are not supported");
    if (type != NidHmac) throw std::invalid_argument("EVP_PKEY_new_mac_key: only HMAC keys are supported");
    if (keylen < 0 || (!key && keylen)) throw std::invalid_argument("EVP_PKEY_new_mac_key: invalid key");
    auto* pkey = NewPkey();
    if (!pkey) return nullptr;
    auto* octets = new Asn1String{keylen, 4, new unsigned char[static_cast<std::size_t>(keylen) + 1](), 0};
    if (keylen) std::memcpy(octets->data, key, static_cast<std::size_t>(keylen));
    pkey->type = NidHmac;
    pkey->saveType = NidHmac;
    pkey->pkey = octets;
    return pkey;
}

int LibreSslPkeySize_nid_no_patch(const EvpPkey* pkey) {
    return PkeySize(RequirePkey(pkey, "EVP_PKEY_size"), "EVP_PKEY_size");
}

int LibreSslPkeyUpRef_nid_no_patch(EvpPkey* pkey) {
    PkeyUpRef(pkey);
    return 1;
}

Rsa* LibreSslPkeyGet0Rsa_nid_no_patch(EvpPkey* pkey) {
    return PkeyRsa(pkey, 121);
}

void* LibreSslPkeyGet0Other_nid_no_patch(EvpPkey* pkey, int dsa) {
    RequirePkey(pkey, dsa ? "EVP_PKEY_get0_DSA" : "EVP_PKEY_get0_DH");
    PushError(ErrLibEvp, dsa ? EvpFunctionPkeyGet1Dsa : EvpFunctionPkeyGet1Dh, dsa ? EvpReasonExpectingADsaKey : EvpReasonExpectingADhKey);
    return nullptr;
}

int LibreSslPkeySet1EcKey_nid_no_patch(EvpPkey* pkey, EcKey* key) {
    auto& target = RequirePkey(pkey, "EVP_PKEY_set1_EC_KEY");
    if (!key) throw std::invalid_argument("EVP_PKEY_set1_EC_KEY: null EC_KEY");
    EcKeyUpRef(key);
    AssignEcKey(target, key);
    return 1;
}

}
