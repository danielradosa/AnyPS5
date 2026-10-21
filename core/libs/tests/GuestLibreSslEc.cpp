#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

struct GuestBignum {
    std::uint64_t* d;
    int top;
    int dmax;
    int neg;
    int flags;
};

struct GuestEvpPkey {
    int type;
    int save_type;
    int references;
    const void* ameth;
    void* engine;
    void* pkey;
    int save_parameters;
    void* attributes;
};

struct GuestEvpMdCtx {
    const void* digest;
    void* engine;
    std::uint64_t flags;
    void* md_data;
    void* pctx;
    void* update;
};

struct GuestGroup;
struct GuestPoint;
struct GuestEcKey;
struct GuestBio;
struct GuestMd;

extern "C" {
GuestGroup* APS5_VABI EC_GROUP_new_by_curve_name_nid_postfix(int);
void APS5_VABI EC_GROUP_free_nid_postfix(GuestGroup*);
int APS5_VABI EC_GROUP_get_degree_nid_postfix(const GuestGroup*);
GuestPoint* APS5_VABI EC_POINT_new_nid_postfix(const GuestGroup*);
void APS5_VABI EC_POINT_free_nid_postfix(GuestPoint*);
int APS5_VABI EC_POINT_mul_nid_postfix(const GuestGroup*, GuestPoint*, const GuestBignum*, const GuestPoint*, const GuestBignum*, void*);
std::size_t APS5_VABI EC_POINT_point2oct_nid_postfix(const GuestGroup*, const GuestPoint*, int, unsigned char*, std::size_t, void*);
int APS5_VABI EC_POINT_oct2point_nid_postfix(const GuestGroup*, GuestPoint*, const unsigned char*, std::size_t, void*);
GuestEcKey* APS5_VABI EC_KEY_new_nid_postfix();
GuestEcKey* APS5_VABI EC_KEY_new_by_curve_name_nid_postfix(int);
void APS5_VABI EC_KEY_free_nid_postfix(GuestEcKey*);
const GuestGroup* APS5_VABI EC_KEY_get0_group_nid_postfix(const GuestEcKey*);
int APS5_VABI EC_KEY_set_group_nid_postfix(GuestEcKey*, const GuestGroup*);
const GuestBignum* APS5_VABI EC_KEY_get0_private_key_nid_postfix(const GuestEcKey*);
int APS5_VABI EC_KEY_set_private_key_nid_postfix(GuestEcKey*, const GuestBignum*);
const GuestPoint* APS5_VABI EC_KEY_get0_public_key_nid_postfix(const GuestEcKey*);
int APS5_VABI EC_KEY_set_public_key_nid_postfix(GuestEcKey*, const GuestPoint*);
int APS5_VABI EC_KEY_generate_key_nid_postfix(GuestEcKey*);
int APS5_VABI EC_KEY_check_key_nid_postfix(const GuestEcKey*);
int APS5_VABI ECDSA_size_nid_postfix(const GuestEcKey*);
int APS5_VABI ECDSA_sign_nid_postfix(int, const unsigned char*, int, unsigned char*, unsigned int*, GuestEcKey*);
int APS5_VABI ECDSA_verify_nid_postfix(int, const unsigned char*, int, const unsigned char*, int, GuestEcKey*);
int APS5_VABI ECDH_compute_key_nid_postfix(void*, std::size_t, const GuestPoint*, GuestEcKey*, void* (APS5_VABI*)(const void*, std::size_t, void*, std::size_t*));
GuestEvpPkey* APS5_VABI PEM_read_bio_PrivateKey_nid_postfix(GuestBio*, GuestEvpPkey**, void*, void*);
void APS5_VABI EVP_PKEY_free_nid_postfix(GuestEvpPkey*);
GuestEcKey* APS5_VABI EVP_PKEY_get1_EC_KEY_nid_postfix(GuestEvpPkey*);
int APS5_VABI EVP_PKEY_copy_parameters_nid_postfix(GuestEvpPkey*, const GuestEvpPkey*);
int APS5_VABI EVP_DigestVerifyInit_nid_postfix(GuestEvpMdCtx*, void**, const GuestMd*, void*, GuestEvpPkey*);
int APS5_VABI EVP_DigestVerifyFinal_nid_postfix(GuestEvpMdCtx*, const unsigned char*, std::size_t);
GuestEvpMdCtx* APS5_VABI EVP_MD_CTX_create_nid_postfix();
void APS5_VABI EVP_MD_CTX_destroy_nid_postfix(GuestEvpMdCtx*);
int APS5_VABI EVP_DigestUpdate_nid_postfix(GuestEvpMdCtx*, const void*, std::size_t);
const GuestMd* APS5_VABI EVP_sha256_nid_postfix();
GuestBignum* APS5_VABI BN_new_nid_postfix();
void APS5_VABI BN_free_nid_postfix(GuestBignum*);
int APS5_VABI BN_set_word_nid_postfix(GuestBignum*, std::uint64_t);
int APS5_VABI BN_bn2bin_nid_postfix(const GuestBignum*, unsigned char*);
GuestBio* APS5_VABI BIO_new_mem_buf_nid_postfix(void*, int);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
}

static const char EcPem[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIN59H0jvfHQ6H7PSNAAmi6lw/aE2tke2uMSbluI/ESDWoAoGCCqGSM49\n"
    "AwEHoUQDQgAET/waFkHEUMHGzcxCVQs6KmQrbCGJxkdWdZvnFsKX+2LsXFx6NYa/\n"
    "ol/+mTBntyNqgNfcv28TvzKiUHkMWZ3q+A==\n"
    "-----END EC PRIVATE KEY-----\n";

static const char EcPkcs8Pem[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQg3n0fSO98dDofs9I0\n"
    "ACaLqXD9oTa2R7a4xJuW4j8RINahRANCAARP/BoWQcRQwcbNzEJVCzoqZCtsIYnG\n"
    "R1Z1m+cWwpf7YuxcXHo1hr+iX/6ZMGe3I2qA19y/bxO/MqJQeQxZner4\n"
    "-----END PRIVATE KEY-----\n";

static const char EcPeerPem[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIGIndAWc3uvMmiy7vNRBldY+p+Dm32I1pZaA10ybtL/AoAoGCCqGSM49\n"
    "AwEHoUQDQgAEBRzFLvNUGS7TArV/nRsBPiD/vysNJqU2hd8blwviNq7Ls/TAx+TA\n"
    "1uVEEjVu2gvbspmmsVeRFbmk0brZ5qq7lQ==\n"
    "-----END EC PRIVATE KEY-----\n";

static const char EcPublicHex[] =
    "044ffc1a1641c450c1c6cdcc42550b3a2a642b6c2189c64756759be716c297fb"
    "62ec5c5c7a3586bfa25ffe993067b7236a80d7dcbf6f13bf32a250790c599dea"
    "f8";

static const char EcPublicCompressedHex[] =
    "024ffc1a1641c450c1c6cdcc42550b3a2a642b6c2189c64756759be716c297fb"
    "62";

static const char EcPeerPublicHex[] =
    "04051cc52ef354192ed302b57f9d1b013e20ffbf2b0d26a53685df1b970be236"
    "aecbb3f4c0c7e4c0d6e54412356eda0bdbb299a6b1579115b9a4d1bad9e6aabb"
    "95";

static const char EcPrivateHex[] =
    "de7d1f48ef7c743a1fb3d23400268ba970fda136b647b6b8c49b96e23f1120d6";

static const char EcdhSecretHex[] =
    "c53da120b7d55ecd9fae3b1570397698546fb5045b6f8723d488b169f383c02d";

static const char EcdsaSignatureHex[] =
    "3046022100e3eda97a227576d587c33eb02ca5249158f0766c4ca0dcd5f9931a"
    "da02e93e84022100d24e771ad449c0a32c3a3a4e0d92ac1041cd9ffd7539d7d0"
    "57f3e1f1c1f0f6a2";

static const char EcSpkiHex[] =
    "3059301306072a8648ce3d020106082a8648ce3d030107034200044ffc1a1641"
    "c450c1c6cdcc42550b3a2a642b6c2189c64756759be716c297fb62ec5c5c7a35"
    "86bfa25ffe993067b7236a80d7dcbf6f13bf32a250790c599deaf8";

static const unsigned char Digest[32] = {0x51, 0xdd, 0x84, 0x95, 0x6e, 0x17, 0x89, 0x4a, 0x10, 0x2a, 0x43, 0x10, 0xb3, 0x7f, 0x91, 0x65,
    0x68, 0x85, 0xdd, 0x9f, 0x5b, 0xd8, 0x12, 0xef, 0xcd, 0x36, 0x97, 0x0c, 0x2f, 0xba, 0x5c, 0x43};
static const char Message[] = "AnyPS5 LibreSSL key test";

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

static std::vector<unsigned char> Hex(const char* text) {
    std::vector<unsigned char> out;
    for (std::size_t index = 0; text[index] && text[index + 1]; index += 2) {
        const auto nibble = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
        out.push_back(static_cast<unsigned char>(nibble(text[index]) << 4 | nibble(text[index + 1])));
    }
    return out;
}

static std::uint64_t Code(std::uint64_t library, std::uint64_t function, std::uint64_t reason) {
    return library << 24 | function << 12 | reason;
}

static bool LastError(std::uint64_t expected) {
    std::uint64_t last = 0;
    while (const auto code = ERR_get_error_nid_postfix()) last = code;
    return last == expected;
}

static GuestEvpPkey* ReadKey(const char* pem) {
    std::string text(pem);
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(text.data(), static_cast<int>(text.size()));
    GuestEvpPkey* pkey = PEM_read_bio_PrivateKey_nid_postfix(bio, nullptr, nullptr, nullptr);
    BIO_free_nid_postfix(bio);
    return pkey;
}

static std::vector<unsigned char> Encode(const GuestGroup* group, const GuestPoint* point, int form) {
    std::vector<unsigned char> out(EC_POINT_point2oct_nid_postfix(group, point, form, nullptr, 0, nullptr));
    Require(EC_POINT_point2oct_nid_postfix(group, point, form, out.data(), out.size(), nullptr) == out.size());
    return out;
}

static void TestGroups() {
    GuestGroup* p256 = EC_GROUP_new_by_curve_name_nid_postfix(415);
    Require(p256 && EC_GROUP_get_degree_nid_postfix(p256) == 256);
    GuestGroup* p521 = EC_GROUP_new_by_curve_name_nid_postfix(716);
    Require(p521 && EC_GROUP_get_degree_nid_postfix(p521) == 521);
    GuestGroup* k256 = EC_GROUP_new_by_curve_name_nid_postfix(714);
    Require(k256 && EC_GROUP_get_degree_nid_postfix(k256) == 256);
    GuestGroup* bp384 = EC_GROUP_new_by_curve_name_nid_postfix(932);
    Require(bp384 && EC_GROUP_get_degree_nid_postfix(bp384) == 384);
    ERR_clear_error_nid_postfix();
    Require(EC_GROUP_new_by_curve_name_nid_postfix(672) == nullptr && LastError(Code(16, 174, 129)));
    Require(Throws([] { EC_GROUP_new_by_curve_name_nid_postfix(410); }));
    GuestPoint* point = EC_POINT_new_nid_postfix(p256);
    GuestPoint* other = EC_POINT_new_nid_postfix(p521);
    const auto encoded = Hex(EcPublicHex);
    Require(EC_POINT_oct2point_nid_postfix(p256, point, encoded.data(), encoded.size(), nullptr) == 1);
    Require(Encode(p256, point, 4) == encoded && Encode(p256, point, 2) == Hex(EcPublicCompressedHex));
    const auto compressed = Hex(EcPublicCompressedHex);
    GuestPoint* copy = EC_POINT_new_nid_postfix(p256);
    Require(EC_POINT_oct2point_nid_postfix(p256, copy, compressed.data(), compressed.size(), nullptr) == 1 && Encode(p256, copy, 4) == encoded);
    unsigned char small[64];
    Require(EC_POINT_point2oct_nid_postfix(p256, point, 4, small, sizeof(small), nullptr) == 0 && LastError(Code(16, 123, 100)));
    Require(EC_POINT_point2oct_nid_postfix(p256, point, 5, small, sizeof(small), nullptr) == 0 && LastError(Code(16, 123, 104)));
    Require(EC_POINT_point2oct_nid_postfix(p521, point, 4, nullptr, 0, nullptr) == 0 && LastError(Code(16, 123, 101)));
    auto broken = encoded;
    broken[64] ^= 1;
    Require(EC_POINT_oct2point_nid_postfix(p256, copy, broken.data(), broken.size(), nullptr) == 0 && LastError(Code(16, 122, 107)));
    Require(EC_POINT_oct2point_nid_postfix(p256, copy, encoded.data(), 64, nullptr) == 0 && LastError(Code(16, 122, 102)));
    const unsigned char infinity[1] = {0};
    Require(EC_POINT_oct2point_nid_postfix(p256, copy, infinity, 1, nullptr) == 1 && Encode(p256, copy, 4) == std::vector<unsigned char>({0}));
    GuestBignum* scalar = BN_new_nid_postfix();
    BN_set_word_nid_postfix(scalar, 1);
    Require(EC_POINT_mul_nid_postfix(p256, copy, nullptr, point, scalar, nullptr) == 1 && Encode(p256, copy, 4) == encoded);
    BN_set_word_nid_postfix(scalar, 0);
    Require(EC_POINT_mul_nid_postfix(p256, copy, scalar, nullptr, nullptr, nullptr) == 1 && Encode(p256, copy, 4) == std::vector<unsigned char>({0}));
    Require(EC_POINT_mul_nid_postfix(p256, other, scalar, nullptr, nullptr, nullptr) == 0 && LastError(Code(16, 184, 101)));
    Require(Throws([&] { EC_POINT_mul_nid_postfix(p256, copy, scalar, point, nullptr, nullptr); }));
    BN_free_nid_postfix(scalar);
    EC_POINT_free_nid_postfix(copy);
    EC_POINT_free_nid_postfix(other);
    EC_POINT_free_nid_postfix(point);
    EC_GROUP_free_nid_postfix(bp384);
    EC_GROUP_free_nid_postfix(k256);
    EC_GROUP_free_nid_postfix(p521);
    EC_GROUP_free_nid_postfix(p256);
    EC_GROUP_free_nid_postfix(nullptr);
}

static std::vector<unsigned char> Bytes(const GuestBignum* bn) {
    std::vector<unsigned char> out(128);
    out.resize(static_cast<std::size_t>(BN_bn2bin_nid_postfix(bn, out.data())));
    return out;
}

static void* APS5_VABI Reverse(const void* in, std::size_t inlen, void* out, std::size_t* outlen) {
    Require(inlen == 32 && *outlen == 40);
    for (std::size_t index = 0; index < inlen; ++index) static_cast<unsigned char*>(out)[index] = static_cast<const unsigned char*>(in)[inlen - 1 - index];
    *outlen = 20;
    return out;
}

static void* APS5_VABI Refuse(const void*, std::size_t, void*, std::size_t*) {
    return nullptr;
}

static void TestKeys() {
    GuestEvpPkey* pkey = ReadKey(EcPem);
    Require(pkey && pkey->type == 408);
    GuestEvpPkey* pkcs8 = ReadKey(EcPkcs8Pem);
    Require(pkcs8 && pkcs8->type == 408);
    GuestEcKey* key = EVP_PKEY_get1_EC_KEY_nid_postfix(pkey);
    Require(key && key == pkey->pkey);
    const GuestGroup* group = EC_KEY_get0_group_nid_postfix(key);
    Require(group && EC_GROUP_get_degree_nid_postfix(group) == 256);
    Require(Bytes(EC_KEY_get0_private_key_nid_postfix(key)) == Hex(EcPrivateHex));
    Require(Encode(group, EC_KEY_get0_public_key_nid_postfix(key), 4) == Hex(EcPublicHex));
    Require(Bytes(EC_KEY_get0_private_key_nid_postfix(static_cast<GuestEcKey*>(pkcs8->pkey))) == Hex(EcPrivateHex));
    Require(EC_KEY_check_key_nid_postfix(key) == 1);
    Require(ECDSA_size_nid_postfix(key) == 72);

    GuestPoint* derived = EC_POINT_new_nid_postfix(group);
    Require(EC_POINT_mul_nid_postfix(group, derived, EC_KEY_get0_private_key_nid_postfix(key), nullptr, nullptr, nullptr) == 1);
    Require(Encode(group, derived, 4) == Hex(EcPublicHex));
    GuestBignum* two = BN_new_nid_postfix();
    BN_set_word_nid_postfix(two, 2);
    GuestPoint* doubled = EC_POINT_new_nid_postfix(group);
    GuestPoint* combined = EC_POINT_new_nid_postfix(group);
    Require(EC_POINT_mul_nid_postfix(group, doubled, nullptr, derived, two, nullptr) == 1);
    BN_set_word_nid_postfix(two, 1);
    Require(EC_POINT_mul_nid_postfix(group, combined, EC_KEY_get0_private_key_nid_postfix(key), derived, two, nullptr) == 1);
    Require(Encode(group, combined, 4) == Encode(group, doubled, 4));

    const auto signature = Hex(EcdsaSignatureHex);
    Require(ECDSA_verify_nid_postfix(0, Digest, 32, signature.data(), static_cast<int>(signature.size()), key) == 1);
    auto changed = signature;
    changed[changed.size() - 1] ^= 1;
    Require(ECDSA_verify_nid_postfix(0, Digest, 32, changed.data(), static_cast<int>(changed.size()), key) == 0);
    auto trailing = signature;
    trailing.push_back(0);
    Require(ECDSA_verify_nid_postfix(0, Digest, 32, trailing.data(), static_cast<int>(trailing.size()), key) == -1);
    unsigned char sig[72];
    unsigned int length = 0;
    Require(ECDSA_sign_nid_postfix(0, Digest, 32, sig, &length, key) == 1 && length <= 72 && length >= 8);
    Require(ECDSA_verify_nid_postfix(0, Digest, 32, sig, static_cast<int>(length), key) == 1);
    Require(ECDSA_verify_nid_postfix(0, Digest, 31, sig, static_cast<int>(length), key) == 0);

    GuestEvpMdCtx* ctx = EVP_MD_CTX_create_nid_postfix();
    Require(EVP_DigestVerifyInit_nid_postfix(ctx, nullptr, EVP_sha256_nid_postfix(), nullptr, pkey) == 1);
    Require(EVP_DigestUpdate_nid_postfix(ctx, Message, std::strlen(Message)) == 1);
    Require(EVP_DigestVerifyFinal_nid_postfix(ctx, signature.data(), signature.size()) == 1);
    Require(EVP_DigestVerifyFinal_nid_postfix(ctx, changed.data(), changed.size()) == 0);
    EVP_MD_CTX_destroy_nid_postfix(ctx);

    GuestEvpPkey* peer = ReadKey(EcPeerPem);
    GuestEcKey* peerKey = static_cast<GuestEcKey*>(peer->pkey);
    unsigned char secret[40] = {};
    Require(ECDH_compute_key_nid_postfix(secret, 32, EC_KEY_get0_public_key_nid_postfix(peerKey), key, nullptr) == 32);
    Require(std::vector<unsigned char>(secret, secret + 32) == Hex(EcdhSecretHex));
    unsigned char mirror[40] = {};
    Require(ECDH_compute_key_nid_postfix(mirror, 40, EC_KEY_get0_public_key_nid_postfix(key), peerKey, nullptr) == 32 && std::memcmp(mirror, secret, 32) == 0);
    unsigned char reversed[40] = {};
    Require(ECDH_compute_key_nid_postfix(reversed, 40, EC_KEY_get0_public_key_nid_postfix(peerKey), key, Reverse) == 20 && reversed[0] == secret[31]);
    ERR_clear_error_nid_postfix();
    Require(ECDH_compute_key_nid_postfix(reversed, 40, EC_KEY_get0_public_key_nid_postfix(peerKey), key, Refuse) == -1 && LastError(Code(43, 100, 102)));
    Require(Throws([&] { ECDH_compute_key_nid_postfix(secret, 16, EC_KEY_get0_public_key_nid_postfix(peerKey), key, nullptr); }));

    Require(EVP_PKEY_copy_parameters_nid_postfix(peer, pkey) == 1);
    Require(EC_GROUP_get_degree_nid_postfix(EC_KEY_get0_group_nid_postfix(peerKey)) == 256);

    GuestEcKey* fresh = EC_KEY_new_nid_postfix();
    Require(fresh && EC_KEY_get0_group_nid_postfix(fresh) == nullptr);
    Require(Throws([&] { EC_KEY_generate_key_nid_postfix(fresh); }));
    Require(EC_KEY_check_key_nid_postfix(fresh) == 0 && LastError(Code(16, 177, 67)));
    Require(EC_KEY_set_group_nid_postfix(fresh, group) == 1 && EC_KEY_get0_group_nid_postfix(fresh) != group);
    Require(EC_KEY_generate_key_nid_postfix(fresh) == 1 && EC_KEY_check_key_nid_postfix(fresh) == 1);
    Require(ECDSA_sign_nid_postfix(0, Digest, 32, sig, &length, fresh) == 1 && ECDSA_verify_nid_postfix(0, Digest, 32, sig, static_cast<int>(length), fresh) == 1);
    Require(ECDSA_verify_nid_postfix(0, Digest, 32, sig, static_cast<int>(length), key) == 0);
    Require(EC_KEY_set_private_key_nid_postfix(fresh, EC_KEY_get0_private_key_nid_postfix(key)) == 1);
    Require(EC_KEY_check_key_nid_postfix(fresh) == 0 && LastError(Code(16, 177, 123)));
    Require(EC_KEY_set_public_key_nid_postfix(fresh, EC_KEY_get0_public_key_nid_postfix(key)) == 1 && EC_KEY_check_key_nid_postfix(fresh) == 1);
    Require(EC_KEY_get0_public_key_nid_postfix(fresh) != EC_KEY_get0_public_key_nid_postfix(key));
    EC_KEY_free_nid_postfix(fresh);

    GuestEcKey* named = EC_KEY_new_by_curve_name_nid_postfix(715);
    Require(named && EC_GROUP_get_degree_nid_postfix(EC_KEY_get0_group_nid_postfix(named)) == 384);
    Require(EC_KEY_generate_key_nid_postfix(named) == 1 && ECDSA_size_nid_postfix(named) == 104);
    GuestPoint* foreign = EC_POINT_new_nid_postfix(EC_KEY_get0_group_nid_postfix(named));
    Require(EC_KEY_set_public_key_nid_postfix(fresh = EC_KEY_new_by_curve_name_nid_postfix(415), foreign) == 0 && LastError(Code(16, 114, 101)));
    Require(ECDH_compute_key_nid_postfix(secret, 48, EC_KEY_get0_public_key_nid_postfix(key), named, nullptr) == -1 && LastError(Code(43, 100, 101)));
    EC_POINT_free_nid_postfix(foreign);
    EC_KEY_free_nid_postfix(fresh);
    EC_KEY_free_nid_postfix(named);
    GuestEcKey* p521 = EC_KEY_new_by_curve_name_nid_postfix(716);
    Require(EC_KEY_generate_key_nid_postfix(p521) == 1 && ECDSA_size_nid_postfix(p521) == 139);
    EC_KEY_free_nid_postfix(p521);
    Require(EC_KEY_new_by_curve_name_nid_postfix(1) == nullptr && LastError(Code(16, 174, 129)));

    EC_POINT_free_nid_postfix(combined);
    EC_POINT_free_nid_postfix(doubled);
    EC_POINT_free_nid_postfix(derived);
    BN_free_nid_postfix(two);
    EC_KEY_free_nid_postfix(key);
    EVP_PKEY_free_nid_postfix(peer);
    EVP_PKEY_free_nid_postfix(pkcs8);
    EVP_PKEY_free_nid_postfix(pkey);
    EC_KEY_free_nid_postfix(nullptr);
}

int main() {
    TestGroups();
    TestKeys();
    return 0;
}
