#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

struct GuestEvpMd;
struct GuestEvpCipher;
struct GuestEvpMdCtx;
struct GuestBio;
struct GuestBioMethod;
struct alignas(8) GuestEvpCipherCtx { unsigned char bytes[168]; };

extern "C" {
const GuestEvpMd* APS5_VABI EVP_sha1_nid_postfix();
const GuestEvpMd* APS5_VABI EVP_sha224_nid_postfix();
GuestEvpMdCtx* APS5_VABI EVP_MD_CTX_new_nid_postfix();
void APS5_VABI EVP_MD_CTX_free_nid_postfix(GuestEvpMdCtx*);
int APS5_VABI EVP_DigestInit_nid_postfix(GuestEvpMdCtx*, const GuestEvpMd*);
int APS5_VABI EVP_EncryptInit_nid_postfix(GuestEvpCipherCtx*, const GuestEvpCipher*, const unsigned char*, const unsigned char*);
void APS5_VABI EVP_CIPHER_CTX_set_flags_nid_postfix(GuestEvpCipherCtx*, int);
const GuestEvpCipher* APS5_VABI EVP_aes_128_wrap_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_192_wrap_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_256_wrap_nid_postfix();
int APS5_VABI BIO_up_ref_nid_postfix(GuestBio*);
void APS5_VABI BIO_vfree_nid_postfix(GuestBio*);
std::uint64_t APS5_VABI ERR_peek_last_error_nid_postfix();
void APS5_VABI ERR_load_crypto_strings_nid_postfix();

int APS5_VABI EVP_MD_size_nid_postfix(const GuestEvpMd*);
int APS5_VABI EVP_Digest_nid_postfix(const void*, std::size_t, unsigned char*, unsigned int*, const GuestEvpMd*, void*);
int APS5_VABI EVP_DigestUpdate_nid_postfix(GuestEvpMdCtx*, const void*, std::size_t);
int APS5_VABI EVP_DigestFinal_ex_nid_postfix(GuestEvpMdCtx*, unsigned char*, unsigned int*);
const GuestEvpCipher* APS5_VABI EVP_aes_128_cbc_nid_postfix();
int APS5_VABI EVP_CIPHER_block_size_nid_postfix(const GuestEvpCipher*);
void APS5_VABI EVP_CIPHER_CTX_init_nid_postfix(GuestEvpCipherCtx*);
int APS5_VABI EVP_CIPHER_CTX_cleanup_nid_postfix(GuestEvpCipherCtx*);
int APS5_VABI EVP_EncryptInit_ex_nid_postfix(GuestEvpCipherCtx*, const GuestEvpCipher*, void*, const unsigned char*, const unsigned char*);
int APS5_VABI EVP_DecryptInit_ex_nid_postfix(GuestEvpCipherCtx*, const GuestEvpCipher*, void*, const unsigned char*, const unsigned char*);
int APS5_VABI EVP_EncryptUpdate_nid_postfix(GuestEvpCipherCtx*, unsigned char*, int*, const unsigned char*, int);
int APS5_VABI EVP_DecryptUpdate_nid_postfix(GuestEvpCipherCtx*, unsigned char*, int*, const unsigned char*, int);
int APS5_VABI EVP_EncryptFinal_ex_nid_postfix(GuestEvpCipherCtx*, unsigned char*, int*);
int APS5_VABI EVP_DecryptFinal_ex_nid_postfix(GuestEvpCipherCtx*, unsigned char*, int*);
const GuestBioMethod* APS5_VABI BIO_s_mem_nid_postfix();
GuestBio* APS5_VABI BIO_new_nid_postfix(const GuestBioMethod*);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
int APS5_VABI BIO_puts_nid_postfix(GuestBio*, const char*);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
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

static std::string Hex(const unsigned char* bytes, std::size_t size) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string text;
    for (std::size_t index = 0; index < size; ++index) {
        text += digits[bytes[index] >> 4];
        text += digits[bytes[index] & 15];
    }
    return text;
}

static std::string FromHex(const char* text) {
    std::string bytes;
    const auto nibble = [](char digit) { return digit <= '9' ? digit - '0' : digit - 'a' + 10; };
    for (std::size_t index = 0; text[index] && text[index + 1]; index += 2)
        bytes += static_cast<char>(nibble(text[index]) << 4 | nibble(text[index + 1]));
    return bytes;
}

static const unsigned char* Bytes(const std::string& text) {
    return reinterpret_cast<const unsigned char*>(text.data());
}

static void TestDigests() {
    unsigned char digest[64];
    unsigned int size = 0;
    Require(EVP_MD_size_nid_postfix(EVP_sha1_nid_postfix()) == 20 && EVP_MD_size_nid_postfix(EVP_sha224_nid_postfix()) == 28);
    Require(EVP_Digest_nid_postfix("abc", 3, digest, &size, EVP_sha1_nid_postfix(), nullptr) == 1 && size == 20);
    Require(Hex(digest, 20) == "a9993e364706816aba3e25717850c26c9cd0d89d");
    GuestEvpMdCtx* ctx = EVP_MD_CTX_new_nid_postfix();
    Require(ctx != nullptr);
    Require(EVP_DigestInit_nid_postfix(ctx, EVP_sha224_nid_postfix()) == 1);
    Require(EVP_DigestUpdate_nid_postfix(ctx, "abc", 3) == 1);
    Require(EVP_DigestFinal_ex_nid_postfix(ctx, digest, &size) == 1 && size == 28);
    Require(Hex(digest, 28) == "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7");
    EVP_MD_CTX_free_nid_postfix(ctx);
    EVP_MD_CTX_free_nid_postfix(nullptr);
}

static void TestCiphers() {
    GuestEvpCipherCtx ctx;
    std::memset(&ctx, 0x5c, sizeof(ctx));
    const std::string key = FromHex("2b7e151628aed2a6abf7158809cf4f3c");
    const std::string iv = FromHex("000102030405060708090a0b0c0d0e0f");
    Require(EVP_EncryptInit_nid_postfix(&ctx, EVP_aes_128_cbc_nid_postfix(), Bytes(key), Bytes(iv)) == 1);
    unsigned char out[64];
    int length = 0;
    int tail = 0;
    Require(EVP_EncryptUpdate_nid_postfix(&ctx, out, &length, Bytes(FromHex("6bc1bee22e409f96e93d7e117393172a")), 16) == 1 && length == 16);
    Require(Hex(out, 16) == "7649abac8119b246cee98e9b12e9197d");
    Require(EVP_EncryptFinal_ex_nid_postfix(&ctx, out, &tail) == 1 && tail == 16);
    Require(EVP_CIPHER_CTX_cleanup_nid_postfix(&ctx) == 1);

    Require(EVP_CIPHER_block_size_nid_postfix(EVP_aes_128_wrap_nid_postfix()) == 8);
    Require(EVP_aes_192_wrap_nid_postfix() != EVP_aes_128_wrap_nid_postfix());
    const std::string kek = FromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    const std::string data = FromHex("00112233445566778899aabbccddeeff000102030405060708090a0b0c0d0e0f");
    EVP_CIPHER_CTX_init_nid_postfix(&ctx);
    ERR_clear_error_nid_postfix();
    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, EVP_aes_128_wrap_nid_postfix(), nullptr, Bytes(kek), nullptr) == 0);
    Require(ERR_peek_last_error_nid_postfix() == ((6ull << 24) | (123ull << 12) | 170));
    EVP_CIPHER_CTX_set_flags_nid_postfix(&ctx, 1);
    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, EVP_aes_128_wrap_nid_postfix(), nullptr, Bytes(kek), nullptr) == 1);
    Require(EVP_EncryptUpdate_nid_postfix(&ctx, out, &length, Bytes(data), 16) == 1 && length == 24);
    Require(Hex(out, 24) == "1fa68b0a8112b447aef34bd8fb5a7b829d3e862371d2cfe5");
    Require(EVP_EncryptFinal_ex_nid_postfix(&ctx, out + 24, &tail) == 1 && tail == 0);
    Require(EVP_EncryptUpdate_nid_postfix(&ctx, out, &length, Bytes(data), 12) == 0);
    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, EVP_aes_192_wrap_nid_postfix(), nullptr, Bytes(kek), nullptr) == 1);
    Require(EVP_EncryptUpdate_nid_postfix(&ctx, out, &length, Bytes(data), 16) == 1 && length == 24);
    Require(Hex(out, 24) == "96778b25ae6ca435f92b5b97c050aed2468ab8a17ad84e5d");
    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, EVP_aes_256_wrap_nid_postfix(), nullptr, Bytes(kek), nullptr) == 1);
    Require(EVP_EncryptUpdate_nid_postfix(&ctx, out, &length, Bytes(data), 32) == 1 && length == 40);
    const std::string wrapped(reinterpret_cast<const char*>(out), 40);
    Require(Hex(out, 40) == "28c9f404c4b810f4cbccb35cfb87f8263f5786e2d80ed326cbc7f0e71a99f43bfb988b9b7a02dd21");
    Require(EVP_DecryptInit_ex_nid_postfix(&ctx, EVP_aes_256_wrap_nid_postfix(), nullptr, Bytes(kek), nullptr) == 1);
    Require(EVP_DecryptUpdate_nid_postfix(&ctx, out, &length, Bytes(wrapped), 40) == 1 && length == 32);
    Require(Hex(out, 32) == Hex(Bytes(data), 32));
    Require(EVP_DecryptFinal_ex_nid_postfix(&ctx, out, &tail) == 1 && tail == 0);
    std::string broken = wrapped;
    broken[5] ^= 1;
    Require(EVP_DecryptUpdate_nid_postfix(&ctx, out, &length, Bytes(broken), 40) == 0);
    Require(Throws([&] { EVP_EncryptInit_ex_nid_postfix(&ctx, nullptr, nullptr, nullptr, Bytes(iv)); }));
    Require(EVP_EncryptInit_nid_postfix(&ctx, EVP_aes_128_wrap_nid_postfix(), Bytes(kek), nullptr) == 0);
    Require(EVP_CIPHER_CTX_cleanup_nid_postfix(&ctx) == 1);
}

static void TestRuntime() {
    GuestBio* bio = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    Require(bio && BIO_up_ref_nid_postfix(bio) == 1);
    Require(BIO_free_nid_postfix(bio) == 1);
    Require(BIO_puts_nid_postfix(bio, "still here") == 10);
    Require(BIO_ctrl_nid_postfix(bio, 10, 0, nullptr) == 10);
    BIO_vfree_nid_postfix(bio);
    BIO_vfree_nid_postfix(nullptr);
    ERR_load_crypto_strings_nid_postfix();
    ERR_clear_error_nid_postfix();
    Require(ERR_peek_last_error_nid_postfix() == 0);
}

int main() {
    TestDigests();
    TestCiphers();
    TestRuntime();
    return 0;
}
