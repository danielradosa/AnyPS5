#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <mbedtls/aes.h>
#include <mbedtls/cipher.h>
#include <mbedtls/cmac.h>
#include <mbedtls/gcm.h>
#include <mbedtls/nist_kw.h>

#include <algorithm>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace LibreSsl {

namespace {

constexpr int EvpFunctionCipherInitEx = 123;
constexpr int EvpFunctionCipherCtxCtrl = 124;
constexpr int EvpFunctionCipherCtxSetKeyLength = 122;
constexpr int EvpFunctionDecryptFinalEx = 101;
constexpr int EvpFunctionEncryptFinalEx = 127;
constexpr int EvpReasonBadDecrypt = 100;
constexpr int EvpReasonWrongFinalBlockLength = 109;
constexpr int EvpReasonInvalidKeyLength = 130;
constexpr int EvpReasonNoCipherSet = 131;
constexpr int EvpReasonCtrlNotImplemented = 132;
constexpr int EvpReasonInitializationError = 134;
constexpr int EvpReasonDataNotMultipleOfBlockLength = 138;
constexpr int EvpReasonWrapModeNotAllowed = 170;

constexpr std::uint64_t FlagNoPadding = 0x100;

constexpr int CtrlInit = 0x0;
constexpr int CtrlGcmSetIvLength = 0x9;
constexpr int CtrlGcmGetTag = 0x10;
constexpr int CtrlGcmSetTag = 0x11;

enum class Mode { Cbc, Cfb8, Ctr, Gcm, Wrap };

}

struct EvpCipher {
    int nid;
    int blockSize;
    int keyLength;
    int ivLength;
    Mode mode;
};

namespace {

constexpr EvpCipher Aes128Cbc{419, 16, 16, 16, Mode::Cbc};
constexpr EvpCipher Aes192Cbc{423, 16, 24, 16, Mode::Cbc};
constexpr EvpCipher Aes256Cbc{427, 16, 32, 16, Mode::Cbc};
constexpr EvpCipher Aes128Cfb8{653, 1, 16, 16, Mode::Cfb8};
constexpr EvpCipher Aes192Cfb8{654, 1, 24, 16, Mode::Cfb8};
constexpr EvpCipher Aes256Cfb8{655, 1, 32, 16, Mode::Cfb8};
constexpr EvpCipher Aes128Ctr{904, 1, 16, 16, Mode::Ctr};
constexpr EvpCipher Aes192Ctr{905, 1, 24, 16, Mode::Ctr};
constexpr EvpCipher Aes256Ctr{906, 1, 32, 16, Mode::Ctr};
constexpr EvpCipher Aes128Gcm{895, 1, 16, 12, Mode::Gcm};
constexpr EvpCipher Aes192Gcm{898, 1, 24, 12, Mode::Gcm};
constexpr EvpCipher Aes256Gcm{901, 1, 32, 12, Mode::Gcm};
constexpr EvpCipher Aes128Wrap{788, 8, 16, 8, Mode::Wrap};
constexpr EvpCipher Aes192Wrap{789, 8, 24, 8, Mode::Wrap};
constexpr EvpCipher Aes256Wrap{790, 8, 32, 8, Mode::Wrap};
constexpr unsigned char WrapDefaultIv[8] = {0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6};

struct CipherState {
    mbedtls_aes_context aes;
    mbedtls_gcm_context gcm;
    mbedtls_nist_kw_context kw;
    bool keySet = false;
    std::size_t ivLength = 12;
    std::vector<unsigned char> iv;
    bool ivSet = false;
    bool started = false;
    unsigned char tag[16]{};
    bool tagReady = false;
    int expectedLength = -1;
    unsigned char expected[16]{};

    CipherState() {
        mbedtls_aes_init(&aes);
        mbedtls_gcm_init(&gcm);
        mbedtls_nist_kw_init(&kw);
    }

    ~CipherState() {
        mbedtls_aes_free(&aes);
        mbedtls_gcm_free(&gcm);
        mbedtls_nist_kw_free(&kw);
    }

    CipherState(const CipherState&) = delete;
    CipherState& operator=(const CipherState&) = delete;
};

EvpCipherCtx& RequireCtx(EvpCipherCtx* ctx, const char* function) {
    if (!ctx) throw std::invalid_argument(std::string(function) + ": null context");
    return *ctx;
}

CipherState* State(const EvpCipherCtx& ctx) {
    return static_cast<CipherState*>(ctx.cipherData);
}

void Release(EvpCipherCtx& ctx) {
    delete State(ctx);
    ctx.cipherData = nullptr;
}

CipherState& Ready(const EvpCipherCtx& ctx, const char* function) {
    auto* state = State(ctx);
    if (!ctx.cipher || !state) throw std::logic_error(std::string(function) + ": the context has no cipher set up");
    return *state;
}

bool Overlaps(const unsigned char* out, const unsigned char* in, std::size_t length, std::size_t slack) {
    return out < in + length && in < out + length + slack;
}

bool StartGcm(EvpCipherCtx& ctx, CipherState& state) {
    if (!state.keySet || !state.ivSet) return true;
    state.tagReady = false;
    state.started = mbedtls_gcm_starts(&state.gcm, ctx.encrypt ? MBEDTLS_GCM_ENCRYPT : MBEDTLS_GCM_DECRYPT, state.iv.data(), state.iv.size()) == 0;
    return state.started;
}

int CipherInit(EvpCipherCtx* context, const EvpCipher* cipher, void* impl, const unsigned char* key, const unsigned char* iv, int enc, const char* function) {
    auto& ctx = RequireCtx(context, function);
    if (impl) throw std::invalid_argument(std::string(function) + ": engines are not supported");
    if (enc != -1) ctx.encrypt = enc ? 1 : 0;
    if (cipher && cipher->mode == Mode::Wrap && !(ctx.flags & CipherCtxFlagWrapAllow)) {
        PushError(ErrLibEvp, EvpFunctionCipherInitEx, EvpReasonWrapModeNotAllowed);
        return 0;
    }
    if (cipher) {
        const auto flags = ctx.flags;
        const auto encrypt = ctx.encrypt;
        Release(ctx);
        std::memset(&ctx, 0, sizeof(EvpCipherCtx));
        ctx.flags = flags;
        ctx.encrypt = encrypt;
        auto* state = new (std::nothrow) CipherState;
        if (!state) {
            PushError(ErrLibEvp, EvpFunctionCipherInitEx, ErrReasonMallocFailure);
            return 0;
        }
        ctx.cipher = cipher;
        ctx.cipherData = state;
        ctx.keyLen = cipher->keyLength;
        ctx.blockMask = cipher->blockSize - 1;
    } else if (!ctx.cipher) {
        PushError(ErrLibEvp, EvpFunctionCipherInitEx, EvpReasonNoCipherSet);
        return 0;
    }
    auto& state = Ready(ctx, function);
    const auto* chosen = ctx.cipher;
    if (chosen->mode == Mode::Gcm) {
        if (key) {
            if (mbedtls_gcm_setkey(&state.gcm, MBEDTLS_CIPHER_ID_AES, key, static_cast<unsigned int>(chosen->keyLength * 8)) != 0) {
                PushError(ErrLibEvp, EvpFunctionCipherInitEx, EvpReasonInitializationError);
                return 0;
            }
            state.keySet = true;
        }
        if (iv) {
            state.iv.assign(iv, iv + state.ivLength);
            std::memcpy(ctx.iv, iv, std::min<std::size_t>(state.ivLength, sizeof(ctx.iv)));
            state.ivSet = true;
        }
        if ((key || iv) && !StartGcm(ctx, state)) return 0;
        ctx.bufLen = 0;
        return 1;
    }
    if (chosen->mode == Mode::Wrap) {
        if (iv && std::memcmp(iv, WrapDefaultIv, sizeof(WrapDefaultIv)) != 0) throw std::invalid_argument(std::string(function) + ": only the RFC 3394 default IV is supported");
        if (key) {
            if (mbedtls_nist_kw_setkey(&state.kw, MBEDTLS_CIPHER_ID_AES, key, static_cast<unsigned int>(chosen->keyLength * 8), ctx.encrypt ? 1 : 0) != 0) {
                PushError(ErrLibEvp, EvpFunctionCipherInitEx, EvpReasonInitializationError);
                return 0;
            }
            state.keySet = true;
        }
        ctx.bufLen = 0;
        return 1;
    }
    if (chosen->mode == Mode::Ctr) {
        ctx.num = 0;
        if (iv) std::memcpy(ctx.iv, iv, 16);
    } else {
        if (iv) std::memcpy(ctx.oiv, iv, 16);
        std::memcpy(ctx.iv, ctx.oiv, 16);
        ctx.num = 0;
    }
    if (key) {
        const bool decryptSchedule = chosen->mode == Mode::Cbc && !ctx.encrypt;
        const auto bits = static_cast<unsigned int>(chosen->keyLength * 8);
        const int result = decryptSchedule ? mbedtls_aes_setkey_dec(&state.aes, key, bits) : mbedtls_aes_setkey_enc(&state.aes, key, bits);
        if (result != 0) {
            PushError(ErrLibEvp, EvpFunctionCipherInitEx, EvpReasonInitializationError);
            return 0;
        }
        state.keySet = true;
    }
    ctx.bufLen = 0;
    ctx.finalUsed = 0;
    return 1;
}

void RequireKeyed(const CipherState& state, const char* function) {
    if (!state.keySet) throw std::logic_error(std::string(function) + ": no key was set");
}

bool CbcBlocks(EvpCipherCtx& ctx, CipherState& state, const unsigned char* in, unsigned char* out, std::size_t length) {
    return mbedtls_aes_crypt_cbc(&state.aes, ctx.encrypt ? MBEDTLS_AES_ENCRYPT : MBEDTLS_AES_DECRYPT, length, ctx.iv, in, out) == 0;
}

bool StreamUpdate(EvpCipherCtx& ctx, CipherState& state, unsigned char* out, const unsigned char* in, std::size_t length) {
    if (ctx.cipher->mode == Mode::Cfb8) {
        return mbedtls_aes_crypt_cfb8(&state.aes, ctx.encrypt ? MBEDTLS_AES_ENCRYPT : MBEDTLS_AES_DECRYPT, length, ctx.iv, in, out) == 0;
    }
    std::size_t offset = static_cast<std::size_t>(ctx.num);
    const bool result = mbedtls_aes_crypt_ctr(&state.aes, length, &offset, ctx.iv, ctx.buf, in, out) == 0;
    ctx.num = static_cast<int>(offset);
    return result;
}

int BlockUpdate(EvpCipherCtx& ctx, CipherState& state, unsigned char* out, int* outl, const unsigned char* in, std::size_t length) {
    std::vector<unsigned char> copy;
    if (ctx.bufLen && Overlaps(out, in, length, 16)) {
        copy.assign(in, in + length);
        in = copy.data();
    }
    int produced = 0;
    if (ctx.bufLen) {
        const std::size_t need = 16 - static_cast<std::size_t>(ctx.bufLen);
        if (length < need) {
            std::memcpy(ctx.buf + ctx.bufLen, in, length);
            ctx.bufLen += static_cast<int>(length);
            *outl = 0;
            return 1;
        }
        std::memcpy(ctx.buf + ctx.bufLen, in, need);
        if (!CbcBlocks(ctx, state, ctx.buf, out, 16)) return 0;
        ctx.bufLen = 0;
        in += need;
        length -= need;
        out += 16;
        produced = 16;
    }
    const std::size_t whole = length & ~static_cast<std::size_t>(15);
    if (whole && !CbcBlocks(ctx, state, in, out, whole)) return 0;
    std::memcpy(ctx.buf, in + whole, length - whole);
    ctx.bufLen = static_cast<int>(length - whole);
    *outl = produced + static_cast<int>(whole);
    return 1;
}

int GcmUpdate(CipherState& state, unsigned char* out, int* outl, const unsigned char* in, std::size_t length) {
    if (!state.started) return 0;
    if (!out) {
        if (mbedtls_gcm_update_ad(&state.gcm, in, length) != 0) return 0;
        *outl = static_cast<int>(length);
        return 1;
    }
    std::size_t written = 0;
    if (mbedtls_gcm_update(&state.gcm, in, length, out, length, &written) != 0) return 0;
    if (written != length) throw std::runtime_error("GCM update: Mbed TLS buffered output");
    *outl = static_cast<int>(length);
    return 1;
}

int WrapUpdate(EvpCipherCtx& ctx, CipherState& state, unsigned char* out, int* outl, const unsigned char* in, std::size_t length) {
    if (length % 8 || length < (ctx.encrypt ? 16u : 24u)) return 0;
    std::vector<unsigned char> copy(in, in + length);
    std::size_t written = 0;
    const std::size_t capacity = ctx.encrypt ? length + 8 : length - 8;
    const int result = ctx.encrypt ? mbedtls_nist_kw_wrap(&state.kw, MBEDTLS_KW_MODE_KW, copy.data(), length, out, &written, capacity)
                                   : mbedtls_nist_kw_unwrap(&state.kw, MBEDTLS_KW_MODE_KW, copy.data(), length, out, &written, capacity);
    if (result != 0) return 0;
    *outl = static_cast<int>(written);
    return 1;
}

int Update(EvpCipherCtx* context, unsigned char* out, int* outl, const unsigned char* in, int inl, bool encrypt, const char* function) {
    auto& ctx = RequireCtx(context, function);
    auto& state = Ready(ctx, function);
    if (!outl) throw std::invalid_argument(std::string(function) + ": null output length");
    if (inl < 0) throw std::invalid_argument(std::string(function) + ": negative input length");
    if (!in && inl) throw std::invalid_argument(std::string(function) + ": null input");
    if ((ctx.encrypt != 0) != encrypt) throw std::logic_error(std::string(function) + ": the context was set up for the other direction");
    const auto length = static_cast<std::size_t>(inl);
    if (ctx.cipher->mode == Mode::Gcm) return GcmUpdate(state, out, outl, in, length);
    if (!out && inl) throw std::invalid_argument(std::string(function) + ": null output");
    RequireKeyed(state, function);
    if (ctx.cipher->mode == Mode::Wrap) return WrapUpdate(ctx, state, out, outl, in, length);
    if (inl == 0) {
        *outl = 0;
        return 1;
    }
    if (ctx.cipher->mode != Mode::Cbc) {
        if (!StreamUpdate(ctx, state, out, in, length)) return 0;
        *outl = inl;
        return 1;
    }
    if (encrypt || (ctx.flags & FlagNoPadding)) return BlockUpdate(ctx, state, out, outl, in, length);
    std::vector<unsigned char> copy;
    if (ctx.finalUsed && Overlaps(out, in, length, 32)) {
        copy.assign(in, in + length);
        in = copy.data();
    }
    int held = 0;
    if (ctx.finalUsed) {
        std::memcpy(out, ctx.finalBlock, 16);
        out += 16;
        held = 16;
    }
    int produced = 0;
    if (!BlockUpdate(ctx, state, out, &produced, in, length)) return 0;
    if (ctx.bufLen == 0) {
        produced -= 16;
        std::memcpy(ctx.finalBlock, out + produced, 16);
        ctx.finalUsed = 1;
    } else {
        ctx.finalUsed = 0;
    }
    *outl = produced + held;
    return 1;
}

int EncryptFinal(EvpCipherCtx* context, unsigned char* out, int* outl, const char* function) {
    auto& ctx = RequireCtx(context, function);
    auto& state = Ready(ctx, function);
    if (!outl) throw std::invalid_argument(std::string(function) + ": null output length");
    if (!ctx.encrypt) throw std::logic_error(std::string(function) + ": the context was set up for decryption");
    *outl = 0;
    if (ctx.cipher->mode == Mode::Gcm) {
        if (!state.started) return 0;
        std::size_t written = 0;
        if (mbedtls_gcm_finish(&state.gcm, nullptr, 0, &written, state.tag, sizeof(state.tag)) != 0) return 0;
        state.started = false;
        state.ivSet = false;
        state.tagReady = true;
        return 1;
    }
    if (ctx.cipher->mode != Mode::Cbc) return 1;
    if (ctx.flags & FlagNoPadding) {
        if (ctx.bufLen) {
            PushError(ErrLibEvp, EvpFunctionEncryptFinalEx, EvpReasonDataNotMultipleOfBlockLength);
            return 0;
        }
        return 1;
    }
    RequireKeyed(state, function);
    if (!out) throw std::invalid_argument(std::string(function) + ": null output");
    const auto pad = static_cast<unsigned char>(16 - ctx.bufLen);
    std::memset(ctx.buf + ctx.bufLen, pad, pad);
    if (!CbcBlocks(ctx, state, ctx.buf, out, 16)) return 0;
    ctx.bufLen = 0;
    *outl = 16;
    return 1;
}

int DecryptFinal(EvpCipherCtx* context, unsigned char* out, int* outl, const char* function) {
    auto& ctx = RequireCtx(context, function);
    auto& state = Ready(ctx, function);
    if (!outl) throw std::invalid_argument(std::string(function) + ": null output length");
    if (ctx.encrypt) throw std::logic_error(std::string(function) + ": the context was set up for encryption");
    *outl = 0;
    if (ctx.cipher->mode == Mode::Gcm) {
        if (!state.started || state.expectedLength < 0) return 0;
        unsigned char computed[16];
        std::size_t written = 0;
        const bool finished = mbedtls_gcm_finish(&state.gcm, nullptr, 0, &written, computed, sizeof(computed)) == 0;
        state.started = false;
        state.ivSet = false;
        if (!finished) return 0;
        unsigned char difference = 0;
        for (int index = 0; index < state.expectedLength; ++index) difference |= computed[index] ^ state.expected[index];
        return difference == 0 ? 1 : 0;
    }
    if (ctx.cipher->mode != Mode::Cbc) return 1;
    if (ctx.flags & FlagNoPadding) {
        if (ctx.bufLen) {
            PushError(ErrLibEvp, EvpFunctionDecryptFinalEx, EvpReasonDataNotMultipleOfBlockLength);
            return 0;
        }
        return 1;
    }
    if (ctx.bufLen || !ctx.finalUsed) {
        PushError(ErrLibEvp, EvpFunctionDecryptFinalEx, EvpReasonWrongFinalBlockLength);
        return 0;
    }
    const unsigned char pad = ctx.finalBlock[15];
    bool valid = pad >= 1 && pad <= 16;
    for (int index = 16 - (valid ? pad : 0); valid && index < 16; ++index) valid = ctx.finalBlock[index] == pad;
    if (!valid) {
        PushError(ErrLibEvp, EvpFunctionDecryptFinalEx, EvpReasonBadDecrypt);
        return 0;
    }
    if (pad < 16) {
        if (!out) throw std::invalid_argument(std::string(function) + ": null output");
        std::memcpy(out, ctx.finalBlock, 16 - pad);
    }
    ctx.finalUsed = 0;
    *outl = 16 - pad;
    return 1;
}

int GcmControl(EvpCipherCtx& ctx, CipherState& state, int type, int arg, void* ptr) {
    switch (type) {
    case CtrlInit:
        state.ivLength = 12;
        state.iv.clear();
        state.ivSet = false;
        state.started = false;
        state.tagReady = false;
        state.expectedLength = -1;
        return 1;
    case CtrlGcmSetIvLength:
        if (arg <= 0) return 0;
        state.ivLength = static_cast<std::size_t>(arg);
        return 1;
    case CtrlGcmGetTag:
        if (arg <= 0 || arg > 16 || !ctx.encrypt || !state.tagReady) return 0;
        if (!ptr) throw std::invalid_argument("EVP_CIPHER_CTX_ctrl: null tag buffer");
        std::memcpy(ptr, state.tag, static_cast<std::size_t>(arg));
        return 1;
    case CtrlGcmSetTag:
        if (arg <= 0 || arg > 16 || ctx.encrypt) return 0;
        if (!ptr) throw std::invalid_argument("EVP_CIPHER_CTX_ctrl: null tag");
        std::memcpy(state.expected, ptr, static_cast<std::size_t>(arg));
        state.expectedLength = arg;
        return 1;
    default:
        throw std::invalid_argument("EVP_CIPHER_CTX_ctrl: unsupported GCM control " + std::to_string(type));
    }
}

struct CmacCtx {
    mbedtls_cipher_context_t cipher;
    const EvpCipher* evp = nullptr;
    bool keyed = false;
    bool finished = false;

    CmacCtx() {
        mbedtls_cipher_init(&cipher);
    }

    ~CmacCtx() {
        mbedtls_cipher_free(&cipher);
    }

    CmacCtx(const CmacCtx&) = delete;
    CmacCtx& operator=(const CmacCtx&) = delete;
};

CmacCtx& RequireCmac(CmacCtx* ctx, const char* function) {
    if (!ctx) throw std::invalid_argument(std::string(function) + ": null context");
    return *ctx;
}

}

}

using namespace LibreSsl;

extern "C" {

const EvpCipher* LibreSslAesWrap_nid_no_patch(int bits) {
    if (bits == 128) return &Aes128Wrap;
    if (bits == 192) return &Aes192Wrap;
    if (bits == 256) return &Aes256Wrap;
    throw std::invalid_argument("AES key wrap: unsupported key size");
}

const EvpCipher* APS5_VABI EVP_aes_128_cbc_nid_postfix() {
    return &Aes128Cbc;
}

const EvpCipher* APS5_VABI EVP_aes_192_cbc_nid_postfix() {
    return &Aes192Cbc;
}

const EvpCipher* APS5_VABI EVP_aes_256_cbc_nid_postfix() {
    return &Aes256Cbc;
}

const EvpCipher* APS5_VABI EVP_aes_128_cfb8_nid_postfix() {
    return &Aes128Cfb8;
}

const EvpCipher* APS5_VABI EVP_aes_192_cfb8_nid_postfix() {
    return &Aes192Cfb8;
}

const EvpCipher* APS5_VABI EVP_aes_256_cfb8_nid_postfix() {
    return &Aes256Cfb8;
}

const EvpCipher* APS5_VABI EVP_aes_128_ctr_nid_postfix() {
    return &Aes128Ctr;
}

const EvpCipher* APS5_VABI EVP_aes_192_ctr_nid_postfix() {
    return &Aes192Ctr;
}

const EvpCipher* APS5_VABI EVP_aes_256_ctr_nid_postfix() {
    return &Aes256Ctr;
}

const EvpCipher* APS5_VABI EVP_aes_128_gcm_nid_postfix() {
    return &Aes128Gcm;
}

const EvpCipher* APS5_VABI EVP_aes_192_gcm_nid_postfix() {
    return &Aes192Gcm;
}

const EvpCipher* APS5_VABI EVP_aes_256_gcm_nid_postfix() {
    return &Aes256Gcm;
}

int APS5_VABI EVP_CIPHER_block_size_nid_postfix(const EvpCipher* cipher) {
    if (!cipher) throw std::invalid_argument("EVP_CIPHER_block_size: null cipher");
    return cipher->blockSize;
}

void APS5_VABI EVP_CIPHER_CTX_init_nid_postfix(EvpCipherCtx* a) {
    std::memset(&RequireCtx(a, __func__), 0, sizeof(EvpCipherCtx));
}

int APS5_VABI EVP_CIPHER_CTX_cleanup_nid_postfix(EvpCipherCtx* a) {
    auto& ctx = RequireCtx(a, __func__);
    Release(ctx);
    std::memset(&ctx, 0, sizeof(EvpCipherCtx));
    return 1;
}

EvpCipherCtx* APS5_VABI EVP_CIPHER_CTX_new_nid_postfix() {
    return new (std::nothrow) EvpCipherCtx{};
}

void APS5_VABI EVP_CIPHER_CTX_free_nid_postfix(EvpCipherCtx* a) {
    if (!a) return;
    Release(*a);
    delete a;
}

int APS5_VABI EVP_CIPHER_CTX_set_padding_nid_postfix(EvpCipherCtx* c, int pad) {
    auto& ctx = RequireCtx(c, __func__);
    if (pad) ctx.flags &= ~FlagNoPadding;
    else ctx.flags |= FlagNoPadding;
    return 1;
}

int APS5_VABI EVP_CIPHER_CTX_ctrl_nid_postfix(EvpCipherCtx* ctx, int type, int arg, void* ptr) {
    auto& context = RequireCtx(ctx, __func__);
    if (!context.cipher) {
        PushError(ErrLibEvp, EvpFunctionCipherCtxCtrl, EvpReasonNoCipherSet);
        return 0;
    }
    if (context.cipher->mode != Mode::Gcm) {
        PushError(ErrLibEvp, EvpFunctionCipherCtxCtrl, EvpReasonCtrlNotImplemented);
        return 0;
    }
    return GcmControl(context, Ready(context, __func__), type, arg, ptr);
}

int APS5_VABI EVP_CipherInit_ex_nid_postfix(EvpCipherCtx* ctx, const EvpCipher* cipher, void* impl, const unsigned char* key, const unsigned char* iv,
    int enc) {
    return CipherInit(ctx, cipher, impl, key, iv, enc, __func__);
}

int APS5_VABI EVP_EncryptInit_ex_nid_postfix(EvpCipherCtx* ctx, const EvpCipher* cipher, void* impl, const unsigned char* key, const unsigned char* iv) {
    return CipherInit(ctx, cipher, impl, key, iv, 1, __func__);
}

int APS5_VABI EVP_DecryptInit_ex_nid_postfix(EvpCipherCtx* ctx, const EvpCipher* cipher, void* impl, const unsigned char* key, const unsigned char* iv) {
    return CipherInit(ctx, cipher, impl, key, iv, 0, __func__);
}

int APS5_VABI EVP_EncryptUpdate_nid_postfix(EvpCipherCtx* ctx, unsigned char* out, int* outl, const unsigned char* in, int inl) {
    return Update(ctx, out, outl, in, inl, true, __func__);
}

int APS5_VABI EVP_DecryptUpdate_nid_postfix(EvpCipherCtx* ctx, unsigned char* out, int* outl, const unsigned char* in, int inl) {
    return Update(ctx, out, outl, in, inl, false, __func__);
}

int APS5_VABI EVP_CipherUpdate_nid_postfix(EvpCipherCtx* ctx, unsigned char* out, int* outl, const unsigned char* in, int inl) {
    return Update(ctx, out, outl, in, inl, RequireCtx(ctx, __func__).encrypt != 0, __func__);
}

int APS5_VABI EVP_EncryptFinal_ex_nid_postfix(EvpCipherCtx* ctx, unsigned char* out, int* outl) {
    return EncryptFinal(ctx, out, outl, __func__);
}

int APS5_VABI EVP_DecryptFinal_ex_nid_postfix(EvpCipherCtx* ctx, unsigned char* outm, int* outl) {
    return DecryptFinal(ctx, outm, outl, __func__);
}

int APS5_VABI EVP_CipherFinal_ex_nid_postfix(EvpCipherCtx* ctx, unsigned char* outm, int* outl) {
    return RequireCtx(ctx, __func__).encrypt ? EncryptFinal(ctx, outm, outl, __func__) : DecryptFinal(ctx, outm, outl, __func__);
}

CmacCtx* APS5_VABI CMAC_CTX_new_nid_postfix() {
    return new (std::nothrow) CmacCtx;
}

void APS5_VABI CMAC_CTX_free_nid_postfix(CmacCtx* ctx) {
    delete ctx;
}

int APS5_VABI CMAC_Init_nid_postfix(CmacCtx* ctx, const void* key, std::size_t keylen, const EvpCipher* cipher, void* impl) {
    auto& context = RequireCmac(ctx, __func__);
    if (impl) throw std::invalid_argument("CMAC_Init: engines are not supported");
    if (!key && !cipher && keylen == 0) {
        if (!context.keyed) return 0;
        if (mbedtls_cipher_cmac_reset(&context.cipher) != 0) return 0;
        context.finished = false;
        return 1;
    }
    if (cipher) {
        if (cipher->mode != Mode::Cbc) throw std::invalid_argument("CMAC_Init: only the AES-CBC ciphers are supported");
        mbedtls_cipher_free(&context.cipher);
        mbedtls_cipher_init(&context.cipher);
        context.evp = nullptr;
        context.keyed = false;
        const auto type = cipher->keyLength == 16 ? MBEDTLS_CIPHER_AES_128_ECB : cipher->keyLength == 24 ? MBEDTLS_CIPHER_AES_192_ECB : MBEDTLS_CIPHER_AES_256_ECB;
        if (mbedtls_cipher_setup(&context.cipher, mbedtls_cipher_info_from_type(type)) != 0) return 0;
        context.evp = cipher;
    }
    if (!key) return context.evp ? 1 : 0;
    if (!context.evp) return 0;
    if (keylen != static_cast<std::size_t>(context.evp->keyLength)) {
        PushError(ErrLibEvp, EvpFunctionCipherCtxSetKeyLength, EvpReasonInvalidKeyLength);
        return 0;
    }
    if (mbedtls_cipher_cmac_starts(&context.cipher, static_cast<const unsigned char*>(key), keylen * 8) != 0) return 0;
    context.keyed = true;
    context.finished = false;
    return 1;
}

int APS5_VABI CMAC_Update_nid_postfix(CmacCtx* ctx, const void* data, std::size_t dlen) {
    auto& context = RequireCmac(ctx, __func__);
    if (!context.keyed) return 0;
    if (context.finished) throw std::logic_error("CMAC_Update: CMAC_Final already ran; continuing the computation is not supported");
    if (!dlen) return 1;
    if (!data) throw std::invalid_argument("CMAC_Update: null data");
    return mbedtls_cipher_cmac_update(&context.cipher, static_cast<const unsigned char*>(data), dlen) == 0 ? 1 : 0;
}

int APS5_VABI CMAC_Final_nid_postfix(CmacCtx* ctx, unsigned char* out, std::size_t* poutlen) {
    auto& context = RequireCmac(ctx, __func__);
    if (!context.keyed) return 0;
    if (!poutlen) throw std::invalid_argument("CMAC_Final: null length");
    *poutlen = 16;
    if (!out) return 1;
    if (context.finished) throw std::logic_error("CMAC_Final: CMAC_Final already ran; repeating it is not supported");
    if (mbedtls_cipher_cmac_finish(&context.cipher, out) != 0) return 0;
    context.finished = true;
    return 1;
}

}
