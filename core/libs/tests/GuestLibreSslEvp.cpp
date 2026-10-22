#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

struct alignas(8) GuestEvpMdCtx { unsigned char bytes[48]; };
struct alignas(8) GuestHmacCtx { unsigned char bytes[288]; };
struct GuestEvpMd;
struct alignas(8) GuestEvpCipherCtx { unsigned char bytes[168]; };
struct GuestEvpCipher;
struct GuestCmacCtx;

extern "C" {
const GuestEvpMd* APS5_VABI EVP_md5_nid_postfix();
const GuestEvpMd* APS5_VABI EVP_sha256_nid_postfix();
const GuestEvpMd* APS5_VABI EVP_sha384_nid_postfix();
const GuestEvpMd* APS5_VABI EVP_sha512_nid_postfix();
int APS5_VABI EVP_MD_size_nid_postfix(const GuestEvpMd*);
void APS5_VABI EVP_MD_CTX_init_nid_postfix(GuestEvpMdCtx*);
int APS5_VABI EVP_MD_CTX_cleanup_nid_postfix(GuestEvpMdCtx*);
GuestEvpMdCtx* APS5_VABI EVP_MD_CTX_create_nid_postfix();
void APS5_VABI EVP_MD_CTX_destroy_nid_postfix(GuestEvpMdCtx*);
int APS5_VABI EVP_DigestInit_ex_nid_postfix(GuestEvpMdCtx*, const GuestEvpMd*, void*);
int APS5_VABI EVP_DigestUpdate_nid_postfix(GuestEvpMdCtx*, const void*, std::size_t);
int APS5_VABI EVP_DigestFinal_ex_nid_postfix(GuestEvpMdCtx*, unsigned char*, unsigned int*);
int APS5_VABI EVP_Digest_nid_postfix(const void*, std::size_t, unsigned char*, unsigned int*, const GuestEvpMd*, void*);
void APS5_VABI HMAC_CTX_init_nid_postfix(GuestHmacCtx*);
void APS5_VABI HMAC_CTX_cleanup_nid_postfix(GuestHmacCtx*);
int APS5_VABI HMAC_Init_nid_postfix(GuestHmacCtx*, const void*, int, const GuestEvpMd*);
int APS5_VABI HMAC_Init_ex_nid_postfix(GuestHmacCtx*, const void*, int, const GuestEvpMd*, void*);
int APS5_VABI HMAC_Update_nid_postfix(GuestHmacCtx*, const unsigned char*, std::size_t);
int APS5_VABI HMAC_Final_nid_postfix(GuestHmacCtx*, unsigned char*, unsigned int*);
unsigned char* APS5_VABI HMAC_nid_postfix(const GuestEvpMd*, const void*, int, const unsigned char*, std::size_t, unsigned char*, unsigned int*);
const GuestEvpCipher* APS5_VABI EVP_aes_128_cbc_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_192_cbc_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_256_cbc_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_128_cfb8_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_192_cfb8_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_256_cfb8_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_128_ctr_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_192_ctr_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_256_ctr_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_128_gcm_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_192_gcm_nid_postfix();
const GuestEvpCipher* APS5_VABI EVP_aes_256_gcm_nid_postfix();
int APS5_VABI EVP_CIPHER_block_size_nid_postfix(const GuestEvpCipher*);
void APS5_VABI EVP_CIPHER_CTX_init_nid_postfix(GuestEvpCipherCtx*);
int APS5_VABI EVP_CIPHER_CTX_cleanup_nid_postfix(GuestEvpCipherCtx*);
GuestEvpCipherCtx* APS5_VABI EVP_CIPHER_CTX_new_nid_postfix();
void APS5_VABI EVP_CIPHER_CTX_free_nid_postfix(GuestEvpCipherCtx*);
int APS5_VABI EVP_CIPHER_CTX_set_padding_nid_postfix(GuestEvpCipherCtx*, int);
int APS5_VABI EVP_CIPHER_CTX_ctrl_nid_postfix(GuestEvpCipherCtx*, int, int, void*);
int APS5_VABI EVP_CipherInit_ex_nid_postfix(GuestEvpCipherCtx*, const GuestEvpCipher*, void*, const unsigned char*, const unsigned char*, int);
int APS5_VABI EVP_EncryptInit_ex_nid_postfix(GuestEvpCipherCtx*, const GuestEvpCipher*, void*, const unsigned char*, const unsigned char*);
int APS5_VABI EVP_DecryptInit_ex_nid_postfix(GuestEvpCipherCtx*, const GuestEvpCipher*, void*, const unsigned char*, const unsigned char*);
int APS5_VABI EVP_EncryptUpdate_nid_postfix(GuestEvpCipherCtx*, unsigned char*, int*, const unsigned char*, int);
int APS5_VABI EVP_DecryptUpdate_nid_postfix(GuestEvpCipherCtx*, unsigned char*, int*, const unsigned char*, int);
int APS5_VABI EVP_CipherUpdate_nid_postfix(GuestEvpCipherCtx*, unsigned char*, int*, const unsigned char*, int);
int APS5_VABI EVP_EncryptFinal_ex_nid_postfix(GuestEvpCipherCtx*, unsigned char*, int*);
int APS5_VABI EVP_DecryptFinal_ex_nid_postfix(GuestEvpCipherCtx*, unsigned char*, int*);
int APS5_VABI EVP_CipherFinal_ex_nid_postfix(GuestEvpCipherCtx*, unsigned char*, int*);
GuestCmacCtx* APS5_VABI CMAC_CTX_new_nid_postfix();
void APS5_VABI CMAC_CTX_free_nid_postfix(GuestCmacCtx*);
int APS5_VABI CMAC_Init_nid_postfix(GuestCmacCtx*, const void*, std::size_t, const GuestEvpCipher*, void*);
int APS5_VABI CMAC_Update_nid_postfix(GuestCmacCtx*, const void*, std::size_t);
int APS5_VABI CMAC_Final_nid_postfix(GuestCmacCtx*, unsigned char*, std::size_t*);
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

static bool AllZero(const void* bytes, std::size_t size) {
    const auto* view = static_cast<const unsigned char*>(bytes);
    for (std::size_t index = 0; index < size; ++index) {
        if (view[index]) return false;
    }
    return true;
}

static const char abc[] = "abc";
static const char twoBlocks[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

static void TestEvpDigest() {
    Require(EVP_md5_nid_postfix() && EVP_md5_nid_postfix() == EVP_md5_nid_postfix());
    Require(EVP_MD_size_nid_postfix(EVP_md5_nid_postfix()) == 16);
    Require(EVP_MD_size_nid_postfix(EVP_sha256_nid_postfix()) == 32);
    Require(EVP_MD_size_nid_postfix(EVP_sha384_nid_postfix()) == 48);
    Require(EVP_MD_size_nid_postfix(EVP_sha512_nid_postfix()) == 64);

    GuestEvpMdCtx context;
    std::memset(&context, 0x77, sizeof(context));
    EVP_MD_CTX_init_nid_postfix(&context);
    Require(AllZero(&context, sizeof(context)));
    Require(EVP_DigestInit_ex_nid_postfix(&context, nullptr, nullptr) == 0);
    Require(EVP_DigestInit_ex_nid_postfix(&context, EVP_sha256_nid_postfix(), nullptr) == 1);
    Require(EVP_DigestUpdate_nid_postfix(&context, "ab", 2) == 1);
    Require(EVP_DigestUpdate_nid_postfix(&context, "c", 1) == 1);
    unsigned char digest[64];
    unsigned int size = 0;
    Require(EVP_DigestFinal_ex_nid_postfix(&context, digest, &size) == 1 && size == 32);
    Require(Hex(digest, 32) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    Require(EVP_DigestInit_ex_nid_postfix(&context, nullptr, nullptr) == 1);
    Require(EVP_DigestUpdate_nid_postfix(&context, twoBlocks, std::strlen(twoBlocks)) == 1);
    Require(EVP_DigestFinal_ex_nid_postfix(&context, digest, nullptr) == 1);
    Require(Hex(digest, 32) == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    Require(EVP_DigestInit_ex_nid_postfix(&context, EVP_md5_nid_postfix(), nullptr) == 1);
    Require(EVP_DigestUpdate_nid_postfix(&context, abc, 3) == 1);
    Require(EVP_DigestFinal_ex_nid_postfix(&context, digest, &size) == 1 && size == 16);
    Require(Hex(digest, 16) == "900150983cd24fb0d6963f7d28e17f72");
    Require(EVP_MD_CTX_cleanup_nid_postfix(&context) == 1);
    Require(AllZero(&context, sizeof(context)));
    Require(Throws([&] { EVP_DigestUpdate_nid_postfix(&context, abc, 3); }));

    GuestEvpMdCtx* created = EVP_MD_CTX_create_nid_postfix();
    Require(created != nullptr);
    Require(EVP_DigestInit_ex_nid_postfix(created, EVP_sha384_nid_postfix(), nullptr) == 1);
    Require(EVP_DigestUpdate_nid_postfix(created, abc, 3) == 1);
    Require(EVP_DigestFinal_ex_nid_postfix(created, digest, &size) == 1 && size == 48);
    Require(Hex(digest, 48) == "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7");
    EVP_MD_CTX_destroy_nid_postfix(created);
    EVP_MD_CTX_destroy_nid_postfix(nullptr);

    Require(EVP_Digest_nid_postfix(abc, 3, digest, &size, EVP_sha512_nid_postfix(), nullptr) == 1 && size == 64);
    Require(Hex(digest, 64) ==
        "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
    Require(EVP_Digest_nid_postfix(abc, 3, digest, &size, nullptr, nullptr) == 0);
}

static void TestHmac() {
    unsigned char key[131];
    std::memset(key, 0x0b, 20);
    const auto* hiThere = reinterpret_cast<const unsigned char*>("Hi There");
    unsigned char mac[64];
    unsigned int size = 0;
    Require(HMAC_nid_postfix(EVP_sha256_nid_postfix(), key, 20, hiThere, 8, mac, &size) == mac && size == 32);
    Require(Hex(mac, 32) == "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    Require(HMAC_nid_postfix(EVP_sha512_nid_postfix(), key, 20, hiThere, 8, mac, &size) == mac && size == 64);
    Require(Hex(mac, 64) ==
        "87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854");
    const unsigned char* shared = HMAC_nid_postfix(EVP_md5_nid_postfix(), key, 16, hiThere, 8, nullptr, nullptr);
    Require(shared && Hex(shared, 16) == "9294727a3638bb1c13f48ef8158bfc9d");

    GuestHmacCtx context;
    std::memset(&context, 0x33, sizeof(context));
    HMAC_CTX_init_nid_postfix(&context);
    Require(AllZero(&context, sizeof(context)));
    Require(HMAC_Init_ex_nid_postfix(&context, key, 20, EVP_sha256_nid_postfix(), nullptr) == 1);
    Require(HMAC_Update_nid_postfix(&context, hiThere, 3) == 1);
    Require(HMAC_Update_nid_postfix(&context, hiThere + 3, 5) == 1);
    Require(HMAC_Final_nid_postfix(&context, mac, &size) == 1 && size == 32);
    Require(Hex(mac, 32) == "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    Require(HMAC_Init_ex_nid_postfix(&context, nullptr, 0, nullptr, nullptr) == 1);
    Require(HMAC_Update_nid_postfix(&context, hiThere, 8) == 1);
    Require(HMAC_Final_nid_postfix(&context, mac, nullptr) == 1);
    Require(Hex(mac, 32) == "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");

    std::memset(key, 0xaa, sizeof(key));
    const char* largeKey = "Test Using Larger Than Block-Size Key - Hash Key First";
    Require(HMAC_Init_ex_nid_postfix(&context, key, 131, nullptr, nullptr) == 1);
    Require(HMAC_Update_nid_postfix(&context, reinterpret_cast<const unsigned char*>(largeKey), std::strlen(largeKey)) == 1);
    Require(HMAC_Final_nid_postfix(&context, mac, &size) == 1 && size == 32);
    Require(Hex(mac, 32) == "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
    HMAC_CTX_cleanup_nid_postfix(&context);
    Require(AllZero(&context, sizeof(context)));
    Require(Throws([&] { HMAC_Update_nid_postfix(&context, hiThere, 8); }));

    GuestHmacCtx fresh;
    std::memset(&fresh, 0x44, sizeof(fresh));
    std::memset(key, 0x0b, 20);
    Require(HMAC_Init_nid_postfix(&fresh, key, 20, EVP_sha256_nid_postfix()) == 1);
    Require(HMAC_Update_nid_postfix(&fresh, hiThere, 8) == 1);
    Require(HMAC_Final_nid_postfix(&fresh, mac, &size) == 1 && size == 32);
    Require(Hex(mac, 32) == "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    HMAC_CTX_cleanup_nid_postfix(&fresh);
}

static const char spKey[] = "2b7e151628aed2a6abf7158809cf4f3c";
static const char spIv[] = "000102030405060708090a0b0c0d0e0f";
static const char spPlain[] = "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710";
static const char spCbc[] = "7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b273bed6b8e3c1743b7116e69e222295163ff1caa1681fac09120eca307586e1a7";

static std::string Run(GuestEvpCipherCtx* ctx, bool encrypt, const std::string& input, const int* chunks, int count, bool final, int* lastOut = nullptr) {
    std::string output(input.size() + 32, '\0');
    auto* out = reinterpret_cast<unsigned char*>(output.data());
    int total = 0;
    std::size_t offset = 0;
    for (int index = 0; index < count; ++index) {
        int produced = -1;
        const int ok = encrypt ? EVP_EncryptUpdate_nid_postfix(ctx, out + total, &produced, Bytes(input) + offset, chunks[index])
                               : EVP_DecryptUpdate_nid_postfix(ctx, out + total, &produced, Bytes(input) + offset, chunks[index]);
        Require(ok == 1 && produced >= 0);
        if (lastOut) lastOut[index] = produced;
        total += produced;
        offset += static_cast<std::size_t>(chunks[index]);
    }
    Require(offset == input.size());
    if (final) {
        int produced = -1;
        Require((encrypt ? EVP_EncryptFinal_ex_nid_postfix(ctx, out + total, &produced) : EVP_DecryptFinal_ex_nid_postfix(ctx, out + total, &produced)) == 1);
        total += produced;
    }
    output.resize(static_cast<std::size_t>(total));
    return output;
}

static void TestEvpCipher() {
    Require(EVP_CIPHER_block_size_nid_postfix(EVP_aes_128_cbc_nid_postfix()) == 16);
    Require(EVP_CIPHER_block_size_nid_postfix(EVP_aes_256_cbc_nid_postfix()) == 16);
    Require(EVP_CIPHER_block_size_nid_postfix(EVP_aes_192_ctr_nid_postfix()) == 1);
    Require(EVP_CIPHER_block_size_nid_postfix(EVP_aes_192_cfb8_nid_postfix()) == 1);
    Require(EVP_CIPHER_block_size_nid_postfix(EVP_aes_192_gcm_nid_postfix()) == 1);
    Require(EVP_aes_128_cbc_nid_postfix() == EVP_aes_128_cbc_nid_postfix() && EVP_aes_128_cbc_nid_postfix() != EVP_aes_192_cbc_nid_postfix());

    const std::string key = FromHex(spKey);
    const std::string iv = FromHex(spIv);
    const std::string plain = FromHex(spPlain);
    GuestEvpCipherCtx ctx;
    std::memset(&ctx, 0x6b, sizeof(ctx));
    EVP_CIPHER_CTX_init_nid_postfix(&ctx);
    Require(AllZero(&ctx, sizeof(ctx)));
    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, nullptr, nullptr, Bytes(key), Bytes(iv)) == 0);
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x9, 12, nullptr) == 0);

    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, EVP_aes_128_cbc_nid_postfix(), nullptr, Bytes(key), Bytes(iv)) == 1);
    Require(EVP_CIPHER_CTX_set_padding_nid_postfix(&ctx, 0) == 1);
    const int encryptChunks[3] = {5, 30, 29};
    int produced[3];
    Require(Hex(Bytes(Run(&ctx, true, plain, encryptChunks, 3, true, produced)), 64) == spCbc);
    Require(produced[0] == 0 && produced[1] == 32 && produced[2] == 32);
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x9, 12, nullptr) == 0);

    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, nullptr, nullptr, nullptr, Bytes(iv)) == 1);
    const int partial[1] = {20};
    Run(&ctx, true, plain.substr(0, 20), partial, 1, false);
    unsigned char block[32];
    int length = -1;
    Require(EVP_EncryptFinal_ex_nid_postfix(&ctx, block, &length) == 0 && length == 0);

    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, EVP_aes_128_cbc_nid_postfix(), nullptr, Bytes(key), Bytes(iv)) == 1);
    Require(EVP_CIPHER_CTX_set_padding_nid_postfix(&ctx, 1) == 1);
    const int whole[1] = {64};
    const std::string padded = Run(&ctx, true, plain, whole, 1, true);
    Require(padded.size() == 80 && Hex(Bytes(padded), 64) == spCbc);

    Require(EVP_DecryptInit_ex_nid_postfix(&ctx, EVP_aes_128_cbc_nid_postfix(), nullptr, Bytes(key), Bytes(iv)) == 1);
    const int decryptChunks[2] = {17, 63};
    int decrypted[2];
    Require(Run(&ctx, false, padded, decryptChunks, 2, true, decrypted) == plain);
    Require(decrypted[0] == 16 && decrypted[1] == 48);
    Require(EVP_DecryptInit_ex_nid_postfix(&ctx, nullptr, nullptr, nullptr, Bytes(iv)) == 1);
    const int all[1] = {80};
    Require(Run(&ctx, false, padded, all, 1, true, decrypted) == plain && decrypted[0] == 64);

    Require(EVP_DecryptInit_ex_nid_postfix(&ctx, nullptr, nullptr, nullptr, Bytes(iv)) == 1);
    const std::string cbc = FromHex(spCbc);
    Run(&ctx, false, cbc, whole, 1, false);
    Require(EVP_DecryptFinal_ex_nid_postfix(&ctx, block, &length) == 0 && length == 0);
    Require(EVP_DecryptInit_ex_nid_postfix(&ctx, nullptr, nullptr, nullptr, Bytes(iv)) == 1);
    Run(&ctx, false, cbc.substr(0, 20), partial, 1, false);
    Require(EVP_DecryptFinal_ex_nid_postfix(&ctx, block, &length) == 0);
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x11, 16, block) == 0);

    Require(EVP_CipherInit_ex_nid_postfix(&ctx, EVP_aes_256_cbc_nid_postfix(), nullptr,
        Bytes(FromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4")), Bytes(iv), 1) == 1);
    Require(EVP_CIPHER_CTX_set_padding_nid_postfix(&ctx, 0) == 1);
    int out = 0;
    Require(EVP_CipherUpdate_nid_postfix(&ctx, block, &out, Bytes(plain), 16) == 1 && out == 16);
    Require(Hex(block, 16) == "f58c4c04d6e5f1ba779eabfb5f7bfbd6");
    Require(EVP_CipherFinal_ex_nid_postfix(&ctx, block, &out) == 1 && out == 0);
    Require(EVP_CipherInit_ex_nid_postfix(&ctx, nullptr, nullptr, nullptr, nullptr, -1) == 1);
    Require(Throws([&] { EVP_DecryptUpdate_nid_postfix(&ctx, block, &out, Bytes(plain), 16); }));
    Require(EVP_CIPHER_CTX_cleanup_nid_postfix(&ctx) == 1);
    Require(AllZero(&ctx, sizeof(ctx)));

    GuestEvpCipherCtx* stream = EVP_CIPHER_CTX_new_nid_postfix();
    Require(stream && AllZero(stream, sizeof(*stream)));
    const std::string cfbPlain = plain.substr(0, 18);
    Require(EVP_EncryptInit_ex_nid_postfix(stream, EVP_aes_128_cfb8_nid_postfix(), nullptr, Bytes(key), Bytes(iv)) == 1);
    const int cfbChunks[2] = {7, 11};
    const std::string cfb = Run(stream, true, cfbPlain, cfbChunks, 2, true, produced);
    Require(Hex(Bytes(cfb), 18) == "3b79424c9c0dd436bace9e0ed4586a4f32b9" && produced[0] == 7 && produced[1] == 11);
    Require(EVP_DecryptInit_ex_nid_postfix(stream, EVP_aes_128_cfb8_nid_postfix(), nullptr, Bytes(key), Bytes(iv)) == 1);
    Require(Run(stream, false, cfb, cfbChunks, 2, true) == cfbPlain);

    const std::string counter = FromHex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    Require(EVP_EncryptInit_ex_nid_postfix(stream, EVP_aes_128_ctr_nid_postfix(), nullptr, Bytes(key), Bytes(counter)) == 1);
    const int ctrChunks[3] = {3, 29, 32};
    Require(Hex(Bytes(Run(stream, true, plain, ctrChunks, 3, true)), 64) ==
        "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee");
    EVP_CIPHER_CTX_free_nid_postfix(stream);
    EVP_CIPHER_CTX_free_nid_postfix(nullptr);
}

static void TestGcm() {
    GuestEvpCipherCtx ctx;
    EVP_CIPHER_CTX_init_nid_postfix(&ctx);
    const std::string zero(16, '\0');
    unsigned char tag[16];
    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, EVP_aes_128_gcm_nid_postfix(), nullptr, Bytes(zero), Bytes(zero)) == 1);
    const int one[1] = {16};
    Require(Hex(Bytes(Run(&ctx, true, zero, one, 1, true)), 16) == "0388dace60b6a392f328c2b971b2fe78");
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x10, 16, tag) == 1);
    Require(Hex(tag, 16) == "ab6e47d42cec13bdf53a67b21257bddf");
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x10, 17, tag) == 0);

    const std::string key = FromHex("feffe9928665731c6d6a8f9467308308");
    const std::string plain = FromHex("d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39");
    const std::string aad = FromHex("feedfacedeadbeeffeedfacedeadbeefabaddad2");
    const std::string iv = FromHex("cafebabefacedbaddecaf888");
    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, EVP_aes_128_gcm_nid_postfix(), nullptr, nullptr, nullptr) == 1);
    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, nullptr, nullptr, Bytes(key), Bytes(iv)) == 1);
    int out = -1;
    Require(EVP_EncryptUpdate_nid_postfix(&ctx, nullptr, &out, Bytes(aad), 7) == 1 && out == 7);
    Require(EVP_EncryptUpdate_nid_postfix(&ctx, nullptr, &out, Bytes(aad) + 7, 13) == 1 && out == 13);
    const int chunks[3] = {5, 32, 23};
    const std::string sealed = Run(&ctx, true, plain, chunks, 3, true);
    Require(Hex(Bytes(sealed), 60) ==
        "42831ec2217774244b7221b784d0d49ce3aa212f2c02a4e035c17e2329aca12e21d514b25466931c7d8f6a5aac84aa051ba30b396a0aac973d58e091");
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x10, 16, tag) == 1);
    Require(Hex(tag, 16) == "5bc94fbc3221a5db94fae95ae7121a47");

    Require(EVP_DecryptInit_ex_nid_postfix(&ctx, EVP_aes_128_gcm_nid_postfix(), nullptr, Bytes(key), Bytes(iv)) == 1);
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x11, 16, tag) == 1);
    Require(EVP_DecryptUpdate_nid_postfix(&ctx, nullptr, &out, Bytes(aad), 20) == 1);
    Require(Run(&ctx, false, sealed, chunks, 3, true) == plain);
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x10, 16, tag) == 0);

    Require(EVP_DecryptInit_ex_nid_postfix(&ctx, nullptr, nullptr, nullptr, Bytes(iv)) == 1);
    tag[0] ^= 1;
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x11, 16, tag) == 1);
    Require(EVP_DecryptUpdate_nid_postfix(&ctx, nullptr, &out, Bytes(aad), 20) == 1);
    Run(&ctx, false, sealed, chunks, 3, false);
    Require(EVP_DecryptFinal_ex_nid_postfix(&ctx, tag, &out) == 0);

    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, EVP_aes_128_gcm_nid_postfix(), nullptr, nullptr, nullptr) == 1);
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x9, 8, nullptr) == 1);
    Require(EVP_EncryptInit_ex_nid_postfix(&ctx, nullptr, nullptr, Bytes(key), Bytes(FromHex("cafebabefacedbad"))) == 1);
    Require(EVP_EncryptUpdate_nid_postfix(&ctx, nullptr, &out, Bytes(aad), 20) == 1);
    const int whole[1] = {60};
    Require(Hex(Bytes(Run(&ctx, true, plain, whole, 1, true)), 60) ==
        "61353b4c2806934a777ff51fa22a4755699b2a714fcdc6f83766e5f97b6c742373806900e49f24b22b097544d4896b424989b5e1ebac0f07c23f4598");
    Require(EVP_CIPHER_CTX_ctrl_nid_postfix(&ctx, 0x10, 16, tag) == 1);
    Require(Hex(tag, 16) == "3612d2e79e3b0785561be14aaca2fccb");
    Require(EVP_EncryptUpdate_nid_postfix(&ctx, tag, &out, Bytes(plain), 16) == 0);
    Require(EVP_CIPHER_CTX_cleanup_nid_postfix(&ctx) == 1);
}

static void TestCmac() {
    const std::string key = FromHex(spKey);
    const std::string plain = FromHex(spPlain);
    GuestCmacCtx* ctx = CMAC_CTX_new_nid_postfix();
    Require(ctx != nullptr);
    unsigned char mac[16];
    std::size_t length = 0;
    Require(CMAC_Update_nid_postfix(ctx, plain.data(), 16) == 0);
    Require(CMAC_Final_nid_postfix(ctx, mac, &length) == 0);
    Require(CMAC_Init_nid_postfix(ctx, nullptr, 0, nullptr, nullptr) == 0);
    Require(CMAC_Init_nid_postfix(ctx, key.data(), 24, EVP_aes_128_cbc_nid_postfix(), nullptr) == 0);
    Require(CMAC_Init_nid_postfix(ctx, key.data(), 16, EVP_aes_128_cbc_nid_postfix(), nullptr) == 1);
    Require(CMAC_Final_nid_postfix(ctx, mac, &length) == 1 && length == 16);
    Require(Hex(mac, 16) == "bb1d6929e95937287fa37d129b756746");
    Require(CMAC_Init_nid_postfix(ctx, nullptr, 0, nullptr, nullptr) == 1);
    Require(CMAC_Update_nid_postfix(ctx, plain.data(), 16) == 1);
    Require(CMAC_Final_nid_postfix(ctx, mac, &length) == 1);
    Require(Hex(mac, 16) == "070a16b46b4d4144f79bdd9dd04a287c");
    Require(CMAC_Init_nid_postfix(ctx, nullptr, 0, nullptr, nullptr) == 1);
    Require(CMAC_Update_nid_postfix(ctx, plain.data(), 9) == 1);
    Require(CMAC_Update_nid_postfix(ctx, plain.data() + 9, 31) == 1);
    Require(CMAC_Update_nid_postfix(ctx, nullptr, 0) == 1);
    length = 0;
    Require(CMAC_Final_nid_postfix(ctx, nullptr, &length) == 1 && length == 16);
    Require(CMAC_Final_nid_postfix(ctx, mac, &length) == 1);
    Require(Hex(mac, 16) == "dfa66747de9ae63030ca32611497c827");
    Require(Throws([&] { CMAC_Update_nid_postfix(ctx, plain.data(), 16); }));
    Require(CMAC_Init_nid_postfix(ctx, key.data(), 16, nullptr, nullptr) == 1);
    Require(CMAC_Update_nid_postfix(ctx, plain.data(), 64) == 1);
    Require(CMAC_Final_nid_postfix(ctx, mac, &length) == 1);
    Require(Hex(mac, 16) == "51f0bebf7e3b9d92fc49741779363cfe");
    CMAC_CTX_free_nid_postfix(ctx);
    CMAC_CTX_free_nid_postfix(nullptr);
}

int main() {
    TestEvpDigest();
    TestHmac();
    TestEvpCipher();
    TestGcm();
    TestCmac();
    return 0;
}
