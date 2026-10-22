#include "prx/libc/include/ApplicationHeap.hpp"
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

struct GuestRsa {
    int pad;
    std::int64_t version;
    const void* meth;
    void* engine;
    GuestBignum* n;
    GuestBignum* e;
    GuestBignum* d;
    GuestBignum* p;
    GuestBignum* q;
    GuestBignum* dmp1;
    GuestBignum* dmq1;
    GuestBignum* iqmp;
    void* exData;
    int references;
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

struct GuestAsn1String {
    int length;
    int type;
    unsigned char* data;
    std::int64_t flags;
};

struct GuestAsn1Object {
    const char* sn;
    const char* ln;
    int nid;
    int length;
    const unsigned char* data;
    int flags;
};

struct GuestAlgor {
    GuestAsn1Object* algorithm;
    void* parameter;
};

struct GuestPkcs8 {
    GuestAsn1String* version;
    GuestAlgor* pkeyalg;
    GuestAsn1String* pkey;
    void* attributes;
};

struct GuestEcKey;
struct GuestGroup;
struct GuestPoint;
struct GuestPkeyCtx;
struct GuestBio;
struct GuestMd;

extern "C" {
int APS5_VABI RSA_up_ref_nid_postfix(GuestRsa*);
void APS5_VABI RSA_get0_key_nid_postfix(const GuestRsa*, const GuestBignum**, const GuestBignum**, const GuestBignum**);
int APS5_VABI RSA_check_key_nid_postfix(const GuestRsa*);
int APS5_VABI RSA_padding_add_PKCS1_PSS_mgf1_nid_postfix(GuestRsa*, unsigned char*, const unsigned char*, const GuestMd*, const GuestMd*, int);
int APS5_VABI EC_KEY_up_ref_nid_postfix(GuestEcKey*);
GuestEcKey* APS5_VABI d2i_ECParameters_nid_postfix(GuestEcKey**, const unsigned char**, std::int64_t);
GuestEcKey* APS5_VABI o2i_ECPublicKey_nid_postfix(GuestEcKey**, const unsigned char**, std::int64_t);
int APS5_VABI EVP_DigestSignInit_nid_postfix(GuestEvpMdCtx*, GuestPkeyCtx**, const GuestMd*, void*, GuestEvpPkey*);
int APS5_VABI EVP_DigestSignFinal_nid_postfix(GuestEvpMdCtx*, unsigned char*, std::size_t*);
int APS5_VABI EVP_SignFinal_nid_postfix(GuestEvpMdCtx*, unsigned char*, unsigned int*, GuestEvpPkey*);
int APS5_VABI EVP_VerifyFinal_nid_postfix(GuestEvpMdCtx*, const unsigned char*, unsigned int, GuestEvpPkey*);
GuestPkeyCtx* APS5_VABI EVP_PKEY_CTX_new_nid_postfix(GuestEvpPkey*, void*);
int APS5_VABI EVP_PKEY_CTX_ctrl_nid_postfix(GuestPkeyCtx*, int, int, int, int, void*);
int APS5_VABI EVP_PKEY_encrypt_init_nid_postfix(GuestPkeyCtx*);
int APS5_VABI EVP_PKEY_decrypt_init_nid_postfix(GuestPkeyCtx*);
int APS5_VABI EVP_PKEY_encrypt_nid_postfix(GuestPkeyCtx*, unsigned char*, std::size_t*, const unsigned char*, std::size_t);
int APS5_VABI EVP_PKEY_decrypt_nid_postfix(GuestPkeyCtx*, unsigned char*, std::size_t*, const unsigned char*, std::size_t);
GuestEvpPkey* APS5_VABI EVP_PKEY_new_mac_key_nid_postfix(int, void*, const unsigned char*, int);
int APS5_VABI EVP_PKEY_size_nid_postfix(const GuestEvpPkey*);
int APS5_VABI EVP_PKEY_up_ref_nid_postfix(GuestEvpPkey*);
GuestRsa* APS5_VABI EVP_PKEY_get0_RSA_nid_postfix(GuestEvpPkey*);
void* APS5_VABI EVP_PKEY_get0_DSA_nid_postfix(GuestEvpPkey*);
void* APS5_VABI EVP_PKEY_get0_DH_nid_postfix(GuestEvpPkey*);
int APS5_VABI EVP_PKEY_set1_EC_KEY_nid_postfix(GuestEvpPkey*, GuestEcKey*);
GuestEvpPkey* APS5_VABI d2i_PrivateKey_bio_nid_postfix(GuestBio*, GuestEvpPkey**);
GuestRsa* APS5_VABI d2i_RSAPublicKey_bio_nid_postfix(GuestBio*, GuestRsa**);
GuestPkcs8* APS5_VABI d2i_PKCS8_PRIV_KEY_INFO_bio_nid_postfix(GuestBio*, GuestPkcs8**);
void APS5_VABI PKCS8_PRIV_KEY_INFO_free_nid_postfix(GuestPkcs8*);
GuestEvpPkey* APS5_VABI EVP_PKCS82PKEY_nid_postfix(const GuestPkcs8*);
GuestEvpMdCtx* APS5_VABI EVP_MD_CTX_new_nid_postfix();
void APS5_VABI EVP_MD_CTX_free_nid_postfix(GuestEvpMdCtx*);
int APS5_VABI EVP_DigestInit_nid_postfix(GuestEvpMdCtx*, const GuestMd*);
const GuestMd* APS5_VABI EVP_sha1_nid_postfix();
std::uint64_t APS5_VABI ERR_peek_last_error_nid_postfix();

int APS5_VABI EVP_DigestUpdate_nid_postfix(GuestEvpMdCtx*, const void*, std::size_t);
const GuestMd* APS5_VABI EVP_sha256_nid_postfix();
GuestEvpPkey* APS5_VABI PEM_read_bio_PrivateKey_nid_postfix(GuestBio*, GuestEvpPkey**, void*, void*);
GuestEvpPkey* APS5_VABI EVP_PKEY_new_nid_postfix();
void APS5_VABI EVP_PKEY_free_nid_postfix(GuestEvpPkey*);
void APS5_VABI RSA_free_nid_postfix(GuestRsa*);
int APS5_VABI RSA_private_encrypt_nid_postfix(int, const unsigned char*, unsigned char*, GuestRsa*, int);
int APS5_VABI RSA_verify_nid_postfix(int, const unsigned char*, unsigned int, const unsigned char*, unsigned int, GuestRsa*);
GuestEcKey* APS5_VABI EC_KEY_new_by_curve_name_nid_postfix(int);
void APS5_VABI EC_KEY_free_nid_postfix(GuestEcKey*);
const GuestGroup* APS5_VABI EC_KEY_get0_group_nid_postfix(const GuestEcKey*);
const GuestPoint* APS5_VABI EC_KEY_get0_public_key_nid_postfix(const GuestEcKey*);
int APS5_VABI EC_GROUP_get_degree_nid_postfix(const GuestGroup*);
std::size_t APS5_VABI EC_POINT_point2oct_nid_postfix(const GuestGroup*, const GuestPoint*, int, unsigned char*, std::size_t, void*);
int APS5_VABI EC_KEY_generate_key_nid_postfix(GuestEcKey*);
int APS5_VABI BN_bn2bin_nid_postfix(const GuestBignum*, unsigned char*);
int APS5_VABI BN_add_word_nid_postfix(GuestBignum*, std::uint64_t);
GuestBio* APS5_VABI BIO_new_mem_buf_nid_postfix(void*, int);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
}

static const char RsaPem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIICXgIBAAKBgQDAGAJNFKocAP/WpV/BxTkx0dVix9/FGEtmDZDNPXroWM2aVony\n"
    "VrJVSFwi3OrXdFFUeoPf55B3qArv0xcLwze6h8GOC2tin4Xy3ugnCVz4kq3jWTPb\n"
    "SmBJ3Sn3X9DHs5RxabrJqZMBojKQMeFlClotGRyigQncjlS0IqYGYtEJiwIDAQAB\n"
    "AoGAZLAxBJZAVE1xzOECQF9Qi3yhjRWXhYgmfWe+7tW7Bu8FVlDmHdCzyWAUa3HA\n"
    "5w5hUxak6UhHwXCjyvGqss+w9XrxxefKQ4bNKkSfLh8rMmMVSvs20l2GIL88X28/\n"
    "8jVHWQ2MzOJvY9RLd85eLCeftC1bfWMnWWtSPwNYPni3sYECQQD51uOkIf4goa04\n"
    "eQ+SCXyvANAUtLyTXWTAAwq4NaTirfLRFgbV7BCtqy7DwXrvXwn8ch7KpukfkREj\n"
    "NO8oVIZNAkEAxNSZwiSz5iUg6X5VXKrA0hdUP1sg8q9xS6z0rRhxPAW/L42NLRtj\n"
    "ZPlHuJpKpLrH6xD6PkfKZUHzM7t7BZ5rNwJBAKkQHSMoTYtPXKRmZ37vJIxYBHa+\n"
    "hw1BmbfFVy+oPVeXVkTenumLwQ2EVTTqbIIm0d8eam+/2ukdhXdJjry/1dECQQCy\n"
    "fTBpk6rnmue7lWqHO/jbC3jEczjvmhUdUNwYU7/rdDKpRq2tJ9b8a+mUB6aEtzQv\n"
    "gBfNfoCVTjbixLKMp9oFAkEAknUKoGj082UNS7vCjF64hB/gTCD5kB/DXf9yVMQ3\n"
    "mnC9YSWhuuudfNBhD/9eTRvsP+ZQTJQ480K8L7PFEzFNcg==\n"
    "-----END RSA PRIVATE KEY-----\n";

static const char EcPem[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIN59H0jvfHQ6H7PSNAAmi6lw/aE2tke2uMSbluI/ESDWoAoGCCqGSM49\n"
    "AwEHoUQDQgAET/waFkHEUMHGzcxCVQs6KmQrbCGJxkdWdZvnFsKX+2LsXFx6NYa/\n"
    "ol/+mTBntyNqgNfcv28TvzKiUHkMWZ3q+A==\n"
    "-----END EC PRIVATE KEY-----\n";

static const char ModulusHex[] =
    "c018024d14aa1c00ffd6a55fc1c53931d1d562c7dfc5184b660d90cd3d7ae858"
    "cd9a5689f256b255485c22dcead77451547a83dfe79077a80aefd3170bc337ba"
    "87c18e0b6b629f85f2dee827095cf892ade35933db4a6049dd29f75fd0c7b394"
    "7169bac9a99301a2329031e1650a5a2d191ca28109dc8e54b422a60662d1098b";

static const char Pkcs1PublicHex[] =
    "30818902818100c018024d14aa1c00ffd6a55fc1c53931d1d562c7dfc5184b66"
    "0d90cd3d7ae858cd9a5689f256b255485c22dcead77451547a83dfe79077a80a"
    "efd3170bc337ba87c18e0b6b629f85f2dee827095cf892ade35933db4a6049dd"
    "29f75fd0c7b3947169bac9a99301a2329031e1650a5a2d191ca28109dc8e54b4"
    "22a60662d1098b0203010001";

static const char Pkcs8Hex[] =
    "30820278020100300d06092a864886f70d0101010500048202623082025e0201"
    "0002818100c018024d14aa1c00ffd6a55fc1c53931d1d562c7dfc5184b660d90"
    "cd3d7ae858cd9a5689f256b255485c22dcead77451547a83dfe79077a80aefd3"
    "170bc337ba87c18e0b6b629f85f2dee827095cf892ade35933db4a6049dd29f7"
    "5fd0c7b3947169bac9a99301a2329031e1650a5a2d191ca28109dc8e54b422a6"
    "0662d1098b020301000102818064b031049640544d71cce102405f508b7ca18d"
    "15978588267d67beeed5bb06ef055650e61dd0b3c960146b71c0e70e615316a4"
    "e94847c170a3caf1aab2cfb0f57af1c5e7ca4386cd2a449f2e1f2b3263154afb"
    "36d25d8620bf3c5f6f3ff23547590d8ccce26f63d44b77ce5e2c279fb42d5b7d"
    "6327596b523f03583e78b7b181024100f9d6e3a421fe20a1ad38790f92097caf"
    "00d014b4bc935d64c0030ab835a4e2adf2d11606d5ec10adab2ec3c17aef5f09"
    "fc721ecaa6e91f91112334ef2854864d024100c4d499c224b3e62520e97e555c"
    "aac0d217543f5b20f2af714bacf4ad18713c05bf2f8d8d2d1b6364f947b89a4a"
    "a4bac7eb10fa3e47ca6541f333bb7b059e6b37024100a9101d23284d8b4f5ca4"
    "66677eef248c580476be870d4199b7c5572fa83d57975644de9ee98bc10d8455"
    "34ea6c8226d1df1e6a6fbfdae91d8577498ebcbfd5d1024100b27d306993aae7"
    "9ae7bb956a873bf8db0b78c47338ef9a151d50dc1853bfeb7432a946adad27d6"
    "fc6be99407a684b7342f8017cd7e80954e36e2c4b28ca7da0502410092750aa0"
    "68f4f3650d4bbbc28c5eb8841fe04c20f9901fc35dff7254c4379a70bd6125a1"
    "baeb9d7cd0610fff5e4d1bec3fe6504c9438f342bc2fb3c513314d72";

static const char EcDerHex[] =
    "30770201010420de7d1f48ef7c743a1fb3d23400268ba970fda136b647b6b8c4"
    "9b96e23f1120d6a00a06082a8648ce3d030107a144034200044ffc1a1641c450"
    "c1c6cdcc42550b3a2a642b6c2189c64756759be716c297fb62ec5c5c7a3586bf"
    "a25ffe993067b7236a80d7dcbf6f13bf32a250790c599deaf8";

static const char EcParamsHex[] =
    "06082a8648ce3d030107";

static const char EcExplicitHex[] =
    "3081f7020101302c06072a8648ce3d0101022100ffffffff0000000100000000"
    "0000000000000000ffffffffffffffffffffffff305b0420ffffffff00000001"
    "000000000000000000000000fffffffffffffffffffffffc04205ac635d8aa3a"
    "93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b031500c49d36"
    "0886e704936a6678e1139d26b7819f7e900441046b17d1f2e12c4247f8bce6e5"
    "63a440f277037d812deb33a0f4a13945d898c2964fe342e2fe1a7f9b8ee7eb4a"
    "7c0f9e162bce33576b315ececbb6406837bf51f5022100ffffffff00000000ff"
    "ffffffffffffffbce6faada7179e84f3b9cac2fc632551020101";

static const char EcPublicHex[] =
    "044ffc1a1641c450c1c6cdcc42550b3a2a642b6c2189c64756759be716c297fb"
    "62ec5c5c7a3586bfa25ffe993067b7236a80d7dcbf6f13bf32a250790c599dea"
    "f8";

static const char SigPkcs1Hex[] =
    "78fd002ef900b9b8ac4002d2049e4830eaea21a2e737a0a618526ec57d055e63"
    "4b7c58ac1bcc1d9ee0a6591abfa6438271d760a71d3560aea2acad12455c1baf"
    "334e762fcd4b20d89744a137d73999c99a4be3478868ae2f4ebacd90f664d96a"
    "ab6a3f64defd25a8f55cfc8d0db6bed367306f1ac2b5c1d64f741fb988e67d43";

static const char SigPss0Hex[] =
    "994a969239c03ef54c567125891cadec517e6d6601fe9ded7b94bdb4fd38f841"
    "4791d795a124349dca29c45e4557a8f16d3e11d6a6ff5be9852df1dc3748ed9a"
    "dc68ba484caa9f57f09765849c0a6aaaacd10c36f3c553f7bec36f563410fdfe"
    "723d368ebc8074eb46d2d659ddebd41e40c85070cd62d6a805490c21e704c466";

static const char SigPss32Hex[] =
    "79b8b4f50bfec901c92c705448405083064ce16bf69fd868854ebd5e48fd39d6"
    "971270f0387dfbd3813204db104cb2ad333e65798b0c025505477e6617798823"
    "8a4ba3535ea56e6b7eb2563e233f9eef689bed2944e56cb8ddf7b45d594f59d9"
    "9ec4d032e510793ae4bb31ea97d4aafd115f6209e9b005d2004b27675c7622fa";

static const char SigPssMgf1Hex[] =
    "4f08cd89f7065967669a2cac12d4b3f2f05a10cb6ce7d124213aefb948a5212f"
    "ad9df9cb7aa30254264fb5e6ff07b8e32b61c3e954de995256e90c1838c5f19c"
    "f2a63e97b8df82bbf559d4c3ca2d9b94453fab2b7429cd318e6fb76da73a2831"
    "50dc2f97cb32967cd6b8217c85cdf64946bf8534fdb9992a374bebb13fabd8a7";

static const char EncOaepHex[] =
    "16ded1231f478ee646fb6cde9de24fd1c460725b4c39d1401c9e0060a51207f8"
    "288405053f72f68d9ab4c19071819e5319021e97222b2de4731806a441f54c32"
    "2b130c59493fd11d3da082d4ecd6e9e7330c821b40ced9b0097f190f26302413"
    "adda4a8e10f27f3ff7e39b20978dd0a409f38886d9487ec315c1a1e6170f7e86";

static const unsigned char Digest[32] = {0x51, 0xdd, 0x84, 0x95, 0x6e, 0x17, 0x89, 0x4a, 0x10, 0x2a, 0x43, 0x10, 0xb3, 0x7f, 0x91, 0x65,
    0x68, 0x85, 0xdd, 0x9f, 0x5b, 0xd8, 0x12, 0xef, 0xcd, 0x36, 0x97, 0x0c, 0x2f, 0xba, 0x5c, 0x43};
static const char Message[] = "AnyPS5 LibreSSL key test";
static const char Plain[] = "secret session key 0123456789";

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
    const bool match = ERR_peek_last_error_nid_postfix() == expected;
    ERR_clear_error_nid_postfix();
    return match;
}

static GuestEvpPkey* ReadKey(const char* pem) {
    std::string text(pem);
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(text.data(), static_cast<int>(text.size()));
    GuestEvpPkey* pkey = PEM_read_bio_PrivateKey_nid_postfix(bio, nullptr, nullptr, nullptr);
    BIO_free_nid_postfix(bio);
    Require(pkey != nullptr);
    return pkey;
}

static std::vector<unsigned char> Bytes(const GuestBignum* bn) {
    std::vector<unsigned char> out(256);
    out.resize(static_cast<std::size_t>(BN_bn2bin_nid_postfix(bn, out.data())));
    return out;
}

static std::vector<unsigned char> DigestSign(GuestEvpPkey* pkey, const GuestMd* md, int padding, int saltLength, const GuestMd* mgf1) {
    GuestEvpMdCtx* ctx = EVP_MD_CTX_new_nid_postfix();
    GuestPkeyCtx* pctx = nullptr;
    Require(EVP_DigestSignInit_nid_postfix(ctx, &pctx, md, nullptr, pkey) == 1 && pctx);
    if (padding) Require(EVP_PKEY_CTX_ctrl_nid_postfix(pctx, 6, -1, 0x1001, padding, nullptr) == 1);
    if (padding == 6) Require(EVP_PKEY_CTX_ctrl_nid_postfix(pctx, 6, (1 << 3) | (1 << 6), 0x1002, saltLength, nullptr) == 1);
    if (mgf1) Require(EVP_PKEY_CTX_ctrl_nid_postfix(pctx, -1, -1, 0x1005, 0, const_cast<GuestMd*>(mgf1)) == 1);
    Require(EVP_DigestUpdate_nid_postfix(ctx, Message, std::strlen(Message)) == 1);
    std::size_t size = 0;
    Require(EVP_DigestSignFinal_nid_postfix(ctx, nullptr, &size) == 1);
    std::vector<unsigned char> sig(size);
    Require(EVP_DigestSignFinal_nid_postfix(ctx, sig.data(), &size) == 1 && size <= sig.size());
    sig.resize(size);
    EVP_MD_CTX_free_nid_postfix(ctx);
    return sig;
}

static void TestRsa() {
    GuestEvpPkey* pkey = ReadKey(RsaPem);
    GuestRsa* rsa = EVP_PKEY_get0_RSA_nid_postfix(pkey);
    Require(rsa && rsa == pkey->pkey && rsa->references == 1);
    Require(RSA_up_ref_nid_postfix(rsa) == 1 && rsa->references == 2);
    RSA_free_nid_postfix(rsa);
    const GuestBignum* n = nullptr;
    const GuestBignum* e = nullptr;
    const GuestBignum* d = nullptr;
    RSA_get0_key_nid_postfix(rsa, &n, &e, &d);
    Require(n == rsa->n && e == rsa->e && d == rsa->d && Bytes(n) == Hex(ModulusHex));
    RSA_get0_key_nid_postfix(rsa, nullptr, nullptr, &d);
    Require(RSA_check_key_nid_postfix(rsa) == 1);
    Require(EVP_PKEY_size_nid_postfix(pkey) == 128);
    Require(EVP_PKEY_up_ref_nid_postfix(pkey) == 1 && pkey->references == 2);
    EVP_PKEY_free_nid_postfix(pkey);
    Require(EVP_PKEY_get0_DSA_nid_postfix(pkey) == nullptr && LastError(Code(6, 120, 129)));
    Require(EVP_PKEY_get0_DH_nid_postfix(pkey) == nullptr && LastError(Code(6, 119, 128)));

    Require(DigestSign(pkey, EVP_sha256_nid_postfix(), 0, 0, nullptr) == Hex(SigPkcs1Hex));
    Require(DigestSign(pkey, EVP_sha256_nid_postfix(), 6, 0, nullptr) == Hex(SigPss0Hex));
    Require(DigestSign(pkey, EVP_sha256_nid_postfix(), 6, 0, EVP_sha1_nid_postfix()) == Hex(SigPssMgf1Hex));
    unsigned char em[128];
    unsigned char sig[128];
    Require(RSA_padding_add_PKCS1_PSS_mgf1_nid_postfix(rsa, em, Digest, EVP_sha256_nid_postfix(), EVP_sha1_nid_postfix(), 0) == 1);
    Require(RSA_private_encrypt_nid_postfix(128, em, sig, rsa, 3) == 128 && std::vector<unsigned char>(sig, sig + 128) == Hex(SigPssMgf1Hex));
    const auto random = DigestSign(pkey, EVP_sha256_nid_postfix(), 6, -2, nullptr);
    Require(random.size() == 128 && random != Hex(SigPss0Hex));

    GuestEvpMdCtx* ctx = EVP_MD_CTX_new_nid_postfix();
    Require(EVP_DigestInit_nid_postfix(ctx, EVP_sha256_nid_postfix()) == 1 && EVP_DigestUpdate_nid_postfix(ctx, Message, std::strlen(Message)) == 1);
    unsigned int length = 0;
    Require(EVP_SignFinal_nid_postfix(ctx, sig, &length, pkey) == 1 && length == 128 && std::vector<unsigned char>(sig, sig + 128) == Hex(SigPkcs1Hex));
    Require(EVP_VerifyFinal_nid_postfix(ctx, sig, length, pkey) == 1);
    sig[0] ^= 1;
    Require(EVP_VerifyFinal_nid_postfix(ctx, sig, length, pkey) == 0);
    EVP_MD_CTX_free_nid_postfix(ctx);

    GuestPkeyCtx* pctx = EVP_PKEY_CTX_new_nid_postfix(pkey, nullptr);
    Require(pctx && pkey->references == 2);
    Require(EVP_PKEY_CTX_ctrl_nid_postfix(pctx, 6, -1, 0x1001, 4, nullptr) == -1 && LastError(Code(6, 137, 149)));
    Require(EVP_PKEY_decrypt_nid_postfix(pctx, em, nullptr, em, 0) == -1 && LastError(Code(6, 104, 151)));
    Require(EVP_PKEY_decrypt_init_nid_postfix(pctx) == 1);
    Require(EVP_PKEY_CTX_ctrl_nid_postfix(pctx, 6, -1, 0x1001, 6, nullptr) == -2 && LastError(Code(6, 137, 147)));
    Require(EVP_PKEY_CTX_ctrl_nid_postfix(pctx, 6, 1 << 8, 0x1001, 4, nullptr) == -1 && LastError(Code(6, 137, 148)));
    Require(EVP_PKEY_CTX_ctrl_nid_postfix(pctx, 408, -1, 0x1001, 4, nullptr) == -1);
    Require(EVP_PKEY_CTX_ctrl_nid_postfix(pctx, 6, 1 << 9, 0x1001, 4, nullptr) == 1);
    int padding = 0;
    Require(EVP_PKEY_CTX_ctrl_nid_postfix(pctx, -1, -1, 0x1006, 0, &padding) == 1 && padding == 4);
    const GuestMd* oaep = nullptr;
    Require(EVP_PKEY_CTX_ctrl_nid_postfix(pctx, -1, -1, 0x100b, 0, &oaep) == 1 && oaep == EVP_sha1_nid_postfix());
    std::size_t outlen = 0;
    const auto encrypted = Hex(EncOaepHex);
    Require(EVP_PKEY_decrypt_nid_postfix(pctx, nullptr, &outlen, encrypted.data(), encrypted.size()) == 1 && outlen == 128);
    unsigned char out[128];
    outlen = 100;
    Require(EVP_PKEY_decrypt_nid_postfix(pctx, out, &outlen, encrypted.data(), encrypted.size()) == 0 && LastError(Code(6, 104, 155)));
    outlen = sizeof(out);
    Require(EVP_PKEY_decrypt_nid_postfix(pctx, out, &outlen, encrypted.data(), encrypted.size()) == 1 && outlen == 29 && std::memcmp(out, Plain, 29) == 0);
    Require(EVP_PKEY_encrypt_init_nid_postfix(pctx) == 1);
    outlen = sizeof(out);
    Require(EVP_PKEY_encrypt_nid_postfix(pctx, out, &outlen, reinterpret_cast<const unsigned char*>(Plain), 29) == 1 && outlen == 128);
    Require(EVP_PKEY_decrypt_init_nid_postfix(pctx) == 1);
    unsigned char back[128];
    std::size_t backlen = sizeof(back);
    Require(EVP_PKEY_decrypt_nid_postfix(pctx, back, &backlen, out, 128) == 1 && backlen == 29 && std::memcmp(back, Plain, 29) == 0);
    backlen = sizeof(back);
    Require(EVP_PKEY_decrypt_nid_postfix(pctx, back, &backlen, encrypted.data(), encrypted.size()) == -1 && LastError(Code(4, 101, 114)));

    BN_add_word_nid_postfix(rsa->d, 2);
    Require(RSA_check_key_nid_postfix(rsa) == 0 && LastError(Code(4, 123, 125)));
    EVP_PKEY_free_nid_postfix(pkey);
}

static void TestMac() {
    const unsigned char key[] = {'J', 'e', 'f', 'e'};
    GuestEvpPkey* mac = EVP_PKEY_new_mac_key_nid_postfix(855, nullptr, key, sizeof(key));
    Require(mac && mac->type == 855 && EVP_PKEY_size_nid_postfix(mac) == 64);
    GuestEvpMdCtx* ctx = EVP_MD_CTX_new_nid_postfix();
    Require(EVP_DigestSignInit_nid_postfix(ctx, nullptr, EVP_sha256_nid_postfix(), nullptr, mac) == 1);
    const char data[] = "what do ya want for nothing?";
    Require(EVP_DigestUpdate_nid_postfix(ctx, data, std::strlen(data)) == 1);
    std::size_t size = 0;
    Require(EVP_DigestSignFinal_nid_postfix(ctx, nullptr, &size) == 1 && size == 32);
    unsigned char tag[32];
    Require(EVP_DigestSignFinal_nid_postfix(ctx, tag, &size) == 1 && size == 32);
    Require(std::vector<unsigned char>(tag, tag + 32) == Hex("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"));
    size = 16;
    Require(EVP_DigestSignFinal_nid_postfix(ctx, tag, &size) == 0 && LastError(Code(6, 140, 155)));
    EVP_MD_CTX_free_nid_postfix(ctx);
    std::vector<unsigned char> longKey(131, 0xaa);
    GuestEvpPkey* wide = EVP_PKEY_new_mac_key_nid_postfix(855, nullptr, longKey.data(), static_cast<int>(longKey.size()));
    ctx = EVP_MD_CTX_new_nid_postfix();
    Require(EVP_DigestSignInit_nid_postfix(ctx, nullptr, EVP_sha256_nid_postfix(), nullptr, wide) == 1);
    const char text[] = "Test Using Larger Than Block-Size Key - Hash Key First";
    Require(EVP_DigestUpdate_nid_postfix(ctx, text, std::strlen(text)) == 1);
    size = sizeof(tag);
    Require(EVP_DigestSignFinal_nid_postfix(ctx, tag, &size) == 1);
    Require(std::vector<unsigned char>(tag, tag + 32) == Hex("60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54"));
    EVP_MD_CTX_free_nid_postfix(ctx);
    unsigned int length = 0;
    GuestEvpMdCtx* plain = EVP_MD_CTX_new_nid_postfix();
    EVP_DigestInit_nid_postfix(plain, EVP_sha256_nid_postfix());
    Require(EVP_SignFinal_nid_postfix(plain, tag, &length, mac) == 0 && LastError(Code(6, 107, 150)));
    EVP_MD_CTX_free_nid_postfix(plain);
    Require(Throws([&] { EVP_PKEY_new_mac_key_nid_postfix(894, nullptr, key, 4); }));
    EVP_PKEY_free_nid_postfix(wide);
    EVP_PKEY_free_nid_postfix(mac);
}

static void TestEc() {
    GuestEvpPkey* pkey = ReadKey(EcPem);
    auto* key = static_cast<GuestEcKey*>(pkey->pkey);
    Require(EVP_PKEY_size_nid_postfix(pkey) == 72);
    Require(EC_KEY_up_ref_nid_postfix(key) == 1);
    GuestEvpPkey* other = EVP_PKEY_new_nid_postfix();
    Require(EVP_PKEY_set1_EC_KEY_nid_postfix(other, key) == 1 && other->type == 408 && other->pkey == key);
    EC_KEY_free_nid_postfix(key);
    const auto sig = DigestSign(other, EVP_sha256_nid_postfix(), 0, 0, nullptr);
    Require(sig.size() >= 8 && sig.size() <= 72);
    GuestEvpMdCtx* ctx = EVP_MD_CTX_new_nid_postfix();
    EVP_DigestInit_nid_postfix(ctx, EVP_sha256_nid_postfix());
    EVP_DigestUpdate_nid_postfix(ctx, Message, std::strlen(Message));
    Require(EVP_VerifyFinal_nid_postfix(ctx, sig.data(), static_cast<unsigned int>(sig.size()), pkey) == 1);
    Require(EVP_VerifyFinal_nid_postfix(ctx, sig.data(), static_cast<unsigned int>(sig.size()) - 1, pkey) == -1);
    unsigned char ecsig[72];
    unsigned int length = 0;
    Require(EVP_SignFinal_nid_postfix(ctx, ecsig, &length, pkey) == 1 && EVP_VerifyFinal_nid_postfix(ctx, ecsig, length, other) == 1);
    EVP_MD_CTX_free_nid_postfix(ctx);
    GuestPkeyCtx* pctx = EVP_PKEY_CTX_new_nid_postfix(pkey, nullptr);
    Require(EVP_PKEY_encrypt_init_nid_postfix(pctx) == -2 && LastError(Code(6, 139, 150)));

    const auto params = Hex(EcParamsHex);
    const unsigned char* cursor = params.data();
    GuestEcKey* fromParams = d2i_ECParameters_nid_postfix(nullptr, &cursor, static_cast<std::int64_t>(params.size()));
    Require(fromParams && cursor == params.data() + params.size() && EC_GROUP_get_degree_nid_postfix(EC_KEY_get0_group_nid_postfix(fromParams)) == 256);
    Require(EC_KEY_get0_public_key_nid_postfix(fromParams) == nullptr);
    const auto point = Hex(EcPublicHex);
    cursor = point.data();
    Require(o2i_ECPublicKey_nid_postfix(&fromParams, &cursor, static_cast<std::int64_t>(point.size())) == fromParams && cursor == point.data() + 65);
    unsigned char encoded[65];
    Require(EC_POINT_point2oct_nid_postfix(EC_KEY_get0_group_nid_postfix(fromParams), EC_KEY_get0_public_key_nid_postfix(fromParams), 4, encoded, 65, nullptr) == 65);
    Require(std::vector<unsigned char>(encoded, encoded + 65) == point);
    cursor = point.data();
    Require(o2i_ECPublicKey_nid_postfix(&fromParams, &cursor, 64) == nullptr && cursor == point.data() && LastError(Code(16, 152, 115)));
    GuestEcKey* reused = EC_KEY_new_by_curve_name_nid_postfix(715);
    cursor = params.data();
    Require(d2i_ECParameters_nid_postfix(&reused, &cursor, static_cast<std::int64_t>(params.size())) == reused);
    Require(EC_GROUP_get_degree_nid_postfix(EC_KEY_get0_group_nid_postfix(reused)) == 256);
    const auto explicitParams = Hex(EcExplicitHex);
    cursor = explicitParams.data();
    Require(Throws([&] { d2i_ECParameters_nid_postfix(nullptr, &cursor, static_cast<std::int64_t>(explicitParams.size())); }));
    const unsigned char junk[] = {0x04, 0x01, 0x00};
    cursor = junk;
    Require(d2i_ECParameters_nid_postfix(nullptr, &cursor, 3) == nullptr && LastError(Code(16, 144, 117)));
    Require(Throws([&] { GuestEcKey* none = nullptr; o2i_ECPublicKey_nid_postfix(&none, &cursor, 3); }));
    EC_KEY_free_nid_postfix(reused);
    EC_KEY_free_nid_postfix(fromParams);
    EVP_PKEY_free_nid_postfix(other);
    EVP_PKEY_free_nid_postfix(pkey);
}

static void TestDer() {
    auto pkcs8 = Hex(Pkcs8Hex);
    pkcs8.push_back(0x30);
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(pkcs8.data(), static_cast<int>(pkcs8.size()));
    GuestEvpPkey* out = nullptr;
    GuestEvpPkey* pkey = d2i_PrivateKey_bio_nid_postfix(bio, &out);
    Require(pkey && out == pkey && pkey->type == 6 && BIO_ctrl_nid_postfix(bio, 10, 0, nullptr) == 1);
    Require(d2i_PrivateKey_bio_nid_postfix(bio, nullptr) == nullptr && BIO_ctrl_nid_postfix(bio, 10, 0, nullptr) == 1);
    BIO_free_nid_postfix(bio);
    EVP_PKEY_free_nid_postfix(pkey);
    auto ec = Hex(EcDerHex);
    bio = BIO_new_mem_buf_nid_postfix(ec.data(), static_cast<int>(ec.size()));
    pkey = d2i_PrivateKey_bio_nid_postfix(bio, nullptr);
    Require(pkey && pkey->type == 408);
    BIO_free_nid_postfix(bio);
    EVP_PKEY_free_nid_postfix(pkey);

    auto pub = Hex(Pkcs1PublicHex);
    bio = BIO_new_mem_buf_nid_postfix(pub.data(), static_cast<int>(pub.size()));
    GuestRsa* rsa = d2i_RSAPublicKey_bio_nid_postfix(bio, nullptr);
    Require(rsa && Bytes(rsa->n) == Hex(ModulusHex) && Bytes(rsa->e) == std::vector<unsigned char>({1, 0, 1}) && !rsa->d);
    Require(RSA_verify_nid_postfix(672, Digest, 32, Hex(SigPkcs1Hex).data(), 128, rsa) == 1);
    RSA_free_nid_postfix(rsa);
    BIO_free_nid_postfix(bio);

    auto info = Hex(Pkcs8Hex);
    bio = BIO_new_mem_buf_nid_postfix(info.data(), static_cast<int>(info.size()));
    GuestPkcs8* p8 = d2i_PKCS8_PRIV_KEY_INFO_bio_nid_postfix(bio, nullptr);
    Require(p8 && p8->version->length == 1 && p8->version->data[0] == 0 && p8->version->type == 2 && p8->pkeyalg->algorithm->nid == 6 && p8->pkey->type == 4 && !p8->attributes);
    Require(p8->pkey->length > 600 && p8->pkey->data[0] == 0x30);
    pkey = EVP_PKCS82PKEY_nid_postfix(p8);
    Require(pkey && pkey->type == 6 && Bytes(static_cast<GuestRsa*>(pkey->pkey)->n) == Hex(ModulusHex));
    EVP_PKEY_free_nid_postfix(pkey);
    PKCS8_PRIV_KEY_INFO_free_nid_postfix(p8);
    PKCS8_PRIV_KEY_INFO_free_nid_postfix(nullptr);
    Require(Throws([&] { PKCS8_PRIV_KEY_INFO_free_nid_postfix(p8); }));
    BIO_free_nid_postfix(bio);
}

int main() {
    void* const heapApi[10] {};
    ApplicationHeapRegister_nid_no_patch(heapApi);
    TestRsa();
    TestMac();
    TestEc();
    TestDer();
    return 0;
}
