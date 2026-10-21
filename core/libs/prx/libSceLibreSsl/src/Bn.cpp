#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <mbedtls/bignum.h>

#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace LibreSsl {

namespace {

constexpr int FlagMalloced = 0x01;
constexpr int BnFunctionModInverse = 110;
constexpr int BnReasonNoInverse = 108;

struct Mpi {
    mbedtls_mpi value;

    Mpi() {
        mbedtls_mpi_init(&value);
    }

    ~Mpi() {
        mbedtls_mpi_free(&value);
    }

    Mpi(const Mpi&) = delete;
    Mpi& operator=(const Mpi&) = delete;
};

void Check(int result, const char* function) {
    if (result == MBEDTLS_ERR_MPI_ALLOC_FAILED) throw std::bad_alloc();
    if (result != 0) throw std::runtime_error(std::string(function) + ": Mbed TLS failed");
}

Bignum& RequireBn(Bignum* bn, const char* function) {
    if (!bn) throw std::invalid_argument(std::string(function) + ": null BIGNUM");
    return *bn;
}

const Bignum& RequireBn(const Bignum* bn, const char* function) {
    if (!bn) throw std::invalid_argument(std::string(function) + ": null BIGNUM");
    return *bn;
}

void Reserve(Bignum& bn, int words) {
    if (words <= bn.dmax) return;
    auto* grown = static_cast<std::uint64_t*>(std::realloc(bn.d, static_cast<std::size_t>(words) * sizeof(std::uint64_t)));
    if (!grown) throw std::bad_alloc();
    bn.d = grown;
    bn.dmax = words;
}

void Normalize(Bignum& bn) {
    while (bn.top > 0 && bn.d[bn.top - 1] == 0) --bn.top;
    if (bn.top == 0) bn.neg = 0;
}

void SetMagnitude(Bignum& bn, const unsigned char* bigEndian, std::size_t length, bool negative) {
    while (length && *bigEndian == 0) {
        ++bigEndian;
        --length;
    }
    const std::size_t words = (length + 7) / 8;
    if (words > static_cast<std::size_t>(std::numeric_limits<int>::max() / 8)) throw std::length_error("BIGNUM too long");
    Reserve(bn, static_cast<int>(words));
    for (std::size_t word = 0; word < words; ++word) {
        std::uint64_t value = 0;
        for (std::size_t byte = 0; byte < 8; ++byte) {
            const std::size_t index = word * 8 + byte;
            if (index < length) value |= static_cast<std::uint64_t>(bigEndian[length - 1 - index]) << (byte * 8);
        }
        bn.d[word] = value;
    }
    bn.top = static_cast<int>(words);
    bn.neg = negative ? 1 : 0;
    Normalize(bn);
}

std::vector<unsigned char> Magnitude(const Bignum& bn) {
    std::vector<unsigned char> bytes(static_cast<std::size_t>(bn.top) * 8);
    for (int word = 0; word < bn.top; ++word) {
        for (int byte = 0; byte < 8; ++byte) bytes[bytes.size() - 1 - static_cast<std::size_t>(word * 8 + byte)] = static_cast<unsigned char>(bn.d[word] >> (byte * 8));
    }
    std::size_t skip = 0;
    while (skip < bytes.size() && bytes[skip] == 0) ++skip;
    bytes.erase(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(skip));
    return bytes;
}

void ToMpi(const Bignum& bn, Mpi& out, const char* function) {
    const auto bytes = Magnitude(bn);
    Check(mbedtls_mpi_read_binary(&out.value, bytes.data(), bytes.size()), function);
    if (bn.neg) {
        Mpi zero;
        Check(mbedtls_mpi_lset(&zero.value, 0), function);
        Check(mbedtls_mpi_sub_mpi(&out.value, &zero.value, &out.value), function);
    }
}

void FromMpi(Bignum& bn, const Mpi& in, const char* function) {
    const bool negative = mbedtls_mpi_cmp_int(&in.value, 0) < 0;
    Mpi magnitude;
    if (negative) {
        Mpi zero;
        Check(mbedtls_mpi_lset(&zero.value, 0), function);
        Check(mbedtls_mpi_sub_mpi(&magnitude.value, &zero.value, &in.value), function);
    } else {
        Check(mbedtls_mpi_copy(&magnitude.value, &in.value), function);
    }
    std::vector<unsigned char> bytes(mbedtls_mpi_size(&magnitude.value));
    Check(mbedtls_mpi_write_binary(&magnitude.value, bytes.data(), bytes.size()), function);
    SetMagnitude(bn, bytes.data(), bytes.size(), negative);
}

void WordToMpi(std::uint64_t word, Mpi& out, const char* function) {
    unsigned char bytes[8];
    for (int index = 0; index < 8; ++index) bytes[index] = static_cast<unsigned char>(word >> (56 - index * 8));
    Check(mbedtls_mpi_read_binary(&out.value, bytes, sizeof(bytes)), function);
}

struct BnCtx {
    int depth = 0;
};

int HexValue(char digit) {
    if (digit >= '0' && digit <= '9') return digit - '0';
    if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
    if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
    return -1;
}

}

Bignum* NewBignum() {
    auto* bn = new (std::nothrow) Bignum{};
    if (bn) bn->flags = FlagMalloced;
    return bn;
}

void FreeBignum(Bignum* bn) {
    if (!bn) return;
    std::free(bn->d);
    delete bn;
}

void SetBignumBytes(Bignum& bn, const unsigned char* bigEndian, std::size_t length) {
    SetMagnitude(bn, bigEndian, length, false);
}

std::vector<unsigned char> BignumBytes(const Bignum& bn) {
    return Magnitude(bn);
}

}

using namespace LibreSsl;

extern "C" {

Bignum* APS5_VABI BN_new_nid_postfix() {
    return NewBignum();
}

void APS5_VABI BN_free_nid_postfix(Bignum* a) {
    if (a && !(a->flags & FlagMalloced)) throw std::invalid_argument("BN_free: BIGNUM not created by BN_new");
    FreeBignum(a);
}

Bignum* APS5_VABI BN_bin2bn_nid_postfix(const unsigned char* s, int len, Bignum* ret) {
    if (len < 0) throw std::invalid_argument("BN_bin2bn: negative length");
    if (!s && len) throw std::invalid_argument("BN_bin2bn: null buffer");
    Bignum* target = ret ? ret : NewBignum();
    if (!target) return nullptr;
    SetMagnitude(*target, s, static_cast<std::size_t>(len), false);
    return target;
}

int APS5_VABI BN_bn2bin_nid_postfix(const Bignum* a, unsigned char* to) {
    const auto bytes = Magnitude(RequireBn(a, __func__));
    if (!to && !bytes.empty()) throw std::invalid_argument("BN_bn2bin: null buffer");
    if (!bytes.empty()) std::memcpy(to, bytes.data(), bytes.size());
    return static_cast<int>(bytes.size());
}

int APS5_VABI BN_hex2bn_nid_postfix(Bignum** a, const char* str) {
    if (!str) throw std::invalid_argument("BN_hex2bn: null string");
    if (*str == '-') throw std::invalid_argument("BN_hex2bn: negative numbers are not supported");
    std::size_t digits = 0;
    while (HexValue(str[digits]) >= 0) ++digits;
    if (digits == 0) return 0;
    if (digits > static_cast<std::size_t>(std::numeric_limits<int>::max() / 4)) return 0;
    if (!a) return static_cast<int>(digits);
    std::vector<unsigned char> bytes((digits + 1) / 2);
    for (std::size_t index = 0; index < digits; ++index) {
        const std::size_t fromEnd = digits - 1 - index;
        bytes[bytes.size() - 1 - fromEnd / 2] |= static_cast<unsigned char>(HexValue(str[index]) << ((fromEnd % 2) * 4));
    }
    Bignum* target = *a ? *a : NewBignum();
    if (!target) return 0;
    SetMagnitude(*target, bytes.data(), bytes.size(), false);
    *a = target;
    return static_cast<int>(digits);
}

int APS5_VABI BN_num_bits_nid_postfix(const Bignum* a) {
    const auto& bn = RequireBn(a, __func__);
    if (bn.top == 0) return 0;
    std::uint64_t high = bn.d[bn.top - 1];
    int bits = 0;
    while (high) {
        ++bits;
        high >>= 1;
    }
    return (bn.top - 1) * 64 + bits;
}

int APS5_VABI BN_set_word_nid_postfix(Bignum* a, std::uint64_t w) {
    auto& bn = RequireBn(a, __func__);
    Reserve(bn, 1);
    bn.d[0] = w;
    bn.top = w ? 1 : 0;
    bn.neg = 0;
    return 1;
}

int APS5_VABI BN_add_word_nid_postfix(Bignum* a, std::uint64_t w) {
    auto& bn = RequireBn(a, __func__);
    Mpi value, word;
    ToMpi(bn, value, __func__);
    WordToMpi(w, word, __func__);
    Check(mbedtls_mpi_add_mpi(&value.value, &value.value, &word.value), __func__);
    FromMpi(bn, value, __func__);
    return 1;
}

int APS5_VABI BN_sub_word_nid_postfix(Bignum* a, std::uint64_t w) {
    auto& bn = RequireBn(a, __func__);
    Mpi value, word;
    ToMpi(bn, value, __func__);
    WordToMpi(w, word, __func__);
    Check(mbedtls_mpi_sub_mpi(&value.value, &value.value, &word.value), __func__);
    FromMpi(bn, value, __func__);
    return 1;
}

int APS5_VABI BN_mul_nid_postfix(Bignum* r, const Bignum* a, const Bignum* b, BnCtx*) {
    auto& result = RequireBn(r, __func__);
    Mpi left, right, product;
    ToMpi(RequireBn(a, __func__), left, __func__);
    ToMpi(RequireBn(b, __func__), right, __func__);
    Check(mbedtls_mpi_mul_mpi(&product.value, &left.value, &right.value), __func__);
    FromMpi(result, product, __func__);
    return 1;
}

Bignum* APS5_VABI BN_mod_inverse_nid_postfix(Bignum* ret, const Bignum* a, const Bignum* n, BnCtx*) {
    Mpi value, modulus, inverse;
    ToMpi(RequireBn(a, __func__), value, __func__);
    ToMpi(RequireBn(n, __func__), modulus, __func__);
    if (mbedtls_mpi_cmp_int(&modulus.value, 1) <= 0) throw std::invalid_argument("BN_mod_inverse: the modulus is not above 1");
    const int result = mbedtls_mpi_inv_mod(&inverse.value, &value.value, &modulus.value);
    if (result == MBEDTLS_ERR_MPI_NOT_ACCEPTABLE) {
        PushError(ErrLibBn, BnFunctionModInverse, BnReasonNoInverse);
        return nullptr;
    }
    Check(result, __func__);
    Bignum* target = ret ? ret : NewBignum();
    if (!target) return nullptr;
    FromMpi(*target, inverse, __func__);
    return target;
}

BnCtx* APS5_VABI BN_CTX_new_nid_postfix() {
    return new (std::nothrow) BnCtx;
}

void APS5_VABI BN_CTX_free_nid_postfix(BnCtx* c) {
    if (c && c->depth) throw std::logic_error("BN_CTX_free: BN_CTX_start without BN_CTX_end");
    delete c;
}

void APS5_VABI BN_CTX_start_nid_postfix(BnCtx* ctx) {
    if (!ctx) throw std::invalid_argument("BN_CTX_start: null context");
    ++ctx->depth;
}

void APS5_VABI BN_CTX_end_nid_postfix(BnCtx* ctx) {
    if (!ctx) throw std::invalid_argument("BN_CTX_end: null context");
    if (!ctx->depth) throw std::logic_error("BN_CTX_end: no matching BN_CTX_start");
    --ctx->depth;
}

}
