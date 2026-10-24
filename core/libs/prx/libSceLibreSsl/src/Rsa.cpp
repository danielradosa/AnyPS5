#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/include/Keys.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/HostEntropy.hpp"

#include <mbedtls/md.h>
#include <mbedtls/platform_util.h>

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace LibreSsl {

namespace {

constexpr int BnFlagMalloced = 0x01;
constexpr int RsaFlagCachePublic = 0x0002;
constexpr int RsaFlagCachePrivate = 0x0004;
constexpr int RsaSslv23Padding = 2;
constexpr int RsaX931Padding = 5;
constexpr int MaxModulusBits = 8192;

struct RsaMethod {
    const char* name;
    void* operations[8];
    int flags;
    char* appData;
    void* sign;
    void* verify;
    void* keygen;
};

static_assert(sizeof(RsaMethod) == 112 && offsetof(RsaMethod, flags) == 72);

constexpr int RsaFunctionCheckKey = 123;
constexpr int RsaFunctionEayPrivateDecrypt = 101;
constexpr int RsaFunctionEayPrivateEncrypt = 102;
constexpr int RsaFunctionEayPublicDecrypt = 103;
constexpr int RsaFunctionEayPublicEncrypt = 104;
constexpr int RsaFunctionPaddingAddNone = 107;
constexpr int RsaFunctionPaddingAddType1 = 108;
constexpr int RsaFunctionPaddingAddType2 = 109;
constexpr int RsaFunctionPaddingAddOaep = 121;
constexpr int RsaFunctionPaddingAddPss = 125;
constexpr int RsaFunctionPaddingAddPssMgf1 = 148;
constexpr int RsaFunctionSign = 117;
constexpr int RsaFunctionVerify = 119;
constexpr int RsaFunctionVerifyPss = 126;

constexpr int RsaReasonBadSignature = 104;
constexpr int RsaReasonDataGreaterThanModLen = 108;
constexpr int RsaReasonDataTooLarge = 109;
constexpr int RsaReasonDataTooLargeForKeySize = 110;
constexpr int RsaReasonDigestTooBigForRsaKey = 112;
constexpr int RsaReasonPaddingCheckFailed = 114;
constexpr int RsaReasonUnknownAlgorithmType = 117;
constexpr int RsaReasonUnknownPaddingType = 118;
constexpr int RsaReasonWrongSignatureLength = 119;
constexpr int RsaReasonDataTooSmallForKeySize = 122;
constexpr int RsaReasonDENotCongruentTo1 = 123;
constexpr int RsaReasonDmp1NotCongruentToD = 124;
constexpr int RsaReasonDmq1NotCongruentToD = 125;
constexpr int RsaReasonIqmpNotInverseOfQ = 126;
constexpr int RsaReasonNDoesNotEqualPQ = 127;
constexpr int RsaReasonPNotPrime = 128;
constexpr int RsaReasonQNotPrime = 129;
constexpr int RsaReasonInvalidMessageLength = 131;
constexpr int RsaReasonDataTooLargeForModulus = 132;
constexpr int RsaReasonFirstOctetInvalid = 133;
constexpr int RsaReasonLastOctetInvalid = 134;
constexpr int RsaReasonSlenRecoveryFailed = 135;
constexpr int RsaReasonSlenCheckFailed = 136;

struct RsaCtx {
    mbedtls_rsa_context value;

    RsaCtx() {
        mbedtls_rsa_init(&value);
    }

    ~RsaCtx() {
        mbedtls_rsa_free(&value);
    }

    RsaCtx(const RsaCtx&) = delete;
    RsaCtx& operator=(const RsaCtx&) = delete;
};

void Check(int status, const char* function) {
    if (status == MBEDTLS_ERR_MPI_ALLOC_FAILED) throw std::bad_alloc();
    if (status != 0) throw std::runtime_error(std::string(function) + ": Mbed TLS failed (" + std::to_string(status) + ")");
}

Rsa& RequireRsa(Rsa* rsa, const char* function) {
    if (!rsa) throw std::invalid_argument(std::string(function) + ": null RSA");
    return *rsa;
}

void RequireBuffers(int flen, const unsigned char* from, const unsigned char* to, const char* function) {
    if (flen < 0) throw std::invalid_argument(std::string(function) + ": negative length");
    if (!from && flen) throw std::invalid_argument(std::string(function) + ": null input");
    if (!to) throw std::invalid_argument(std::string(function) + ": null output");
}

void LoadContext(const Rsa& rsa, RsaCtx& ctx, bool needPrivate, const char* function) {
    if (!RsaContext(rsa, ctx.value, needPrivate, function))
        throw std::invalid_argument(std::string(function) + ": the RSA key components are inconsistent");
}

bool BelowModulus(const RsaCtx& ctx, const unsigned char* block, std::size_t length, const char* function) {
    Mpi value, modulus;
    Check(mbedtls_mpi_read_binary(&value.value, block, length), function);
    Check(mbedtls_rsa_export(&ctx.value, &modulus.value, nullptr, nullptr, nullptr, nullptr), function);
    return mbedtls_mpi_cmp_mpi(&value.value, &modulus.value) < 0;
}

std::vector<unsigned char> InputBlock(int flen, const unsigned char* from, std::size_t k) {
    std::vector<unsigned char> block(k);
    if (flen) std::memcpy(block.data() + (k - static_cast<std::size_t>(flen)), from, static_cast<std::size_t>(flen));
    return block;
}

mbedtls_md_type_t MbedDigestByNid(int nid, const char* function, bool& known) {
    known = true;
    switch (nid) {
    case NidMd5:
        return MBEDTLS_MD_MD5;
    case NidSha1:
        return MBEDTLS_MD_SHA1;
    case NidSha224:
        return MBEDTLS_MD_SHA224;
    case NidSha256:
        return MBEDTLS_MD_SHA256;
    case NidSha384:
        return MBEDTLS_MD_SHA384;
    case NidSha512:
        return MBEDTLS_MD_SHA512;
    case NidRipemd160:
        return MBEDTLS_MD_RIPEMD160;
    case 3:
    case 95:
    case 257:
    case 809:
    case 942:
    case 943:
        throw std::invalid_argument(std::string(function) + ": digest NID " + std::to_string(nid) + " is not supported by this build");
    default:
        known = false;
        return MBEDTLS_MD_NONE;
    }
}

void Mgf1(unsigned char* mask, std::size_t length, const unsigned char* seed, std::size_t seedLength, const EvpMd& md) {
    const auto* info = mbedtls_md_info_from_type(md.md);
    std::vector<unsigned char> block(seedLength + 4);
    std::memcpy(block.data(), seed, seedLength);
    std::vector<unsigned char> digest(static_cast<std::size_t>(md.mdSize));
    std::uint32_t counter = 0;
    for (std::size_t done = 0; done < length; ++counter) {
        for (int index = 0; index < 4; ++index) block[seedLength + index] = static_cast<unsigned char>(counter >> (24 - index * 8));
        if (mbedtls_md(info, block.data(), block.size(), digest.data()) != 0) throw std::runtime_error("PKCS1 MGF1: Mbed TLS failed");
        const std::size_t take = length - done < digest.size() ? length - done : digest.size();
        std::memcpy(mask + done, digest.data(), take);
        done += take;
    }
}

std::vector<unsigned char> PssHash(const EvpMd& md, const unsigned char* mHash, const unsigned char* salt, std::size_t saltLength) {
    std::vector<unsigned char> message(8 + static_cast<std::size_t>(md.mdSize) + saltLength);
    std::memcpy(message.data() + 8, mHash, static_cast<std::size_t>(md.mdSize));
    if (saltLength) std::memcpy(message.data() + 8 + md.mdSize, salt, saltLength);
    std::vector<unsigned char> digest(static_cast<std::size_t>(md.mdSize));
    if (mbedtls_md(mbedtls_md_info_from_type(md.md), message.data(), message.size(), digest.data()) != 0) throw std::runtime_error("PKCS1 PSS: Mbed TLS failed");
    return digest;
}

int ModulusBits(const Rsa& rsa, const char* function) {
    if (!rsa.n) throw std::invalid_argument(std::string(function) + ": the RSA key has no modulus");
    const auto bytes = BignumBytes(*rsa.n);
    if (bytes.empty()) throw std::invalid_argument(std::string(function) + ": the RSA modulus is zero");
    int bits = static_cast<int>(bytes.size()) * 8;
    for (unsigned char top = bytes[0]; !(top & 0x80); top <<= 1) --bits;
    return bits;
}

bool PrimeCheck(const mbedtls_mpi& value) {
    const int status = mbedtls_mpi_is_prime_ext(&value, 0, RandomBytes, nullptr);
    if (status == MBEDTLS_ERR_MPI_ALLOC_FAILED) throw std::bad_alloc();
    return status == 0;
}

}

int RandomBytes(void*, unsigned char* output, std::size_t length) {
    if (length) HostEntropy_nid_no_patch(output, length);
    return 0;
}

void ReadMpi(const Bignum& bn, mbedtls_mpi& out, const char* function) {
    if (bn.neg) throw std::invalid_argument(std::string(function) + ": negative BIGNUM in a key");
    const auto bytes = BignumBytes(bn);
    Check(mbedtls_mpi_read_binary(&out, bytes.data(), bytes.size()), function);
}

void WriteMpi(Bignum& bn, const mbedtls_mpi& in, const char* function) {
    if (mbedtls_mpi_cmp_int(&in, 0) < 0) throw std::logic_error(std::string(function) + ": negative key value");
    std::vector<unsigned char> bytes(mbedtls_mpi_size(&in));
    Check(mbedtls_mpi_write_binary(&in, bytes.data(), bytes.size()), function);
    SetBignumBytes(bn, bytes.data(), bytes.size());
}

Bignum* MpiBignum(const mbedtls_mpi& in, const char* function) {
    auto* bn = NewBignum();
    if (!bn) throw std::bad_alloc();
    try {
        WriteMpi(*bn, in, function);
    } catch (...) {
        FreeBignum(bn);
        throw;
    }
    return bn;
}

void ClearFreeBignum(Bignum* bn) {
    if (!bn) return;
    if (!(bn->flags & BnFlagMalloced)) throw std::invalid_argument("RSA/EC_KEY free: a key BIGNUM was not created by BN_new");
    if (bn->d && bn->dmax > 0) mbedtls_platform_zeroize(bn->d, static_cast<std::size_t>(bn->dmax) * sizeof(std::uint64_t));
    FreeBignum(bn);
}

const EvpMd* DigestByNid(int nid) {
    switch (nid) {
    case NidMd5:
        return DigestByMbedType(MBEDTLS_MD_MD5);
    case NidSha1:
        return DigestByMbedType(MBEDTLS_MD_SHA1);
    case NidSha224:
        return DigestByMbedType(MBEDTLS_MD_SHA224);
    case NidSha256:
        return DigestByMbedType(MBEDTLS_MD_SHA256);
    case NidSha384:
        return DigestByMbedType(MBEDTLS_MD_SHA384);
    case NidSha512:
        return DigestByMbedType(MBEDTLS_MD_SHA512);
    default:
        return nullptr;
    }
}

Rsa* NewRsa() {
    auto* rsa = new (std::nothrow) Rsa{};
    if (!rsa) return nullptr;
    rsa->references = 1;
    rsa->flags = RsaFlagCachePublic | RsaFlagCachePrivate;
    return rsa;
}

void RsaUpRef(Rsa* rsa) {
    std::atomic_ref<int>(RequireRsa(rsa, "RSA_up_ref").references).fetch_add(1);
}

void FreeRsa(Rsa* rsa) {
    if (!rsa) return;
    if (std::atomic_ref<int>(rsa->references).fetch_sub(1) > 1) return;
    for (Bignum* component : {rsa->n, rsa->e, rsa->d, rsa->p, rsa->q, rsa->dmp1, rsa->dmq1, rsa->iqmp}) ClearFreeBignum(component);
    delete rsa;
}

bool RsaContext(const Rsa& rsa, mbedtls_rsa_context& ctx, bool needPrivate, const char* function) {
    if (!rsa.n || !rsa.e) throw std::invalid_argument(std::string(function) + ": the RSA key has no modulus or public exponent");
    if (needPrivate && !rsa.d) throw std::invalid_argument(std::string(function) + ": the RSA key has no private exponent");
    if (ModulusBits(rsa, function) > MaxModulusBits) throw std::invalid_argument(std::string(function) + ": RSA moduli above 8192 bits are not supported");
    Mpi n, e, d, p, q;
    ReadMpi(*rsa.n, n.value, function);
    ReadMpi(*rsa.e, e.value, function);
    const bool factors = needPrivate && rsa.p && rsa.q;
    if (needPrivate) ReadMpi(*rsa.d, d.value, function);
    if (factors) {
        ReadMpi(*rsa.p, p.value, function);
        ReadMpi(*rsa.q, q.value, function);
    }
    if (mbedtls_rsa_import(&ctx, &n.value, factors ? &p.value : nullptr, factors ? &q.value : nullptr, needPrivate ? &d.value : nullptr, &e.value) != 0) return false;
    if (mbedtls_rsa_complete(&ctx) != 0) return false;
    return needPrivate ? mbedtls_rsa_check_privkey(&ctx) == 0 : mbedtls_rsa_check_pubkey(&ctx) == 0;
}

void FillRsa(Rsa& rsa, const mbedtls_rsa_context& ctx, const char* function) {
    Mpi n, p, q, d, e, dp, dq, qp;
    Check(mbedtls_rsa_export(&ctx, &n.value, nullptr, nullptr, nullptr, &e.value), function);
    const bool isPrivate = mbedtls_rsa_export(&ctx, nullptr, &p.value, &q.value, &d.value, nullptr) == 0 && mbedtls_mpi_cmp_int(&d.value, 0) != 0;
    if (isPrivate) Check(mbedtls_rsa_export_crt(&ctx, &dp.value, &dq.value, &qp.value), function);
    const auto set = [&](Bignum*& target, const Mpi& value, bool present) {
        if (!present) {
            ClearFreeBignum(target);
            target = nullptr;
            return;
        }
        if (target) WriteMpi(*target, value.value, function);
        else target = MpiBignum(value.value, function);
    };
    set(rsa.n, n, true);
    set(rsa.e, e, true);
    set(rsa.d, d, isPrivate);
    set(rsa.p, p, isPrivate);
    set(rsa.q, q, isPrivate);
    set(rsa.dmp1, dp, isPrivate);
    set(rsa.dmq1, dq, isPrivate);
    set(rsa.iqmp, qp, isPrivate);
}

int RsaSize(const Rsa& rsa, const char* function) {
    return (ModulusBits(rsa, function) + 7) / 8;
}

int RsaPublicEncrypt(int flen, const unsigned char* from, unsigned char* to, Rsa& rsa, int padding) {
    constexpr const char* function = "RSA_public_encrypt";
    RequireBuffers(flen, from, to, function);
    RsaCtx ctx;
    LoadContext(rsa, ctx, false, function);
    const int k = static_cast<int>(mbedtls_rsa_get_len(&ctx.value));
    std::vector<unsigned char> output(static_cast<std::size_t>(k));
    const std::vector<unsigned char> input(from, from + flen);
    int status = 0;
    switch (padding) {
    case RsaPkcs1Padding:
        if (flen > k - 11) {
            PushError(ErrLibRsa, RsaFunctionPaddingAddType2, RsaReasonDataTooLargeForKeySize);
            return -1;
        }
        status = mbedtls_rsa_rsaes_pkcs1_v15_encrypt(&ctx.value, RandomBytes, nullptr, input.size(), input.data(), output.data());
        break;
    case RsaPkcs1OaepPadding:
        if (flen > k - 42) {
            PushError(ErrLibRsa, RsaFunctionPaddingAddOaep, RsaReasonDataTooLargeForKeySize);
            return -1;
        }
        Check(mbedtls_rsa_set_padding(&ctx.value, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1), function);
        status = mbedtls_rsa_rsaes_oaep_encrypt(&ctx.value, RandomBytes, nullptr, nullptr, 0, input.size(), input.data(), output.data());
        break;
    case RsaNoPadding:
        if (flen != k) {
            PushError(ErrLibRsa, RsaFunctionPaddingAddNone, flen > k ? RsaReasonDataTooLargeForKeySize : RsaReasonDataTooSmallForKeySize);
            return -1;
        }
        if (!BelowModulus(ctx, input.data(), input.size(), function)) {
            PushError(ErrLibRsa, RsaFunctionEayPublicEncrypt, RsaReasonDataTooLargeForModulus);
            return -1;
        }
        status = mbedtls_rsa_public(&ctx.value, input.data(), output.data());
        break;
    case RsaSslv23Padding:
    case RsaX931Padding:
        throw std::invalid_argument("RSA_public_encrypt: SSLv23 and X9.31 padding are not supported");
    default:
        PushError(ErrLibRsa, RsaFunctionEayPublicEncrypt, RsaReasonUnknownPaddingType);
        return -1;
    }
    Check(status, function);
    std::memcpy(to, output.data(), output.size());
    return k;
}

int RsaPrivateDecrypt(int flen, const unsigned char* from, unsigned char* to, Rsa& rsa, int padding) {
    constexpr const char* function = "RSA_private_decrypt";
    RequireBuffers(flen, from, to, function);
    RsaCtx ctx;
    LoadContext(rsa, ctx, true, function);
    const int k = static_cast<int>(mbedtls_rsa_get_len(&ctx.value));
    if (padding == RsaSslv23Padding || padding == RsaX931Padding) throw std::invalid_argument("RSA_private_decrypt: SSLv23 and X9.31 padding are not supported");
    if (padding != RsaPkcs1Padding && padding != RsaPkcs1OaepPadding && padding != RsaNoPadding) {
        PushError(ErrLibRsa, RsaFunctionEayPrivateDecrypt, RsaReasonUnknownPaddingType);
        return -1;
    }
    if (flen > k) {
        PushError(ErrLibRsa, RsaFunctionEayPrivateDecrypt, RsaReasonDataGreaterThanModLen);
        return -1;
    }
    const auto block = InputBlock(flen, from, static_cast<std::size_t>(k));
    if (!BelowModulus(ctx, block.data(), block.size(), function)) {
        PushError(ErrLibRsa, RsaFunctionEayPrivateDecrypt, RsaReasonDataTooLargeForModulus);
        return -1;
    }
    std::vector<unsigned char> output(static_cast<std::size_t>(k));
    std::size_t length = output.size();
    int status = 0;
    if (padding == RsaNoPadding) {
        status = mbedtls_rsa_private(&ctx.value, RandomBytes, nullptr, block.data(), output.data());
    } else {
        if (padding == RsaPkcs1OaepPadding) {
            Check(mbedtls_rsa_set_padding(&ctx.value, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1), function);
            status = mbedtls_rsa_rsaes_oaep_decrypt(&ctx.value, RandomBytes, nullptr, nullptr, 0, &length, block.data(), output.data(), output.size());
        } else {
            status = mbedtls_rsa_rsaes_pkcs1_v15_decrypt(&ctx.value, RandomBytes, nullptr, &length, block.data(), output.data(), output.size());
        }
        if (status == MBEDTLS_ERR_RSA_INVALID_PADDING) {
            PushError(ErrLibRsa, RsaFunctionEayPrivateDecrypt, RsaReasonPaddingCheckFailed);
            return -1;
        }
    }
    Check(status, function);
    if (length) std::memcpy(to, output.data(), length);
    return static_cast<int>(length);
}

int RsaSignDigest(int type, const unsigned char* m, unsigned int length, unsigned char* sig, unsigned int* siglen, Rsa& rsa, const char* function) {
    if (!m && length) throw std::invalid_argument(std::string(function) + ": null digest");
    if (!sig || !siglen) throw std::invalid_argument(std::string(function) + ": null signature output");
    bool known = true;
    mbedtls_md_type_t md = MBEDTLS_MD_NONE;
    if (type == NidMd5Sha1) {
        if (length != 36) {
            PushError(ErrLibRsa, RsaFunctionSign, RsaReasonInvalidMessageLength);
            return 0;
        }
    } else {
        md = MbedDigestByNid(type, function, known);
        if (!known) {
            PushError(ErrLibRsa, RsaFunctionSign, RsaReasonUnknownAlgorithmType);
            return 0;
        }
        if (length != mbedtls_md_get_size_from_type(md))
            throw std::invalid_argument(std::string(function) + ": a digest length other than the algorithm's is not supported");
    }
    RsaCtx ctx;
    LoadContext(rsa, ctx, true, function);
    std::vector<unsigned char> output(mbedtls_rsa_get_len(&ctx.value));
    const std::vector<unsigned char> digest(m, m + length);
    const int status = mbedtls_rsa_rsassa_pkcs1_v15_sign(&ctx.value, RandomBytes, nullptr, md, length, digest.data(), output.data());
    if (status == MBEDTLS_ERR_RSA_BAD_INPUT_DATA) {
        PushError(ErrLibRsa, RsaFunctionSign, RsaReasonDigestTooBigForRsaKey);
        return 0;
    }
    Check(status, function);
    std::memcpy(sig, output.data(), output.size());
    *siglen = static_cast<unsigned int>(output.size());
    return 1;
}

int RsaVerifyDigest(int type, const unsigned char* m, unsigned int length, const unsigned char* sig, unsigned int siglen, Rsa& rsa, const char* function) {
    if (!m && length) throw std::invalid_argument(std::string(function) + ": null digest");
    if (!sig && siglen) throw std::invalid_argument(std::string(function) + ": null signature");
    RsaCtx ctx;
    LoadContext(rsa, ctx, false, function);
    const std::size_t k = mbedtls_rsa_get_len(&ctx.value);
    if (siglen != k) {
        PushError(ErrLibRsa, RsaFunctionVerify, RsaReasonWrongSignatureLength);
        return 0;
    }
    bool known = true;
    mbedtls_md_type_t md = MBEDTLS_MD_NONE;
    if (type == NidMd5Sha1) {
        if (length != 36) {
            PushError(ErrLibRsa, RsaFunctionVerify, RsaReasonInvalidMessageLength);
            return 0;
        }
    } else {
        md = MbedDigestByNid(type, function, known);
        if (!known) {
            PushError(ErrLibRsa, RsaFunctionVerify, RsaReasonUnknownAlgorithmType);
            return 0;
        }
        if (length != mbedtls_md_get_size_from_type(md))
            throw std::invalid_argument(std::string(function) + ": a digest length other than the algorithm's is not supported");
    }
    if (!BelowModulus(ctx, sig, siglen, function)) {
        PushError(ErrLibRsa, RsaFunctionVerify, RsaReasonDataTooLargeForModulus);
        return 0;
    }
    const std::vector<unsigned char> digest(m, m + length);
    const int status = mbedtls_rsa_rsassa_pkcs1_v15_verify(&ctx.value, md, length, digest.data(), sig);
    if (status == 0) return 1;
    if (status == MBEDTLS_ERR_RSA_VERIFY_FAILED || status == MBEDTLS_ERR_RSA_INVALID_PADDING) {
        PushError(ErrLibRsa, RsaFunctionVerify, RsaReasonBadSignature);
        return 0;
    }
    Check(status, function);
    return 0;
}

int RsaPssEncode(Rsa& rsa, unsigned char* em, const unsigned char* mHash, const EvpMd* hash, const EvpMd* mgf1Hash, int sLen) {
    const int function = mgf1Hash == hash ? RsaFunctionPaddingAddPss : RsaFunctionPaddingAddPssMgf1;
    constexpr const char* name = "RSA_padding_add_PKCS1_PSS";
    if (!em || !mHash || !hash || !mgf1Hash) throw std::invalid_argument(std::string(name) + ": null argument");
    const int modBits = ModulusBits(rsa, name);
    const int hLen = hash->mdSize;
    const int msBits = (modBits - 1) & 7;
    const int total = (modBits + 7) / 8;
    int emLen = total;
    unsigned char* out = em;
    if (msBits == 0) {
        *out++ = 0;
        --emLen;
    }
    if (sLen == -1) sLen = hLen;
    else if (sLen == -2) sLen = emLen - hLen - 2;
    else if (sLen < -2) {
        PushError(ErrLibRsa, function, RsaReasonSlenCheckFailed);
        return 0;
    }
    if (sLen < 0 || emLen < hLen + sLen + 2) {
        PushError(ErrLibRsa, function, RsaReasonDataTooLargeForKeySize);
        return 0;
    }
    std::vector<unsigned char> salt(static_cast<std::size_t>(sLen));
    RandomBytes(nullptr, salt.data(), salt.size());
    const auto h = PssHash(*hash, mHash, salt.data(), salt.size());
    const int dbLen = emLen - hLen - 1;
    std::vector<unsigned char> db(static_cast<std::size_t>(dbLen));
    db[static_cast<std::size_t>(dbLen - sLen - 1)] = 0x01;
    if (sLen) std::memcpy(db.data() + (dbLen - sLen), salt.data(), salt.size());
    std::vector<unsigned char> mask(db.size());
    Mgf1(mask.data(), mask.size(), h.data(), h.size(), *mgf1Hash);
    for (std::size_t index = 0; index < db.size(); ++index) out[index] = static_cast<unsigned char>(db[index] ^ mask[index]);
    if (msBits) out[0] &= static_cast<unsigned char>(0xff >> (8 - msBits));
    std::memcpy(out + dbLen, h.data(), h.size());
    out[emLen - 1] = 0xbc;
    return 1;
}

int RsaPssVerify(Rsa& rsa, const unsigned char* mHash, const EvpMd* hash, const EvpMd* mgf1Hash, const unsigned char* em, int sLen) {
    constexpr const char* name = "RSA_verify_PKCS1_PSS";
    if (!em || !mHash || !hash || !mgf1Hash) throw std::invalid_argument(std::string(name) + ": null argument");
    const int modBits = ModulusBits(rsa, name);
    const int hLen = hash->mdSize;
    const int msBits = (modBits - 1) & 7;
    int emLen = (modBits + 7) / 8;
    const unsigned char* in = em;
    if (sLen == -1) sLen = hLen;
    else if (sLen < -2) {
        PushError(ErrLibRsa, RsaFunctionVerifyPss, RsaReasonSlenCheckFailed);
        return 0;
    }
    if (in[0] & static_cast<unsigned char>(0xff << msBits)) {
        PushError(ErrLibRsa, RsaFunctionVerifyPss, RsaReasonFirstOctetInvalid);
        return 0;
    }
    if (msBits == 0) {
        ++in;
        --emLen;
    }
    if (emLen < hLen + 2 || (sLen >= 0 && emLen < hLen + sLen + 2)) {
        PushError(ErrLibRsa, RsaFunctionVerifyPss, RsaReasonDataTooLarge);
        return 0;
    }
    if (in[emLen - 1] != 0xbc) {
        PushError(ErrLibRsa, RsaFunctionVerifyPss, RsaReasonLastOctetInvalid);
        return 0;
    }
    const int dbLen = emLen - hLen - 1;
    const unsigned char* h = in + dbLen;
    std::vector<unsigned char> db(static_cast<std::size_t>(dbLen));
    Mgf1(db.data(), db.size(), h, static_cast<std::size_t>(hLen), *mgf1Hash);
    for (std::size_t index = 0; index < db.size(); ++index) db[index] ^= in[index];
    if (msBits) db[0] &= static_cast<unsigned char>(0xff >> (8 - msBits));
    int index = 0;
    while (index < dbLen - 1 && db[static_cast<std::size_t>(index)] == 0) ++index;
    if (db[static_cast<std::size_t>(index)] != 0x01) {
        PushError(ErrLibRsa, RsaFunctionVerifyPss, RsaReasonSlenRecoveryFailed);
        return 0;
    }
    ++index;
    if (sLen >= 0 && dbLen - index != sLen) {
        PushError(ErrLibRsa, RsaFunctionVerifyPss, RsaReasonSlenCheckFailed);
        return 0;
    }
    const auto expected = PssHash(*hash, mHash, db.data() + index, static_cast<std::size_t>(dbLen - index));
    if (std::memcmp(expected.data(), h, static_cast<std::size_t>(hLen)) != 0) {
        PushError(ErrLibRsa, RsaFunctionVerifyPss, RsaReasonBadSignature);
        return 0;
    }
    return 1;
}

}

using namespace LibreSsl;

extern "C" {

Rsa* APS5_VABI RSA_new_nid_postfix() {
    auto* rsa = NewRsa();
    if (!rsa) PushError(ErrLibRsa, 106, ErrReasonMallocFailure);
    return rsa;
}

void APS5_VABI RSA_free_nid_postfix(Rsa* r) {
    FreeRsa(r);
}

int APS5_VABI RSA_flags_nid_postfix(const Rsa* rsa) {
    if (!rsa) return 0;
    if (!rsa->meth) throw std::invalid_argument("RSA_flags: null method");
    return static_cast<const RsaMethod*>(rsa->meth)->flags;
}

int APS5_VABI RSA_size_nid_postfix(const Rsa* rsa) {
    if (!rsa) throw std::invalid_argument("RSA_size: null RSA");
    const Bignum* modulus = rsa->n;
    if (!modulus) throw std::invalid_argument("RSA_size: null modulus");
    if (modulus->top == 0) return 0;
    if (modulus->top < 0) throw std::invalid_argument("RSA_size: negative modulus word count");
    if (!modulus->d) throw std::invalid_argument("RSA_size: null modulus words");
    const std::int64_t bits = static_cast<std::int64_t>(modulus->top - 1) * 64 + std::bit_width(modulus->d[modulus->top - 1]);
    if (bits > std::numeric_limits<int>::max() - 7) throw std::invalid_argument("RSA_size: modulus bit count overflows int");
    return static_cast<int>((bits + 7) / 8);
}

int APS5_VABI RSA_public_encrypt_nid_postfix(int flen, const unsigned char* from, unsigned char* to, Rsa* rsa, int padding) {
    return RsaPublicEncrypt(flen, from, to, RequireRsa(rsa, __func__), padding);
}

int APS5_VABI RSA_private_decrypt_nid_postfix(int flen, const unsigned char* from, unsigned char* to, Rsa* rsa, int padding) {
    return RsaPrivateDecrypt(flen, from, to, RequireRsa(rsa, __func__), padding);
}

int APS5_VABI RSA_private_encrypt_nid_postfix(int flen, const unsigned char* from, unsigned char* to, Rsa* rsa, int padding) {
    auto& key = RequireRsa(rsa, __func__);
    RequireBuffers(flen, from, to, __func__);
    if (padding == RsaX931Padding) throw std::invalid_argument("RSA_private_encrypt: X9.31 padding is not supported");
    if (padding != RsaPkcs1Padding && padding != RsaNoPadding) {
        PushError(ErrLibRsa, RsaFunctionEayPrivateEncrypt, RsaReasonUnknownPaddingType);
        return -1;
    }
    RsaCtx ctx;
    LoadContext(key, ctx, true, __func__);
    const int k = static_cast<int>(mbedtls_rsa_get_len(&ctx.value));
    std::vector<unsigned char> output(static_cast<std::size_t>(k));
    const std::vector<unsigned char> input(from, from + flen);
    int status = 0;
    if (padding == RsaPkcs1Padding) {
        if (flen > k - 11) {
            PushError(ErrLibRsa, RsaFunctionPaddingAddType1, RsaReasonDataTooLargeForKeySize);
            return -1;
        }
        status = mbedtls_rsa_rsassa_pkcs1_v15_sign(&ctx.value, RandomBytes, nullptr, MBEDTLS_MD_NONE, input.size(), input.data(), output.data());
    } else {
        if (flen != k) {
            PushError(ErrLibRsa, RsaFunctionPaddingAddNone, flen > k ? RsaReasonDataTooLargeForKeySize : RsaReasonDataTooSmallForKeySize);
            return -1;
        }
        if (!BelowModulus(ctx, input.data(), input.size(), __func__)) {
            PushError(ErrLibRsa, RsaFunctionEayPrivateEncrypt, RsaReasonDataTooLargeForModulus);
            return -1;
        }
        status = mbedtls_rsa_private(&ctx.value, RandomBytes, nullptr, input.data(), output.data());
    }
    Check(status, __func__);
    std::memcpy(to, output.data(), output.size());
    return k;
}

int APS5_VABI RSA_public_decrypt_nid_postfix(int flen, const unsigned char* from, unsigned char* to, Rsa* rsa, int padding) {
    auto& key = RequireRsa(rsa, __func__);
    RequireBuffers(flen, from, to, __func__);
    if (padding == RsaX931Padding) throw std::invalid_argument("RSA_public_decrypt: X9.31 padding is not supported");
    if (padding != RsaPkcs1Padding && padding != RsaNoPadding) {
        PushError(ErrLibRsa, RsaFunctionEayPublicDecrypt, RsaReasonUnknownPaddingType);
        return -1;
    }
    RsaCtx ctx;
    LoadContext(key, ctx, false, __func__);
    const int k = static_cast<int>(mbedtls_rsa_get_len(&ctx.value));
    if (flen > k) {
        PushError(ErrLibRsa, RsaFunctionEayPublicDecrypt, RsaReasonDataGreaterThanModLen);
        return -1;
    }
    const auto block = InputBlock(flen, from, static_cast<std::size_t>(k));
    if (!BelowModulus(ctx, block.data(), block.size(), __func__)) {
        PushError(ErrLibRsa, RsaFunctionEayPublicDecrypt, RsaReasonDataTooLargeForModulus);
        return -1;
    }
    std::vector<unsigned char> em(static_cast<std::size_t>(k));
    Check(mbedtls_rsa_public(&ctx.value, block.data(), em.data()), __func__);
    if (padding == RsaNoPadding) {
        std::memcpy(to, em.data(), em.size());
        return k;
    }
    std::size_t index = 2;
    bool valid = k >= 11 && em[0] == 0x00 && em[1] == 0x01;
    while (valid && index < em.size() && em[index] == 0xff) ++index;
    valid = valid && index < em.size() && em[index] == 0x00 && index - 2 >= 8;
    if (!valid) {
        PushError(ErrLibRsa, RsaFunctionEayPublicDecrypt, RsaReasonPaddingCheckFailed);
        return -1;
    }
    ++index;
    const std::size_t length = em.size() - index;
    if (length) std::memcpy(to, em.data() + index, length);
    return static_cast<int>(length);
}

int APS5_VABI RSA_sign_nid_postfix(int type, const unsigned char* m, unsigned int m_length, unsigned char* sigret, unsigned int* siglen, Rsa* rsa) {
    return RsaSignDigest(type, m, m_length, sigret, siglen, RequireRsa(rsa, __func__), __func__);
}

int APS5_VABI RSA_verify_nid_postfix(int type, const unsigned char* m, unsigned int m_length, const unsigned char* sigbuf, unsigned int siglen, Rsa* rsa) {
    return RsaVerifyDigest(type, m, m_length, sigbuf, siglen, RequireRsa(rsa, __func__), __func__);
}

int APS5_VABI RSA_generate_key_ex_nid_postfix(Rsa* rsa, int bits, Bignum* e, void* cb) {
    auto& key = RequireRsa(rsa, __func__);
    if (cb) throw std::invalid_argument("RSA_generate_key_ex: progress callbacks are not supported");
    if (!e) throw std::invalid_argument("RSA_generate_key_ex: null exponent");
    const auto exponent = BignumBytes(*e);
    if (e->neg || exponent.size() > 4) throw std::invalid_argument("RSA_generate_key_ex: exponents outside 3..INT_MAX are not supported");
    std::uint64_t value = 0;
    for (const auto byte : exponent) value = value << 8 | byte;
    if (value < 3 || value > static_cast<std::uint64_t>(std::numeric_limits<int>::max()) || !(value & 1))
        throw std::invalid_argument("RSA_generate_key_ex: exponents that are even or outside 3..INT_MAX are not supported");
    if (bits < 1024 || bits > MaxModulusBits || (bits & 1)) throw std::invalid_argument("RSA_generate_key_ex: only even key sizes from 1024 to 8192 bits are supported");
    RsaCtx ctx;
    Check(mbedtls_rsa_gen_key(&ctx.value, RandomBytes, nullptr, static_cast<unsigned int>(bits), static_cast<int>(value)), __func__);
    FillRsa(key, ctx.value, __func__);
    return 1;
}

int APS5_VABI RSA_padding_add_PKCS1_PSS_nid_postfix(Rsa* rsa, unsigned char* EM, const unsigned char* mHash, const EvpMd* Hash, int sLen) {
    return RsaPssEncode(RequireRsa(rsa, __func__), EM, mHash, Hash, Hash, sLen);
}

int APS5_VABI RSA_verify_PKCS1_PSS_nid_postfix(Rsa* rsa, const unsigned char* mHash, const EvpMd* Hash, const unsigned char* EM, int sLen) {
    return RsaPssVerify(RequireRsa(rsa, __func__), mHash, Hash, Hash, EM, sLen);
}

int LibreSslRsaUpRef_nid_no_patch(Rsa* r) {
    RsaUpRef(r);
    return 1;
}

void LibreSslRsaGet0Key_nid_no_patch(const Rsa* r, const Bignum** n, const Bignum** e, const Bignum** d) {
    if (!r) throw std::invalid_argument("RSA_get0_key: null RSA");
    if (n) *n = r->n;
    if (e) *e = r->e;
    if (d) *d = r->d;
}

int LibreSslRsaPaddingAddPssMgf1_nid_no_patch(Rsa* rsa, unsigned char* EM, const unsigned char* mHash, const EvpMd* Hash, const EvpMd* mgf1Hash, int sLen) {
    if (!mgf1Hash) throw std::invalid_argument("RSA_padding_add_PKCS1_PSS_mgf1: null MGF1 digest");
    return RsaPssEncode(RequireRsa(rsa, "RSA_padding_add_PKCS1_PSS_mgf1"), EM, mHash, Hash, mgf1Hash, sLen);
}

int LibreSslRsaCheckKey_nid_no_patch(const Rsa* rsa) {
    constexpr const char* function = "RSA_check_key";
    if (!rsa) throw std::invalid_argument("RSA_check_key: null RSA");
    if (!rsa->n || !rsa->e || !rsa->d || !rsa->p || !rsa->q) throw std::invalid_argument("RSA_check_key: keys without n, e, d, p and q are not supported");
    Mpi n, e, d, p, q, product, pMinus, qMinus, gcd, lcm, rest, scratch;
    ReadMpi(*rsa->n, n.value, function);
    ReadMpi(*rsa->e, e.value, function);
    ReadMpi(*rsa->d, d.value, function);
    ReadMpi(*rsa->p, p.value, function);
    ReadMpi(*rsa->q, q.value, function);
    int result = 1;
    const auto fail = [&](int reason) {
        PushError(ErrLibRsa, RsaFunctionCheckKey, reason);
        result = 0;
    };
    if (!PrimeCheck(p.value)) fail(RsaReasonPNotPrime);
    if (!PrimeCheck(q.value)) fail(RsaReasonQNotPrime);
    Check(mbedtls_mpi_mul_mpi(&product.value, &p.value, &q.value), function);
    if (mbedtls_mpi_cmp_mpi(&product.value, &n.value) != 0) fail(RsaReasonNDoesNotEqualPQ);
    Check(mbedtls_mpi_sub_int(&pMinus.value, &p.value, 1), function);
    Check(mbedtls_mpi_sub_int(&qMinus.value, &q.value, 1), function);
    if (mbedtls_mpi_cmp_int(&pMinus.value, 0) > 0 && mbedtls_mpi_cmp_int(&qMinus.value, 0) > 0) {
        Check(mbedtls_mpi_gcd(&gcd.value, &pMinus.value, &qMinus.value), function);
        Check(mbedtls_mpi_mul_mpi(&scratch.value, &pMinus.value, &qMinus.value), function);
        Check(mbedtls_mpi_div_mpi(&lcm.value, nullptr, &scratch.value, &gcd.value), function);
        Check(mbedtls_mpi_mul_mpi(&scratch.value, &d.value, &e.value), function);
        Check(mbedtls_mpi_mod_mpi(&rest.value, &scratch.value, &lcm.value), function);
        if (mbedtls_mpi_cmp_int(&rest.value, 1) != 0) fail(RsaReasonDENotCongruentTo1);
        if (rsa->dmp1 && rsa->dmq1 && rsa->iqmp) {
            Mpi dmp1, dmq1, iqmp;
            ReadMpi(*rsa->dmp1, dmp1.value, function);
            ReadMpi(*rsa->dmq1, dmq1.value, function);
            ReadMpi(*rsa->iqmp, iqmp.value, function);
            Check(mbedtls_mpi_mod_mpi(&rest.value, &d.value, &pMinus.value), function);
            if (mbedtls_mpi_cmp_mpi(&rest.value, &dmp1.value) != 0) fail(RsaReasonDmp1NotCongruentToD);
            Check(mbedtls_mpi_mod_mpi(&rest.value, &d.value, &qMinus.value), function);
            if (mbedtls_mpi_cmp_mpi(&rest.value, &dmq1.value) != 0) fail(RsaReasonDmq1NotCongruentToD);
            Check(mbedtls_mpi_mul_mpi(&scratch.value, &iqmp.value, &q.value), function);
            Check(mbedtls_mpi_mod_mpi(&rest.value, &scratch.value, &p.value), function);
            if (mbedtls_mpi_cmp_int(&rest.value, 1) != 0) fail(RsaReasonIqmpNotInverseOfQ);
        } else if (rsa->dmp1 || rsa->dmq1 || rsa->iqmp) {
            throw std::invalid_argument("RSA_check_key: keys with only some of dmp1, dmq1 and iqmp are not supported");
        }
    } else {
        fail(RsaReasonDENotCongruentTo1);
    }
    return result;
}

}
