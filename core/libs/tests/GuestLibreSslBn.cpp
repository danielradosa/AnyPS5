#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

struct GuestBignum {
    std::uint64_t* d;
    int top;
    int dmax;
    int neg;
    int flags;
};
struct GuestBnCtx;

extern "C" {
GuestBignum* APS5_VABI BN_new_nid_postfix();
void APS5_VABI BN_free_nid_postfix(GuestBignum*);
GuestBignum* APS5_VABI BN_bin2bn_nid_postfix(const unsigned char*, int, GuestBignum*);
int APS5_VABI BN_bn2bin_nid_postfix(const GuestBignum*, unsigned char*);
int APS5_VABI BN_hex2bn_nid_postfix(GuestBignum**, const char*);
int APS5_VABI BN_num_bits_nid_postfix(const GuestBignum*);
int APS5_VABI BN_set_word_nid_postfix(GuestBignum*, std::uint64_t);
int APS5_VABI BN_add_word_nid_postfix(GuestBignum*, std::uint64_t);
int APS5_VABI BN_sub_word_nid_postfix(GuestBignum*, std::uint64_t);
int APS5_VABI BN_mul_nid_postfix(GuestBignum*, const GuestBignum*, const GuestBignum*, GuestBnCtx*);
GuestBignum* APS5_VABI BN_mod_inverse_nid_postfix(GuestBignum*, const GuestBignum*, const GuestBignum*, GuestBnCtx*);
GuestBnCtx* APS5_VABI BN_CTX_new_nid_postfix();
void APS5_VABI BN_CTX_free_nid_postfix(GuestBnCtx*);
void APS5_VABI BN_CTX_start_nid_postfix(GuestBnCtx*);
void APS5_VABI BN_CTX_end_nid_postfix(GuestBnCtx*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
}

static void Require(bool value) {
    if (!value) std::abort();
}

template <typename TCall>
static bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

static std::string Hex(const GuestBignum* bn) {
    static constexpr char digits[] = "0123456789abcdef";
    unsigned char bytes[256];
    const int length = BN_bn2bin_nid_postfix(bn, bytes);
    std::string text;
    for (int index = 0; index < length; ++index) {
        text += digits[bytes[index] >> 4];
        text += digits[bytes[index] & 15];
    }
    return text;
}

static void TestLayout() {
    GuestBignum* bn = BN_new_nid_postfix();
    Require(bn && bn->top == 0 && bn->neg == 0 && (bn->flags & 1));
    Require(BN_num_bits_nid_postfix(bn) == 0 && Hex(bn).empty());
    Require(BN_hex2bn_nid_postfix(&bn, "123456789ABCDEF0123456789abcdefXYZ") == 31);
    Require(bn->top == 2 && bn->d[0] == 0x0123456789abcdefull && bn->d[1] == 0x123456789abcdefull && bn->neg == 0);
    Require(BN_num_bits_nid_postfix(bn) == 121);
    Require(Hex(bn) == "0123456789abcdef0123456789abcdef");
    Require(BN_hex2bn_nid_postfix(nullptr, "00ff") == 4);
    Require(BN_hex2bn_nid_postfix(&bn, "xyz") == 0 && bn->top == 2);
    Require(Throws([&] { BN_hex2bn_nid_postfix(&bn, "-1"); }));
    BN_free_nid_postfix(bn);

    GuestBignum* created = nullptr;
    Require(BN_hex2bn_nid_postfix(&created, "0001") == 4 && created && created->top == 1 && created->d[0] == 1);
    BN_free_nid_postfix(created);
    BN_free_nid_postfix(nullptr);
}

static void TestBytes() {
    const unsigned char bytes[] = {0, 0, 1, 2};
    GuestBignum* bn = BN_bin2bn_nid_postfix(bytes, 4, nullptr);
    Require(bn && bn->top == 1 && bn->d[0] == 0x102 && BN_num_bits_nid_postfix(bn) == 9);
    unsigned char out[16];
    Require(BN_bn2bin_nid_postfix(bn, out) == 2 && out[0] == 1 && out[1] == 2);
    unsigned char wide[17] = {0x80};
    Require(BN_bin2bn_nid_postfix(wide, 17, bn) == bn && bn->top == 3 && bn->d[2] == 0x80 && bn->d[1] == 0 && bn->d[0] == 0);
    Require(BN_num_bits_nid_postfix(bn) == 136);
    Require(BN_bin2bn_nid_postfix(bytes, 0, bn) == bn && bn->top == 0 && BN_num_bits_nid_postfix(bn) == 0);
    Require(Throws([&] { BN_bin2bn_nid_postfix(bytes, -1, bn); }));
    BN_free_nid_postfix(bn);
}

static void TestWords() {
    GuestBignum* bn = BN_new_nid_postfix();
    Require(BN_set_word_nid_postfix(bn, ~0ull) == 1 && bn->top == 1 && bn->d[0] == ~0ull);
    Require(BN_add_word_nid_postfix(bn, 1) == 1 && bn->top == 2 && bn->d[0] == 0 && bn->d[1] == 1);
    Require(BN_sub_word_nid_postfix(bn, 1) == 1 && bn->top == 1 && bn->d[0] == ~0ull);
    Require(BN_set_word_nid_postfix(bn, 5) == 1);
    Require(BN_sub_word_nid_postfix(bn, 7) == 1 && bn->neg == 1 && bn->top == 1 && bn->d[0] == 2);
    Require(Hex(bn) == "02");
    Require(BN_add_word_nid_postfix(bn, 3) == 1 && bn->neg == 0 && bn->d[0] == 1);
    Require(BN_sub_word_nid_postfix(bn, 1) == 1 && bn->top == 0 && bn->neg == 0);
    Require(BN_set_word_nid_postfix(bn, 0) == 1 && bn->top == 0);
    BN_free_nid_postfix(bn);
}

static void TestArithmetic() {
    GuestBnCtx* ctx = BN_CTX_new_nid_postfix();
    Require(ctx != nullptr);
    BN_CTX_start_nid_postfix(ctx);
    GuestBignum* a = BN_new_nid_postfix();
    GuestBignum* b = BN_new_nid_postfix();
    GuestBignum* r = BN_new_nid_postfix();
    BN_set_word_nid_postfix(a, ~0ull);
    BN_set_word_nid_postfix(b, ~0ull);
    Require(BN_mul_nid_postfix(r, a, b, ctx) == 1);
    Require(Hex(r) == "fffffffffffffffe0000000000000001");
    Require(BN_mul_nid_postfix(a, a, a, ctx) == 1 && Hex(a) == Hex(r));
    BN_sub_word_nid_postfix(b, ~0ull);
    BN_sub_word_nid_postfix(b, 3);
    Require(BN_mul_nid_postfix(r, r, b, ctx) == 1 && r->neg == 1);
    Require(Hex(r) == "02fffffffffffffffa0000000000000003");
    BN_set_word_nid_postfix(b, 0);
    Require(BN_mul_nid_postfix(r, r, b, ctx) == 1 && r->top == 0 && r->neg == 0);

    GuestBignum* n = BN_new_nid_postfix();
    BN_set_word_nid_postfix(a, 3);
    BN_set_word_nid_postfix(n, 11);
    GuestBignum* inverse = BN_mod_inverse_nid_postfix(nullptr, a, n, ctx);
    Require(inverse && inverse->top == 1 && inverse->d[0] == 4);
    BN_set_word_nid_postfix(a, 3);
    BN_sub_word_nid_postfix(a, 6);
    Require(BN_mod_inverse_nid_postfix(inverse, a, n, ctx) == inverse && inverse->d[0] == 7 && inverse->neg == 0);
    Require(BN_hex2bn_nid_postfix(&n, "fffffffffffffffffffffffffffffffeffffffffffffffff") == 48);
    BN_set_word_nid_postfix(a, 2);
    Require(BN_mod_inverse_nid_postfix(a, a, n, ctx) == a);
    Require(Hex(a) == "7fffffffffffffffffffffffffffffff8000000000000000");
    while (ERR_get_error_nid_postfix()) {
    }
    BN_set_word_nid_postfix(a, 2);
    BN_set_word_nid_postfix(n, 4);
    Require(BN_mod_inverse_nid_postfix(nullptr, a, n, ctx) == nullptr);
    Require(ERR_get_error_nid_postfix() == ((3ull << 24) | (110ull << 12) | 108));
    BN_free_nid_postfix(inverse);
    BN_free_nid_postfix(n);
    BN_free_nid_postfix(r);
    BN_free_nid_postfix(b);
    BN_free_nid_postfix(a);
    Require(Throws([&] { BN_CTX_free_nid_postfix(ctx); }));
    BN_CTX_end_nid_postfix(ctx);
    Require(Throws([&] { BN_CTX_end_nid_postfix(ctx); }));
    BN_CTX_free_nid_postfix(ctx);
    BN_CTX_free_nid_postfix(nullptr);
}

int main() {
    TestLayout();
    TestBytes();
    TestWords();
    TestArithmetic();
    return 0;
}
