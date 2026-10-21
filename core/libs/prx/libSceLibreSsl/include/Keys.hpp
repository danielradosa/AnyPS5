#ifndef CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_KEYS_HPP
#define CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_KEYS_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <mbedtls/bignum.h>
#include <mbedtls/ecp.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>

#include "LibreSsl.hpp"

namespace LibreSsl {

struct EvpMd;
struct Asn1String;

struct CryptoExData {
    void* sk;
};

struct Rsa {
    int pad;
    std::int64_t version;
    const void* meth;
    void* engine;
    Bignum* n;
    Bignum* e;
    Bignum* d;
    Bignum* p;
    Bignum* q;
    Bignum* dmp1;
    Bignum* dmq1;
    Bignum* iqmp;
    CryptoExData exData;
    int references;
    int flags;
    void* methodModN;
    void* methodModP;
    void* methodModQ;
    void* blinding;
    void* mtBlinding;
};

static_assert(sizeof(Rsa) == 152 && offsetof(Rsa, n) == 32 && offsetof(Rsa, iqmp) == 88);
static_assert(offsetof(Rsa, references) == 104 && offsetof(Rsa, flags) == 108 && offsetof(Rsa, mtBlinding) == 144);

struct EvpPkey {
    int type;
    int saveType;
    int references;
    const void* ameth;
    void* engine;
    void* pkey;
    int saveParameters;
    void* attributes;
};

static_assert(sizeof(EvpPkey) == 56 && offsetof(EvpPkey, ameth) == 16 && offsetof(EvpPkey, pkey) == 32 && offsetof(EvpPkey, attributes) == 48);

struct EcGroup {
    mbedtls_ecp_group group;
    int nid = 0;
    int asn1Flag = 0;
    int form = 4;

    EcGroup() {
        mbedtls_ecp_group_init(&group);
    }

    ~EcGroup() {
        mbedtls_ecp_group_free(&group);
    }

    EcGroup(const EcGroup&) = delete;
    EcGroup& operator=(const EcGroup&) = delete;
};

struct EcPoint {
    mbedtls_ecp_point point;
    mbedtls_ecp_group_id curve = MBEDTLS_ECP_DP_NONE;

    EcPoint() {
        mbedtls_ecp_point_init(&point);
    }

    ~EcPoint() {
        mbedtls_ecp_point_free(&point);
    }

    EcPoint(const EcPoint&) = delete;
    EcPoint& operator=(const EcPoint&) = delete;
};

struct EcKey {
    std::atomic<int> references{1};
    EcGroup* group = nullptr;
    Bignum* privateKey = nullptr;
    EcPoint* publicKey = nullptr;
    int form = 4;
    unsigned encodingFlags = 0;
    int flags = 0;
};

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

constexpr int NidRsaEncryption = 6;
constexpr int NidRsa = 19;
constexpr int NidDhKeyAgreement = 28;
constexpr int NidDsa = 116;
constexpr int NidEcPublicKey = 408;
constexpr int NidHmac = 855;
constexpr int NidMd5Sha1 = 114;
constexpr int NidRipemd160 = 117;

constexpr int ErrLibRsa = 4;
constexpr int ErrLibX509 = 11;
constexpr int ErrLibAsn1 = 13;
constexpr int ErrLibEc = 16;
constexpr int ErrLibX509v3 = 34;
constexpr int ErrLibPkcs12 = 35;
constexpr int ErrLibEcdsa = 42;
constexpr int ErrLibEcdh = 43;

constexpr int RsaPkcs1Padding = 1;
constexpr int RsaNoPadding = 3;
constexpr int RsaPkcs1OaepPadding = 4;
constexpr int RsaPkcs1PssPadding = 6;

int RandomBytes(void* state, unsigned char* output, std::size_t length);
void ReadMpi(const Bignum& bn, mbedtls_mpi& out, const char* function);
void WriteMpi(Bignum& bn, const mbedtls_mpi& in, const char* function);
Bignum* MpiBignum(const mbedtls_mpi& in, const char* function);
void ClearFreeBignum(Bignum* bn);
const EvpMd* DigestByNid(int nid);

Rsa* NewRsa();
void RsaUpRef(Rsa* rsa);
void FreeRsa(Rsa* rsa);
bool RsaContext(const Rsa& rsa, mbedtls_rsa_context& ctx, bool needPrivate, const char* function);
void FillRsa(Rsa& rsa, const mbedtls_rsa_context& ctx, const char* function);
int RsaSize(const Rsa& rsa, const char* function);
int RsaSignDigest(int type, const unsigned char* m, unsigned int length, unsigned char* sig, unsigned int* siglen, Rsa& rsa, const char* function);
int RsaVerifyDigest(int type, const unsigned char* m, unsigned int length, const unsigned char* sig, unsigned int siglen, Rsa& rsa, const char* function);
int RsaPublicEncrypt(int flen, const unsigned char* from, unsigned char* to, Rsa& rsa, int padding);
int RsaPrivateDecrypt(int flen, const unsigned char* from, unsigned char* to, Rsa& rsa, int padding);
int RsaPssEncode(Rsa& rsa, unsigned char* em, const unsigned char* mHash, const EvpMd* hash, const EvpMd* mgf1Hash, int sLen);
int RsaPssVerify(Rsa& rsa, const unsigned char* mHash, const EvpMd* hash, const EvpMd* mgf1Hash, const unsigned char* em, int sLen);

EcGroup* NewGroupByNid(int nid, const char* function);
EcGroup* CopyGroup(const EcGroup& group);
int CurveNid(mbedtls_ecp_group_id id);
EcKey* NewEcKey();
void EcKeyUpRef(EcKey* key);
void FreeEcKey(EcKey* key);
EcKey* EcKeyFromKeypair(const mbedtls_ecp_keypair& keypair, const char* function);
bool EcKeypair(const EcKey& key, mbedtls_ecp_keypair& keypair, bool needPrivate, const char* function);
int EcdsaSize(const EcKey& key);
int EcdsaSign(const unsigned char* dgst, int dgstlen, unsigned char* sig, unsigned int* siglen, EcKey& key, const char* function);
int EcdsaVerify(const unsigned char* dgst, int dgstlen, const unsigned char* sig, int siglen, EcKey& key, const char* function);

EvpPkey* NewPkey();
void PkeyUpRef(EvpPkey* pkey);
void FreePkey(EvpPkey* pkey);
void AssignRsa(EvpPkey& pkey, Rsa* rsa);
void AssignEcKey(EvpPkey& pkey, EcKey* key);
EvpPkey* PkeyFromMbed(const mbedtls_pk_context& pk, const char* function);
bool PkeyToMbed(const EvpPkey& pkey, mbedtls_pk_context& pk, const char* function);
int PkeySize(const EvpPkey& pkey, const char* function);
Rsa* PkeyRsa(const EvpPkey* pkey, int function);
EcKey* PkeyEcKey(const EvpPkey* pkey, int function);
void FreePkeyCtx(void* ctx);

struct PemPassword {
    void* callback;
    void* userdata;
};

enum class KeyParse { Ok, WrongPassword, Malformed };

bool PemPassphrase(const PemPassword& password, std::string& out, const char* function);
KeyParse ParsePrivateKey(const unsigned char* data, std::size_t length, const std::string* password, EvpPkey*& out, const char* function);
std::vector<unsigned char> EncodeSubjectPublicKey(const EvpPkey& pkey, const char* function);

}

#endif
