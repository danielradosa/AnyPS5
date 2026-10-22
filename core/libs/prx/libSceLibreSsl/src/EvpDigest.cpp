#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/include/Keys.hpp"
#include "prx/libc/include/General.hpp"

#include <cstring>
#include <new>
#include <stdexcept>
#include <string>

namespace LibreSsl {

namespace {

constexpr int EvpFunctionDigestInitEx = 128;
constexpr int EvpReasonNoDigestSet = 139;

constexpr EvpMd Md5{NidMd5, 8, 16, 64, MBEDTLS_MD_MD5};
constexpr EvpMd Sha1{NidSha1, 65, 20, 64, MBEDTLS_MD_SHA1};
constexpr EvpMd Sha224{NidSha224, 671, 28, 64, MBEDTLS_MD_SHA224};
constexpr EvpMd Sha256{NidSha256, 668, 32, 64, MBEDTLS_MD_SHA256};
constexpr EvpMd Sha384{NidSha384, 669, 48, 128, MBEDTLS_MD_SHA384};
constexpr EvpMd Sha512{NidSha512, 670, 64, 128, MBEDTLS_MD_SHA512};

void RequireData(const void* data, std::size_t length, const char* function) {
    if (!data && length) throw std::invalid_argument(std::string(function) + ": null data");
}

mbedtls_md_context_t* MdContext(const EvpMdCtx& ctx) {
    return static_cast<mbedtls_md_context_t*>(ctx.mdData);
}

void ReleaseContext(EvpMdCtx& ctx) {
    if (auto* context = MdContext(ctx)) {
        mbedtls_md_free(context);
        delete context;
    }
    ctx.mdData = nullptr;
}

bool PrepareContext(EvpMdCtx& ctx, const EvpMd* md, bool hmac) {
    if (MdContext(ctx) && ctx.digest == md) return true;
    ReleaseContext(ctx);
    ctx.digest = nullptr;
    auto* context = new (std::nothrow) mbedtls_md_context_t;
    if (!context) return false;
    mbedtls_md_init(context);
    if (mbedtls_md_setup(context, mbedtls_md_info_from_type(md->md), hmac ? 1 : 0) != 0) {
        mbedtls_md_free(context);
        delete context;
        return false;
    }
    ctx.mdData = context;
    ctx.digest = md;
    return true;
}

EvpMdCtx& RequireMdCtx(EvpMdCtx* ctx, const char* function) {
    if (!ctx) throw std::invalid_argument(std::string(function) + ": null context");
    return *ctx;
}

mbedtls_md_context_t& StartedContext(const EvpMdCtx& ctx, const char* function) {
    auto* context = MdContext(ctx);
    if (!context || !ctx.digest) throw std::logic_error(std::string(function) + ": the context has no digest set up");
    return *context;
}

HmacCtx& RequireHmac(HmacCtx* ctx, const char* function) {
    if (!ctx) throw std::invalid_argument(std::string(function) + ": null context");
    return *ctx;
}

}

bool DigestCopyFinal(const EvpMdCtx& ctx, unsigned char* out, const char* function) {
    const auto& context = StartedContext(ctx, function);
    mbedtls_md_context_t copy;
    mbedtls_md_init(&copy);
    int status = mbedtls_md_setup(&copy, mbedtls_md_info_from_type(ctx.digest->md), 0);
    if (status == 0) status = mbedtls_md_clone(&copy, &context);
    if (status == 0) status = mbedtls_md_finish(&copy, out);
    mbedtls_md_free(&copy);
    return status == 0;
}

const EvpMd* DigestByMbedType(mbedtls_md_type_t type) {
    for (const EvpMd* md : {&Md5, &Sha1, &Sha224, &Sha256, &Sha384, &Sha512}) {
        if (md->md == type) return md;
    }
    return nullptr;
}

}

using namespace LibreSsl;

extern "C" {

const EvpMd* LibreSslSha1_nid_no_patch() {
    return &Sha1;
}

const EvpMd* LibreSslSha224_nid_no_patch() {
    return &Sha224;
}

const EvpMd* APS5_VABI EVP_md5_nid_postfix() {
    return &Md5;
}

const EvpMd* APS5_VABI EVP_sha256_nid_postfix() {
    return &Sha256;
}

const EvpMd* APS5_VABI EVP_sha384_nid_postfix() {
    return &Sha384;
}

const EvpMd* APS5_VABI EVP_sha512_nid_postfix() {
    return &Sha512;
}

int APS5_VABI EVP_MD_size_nid_postfix(const EvpMd* md) {
    if (!md) throw std::invalid_argument("EVP_MD_size: null digest");
    return md->mdSize;
}

void APS5_VABI EVP_MD_CTX_init_nid_postfix(EvpMdCtx* ctx) {
    std::memset(&RequireMdCtx(ctx, __func__), 0, sizeof(EvpMdCtx));
}

int APS5_VABI EVP_MD_CTX_cleanup_nid_postfix(EvpMdCtx* ctx) {
    auto& context = RequireMdCtx(ctx, __func__);
    ReleaseContext(context);
    FreePkeyCtx(context.pctx);
    std::memset(&context, 0, sizeof(EvpMdCtx));
    return 1;
}

EvpMdCtx* APS5_VABI EVP_MD_CTX_create_nid_postfix() {
    return new (std::nothrow) EvpMdCtx{};
}

void APS5_VABI EVP_MD_CTX_destroy_nid_postfix(EvpMdCtx* ctx) {
    if (!ctx) return;
    ReleaseContext(*ctx);
    FreePkeyCtx(ctx->pctx);
    delete ctx;
}

int APS5_VABI EVP_DigestInit_ex_nid_postfix(EvpMdCtx* ctx, const EvpMd* type, void* impl) {
    auto& context = RequireMdCtx(ctx, __func__);
    if (impl) throw std::invalid_argument("EVP_DigestInit_ex: engines are not supported");
    if (!type) type = context.digest;
    if (!type) {
        PushError(ErrLibEvp, EvpFunctionDigestInitEx, EvpReasonNoDigestSet);
        return 0;
    }
    if (!PrepareContext(context, type, false)) {
        PushError(ErrLibEvp, EvpFunctionDigestInitEx, ErrReasonMallocFailure);
        return 0;
    }
    return mbedtls_md_starts(MdContext(context)) == 0 ? 1 : 0;
}

int APS5_VABI EVP_DigestUpdate_nid_postfix(EvpMdCtx* ctx, const void* d, std::size_t cnt) {
    auto& context = StartedContext(RequireMdCtx(ctx, __func__), __func__);
    RequireData(d, cnt, __func__);
    return mbedtls_md_update(&context, static_cast<const unsigned char*>(d), cnt) == 0 ? 1 : 0;
}

int APS5_VABI EVP_DigestFinal_ex_nid_postfix(EvpMdCtx* ctx, unsigned char* md, unsigned int* s) {
    auto& wrapper = RequireMdCtx(ctx, __func__);
    auto& context = StartedContext(wrapper, __func__);
    if (!md) throw std::invalid_argument("EVP_DigestFinal_ex: null digest buffer");
    if (mbedtls_md_finish(&context, md) != 0) return 0;
    if (s) *s = static_cast<unsigned int>(wrapper.digest->mdSize);
    return 1;
}

int APS5_VABI EVP_Digest_nid_postfix(const void* data, std::size_t count, unsigned char* md, unsigned int* size, const EvpMd* type, void* impl) {
    EvpMdCtx context{};
    int result = EVP_DigestInit_ex_nid_postfix(&context, type, impl);
    if (result) result = EVP_DigestUpdate_nid_postfix(&context, data, count);
    if (result) result = EVP_DigestFinal_ex_nid_postfix(&context, md, size);
    ReleaseContext(context);
    return result;
}

void APS5_VABI HMAC_CTX_init_nid_postfix(HmacCtx* ctx) {
    std::memset(&RequireHmac(ctx, __func__), 0, sizeof(HmacCtx));
}

void APS5_VABI HMAC_CTX_cleanup_nid_postfix(HmacCtx* ctx) {
    auto& context = RequireHmac(ctx, __func__);
    ReleaseContext(context.mdCtx);
    std::memset(&context, 0, sizeof(HmacCtx));
}

int APS5_VABI HMAC_Init_ex_nid_postfix(HmacCtx* ctx, const void* key, int len, const EvpMd* md, void* impl) {
    auto& context = RequireHmac(ctx, __func__);
    if (impl) throw std::invalid_argument("HMAC_Init_ex: engines are not supported");
    if (!md) md = context.md;
    if (!md) throw std::logic_error("HMAC_Init_ex: no digest given and none set up");
    if (key) {
        if (len < 0) throw std::invalid_argument("HMAC_Init_ex: negative key length");
        if (len > md->blockSize) {
            if (mbedtls_md(mbedtls_md_info_from_type(md->md), static_cast<const unsigned char*>(key), static_cast<std::size_t>(len), context.key) != 0) return 0;
            context.keyLength = static_cast<std::uint32_t>(md->mdSize);
        } else {
            std::memcpy(context.key, key, static_cast<std::size_t>(len));
            context.keyLength = static_cast<std::uint32_t>(len);
        }
    }
    if (!PrepareContext(context.mdCtx, md, true)) return 0;
    context.md = md;
    return mbedtls_md_hmac_starts(MdContext(context.mdCtx), context.key, context.keyLength) == 0 ? 1 : 0;
}

int APS5_VABI HMAC_Init_nid_postfix(HmacCtx* ctx, const void* key, int len, const EvpMd* md) {
    if (key && md) HMAC_CTX_init_nid_postfix(ctx);
    return HMAC_Init_ex_nid_postfix(ctx, key, len, md, nullptr);
}

int APS5_VABI HMAC_Update_nid_postfix(HmacCtx* ctx, const unsigned char* data, std::size_t len) {
    auto& context = StartedContext(RequireHmac(ctx, __func__).mdCtx, __func__);
    RequireData(data, len, __func__);
    return mbedtls_md_hmac_update(&context, data, len) == 0 ? 1 : 0;
}

int APS5_VABI HMAC_Final_nid_postfix(HmacCtx* ctx, unsigned char* md, unsigned int* len) {
    auto& wrapper = RequireHmac(ctx, __func__);
    auto& context = StartedContext(wrapper.mdCtx, __func__);
    if (!md) throw std::invalid_argument("HMAC_Final: null output buffer");
    if (mbedtls_md_hmac_finish(&context, md) != 0) return 0;
    if (len) *len = static_cast<unsigned int>(wrapper.md->mdSize);
    return 1;
}

unsigned char* APS5_VABI HMAC_nid_postfix(const EvpMd* evp_md, const void* key, int key_len, const unsigned char* d, std::size_t n, unsigned char* md,
    unsigned int* md_len) {
    static unsigned char result[64];
    if (!evp_md) throw std::invalid_argument("HMAC: null digest");
    if (key_len < 0 || (!key && key_len)) throw std::invalid_argument("HMAC: invalid key");
    RequireData(d, n, __func__);
    static constexpr unsigned char empty[1] = {};
    unsigned char* output = md ? md : result;
    if (mbedtls_md_hmac(mbedtls_md_info_from_type(evp_md->md), key ? static_cast<const unsigned char*>(key) : empty, static_cast<std::size_t>(key_len),
            d ? d : empty, n, output) != 0)
        return nullptr;
    if (md_len) *md_len = static_cast<unsigned int>(evp_md->mdSize);
    return output;
}

}
