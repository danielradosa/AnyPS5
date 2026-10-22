#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
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
    void* reserved[5];
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

struct GuestBio;
struct GuestBioMethod;
struct GuestMd;
struct FileStream;

extern "C" {
GuestRsa* APS5_VABI RSA_new_nid_postfix();
void APS5_VABI RSA_free_nid_postfix(GuestRsa*);
int APS5_VABI RSA_size_nid_postfix(const GuestRsa*);
int APS5_VABI RSA_public_encrypt_nid_postfix(int, const unsigned char*, unsigned char*, GuestRsa*, int);
int APS5_VABI RSA_private_decrypt_nid_postfix(int, const unsigned char*, unsigned char*, GuestRsa*, int);
int APS5_VABI RSA_private_encrypt_nid_postfix(int, const unsigned char*, unsigned char*, GuestRsa*, int);
int APS5_VABI RSA_public_decrypt_nid_postfix(int, const unsigned char*, unsigned char*, GuestRsa*, int);
int APS5_VABI RSA_sign_nid_postfix(int, const unsigned char*, unsigned int, unsigned char*, unsigned int*, GuestRsa*);
int APS5_VABI RSA_verify_nid_postfix(int, const unsigned char*, unsigned int, const unsigned char*, unsigned int, GuestRsa*);
int APS5_VABI RSA_generate_key_ex_nid_postfix(GuestRsa*, int, GuestBignum*, void*);
int APS5_VABI RSA_padding_add_PKCS1_PSS_nid_postfix(GuestRsa*, unsigned char*, const unsigned char*, const GuestMd*, int);
int APS5_VABI RSA_verify_PKCS1_PSS_nid_postfix(GuestRsa*, const unsigned char*, const GuestMd*, const unsigned char*, int);
GuestRsa* APS5_VABI d2i_RSA_PUBKEY_nid_postfix(GuestRsa**, const unsigned char**, std::int64_t);
GuestRsa* APS5_VABI PEM_read_RSAPrivateKey_nid_postfix(FileStream*, GuestRsa**, void*, void*);
GuestEvpPkey* APS5_VABI PEM_read_bio_PrivateKey_nid_postfix(GuestBio*, GuestEvpPkey**, void*, void*);
GuestEvpPkey* APS5_VABI EVP_PKEY_new_nid_postfix();
void APS5_VABI EVP_PKEY_free_nid_postfix(GuestEvpPkey*);
int APS5_VABI EVP_PKEY_id_nid_postfix(const GuestEvpPkey*);
int APS5_VABI EVP_PKEY_type_nid_postfix(int);
int APS5_VABI EVP_PKEY_set1_RSA_nid_postfix(GuestEvpPkey*, GuestRsa*);
GuestRsa* APS5_VABI EVP_PKEY_get1_RSA_nid_postfix(GuestEvpPkey*);
void* APS5_VABI EVP_PKEY_get1_EC_KEY_nid_postfix(GuestEvpPkey*);
int APS5_VABI EVP_PKEY_copy_parameters_nid_postfix(GuestEvpPkey*, const GuestEvpPkey*);
int APS5_VABI EVP_DigestVerifyInit_nid_postfix(GuestEvpMdCtx*, void**, const GuestMd*, void*, GuestEvpPkey*);
int APS5_VABI EVP_DigestVerifyFinal_nid_postfix(GuestEvpMdCtx*, const unsigned char*, std::size_t);
GuestEvpMdCtx* APS5_VABI EVP_MD_CTX_create_nid_postfix();
void APS5_VABI EVP_MD_CTX_destroy_nid_postfix(GuestEvpMdCtx*);
int APS5_VABI EVP_MD_CTX_cleanup_nid_postfix(GuestEvpMdCtx*);
void APS5_VABI EVP_MD_CTX_init_nid_postfix(GuestEvpMdCtx*);
int APS5_VABI EVP_DigestUpdate_nid_postfix(GuestEvpMdCtx*, const void*, std::size_t);
const GuestMd* APS5_VABI EVP_sha256_nid_postfix();
const GuestMd* APS5_VABI EVP_sha512_nid_postfix();
GuestBignum* APS5_VABI BN_new_nid_postfix();
void APS5_VABI BN_free_nid_postfix(GuestBignum*);
int APS5_VABI BN_set_word_nid_postfix(GuestBignum*, std::uint64_t);
int APS5_VABI BN_bn2bin_nid_postfix(const GuestBignum*, unsigned char*);
GuestBio* APS5_VABI BIO_new_mem_buf_nid_postfix(void*, int);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
FileStream* APS5_VABI fopen_nid_postfix(const char*, const char*);
int APS5_VABI fclose_nid_postfix(FileStream*);
std::size_t APS5_VABI fwrite_nid_postfix(const void*, std::size_t, std::size_t, FileStream*);
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

static const char RsaPkcs8Pem[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIICeAIBADANBgkqhkiG9w0BAQEFAASCAmIwggJeAgEAAoGBAMAYAk0UqhwA/9al\n"
    "X8HFOTHR1WLH38UYS2YNkM09euhYzZpWifJWslVIXCLc6td0UVR6g9/nkHeoCu/T\n"
    "FwvDN7qHwY4La2KfhfLe6CcJXPiSreNZM9tKYEndKfdf0MezlHFpusmpkwGiMpAx\n"
    "4WUKWi0ZHKKBCdyOVLQipgZi0QmLAgMBAAECgYBksDEElkBUTXHM4QJAX1CLfKGN\n"
    "FZeFiCZ9Z77u1bsG7wVWUOYd0LPJYBRrccDnDmFTFqTpSEfBcKPK8aqyz7D1evHF\n"
    "58pDhs0qRJ8uHysyYxVK+zbSXYYgvzxfbz/yNUdZDYzM4m9j1Et3zl4sJ5+0LVt9\n"
    "YydZa1I/A1g+eLexgQJBAPnW46Qh/iChrTh5D5IJfK8A0BS0vJNdZMADCrg1pOKt\n"
    "8tEWBtXsEK2rLsPBeu9fCfxyHsqm6R+RESM07yhUhk0CQQDE1JnCJLPmJSDpflVc\n"
    "qsDSF1Q/WyDyr3FLrPStGHE8Bb8vjY0tG2Nk+Ue4mkqkusfrEPo+R8plQfMzu3sF\n"
    "nms3AkEAqRAdIyhNi09cpGZnfu8kjFgEdr6HDUGZt8VXL6g9V5dWRN6e6YvBDYRV\n"
    "NOpsgibR3x5qb7/a6R2Fd0mOvL/V0QJBALJ9MGmTquea57uVaoc7+NsLeMRzOO+a\n"
    "FR1Q3BhTv+t0MqlGra0n1vxr6ZQHpoS3NC+AF81+gJVONuLEsoyn2gUCQQCSdQqg\n"
    "aPTzZQ1Lu8KMXriEH+BMIPmQH8Nd/3JUxDeacL1hJaG665180GEP/15NG+w/5lBM\n"
    "lDjzQrwvs8UTMU1y\n"
    "-----END PRIVATE KEY-----\n";

static const char RsaPkcs8EncryptedPem[] =
    "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
    "MIIC5TBfBgkqhkiG9w0BBQ0wUjAxBgkqhkiG9w0BBQwwJAQQLNdAa7klBusT9di3\n"
    "x/h8igICCAAwDAYIKoZIhvcNAgkFADAdBglghkgBZQMEASoEEMJUa1WnaTVQenOz\n"
    "3jyGkhIEggKA2LLzPRymblumAIPWFBzsViXBG061J+zGsXq2/6a1YBngRpyTVkWQ\n"
    "FEaVPX443TPPIiPtrP7xCdinhj6xoOeOZrOQL62nfPFA6M4puNP14IKnvq4Mm/Qf\n"
    "Lic1in6XDBxi2A5XWE1JxqGlfV4RkpXdQF2mpkOBkYCv/K1g5dqw16Y/q1TOsqsV\n"
    "eBOsbYQfbR/mmOQuGg4MjKz8fuTfz8AI9UnJiOGrkKfW5mgTwTkmJ5uREv9xu+yT\n"
    "R2HllXzZtEDYL248wDi24tBTgf6aouZ2ZKitsKmlU+WFuelnQmvQI3zxMRZtUf/r\n"
    "z4RutjpLKhv2nA17EL2ZPOONEMkGf3v8SauKvAYK5/bqAVGA1Y7OZGgAfdMXcj9j\n"
    "HL9rTWFaYG3LkXYant5DrhWylCwXFT5Kuy0zeeJtZbMB9yeWsUwNOswBXGnUuoot\n"
    "5GI04onUXmODdC0UP/yKyvGwa6Gg+b3x1gwbmLBqHqU1dMRbc7REhZqg1yYRuu1i\n"
    "a/iJT+/YLCTj9Sflu5gWWnxPdHIUnINX53Pvc1TyPxEDF0ypnBEx9HMbFrwiuo89\n"
    "x81063+8u+lyOYJavo9AUT4S2unB9yoiwxotKNkSgVm1nHRoDb9BSsmHzpmfXIzc\n"
    "a5qAVdW+k8GKFANvTgJK34tk0p9LmqDkwsiNn2bT8HagLLKpu9UQx4jhNJDI0ORk\n"
    "NXrxxzIGj+TawSFNFgFxZzJdUOlNL+4IbUxTf+rf/JQVn/Rxspg30peRBQIDsr7C\n"
    "FnUcdikxoZ6ZXMTW11cEMncvuZCJ1TgXUrsMwFnjJL5yCG7v+V/Vm09j6RPiJaQP\n"
    "gs5kGvkhtOuh7ef3sSDeUAy0Cjq2p24xgw==\n"
    "-----END ENCRYPTED PRIVATE KEY-----\n";

static const char RsaLegacyPem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: AES-128-CBC,451019F0834043630DB517773C851B48\n"
    "\n"
    "nelLYzhGcCHjif9vxai8tIsRprwxXA0NBHpKsR0QPNjJH4t1RlajIxjGrZ+0zA1Q\n"
    "6ti4zSEqf1hWZqvcR2idTAgQFbVYV3bNcjKm4xAacWkwayDlA+mZ3AoMUY4+fFPf\n"
    "vbQsmra+d07elAVYuGc6sVJ6S8jizG4eMIlwON9lo9BizRCPobuAoVPCgmNxGKa8\n"
    "pVCKJJLvrqRQOs42lk6fE0IkhOVckcJhsmJw5epdvKtESzYjPdj2IwtxjwOjMTgV\n"
    "sMo/l2GS7vfkvLVTG59QZUa12y5LvThlih+zN/TRlXxyL/earoQpSU+71IzkTF/w\n"
    "xaO5xon3UePtB9nVhofzEfFHkRSedZEgAcJjQp29I7XEK3Ry60HfIZvpxCkUFkWo\n"
    "XNvc9EszoG7b0PyFUE+PuHeMTsduBCbc6eBSux1b0jlKETZDJ95s0Cn70HrSFWuw\n"
    "FpS9UCEyj25ID5As12riZYPkw8s2uPwQ2UQP4lBQvuRa685G2Jz9XnqDmXa76EJX\n"
    "fmXeo6KsERi/J2x1YsuW6nQnyz1NDlP8mYR5kUg/HpPvkEw7EiTDIM778WCEb3eV\n"
    "YtwiuRqpWGkOQM50koGID14dZT0epv+jdpndDmzb8zeSk+c4ApGa90wp0PcBYKnF\n"
    "QaiahRDPredF+VHxtH2qqeMrHLiNr43ej45b17W+UL2H820+NvC3FhkXtUYitzyb\n"
    "2hMSjy4wO8F/a0IKEuGdXkxV+zynco7kk8NLiChyA8YKUDrCWj4JesSwNDR5TNi0\n"
    "7ECA3R2TmcOm4mWgcK3Bznl6wOOPJWzfiqhR2KRqteBL1XOcApMP11CzMTKkzVK2\n"
    "-----END RSA PRIVATE KEY-----\n";

static const char ModulusHex[] =
    "c018024d14aa1c00ffd6a55fc1c53931d1d562c7dfc5184b660d90cd3d7ae858"
    "cd9a5689f256b255485c22dcead77451547a83dfe79077a80aefd3170bc337ba"
    "87c18e0b6b629f85f2dee827095cf892ade35933db4a6049dd29f75fd0c7b394"
    "7169bac9a99301a2329031e1650a5a2d191ca28109dc8e54b422a60662d1098b";

static const char SpkiHex[] =
    "30819f300d06092a864886f70d010101050003818d0030818902818100c01802"
    "4d14aa1c00ffd6a55fc1c53931d1d562c7dfc5184b660d90cd3d7ae858cd9a56"
    "89f256b255485c22dcead77451547a83dfe79077a80aefd3170bc337ba87c18e"
    "0b6b629f85f2dee827095cf892ade35933db4a6049dd29f75fd0c7b3947169ba"
    "c9a99301a2329031e1650a5a2d191ca28109dc8e54b422a60662d1098b020301"
    "0001";

static const char Pkcs1PublicHex[] =
    "30818902818100c018024d14aa1c00ffd6a55fc1c53931d1d562c7dfc5184b66"
    "0d90cd3d7ae858cd9a5689f256b255485c22dcead77451547a83dfe79077a80a"
    "efd3170bc337ba87c18e0b6b629f85f2dee827095cf892ade35933db4a6049dd"
    "29f75fd0c7b3947169bac9a99301a2329031e1650a5a2d191ca28109dc8e54b4"
    "22a60662d1098b0203010001";

static const char SigPkcs1Hex[] =
    "78fd002ef900b9b8ac4002d2049e4830eaea21a2e737a0a618526ec57d055e63"
    "4b7c58ac1bcc1d9ee0a6591abfa6438271d760a71d3560aea2acad12455c1baf"
    "334e762fcd4b20d89744a137d73999c99a4be3478868ae2f4ebacd90f664d96a"
    "ab6a3f64defd25a8f55cfc8d0db6bed367306f1ac2b5c1d64f741fb988e67d43";

static const char SigRaw36Hex[] =
    "34d773edded655844046432adcaa6238dd9d5f15efa206d833022c373ebf6871"
    "51d083969b914ea439559952cc5cd0123c0dab57cdfec21545db7eb6dae6f869"
    "99bbb3eb5a1c63c153b7e4ee9706e77203fa819073d4dd33cfb10b46dbf53fc0"
    "b40221321a418ce5f5b774efd9b0777f36bfe8d3b3b4915ff7783dc85f3d86dc";

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

static const char EncPkcs1Hex[] =
    "61300b2a3c2b6057dc3977584cd06576f346fed5a2c7fe79432fae345d6d7092"
    "f2f0c3ea1b0fd26b40184eecef69c5e378d1b23ba3486b3295af900e07f89816"
    "124070bd47d57457e99ce6e702838e2c4e82c824f6b29686de3e4e4d53cf0878"
    "414770a3701d5670d406d5418e1fad0c5d81e697bb7f4c89f5441c326ce58dc8";

static const char EncOaepHex[] =
    "16ded1231f478ee646fb6cde9de24fd1c460725b4c39d1401c9e0060a51207f8"
    "288405053f72f68d9ab4c19071819e5319021e97222b2de4731806a441f54c32"
    "2b130c59493fd11d3da082d4ecd6e9e7330c821b40ced9b0097f190f26302413"
    "adda4a8e10f27f3ff7e39b20978dd0a409f38886d9487ec315c1a1e6170f7e86";

static const char EncRawHex[] =
    "72cacf4e58d414c1678549d5d4905dcfe7d6cc413501adccf4ac650b6941aea6"
    "8b09e01b7909e4360b74425617a740d3e3c82ff4ab7512f5bd94bf71d208b504"
    "d03478f67283679e93deb0ee44d3df0cd1c090be119e0461b76b9b90c5756c05"
    "9ccbd65de83268bf9aaff9c5d3ea2eece98d4e70e9eb16d7eded150e84addc97";

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
    std::uint64_t last = 0;
    while (const auto code = ERR_get_error_nid_postfix()) last = code;
    return last == expected;
}

static std::vector<unsigned char> Bytes(const GuestBignum* bn) {
    std::vector<unsigned char> out(1024);
    out.resize(static_cast<std::size_t>(BN_bn2bin_nid_postfix(bn, out.data())));
    return out;
}

static GuestEvpPkey* ReadKey(const char* pem, void* cb, void* u) {
    std::string text(pem);
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(text.data(), static_cast<int>(text.size()));
    GuestEvpPkey* pkey = PEM_read_bio_PrivateKey_nid_postfix(bio, nullptr, cb, u);
    BIO_free_nid_postfix(bio);
    return pkey;
}

static int callbackCalls = 0;

static int APS5_VABI GoodPassword(char* buf, int size, int rwflag, void* u) {
    ++callbackCalls;
    Require(size >= 1024 && rwflag == 0 && u == &callbackCalls);
    std::memcpy(buf, "secret", 6);
    return 6;
}

static int APS5_VABI FailingPassword(char*, int, int, void*) {
    ++callbackCalls;
    return 0;
}

static GuestRsa* LoadRsa() {
    GuestEvpPkey* pkey = ReadKey(RsaPem, nullptr, nullptr);
    Require(pkey && EVP_PKEY_id_nid_postfix(pkey) == 6 && pkey->type == 6 && pkey->references == 1);
    GuestRsa* rsa = EVP_PKEY_get1_RSA_nid_postfix(pkey);
    Require(rsa && rsa == pkey->pkey && rsa->references == 2);
    EVP_PKEY_free_nid_postfix(pkey);
    Require(rsa->references == 1);
    return rsa;
}

static void TestKeyLayout(GuestRsa* rsa) {
    Require(RSA_size_nid_postfix(rsa) == 128);
    Require(Bytes(rsa->n) == Hex(ModulusHex));
    Require(Bytes(rsa->e) == std::vector<unsigned char>({0x01, 0x00, 0x01}));
    Require(rsa->d && rsa->p && rsa->q && rsa->dmp1 && rsa->dmq1 && rsa->iqmp);
    Require(Bytes(rsa->p).size() == 64 && Bytes(rsa->q).size() == 64);
    GuestRsa* empty = RSA_new_nid_postfix();
    Require(empty && empty->references == 1 && !empty->n && !empty->e && empty->pad == 0 && empty->flags == 6);
    Require(Throws([&] { RSA_size_nid_postfix(empty); }));
    RSA_free_nid_postfix(empty);
    RSA_free_nid_postfix(nullptr);
}

static void TestSignatures(GuestRsa* rsa) {
    unsigned char sig[128];
    unsigned int length = 0;
    Require(RSA_sign_nid_postfix(672, Digest, 32, sig, &length, rsa) == 1 && length == 128);
    Require(std::vector<unsigned char>(sig, sig + 128) == Hex(SigPkcs1Hex));
    Require(RSA_verify_nid_postfix(672, Digest, 32, sig, 128, rsa) == 1);
    ERR_clear_error_nid_postfix();
    sig[5] ^= 1;
    Require(RSA_verify_nid_postfix(672, Digest, 32, sig, 128, rsa) == 0 && LastError(Code(4, 119, 104)));
    Require(RSA_verify_nid_postfix(672, Digest, 32, sig, 127, rsa) == 0 && LastError(Code(4, 119, 119)));
    Require(RSA_verify_nid_postfix(64, Digest, 20, Hex(SigPkcs1Hex).data(), 128, rsa) == 0 && LastError(Code(4, 119, 104)));
    Require(RSA_sign_nid_postfix(999, Digest, 32, sig, &length, rsa) == 0 && LastError(Code(4, 117, 117)));
    Require(Throws([&] { RSA_sign_nid_postfix(672, Digest, 31, sig, &length, rsa); }));

    unsigned char block[36];
    std::memset(block, 'A', sizeof(block));
    Require(RSA_sign_nid_postfix(114, block, 36, sig, &length, rsa) == 1 && length == 128);
    Require(std::vector<unsigned char>(sig, sig + 128) == Hex(SigRaw36Hex));
    Require(RSA_verify_nid_postfix(114, block, 36, sig, 128, rsa) == 1);
    Require(RSA_sign_nid_postfix(114, block, 35, sig, &length, rsa) == 0 && LastError(Code(4, 117, 131)));
    unsigned char raw[128];
    Require(RSA_private_encrypt_nid_postfix(36, block, raw, rsa, 1) == 128 && std::memcmp(raw, sig, 128) == 0);
    unsigned char recovered[128];
    Require(RSA_public_decrypt_nid_postfix(128, sig, recovered, rsa, 1) == 36 && std::memcmp(recovered, block, 36) == 0);
    Require(RSA_private_encrypt_nid_postfix(118, raw, sig, rsa, 1) == -1 && LastError(Code(4, 108, 110)));
    Require(RSA_private_encrypt_nid_postfix(36, block, sig, rsa, 4) == -1 && LastError(Code(4, 102, 118)));
    const auto encrypted = Hex(EncRawHex);
    Require(RSA_public_decrypt_nid_postfix(128, encrypted.data(), recovered, rsa, 1) == -1 && LastError(Code(4, 103, 114)));
}

static void TestEncryption(GuestRsa* rsa) {
    unsigned char out[128];
    const auto pkcs1 = Hex(EncPkcs1Hex);
    Require(RSA_private_decrypt_nid_postfix(128, pkcs1.data(), out, rsa, 1) == 29 && std::memcmp(out, Plain, 29) == 0);
    const auto oaep = Hex(EncOaepHex);
    Require(RSA_private_decrypt_nid_postfix(128, oaep.data(), out, rsa, 4) == 29 && std::memcmp(out, Plain, 29) == 0);
    ERR_clear_error_nid_postfix();
    Require(RSA_private_decrypt_nid_postfix(128, oaep.data(), out, rsa, 1) == -1 && LastError(Code(4, 101, 114)));
    std::vector<unsigned char> block(128, 0x5a);
    block[0] = 0;
    Require(RSA_public_encrypt_nid_postfix(128, block.data(), out, rsa, 3) == 128);
    Require(std::vector<unsigned char>(out, out + 128) == Hex(EncRawHex));
    unsigned char back[128];
    Require(RSA_private_decrypt_nid_postfix(128, out, back, rsa, 3) == 128 && std::memcmp(back, block.data(), 128) == 0);
    Require(RSA_public_encrypt_nid_postfix(127, block.data(), out, rsa, 3) == -1 && LastError(Code(4, 107, 122)));
    std::vector<unsigned char> big(128, 0xff);
    Require(RSA_public_encrypt_nid_postfix(128, big.data(), out, rsa, 3) == -1 && LastError(Code(4, 104, 132)));
    Require(RSA_private_decrypt_nid_postfix(129, big.data(), out, rsa, 1) == -1 && LastError(Code(4, 101, 108)));
    for (int padding : {1, 4}) {
        Require(RSA_public_encrypt_nid_postfix(29, reinterpret_cast<const unsigned char*>(Plain), out, rsa, padding) == 128);
        Require(RSA_private_decrypt_nid_postfix(128, out, back, rsa, padding) == 29 && std::memcmp(back, Plain, 29) == 0);
    }
    Require(RSA_public_encrypt_nid_postfix(118, big.data(), out, rsa, 1) == -1 && LastError(Code(4, 109, 110)));
    Require(RSA_public_encrypt_nid_postfix(87, big.data(), out, rsa, 4) == -1 && LastError(Code(4, 121, 110)));
    Require(RSA_public_encrypt_nid_postfix(16, big.data(), out, rsa, 99) == -1 && LastError(Code(4, 104, 118)));
    Require(Throws([&] { RSA_public_encrypt_nid_postfix(16, big.data(), out, rsa, 2); }));
}

static void TestPss(GuestRsa* rsa) {
    unsigned char em[128];
    unsigned char sig[128];
    Require(RSA_padding_add_PKCS1_PSS_nid_postfix(rsa, em, Digest, EVP_sha256_nid_postfix(), 0) == 1);
    Require(RSA_private_encrypt_nid_postfix(128, em, sig, rsa, 3) == 128);
    Require(std::vector<unsigned char>(sig, sig + 128) == Hex(SigPss0Hex));
    const auto salted = Hex(SigPss32Hex);
    Require(RSA_public_decrypt_nid_postfix(128, salted.data(), em, rsa, 3) == 128);
    Require(RSA_verify_PKCS1_PSS_nid_postfix(rsa, Digest, EVP_sha256_nid_postfix(), em, 32) == 1);
    Require(RSA_verify_PKCS1_PSS_nid_postfix(rsa, Digest, EVP_sha256_nid_postfix(), em, -1) == 1);
    Require(RSA_verify_PKCS1_PSS_nid_postfix(rsa, Digest, EVP_sha256_nid_postfix(), em, -2) == 1);
    ERR_clear_error_nid_postfix();
    Require(RSA_verify_PKCS1_PSS_nid_postfix(rsa, Digest, EVP_sha256_nid_postfix(), em, 0) == 0 && LastError(Code(4, 126, 136)));
    unsigned char other[32];
    std::memcpy(other, Digest, 32);
    other[0] ^= 1;
    Require(RSA_verify_PKCS1_PSS_nid_postfix(rsa, other, EVP_sha256_nid_postfix(), em, -2) == 0 && LastError(Code(4, 126, 104)));
    em[127] = 0xbb;
    Require(RSA_verify_PKCS1_PSS_nid_postfix(rsa, Digest, EVP_sha256_nid_postfix(), em, -2) == 0 && LastError(Code(4, 126, 134)));
    Require(RSA_padding_add_PKCS1_PSS_nid_postfix(rsa, em, Digest, EVP_sha256_nid_postfix(), -2) == 1);
    Require(RSA_verify_PKCS1_PSS_nid_postfix(rsa, Digest, EVP_sha256_nid_postfix(), em, -2) == 1);
    Require(RSA_verify_PKCS1_PSS_nid_postfix(rsa, Digest, EVP_sha256_nid_postfix(), em, 128 - 32 - 2) == 1);
    Require(RSA_padding_add_PKCS1_PSS_nid_postfix(rsa, em, Digest, EVP_sha256_nid_postfix(), 95) == 0 && LastError(Code(4, 125, 110)));
    Require(RSA_padding_add_PKCS1_PSS_nid_postfix(rsa, em, Digest, EVP_sha256_nid_postfix(), -3) == 0 && LastError(Code(4, 125, 136)));
    unsigned char wide[64] = {};
    Require(RSA_padding_add_PKCS1_PSS_nid_postfix(rsa, em, wide, EVP_sha512_nid_postfix(), -1) == 0 && LastError(Code(4, 125, 110)));
    Require(RSA_padding_add_PKCS1_PSS_nid_postfix(rsa, em, wide, EVP_sha512_nid_postfix(), 10) == 1);
    Require(RSA_verify_PKCS1_PSS_nid_postfix(rsa, wide, EVP_sha512_nid_postfix(), em, 10) == 1);
}

static void TestGenerate() {
    GuestRsa* rsa = RSA_new_nid_postfix();
    GuestBignum* e = BN_new_nid_postfix();
    BN_set_word_nid_postfix(e, 65537);
    Require(RSA_generate_key_ex_nid_postfix(rsa, 1024, e, nullptr) == 1);
    Require(RSA_size_nid_postfix(rsa) == 128 && rsa->d && rsa->p && rsa->q && rsa->iqmp);
    Require(Bytes(rsa->e) == std::vector<unsigned char>({0x01, 0x00, 0x01}));
    unsigned char sig[128];
    unsigned int length = 0;
    Require(RSA_sign_nid_postfix(672, Digest, 32, sig, &length, rsa) == 1 && RSA_verify_nid_postfix(672, Digest, 32, sig, length, rsa) == 1);
    GuestBignum* n = rsa->n;
    Require(RSA_generate_key_ex_nid_postfix(rsa, 1024, e, nullptr) == 1 && rsa->n == n);
    Require(Throws([&] { RSA_generate_key_ex_nid_postfix(rsa, 1024, e, sig); }));
    Require(Throws([&] { RSA_generate_key_ex_nid_postfix(rsa, 512, e, nullptr); }));
    BN_set_word_nid_postfix(e, 4);
    Require(Throws([&] { RSA_generate_key_ex_nid_postfix(rsa, 1024, e, nullptr); }));
    BN_free_nid_postfix(e);
    RSA_free_nid_postfix(rsa);
}

static void TestPkey(GuestRsa* rsa) {
    Require(EVP_PKEY_type_nid_postfix(6) == 6 && EVP_PKEY_type_nid_postfix(19) == 6 && EVP_PKEY_type_nid_postfix(67) == 116);
    Require(EVP_PKEY_type_nid_postfix(408) == 408 && EVP_PKEY_type_nid_postfix(855) == 855 && EVP_PKEY_type_nid_postfix(672) == 0);
    GuestEvpPkey* pkey = EVP_PKEY_new_nid_postfix();
    Require(pkey && pkey->type == 0 && pkey->references == 1 && pkey->save_parameters == 1 && EVP_PKEY_id_nid_postfix(pkey) == 0);
    ERR_clear_error_nid_postfix();
    Require(EVP_PKEY_get1_RSA_nid_postfix(pkey) == nullptr && LastError(Code(6, 121, 127)));
    Require(EVP_PKEY_set1_RSA_nid_postfix(pkey, rsa) == 1 && rsa->references == 2 && pkey->pkey == rsa && pkey->type == 6);
    Require(EVP_PKEY_get1_EC_KEY_nid_postfix(pkey) == nullptr && LastError(Code(6, 131, 142)));
    Require(Throws([&] { EVP_PKEY_copy_parameters_nid_postfix(pkey, pkey); }));

    GuestEvpMdCtx* ctx = EVP_MD_CTX_create_nid_postfix();
    void* pctx = nullptr;
    Require(EVP_DigestVerifyInit_nid_postfix(ctx, &pctx, EVP_sha256_nid_postfix(), nullptr, pkey) == 1 && pctx && ctx->pctx == pctx);
    Require(EVP_DigestUpdate_nid_postfix(ctx, Message, 10) == 1 && EVP_DigestUpdate_nid_postfix(ctx, Message + 10, std::strlen(Message) - 10) == 1);
    const auto signature = Hex(SigPkcs1Hex);
    Require(EVP_DigestVerifyFinal_nid_postfix(ctx, signature.data(), signature.size()) == 1);
    Require(EVP_DigestVerifyFinal_nid_postfix(ctx, signature.data(), signature.size()) == 1);
    auto broken = signature;
    broken[100] ^= 0x10;
    Require(EVP_DigestVerifyFinal_nid_postfix(ctx, broken.data(), broken.size()) == 0);
    EVP_MD_CTX_destroy_nid_postfix(ctx);
    Require(rsa->references == 2);
    GuestEvpMdCtx local;
    EVP_MD_CTX_init_nid_postfix(&local);
    Require(EVP_DigestVerifyInit_nid_postfix(&local, nullptr, EVP_sha256_nid_postfix(), nullptr, pkey) == 1 && pkey->references == 2);
    Require(EVP_DigestUpdate_nid_postfix(&local, Message, std::strlen(Message)) == 1);
    Require(EVP_DigestVerifyFinal_nid_postfix(&local, signature.data(), signature.size()) == 1);
    Require(EVP_MD_CTX_cleanup_nid_postfix(&local) == 1 && pkey->references == 1 && !local.pctx);
    Require(Throws([&] { EVP_DigestVerifyFinal_nid_postfix(&local, signature.data(), signature.size()); }));
    EVP_PKEY_free_nid_postfix(pkey);
    Require(rsa->references == 1);
    EVP_PKEY_free_nid_postfix(nullptr);
}

static void TestPem(const char* path) {
    for (const char* pem : {RsaPkcs8Pem, RsaPem}) {
        GuestEvpPkey* pkey = ReadKey(pem, nullptr, nullptr);
        Require(pkey && pkey->type == 6 && Bytes(static_cast<GuestRsa*>(pkey->pkey)->n) == Hex(ModulusHex));
        EVP_PKEY_free_nid_postfix(pkey);
    }
    char secret[] = "secret";
    for (const char* pem : {RsaPkcs8EncryptedPem, RsaLegacyPem}) {
        GuestEvpPkey* pkey = ReadKey(pem, nullptr, secret);
        Require(pkey && pkey->type == 6 && Bytes(static_cast<GuestRsa*>(pkey->pkey)->n) == Hex(ModulusHex));
        EVP_PKEY_free_nid_postfix(pkey);
        callbackCalls = 0;
        pkey = ReadKey(pem, reinterpret_cast<void*>(GoodPassword), &callbackCalls);
        Require(pkey && callbackCalls == 1);
        EVP_PKEY_free_nid_postfix(pkey);
        char wrong[] = "wrong";
        ERR_clear_error_nid_postfix();
        Require(ReadKey(pem, nullptr, wrong) == nullptr && LastError(Code(9, 123, 101)));
        Require(ReadKey(pem, reinterpret_cast<void*>(FailingPassword), nullptr) == nullptr && LastError(Code(9, 123, 104)));
        Require(Throws([&] { ReadKey(pem, nullptr, nullptr); }));
    }
    callbackCalls = 0;
    GuestEvpPkey* plain = ReadKey(RsaPem, reinterpret_cast<void*>(GoodPassword), &callbackCalls);
    Require(plain && callbackCalls == 0);
    EVP_PKEY_free_nid_postfix(plain);
    std::string bundle = std::string("text\n-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----\n") + RsaPkcs8Pem + RsaPem;
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(bundle.data(), static_cast<int>(bundle.size()));
    GuestEvpPkey* first = PEM_read_bio_PrivateKey_nid_postfix(bio, nullptr, nullptr, nullptr);
    GuestEvpPkey* second = nullptr;
    Require(first && PEM_read_bio_PrivateKey_nid_postfix(bio, &second, nullptr, nullptr) == second && second);
    Require(BIO_ctrl_nid_postfix(bio, 10, 0, nullptr) == 0);
    ERR_clear_error_nid_postfix();
    Require(PEM_read_bio_PrivateKey_nid_postfix(bio, nullptr, nullptr, nullptr) == nullptr && LastError(Code(9, 109, 108)));
    Require(Throws([&] { PEM_read_bio_PrivateKey_nid_postfix(bio, &second, nullptr, nullptr); }));
    EVP_PKEY_free_nid_postfix(first);
    EVP_PKEY_free_nid_postfix(second);
    BIO_free_nid_postfix(bio);

    FileStream* stream = fopen_nid_postfix(path, "wb");
    Require(stream && fwrite_nid_postfix(RsaPem, 1, std::strlen(RsaPem), stream) == std::strlen(RsaPem));
    Require(fclose_nid_postfix(stream) == 0);
    stream = fopen_nid_postfix(path, "rb");
    GuestRsa* out = nullptr;
    GuestRsa* rsa = PEM_read_RSAPrivateKey_nid_postfix(stream, &out, nullptr, nullptr);
    Require(rsa && out == rsa && rsa->references == 1 && Bytes(rsa->n) == Hex(ModulusHex));
    Require(PEM_read_RSAPrivateKey_nid_postfix(stream, nullptr, nullptr, nullptr) == nullptr);
    Require(fclose_nid_postfix(stream) == 0);
    RSA_free_nid_postfix(rsa);
    std::remove(path);
}

static void TestPublicKey(GuestRsa* key) {
    const auto spki = Hex(SpkiHex);
    std::vector<unsigned char> input = spki;
    input.push_back(0);
    const unsigned char* cursor = input.data();
    GuestRsa* out = nullptr;
    GuestRsa* rsa = d2i_RSA_PUBKEY_nid_postfix(&out, &cursor, static_cast<std::int64_t>(input.size()));
    Require(rsa && out == rsa && cursor == input.data() + spki.size());
    Require(Bytes(rsa->n) == Hex(ModulusHex) && !rsa->d && RSA_size_nid_postfix(rsa) == 128);
    Require(RSA_verify_nid_postfix(672, Digest, 32, Hex(SigPkcs1Hex).data(), 128, rsa) == 1);
    unsigned char sealed[128];
    Require(RSA_public_encrypt_nid_postfix(29, reinterpret_cast<const unsigned char*>(Plain), sealed, rsa, 4) == 128);
    unsigned char opened[128];
    Require(RSA_private_decrypt_nid_postfix(128, sealed, opened, key, 4) == 29);
    Require(Throws([&] { RSA_private_decrypt_nid_postfix(128, sealed, opened, rsa, 4); }));
    Require(Throws([&] { d2i_RSA_PUBKEY_nid_postfix(&out, &cursor, 10); }));
    RSA_free_nid_postfix(rsa);
    cursor = input.data() + 1;
    Require(d2i_RSA_PUBKEY_nid_postfix(nullptr, &cursor, 20) == nullptr && cursor == input.data() + 1);
}

int main(int argc, char** argv) {
    Require(argc == 2);
    void* const heapApi[10] {};
    ApplicationHeapRegister_nid_no_patch(heapApi);
    GuestRsa* rsa = LoadRsa();
    TestKeyLayout(rsa);
    TestSignatures(rsa);
    TestEncryption(rsa);
    TestPss(rsa);
    TestGenerate();
    TestPkey(rsa);
    TestPem(argv[1]);
    TestPublicKey(rsa);
    RSA_free_nid_postfix(rsa);
    return 0;
}
