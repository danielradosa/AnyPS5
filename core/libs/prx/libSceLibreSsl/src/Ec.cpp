#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/include/Keys.hpp"
#include "prx/libc/include/General.hpp"

#include <mbedtls/asn1.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/oid.h>

#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace LibreSsl {

namespace {

constexpr int PointCompressed = 2;
constexpr int PointUncompressed = 4;
constexpr int PointHybrid = 6;

constexpr int EcFunctionGroupGetDegree = 173;
constexpr int EcFunctionGroupNewByCurveName = 174;
constexpr int EcFunctionKeyCheckKey = 177;
constexpr int EcFunctionPointMul = 184;
constexpr int EcFunctionPointOct2Point = 122;
constexpr int EcFunctionPointPoint2Oct = 123;
constexpr int EcFunctionD2iEcParameters = 144;
constexpr int EcFunctionO2iEcPublicKey = 152;
constexpr int EcFunctionKeyNew = 182;
constexpr int EcFunctionPointCopy = 114;

constexpr int EcReasonAsn1Error = 115;
constexpr int EcReasonD2iEcpkparametersFailure = 117;
constexpr int EcReasonBufferTooSmall = 100;
constexpr int EcReasonIncompatibleObjects = 101;
constexpr int EcReasonInvalidEncoding = 102;
constexpr int EcReasonInvalidForm = 104;
constexpr int EcReasonPointAtInfinity = 106;
constexpr int EcReasonPointIsNotOnCurve = 107;
constexpr int EcReasonInvalidPrivateKey = 123;
constexpr int EcReasonUnknownGroup = 129;
constexpr int EcReasonWrongOrder = 130;

constexpr int EcdhFunctionComputeKey = 100;
constexpr int EcdhReasonNoPrivateValue = 100;
constexpr int EcdhReasonPointArithmeticFailure = 101;
constexpr int EcdhReasonKdfFailed = 102;

struct Curve {
    int nid;
    mbedtls_ecp_group_id id;
};

constexpr Curve Curves[] = {
    {409, MBEDTLS_ECP_DP_SECP192R1},
    {713, MBEDTLS_ECP_DP_SECP224R1},
    {415, MBEDTLS_ECP_DP_SECP256R1},
    {715, MBEDTLS_ECP_DP_SECP384R1},
    {716, MBEDTLS_ECP_DP_SECP521R1},
    {711, MBEDTLS_ECP_DP_SECP192K1},
    {712, MBEDTLS_ECP_DP_SECP224K1},
    {714, MBEDTLS_ECP_DP_SECP256K1},
    {928, MBEDTLS_ECP_DP_BP256R1},
    {932, MBEDTLS_ECP_DP_BP384R1},
    {934, MBEDTLS_ECP_DP_BP512R1},
};

bool OtherLibreSslCurve(int nid) {
    return (nid >= 409 && nid <= 415) || (nid >= 684 && nid <= 745) || nid == 749 || nid == 750 || (nid >= 840 && nid <= 844) ||
        (nid >= 921 && nid <= 936) || nid == 943 || nid == 944;
}

void Check(int status, const char* function) {
    if (status == MBEDTLS_ERR_MPI_ALLOC_FAILED || status == MBEDTLS_ERR_ECP_ALLOC_FAILED) throw std::bad_alloc();
    if (status != 0) throw std::runtime_error(std::string(function) + ": Mbed TLS failed (" + std::to_string(status) + ")");
}

EcKey& RequireKey(EcKey* key, const char* function) {
    if (!key) throw std::invalid_argument(std::string(function) + ": null EC_KEY");
    return *key;
}

const EcKey& RequireKey(const EcKey* key, const char* function) {
    if (!key) throw std::invalid_argument(std::string(function) + ": null EC_KEY");
    return *key;
}

const EcGroup& RequireGroup(const EcGroup* group, const char* function) {
    if (!group) throw std::invalid_argument(std::string(function) + ": null EC_GROUP");
    return *group;
}

EcGroup& KeyGroup(const EcKey& key, const char* function) {
    if (!key.group) throw std::invalid_argument(std::string(function) + ": the EC_KEY has no group");
    return *key.group;
}

std::size_t FieldBytes(const EcGroup& group) {
    return (group.group.pbits + 7) / 8;
}

EcPoint* NewPoint(mbedtls_ecp_group_id curve) {
    auto* point = new (std::nothrow) EcPoint;
    if (!point) throw std::bad_alloc();
    point->curve = curve;
    return point;
}

void ScalarMpi(const Bignum& scalar, const EcGroup& group, mbedtls_mpi& out, const char* function) {
    const auto bytes = BignumBytes(scalar);
    Check(mbedtls_mpi_read_binary(&out, bytes.data(), bytes.size()), function);
    Check(mbedtls_mpi_mod_mpi(&out, &out, &group.group.N), function);
    if (scalar.neg && mbedtls_mpi_cmp_int(&out, 0) != 0) Check(mbedtls_mpi_sub_mpi(&out, &group.group.N, &out), function);
}

void AppendLength(std::vector<unsigned char>& out, std::size_t length) {
    if (length < 0x80) {
        out.push_back(static_cast<unsigned char>(length));
        return;
    }
    unsigned char bytes[8];
    int count = 0;
    while (length) {
        bytes[count++] = static_cast<unsigned char>(length & 0xff);
        length >>= 8;
    }
    out.push_back(static_cast<unsigned char>(0x80 | count));
    while (count--) out.push_back(bytes[count]);
}

std::vector<unsigned char> IntegerContent(const mbedtls_mpi& value, const char* function) {
    std::vector<unsigned char> bytes(mbedtls_mpi_size(&value));
    Check(mbedtls_mpi_write_binary(&value, bytes.data(), bytes.size()), function);
    if (bytes.empty() || (bytes[0] & 0x80)) bytes.insert(bytes.begin(), 0);
    return bytes;
}

std::vector<unsigned char> EncodeSignature(const mbedtls_mpi& r, const mbedtls_mpi& s, const char* function) {
    std::vector<unsigned char> body;
    for (const auto* value : {&r, &s}) {
        const auto content = IntegerContent(*value, function);
        body.push_back(MBEDTLS_ASN1_INTEGER);
        AppendLength(body, content.size());
        body.insert(body.end(), content.begin(), content.end());
    }
    std::vector<unsigned char> out{static_cast<unsigned char>(MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)};
    AppendLength(out, body.size());
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

std::size_t LengthOfLength(std::size_t length) {
    std::size_t count = 1;
    if (length >= 0x80) {
        while (length) {
            ++count;
            length >>= 8;
        }
    }
    return count;
}

enum class SignatureParse { Ok, Malformed, Negative };

SignatureParse ParseSignature(const unsigned char* sig, std::size_t length, mbedtls_mpi& r, mbedtls_mpi& s) {
    auto* cursor = const_cast<unsigned char*>(sig);
    const auto* end = sig + length;
    std::size_t sequence = 0;
    if (mbedtls_asn1_get_tag(&cursor, end, &sequence, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0 || cursor + sequence != end) return SignatureParse::Malformed;
    bool negative = false;
    for (auto* value : {&r, &s}) {
        std::size_t size = 0;
        const auto* header = cursor;
        if (mbedtls_asn1_get_tag(&cursor, end, &size, MBEDTLS_ASN1_INTEGER) != 0 || size == 0) return SignatureParse::Malformed;
        if (static_cast<std::size_t>(cursor - header) != 1 + LengthOfLength(size)) return SignatureParse::Malformed;
        if (size > 1 && ((cursor[0] == 0 && !(cursor[1] & 0x80)) || (cursor[0] == 0xff && (cursor[1] & 0x80)))) return SignatureParse::Malformed;
        if (cursor[0] & 0x80) negative = true;
        if (mbedtls_mpi_read_binary(value, cursor, size) != 0) return SignatureParse::Malformed;
        cursor += size;
    }
    if (cursor != end) return SignatureParse::Malformed;
    return negative ? SignatureParse::Negative : SignatureParse::Ok;
}

bool SameCurve(const EcGroup& group, const EcPoint& point) {
    return point.curve == group.group.id;
}

}

EcGroup* NewGroupByNid(int nid, const char* function) {
    for (const auto& curve : Curves) {
        if (curve.nid != nid) continue;
        auto* group = new (std::nothrow) EcGroup;
        if (!group) throw std::bad_alloc();
        const int status = mbedtls_ecp_group_load(&group->group, curve.id);
        if (status != 0) {
            delete group;
            Check(status, function);
        }
        group->nid = nid;
        return group;
    }
    if (OtherLibreSslCurve(nid)) throw std::invalid_argument(std::string(function) + ": curve NID " + std::to_string(nid) + " is not supported by this build");
    PushError(ErrLibEc, EcFunctionGroupNewByCurveName, EcReasonUnknownGroup);
    return nullptr;
}

EcGroup* CopyGroup(const EcGroup& group) {
    auto* copy = new (std::nothrow) EcGroup;
    if (!copy) throw std::bad_alloc();
    const int status = mbedtls_ecp_group_copy(&copy->group, &group.group);
    if (status != 0) {
        delete copy;
        Check(status, "EC_GROUP copy");
    }
    copy->nid = group.nid;
    copy->asn1Flag = group.asn1Flag;
    copy->form = group.form;
    return copy;
}

int CurveNid(mbedtls_ecp_group_id id) {
    for (const auto& curve : Curves) {
        if (curve.id == id) return curve.nid;
    }
    return 0;
}

EcKey* NewEcKey() {
    return new (std::nothrow) EcKey;
}

void EcKeyUpRef(EcKey* key) {
    RequireKey(key, "EC_KEY_up_ref").references.fetch_add(1);
}

void FreeEcKey(EcKey* key) {
    if (!key) return;
    if (key->references.fetch_sub(1) > 1) return;
    delete key->group;
    ClearFreeBignum(key->privateKey);
    delete key->publicKey;
    delete key;
}

EcKey* EcKeyFromKeypair(const mbedtls_ecp_keypair& keypair, const char* function) {
    const auto id = mbedtls_ecp_keypair_get_group_id(&keypair);
    const int nid = CurveNid(id);
    if (!nid) throw std::invalid_argument(std::string(function) + ": the key's curve is not supported");
    auto* key = NewEcKey();
    if (!key) throw std::bad_alloc();
    try {
        key->group = NewGroupByNid(nid, function);
        mbedtls_ecp_group group;
        mbedtls_ecp_group_init(&group);
        Mpi d;
        key->publicKey = NewPoint(id);
        const int status = mbedtls_ecp_export(&keypair, &group, &d.value, &key->publicKey->point);
        mbedtls_ecp_group_free(&group);
        Check(status, function);
        if (mbedtls_mpi_cmp_int(&d.value, 0) != 0) key->privateKey = MpiBignum(d.value, function);
        if (mbedtls_ecp_is_zero(&key->publicKey->point)) {
            if (!key->privateKey) throw std::invalid_argument(std::string(function) + ": the EC key has neither a private nor a public part");
            Check(mbedtls_ecp_mul(&key->group->group, &key->publicKey->point, &d.value, &key->group->group.G, RandomBytes, nullptr), function);
        }
    } catch (...) {
        FreeEcKey(key);
        throw;
    }
    return key;
}

bool EcKeypair(const EcKey& key, mbedtls_ecp_keypair& keypair, bool needPrivate, const char* function) {
    const auto& group = KeyGroup(key, function);
    if (needPrivate && !key.privateKey) throw std::invalid_argument(std::string(function) + ": the EC_KEY has no private key");
    if (!needPrivate && !key.publicKey) throw std::invalid_argument(std::string(function) + ": the EC_KEY has no public key");
    if (key.publicKey) {
        if (!SameCurve(group, *key.publicKey)) return false;
        if (mbedtls_ecp_set_public_key(group.group.id, &keypair, &key.publicKey->point) != 0) return false;
    }
    if (needPrivate) {
        Mpi d;
        ScalarMpi(*key.privateKey, group, d.value, function);
        std::vector<unsigned char> bytes(mbedtls_mpi_size(&d.value));
        Check(mbedtls_mpi_write_binary(&d.value, bytes.data(), bytes.size()), function);
        if (bytes.empty() || mbedtls_ecp_read_key(group.group.id, &keypair, bytes.data(), bytes.size()) != 0) return false;
    }
    return true;
}

int EcdsaSize(const EcKey& key) {
    const auto& group = KeyGroup(key, "ECDSA_size");
    std::size_t integer = mbedtls_mpi_size(&group.group.N);
    if (mbedtls_mpi_get_bit(&group.group.N, integer * 8 - 1)) ++integer;
    const std::size_t element = 1 + LengthOfLength(integer) + integer;
    const std::size_t body = element * 2;
    return static_cast<int>(1 + LengthOfLength(body) + body);
}

int EcdsaSign(const unsigned char* dgst, int dgstlen, unsigned char* sig, unsigned int* siglen, EcKey& key, const char* function) {
    if (dgstlen < 0 || (!dgst && dgstlen)) throw std::invalid_argument(std::string(function) + ": invalid digest");
    if (!sig || !siglen) throw std::invalid_argument(std::string(function) + ": null signature output");
    auto& group = KeyGroup(key, function);
    if (!key.privateKey) throw std::invalid_argument(std::string(function) + ": the EC_KEY has no private key");
    Mpi d, r, s;
    ScalarMpi(*key.privateKey, group, d.value, function);
    if (mbedtls_mpi_cmp_int(&d.value, 0) == 0) throw std::invalid_argument(std::string(function) + ": the private key is zero modulo the order");
    const std::vector<unsigned char> digest(dgst, dgst + dgstlen);
    Check(mbedtls_ecdsa_sign(&group.group, &r.value, &s.value, &d.value, digest.data(), digest.size(), RandomBytes, nullptr), function);
    const auto der = EncodeSignature(r.value, s.value, function);
    std::memcpy(sig, der.data(), der.size());
    *siglen = static_cast<unsigned int>(der.size());
    return 1;
}

int EcdsaVerify(const unsigned char* dgst, int dgstlen, const unsigned char* sig, int siglen, EcKey& key, const char* function) {
    if (dgstlen < 0 || (!dgst && dgstlen)) throw std::invalid_argument(std::string(function) + ": invalid digest");
    if (siglen < 0 || (!sig && siglen)) throw std::invalid_argument(std::string(function) + ": invalid signature buffer");
    auto& group = KeyGroup(key, function);
    if (!key.publicKey) throw std::invalid_argument(std::string(function) + ": the EC_KEY has no public key");
    if (!SameCurve(group, *key.publicKey)) throw std::invalid_argument(std::string(function) + ": the public key is on another curve");
    Mpi r, s;
    const auto parsed = ParseSignature(sig, static_cast<std::size_t>(siglen), r.value, s.value);
    if (parsed == SignatureParse::Malformed) return -1;
    if (parsed == SignatureParse::Negative) return 0;
    const std::vector<unsigned char> digest(dgst, dgst + dgstlen);
    const int status = mbedtls_ecdsa_verify(&group.group, digest.data(), digest.size(), &key.publicKey->point, &r.value, &s.value);
    if (status == 0) return 1;
    if (status == MBEDTLS_ERR_ECP_VERIFY_FAILED || status == MBEDTLS_ERR_ECP_INVALID_KEY || status == MBEDTLS_ERR_ECP_BAD_INPUT_DATA) return 0;
    Check(status, function);
    return -1;
}

}

using namespace LibreSsl;

extern "C" {

EcGroup* APS5_VABI EC_GROUP_new_by_curve_name_nid_postfix(int nid) {
    return NewGroupByNid(nid, __func__);
}

void APS5_VABI EC_GROUP_free_nid_postfix(EcGroup* group) {
    delete group;
}

int APS5_VABI EC_GROUP_get_degree_nid_postfix(const EcGroup* group) {
    return static_cast<int>(RequireGroup(group, __func__).group.pbits);
}

EcPoint* APS5_VABI EC_POINT_new_nid_postfix(const EcGroup* group) {
    return NewPoint(RequireGroup(group, __func__).group.id);
}

void APS5_VABI EC_POINT_free_nid_postfix(EcPoint* point) {
    delete point;
}

int APS5_VABI EC_POINT_mul_nid_postfix(const EcGroup* group, EcPoint* r, const Bignum* n, const EcPoint* q, const Bignum* m, void*) {
    auto& curve = const_cast<EcGroup&>(RequireGroup(group, __func__));
    if (!r) throw std::invalid_argument("EC_POINT_mul: null result point");
    if (!q != !m) throw std::invalid_argument("EC_POINT_mul: a point without a scalar (or the reverse) is not supported");
    if (!n && !q) throw std::invalid_argument("EC_POINT_mul: no scalar given");
    if (!SameCurve(curve, *r) || (q && !SameCurve(curve, *q))) {
        PushError(ErrLibEc, EcFunctionPointMul, EcReasonIncompatibleObjects);
        return 0;
    }
    Mpi first, second;
    if (n) ScalarMpi(*n, curve, first.value, __func__);
    if (m) ScalarMpi(*m, curve, second.value, __func__);
    mbedtls_ecp_point result;
    mbedtls_ecp_point_init(&result);
    mbedtls_ecp_point zero;
    mbedtls_ecp_point_init(&zero);
    mbedtls_ecp_set_zero(&zero);
    int status = 0;
    const bool useN = n && mbedtls_mpi_cmp_int(&first.value, 0) != 0;
    const bool useM = q && mbedtls_mpi_cmp_int(&second.value, 0) != 0 && !mbedtls_ecp_is_zero(const_cast<mbedtls_ecp_point*>(&q->point));
    if (useN && useM) {
        status = mbedtls_ecp_muladd(&curve.group, &result, &first.value, &curve.group.G, &second.value, &q->point);
    } else if (useN) {
        status = mbedtls_ecp_mul(&curve.group, &result, &first.value, &curve.group.G, RandomBytes, nullptr);
    } else if (useM) {
        status = mbedtls_ecp_mul(&curve.group, &result, &second.value, &q->point, RandomBytes, nullptr);
    } else {
        status = mbedtls_ecp_copy(&result, &zero);
    }
    if (status == 0) status = mbedtls_ecp_copy(&r->point, &result);
    mbedtls_ecp_point_free(&result);
    mbedtls_ecp_point_free(&zero);
    if (status == MBEDTLS_ERR_ECP_INVALID_KEY) {
        PushError(ErrLibEc, EcFunctionPointMul, EcReasonPointIsNotOnCurve);
        return 0;
    }
    Check(status, __func__);
    return 1;
}

std::size_t APS5_VABI EC_POINT_point2oct_nid_postfix(const EcGroup* group, const EcPoint* p, int form, unsigned char* buf, std::size_t len, void*) {
    const auto& curve = RequireGroup(group, __func__);
    if (!p) throw std::invalid_argument("EC_POINT_point2oct: null point");
    if (!SameCurve(curve, *p)) {
        PushError(ErrLibEc, EcFunctionPointPoint2Oct, EcReasonIncompatibleObjects);
        return 0;
    }
    if (form == PointHybrid) throw std::invalid_argument("EC_POINT_point2oct: the hybrid form is not supported");
    if (form != PointCompressed && form != PointUncompressed) {
        PushError(ErrLibEc, EcFunctionPointPoint2Oct, EcReasonInvalidForm);
        return 0;
    }
    const bool zero = mbedtls_ecp_is_zero(const_cast<mbedtls_ecp_point*>(&p->point)) != 0;
    const std::size_t size = zero ? 1 : 1 + FieldBytes(curve) * (form == PointCompressed ? 1 : 2);
    if (!buf) return size;
    if (len < size) {
        PushError(ErrLibEc, EcFunctionPointPoint2Oct, EcReasonBufferTooSmall);
        return 0;
    }
    std::size_t written = 0;
    Check(mbedtls_ecp_point_write_binary(&curve.group, &p->point, form == PointCompressed ? MBEDTLS_ECP_PF_COMPRESSED : MBEDTLS_ECP_PF_UNCOMPRESSED, &written, buf, len), __func__);
    return written;
}

int APS5_VABI EC_POINT_oct2point_nid_postfix(const EcGroup* group, EcPoint* p, const unsigned char* buf, std::size_t len, void*) {
    const auto& curve = RequireGroup(group, __func__);
    if (!p) throw std::invalid_argument("EC_POINT_oct2point: null point");
    if (!buf && len) throw std::invalid_argument("EC_POINT_oct2point: null buffer");
    if (!SameCurve(curve, *p)) {
        PushError(ErrLibEc, EcFunctionPointOct2Point, EcReasonIncompatibleObjects);
        return 0;
    }
    if (len && (buf[0] == 0x06 || buf[0] == 0x07)) throw std::invalid_argument("EC_POINT_oct2point: the hybrid form is not supported");
    mbedtls_ecp_point point;
    mbedtls_ecp_point_init(&point);
    int status = len ? mbedtls_ecp_point_read_binary(&curve.group, &point, buf, len) : MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
    if (status != 0) {
        mbedtls_ecp_point_free(&point);
        if (status == MBEDTLS_ERR_MPI_ALLOC_FAILED) throw std::bad_alloc();
        PushError(ErrLibEc, EcFunctionPointOct2Point, EcReasonInvalidEncoding);
        return 0;
    }
    if (!mbedtls_ecp_is_zero(&point) && mbedtls_ecp_check_pubkey(&curve.group, &point) != 0) {
        mbedtls_ecp_point_free(&point);
        PushError(ErrLibEc, EcFunctionPointOct2Point, EcReasonPointIsNotOnCurve);
        return 0;
    }
    status = mbedtls_ecp_copy(&p->point, &point);
    mbedtls_ecp_point_free(&point);
    Check(status, __func__);
    return 1;
}

EcKey* APS5_VABI EC_KEY_new_nid_postfix() {
    auto* key = NewEcKey();
    if (!key) PushError(ErrLibEc, EcFunctionKeyNew, ErrReasonMallocFailure);
    return key;
}

EcKey* APS5_VABI EC_KEY_new_by_curve_name_nid_postfix(int nid) {
    auto* group = NewGroupByNid(nid, __func__);
    if (!group) return nullptr;
    auto* key = NewEcKey();
    if (!key) {
        delete group;
        return nullptr;
    }
    key->group = group;
    return key;
}

void APS5_VABI EC_KEY_free_nid_postfix(EcKey* key) {
    FreeEcKey(key);
}

const EcGroup* APS5_VABI EC_KEY_get0_group_nid_postfix(const EcKey* key) {
    return RequireKey(key, __func__).group;
}

int APS5_VABI EC_KEY_set_group_nid_postfix(EcKey* key, const EcGroup* group) {
    auto& target = RequireKey(key, __func__);
    auto* copy = CopyGroup(RequireGroup(group, __func__));
    delete target.group;
    target.group = copy;
    return 1;
}

const Bignum* APS5_VABI EC_KEY_get0_private_key_nid_postfix(const EcKey* key) {
    return RequireKey(key, __func__).privateKey;
}

int APS5_VABI EC_KEY_set_private_key_nid_postfix(EcKey* key, const Bignum* prv) {
    auto& target = RequireKey(key, __func__);
    if (!prv) throw std::invalid_argument("EC_KEY_set_private_key: null BIGNUM");
    auto* copy = NewBignum();
    if (!copy) return 0;
    const auto bytes = BignumBytes(*prv);
    SetBignumBytes(*copy, bytes.data(), bytes.size());
    copy->neg = bytes.empty() ? 0 : prv->neg;
    ClearFreeBignum(target.privateKey);
    target.privateKey = copy;
    return 1;
}

const EcPoint* APS5_VABI EC_KEY_get0_public_key_nid_postfix(const EcKey* key) {
    return RequireKey(key, __func__).publicKey;
}

int APS5_VABI EC_KEY_set_public_key_nid_postfix(EcKey* key, const EcPoint* pub) {
    auto& target = RequireKey(key, __func__);
    if (!pub) throw std::invalid_argument("EC_KEY_set_public_key: null point");
    const auto& group = KeyGroup(target, __func__);
    if (!SameCurve(group, *pub)) {
        PushError(ErrLibEc, EcFunctionPointCopy, EcReasonIncompatibleObjects);
        return 0;
    }
    auto* copy = NewPoint(pub->curve);
    const int status = mbedtls_ecp_copy(&copy->point, &pub->point);
    if (status != 0) {
        delete copy;
        Check(status, __func__);
    }
    delete target.publicKey;
    target.publicKey = copy;
    return 1;
}

int APS5_VABI EC_KEY_generate_key_nid_postfix(EcKey* key) {
    auto& target = RequireKey(key, __func__);
    auto& group = KeyGroup(target, __func__);
    Mpi d;
    mbedtls_ecp_point q;
    mbedtls_ecp_point_init(&q);
    const int status = mbedtls_ecp_gen_keypair(&group.group, &d.value, &q, RandomBytes, nullptr);
    if (status != 0) {
        mbedtls_ecp_point_free(&q);
        Check(status, __func__);
    }
    if (!target.publicKey) target.publicKey = NewPoint(group.group.id);
    target.publicKey->curve = group.group.id;
    const int copied = mbedtls_ecp_copy(&target.publicKey->point, &q);
    mbedtls_ecp_point_free(&q);
    Check(copied, __func__);
    if (target.privateKey) WriteMpi(*target.privateKey, d.value, __func__);
    else target.privateKey = MpiBignum(d.value, __func__);
    target.privateKey->neg = 0;
    return 1;
}

int APS5_VABI EC_KEY_check_key_nid_postfix(const EcKey* key) {
    const auto& target = RequireKey(key, __func__);
    if (!target.group || !target.publicKey) {
        PushError(ErrLibEc, EcFunctionKeyCheckKey, ErrReasonPassedNullParameter);
        return 0;
    }
    const auto& group = *target.group;
    if (!SameCurve(group, *target.publicKey)) {
        PushError(ErrLibEc, EcFunctionKeyCheckKey, EcReasonIncompatibleObjects);
        return 0;
    }
    if (mbedtls_ecp_is_zero(const_cast<mbedtls_ecp_point*>(&target.publicKey->point))) {
        PushError(ErrLibEc, EcFunctionKeyCheckKey, EcReasonPointAtInfinity);
        return 0;
    }
    if (mbedtls_ecp_check_pubkey(&group.group, &target.publicKey->point) != 0) {
        PushError(ErrLibEc, EcFunctionKeyCheckKey, EcReasonPointIsNotOnCurve);
        return 0;
    }
    if (!target.privateKey) return 1;
    Mpi d;
    const auto bytes = BignumBytes(*target.privateKey);
    Check(mbedtls_mpi_read_binary(&d.value, bytes.data(), bytes.size()), __func__);
    if (target.privateKey->neg || mbedtls_mpi_cmp_mpi(&d.value, &group.group.N) >= 0) {
        PushError(ErrLibEc, EcFunctionKeyCheckKey, EcReasonWrongOrder);
        return 0;
    }
    mbedtls_ecp_point expected;
    mbedtls_ecp_point_init(&expected);
    int status = mbedtls_mpi_cmp_int(&d.value, 0) == 0 ? MBEDTLS_ERR_ECP_INVALID_KEY
                                                     : mbedtls_ecp_mul(const_cast<mbedtls_ecp_group*>(&group.group), &expected, &d.value, &group.group.G, RandomBytes, nullptr);
    if (status == 0) status = mbedtls_ecp_point_cmp(&expected, &target.publicKey->point);
    mbedtls_ecp_point_free(&expected);
    if (status == MBEDTLS_ERR_MPI_ALLOC_FAILED) throw std::bad_alloc();
    if (status != 0) {
        PushError(ErrLibEc, EcFunctionKeyCheckKey, EcReasonInvalidPrivateKey);
        return 0;
    }
    return 1;
}

int APS5_VABI ECDSA_size_nid_postfix(const EcKey* eckey) {
    return EcdsaSize(RequireKey(eckey, __func__));
}

int APS5_VABI ECDSA_sign_nid_postfix(int, const unsigned char* dgst, int dgstlen, unsigned char* sig, unsigned int* siglen, EcKey* eckey) {
    return EcdsaSign(dgst, dgstlen, sig, siglen, RequireKey(eckey, __func__), __func__);
}

int APS5_VABI ECDSA_verify_nid_postfix(int, const unsigned char* dgst, int dgstlen, const unsigned char* sig, int siglen, EcKey* eckey) {
    return EcdsaVerify(dgst, dgstlen, sig, siglen, RequireKey(eckey, __func__), __func__);
}

int APS5_VABI ECDH_compute_key_nid_postfix(void* out, std::size_t outlen, const EcPoint* pub_key, EcKey* ecdh,
    void* (APS5_VABI* KDF)(const void* in, std::size_t inlen, void* out, std::size_t* outlen)) {
    auto& key = RequireKey(ecdh, __func__);
    if (!out) throw std::invalid_argument("ECDH_compute_key: null output");
    if (!pub_key) throw std::invalid_argument("ECDH_compute_key: null public key");
    if (!key.privateKey) {
        PushError(ErrLibEcdh, EcdhFunctionComputeKey, EcdhReasonNoPrivateValue);
        return -1;
    }
    auto& group = KeyGroup(key, __func__);
    const std::size_t size = FieldBytes(group);
    if (!KDF && outlen < size) throw std::invalid_argument("ECDH_compute_key: an output shorter than the field size is handled differently between LibreSSL versions");
    if (!SameCurve(group, *pub_key)) {
        PushError(ErrLibEcdh, EcdhFunctionComputeKey, EcdhReasonPointArithmeticFailure);
        return -1;
    }
    Mpi d, z;
    ScalarMpi(*key.privateKey, group, d.value, __func__);
    const int status = mbedtls_ecdh_compute_shared(&group.group, &z.value, &pub_key->point, &d.value, RandomBytes, nullptr);
    if (status == MBEDTLS_ERR_MPI_ALLOC_FAILED || status == MBEDTLS_ERR_ECP_ALLOC_FAILED) throw std::bad_alloc();
    if (status != 0) {
        PushError(ErrLibEcdh, EcdhFunctionComputeKey, EcdhReasonPointArithmeticFailure);
        return -1;
    }
    std::vector<unsigned char> secret(size);
    Check(mbedtls_mpi_write_binary(&z.value, secret.data(), secret.size()), __func__);
    if (KDF) {
        std::size_t length = outlen;
        if (!KDF(secret.data(), secret.size(), out, &length)) {
            PushError(ErrLibEcdh, EcdhFunctionComputeKey, EcdhReasonKdfFailed);
            return -1;
        }
        return static_cast<int>(length);
    }
    std::memcpy(out, secret.data(), secret.size());
    return static_cast<int>(secret.size());
}

int LibreSslEcKeyUpRef_nid_no_patch(EcKey* key) {
    EcKeyUpRef(key);
    return 1;
}

EcKey* LibreSslD2iEcParameters_nid_no_patch(EcKey** key, const unsigned char** in, std::int64_t len) {
    constexpr const char* function = "d2i_ECParameters";
    if (!in || !*in) throw std::invalid_argument("d2i_ECParameters: null input");
    if (len <= 0) return nullptr;
    auto* cursor = const_cast<unsigned char*>(*in);
    const auto* end = cursor + len;
    if (*cursor != MBEDTLS_ASN1_OID) {
        if (*cursor == (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) throw std::invalid_argument("d2i_ECParameters: explicit curve parameters are not supported");
        PushError(ErrLibEc, EcFunctionD2iEcParameters, EcReasonD2iEcpkparametersFailure);
        return nullptr;
    }
    mbedtls_asn1_buf oid{};
    std::size_t length = 0;
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_OID) != 0) {
        PushError(ErrLibEc, EcFunctionD2iEcParameters, EcReasonD2iEcpkparametersFailure);
        return nullptr;
    }
    oid.tag = MBEDTLS_ASN1_OID;
    oid.len = length;
    oid.p = cursor;
    mbedtls_ecp_group_id id = MBEDTLS_ECP_DP_NONE;
    if (mbedtls_oid_get_ec_grp(&oid, &id) != 0 || !CurveNid(id)) throw std::invalid_argument("d2i_ECParameters: the named curve is not supported");
    auto* group = NewGroupByNid(CurveNid(id), function);
    if (!group) return nullptr;
    EcKey* result = key && *key ? *key : NewEcKey();
    if (!result) {
        delete group;
        return nullptr;
    }
    delete result->group;
    result->group = group;
    *in = cursor + length;
    if (key) *key = result;
    return result;
}

EcKey* LibreSslO2iEcPublicKey_nid_no_patch(EcKey** key, const unsigned char** in, std::int64_t len) {
    if (!key || !*key || !(*key)->group) throw std::invalid_argument("o2i_ECPublicKey: the EC_KEY and its group must be set");
    if (!in || !*in) throw std::invalid_argument("o2i_ECPublicKey: null input");
    if (len < 0) throw std::invalid_argument("o2i_ECPublicKey: negative length");
    auto& target = **key;
    auto* point = target.publicKey ? target.publicKey : NewPoint(target.group->group.id);
    point->curve = target.group->group.id;
    if (!EC_POINT_oct2point_nid_postfix(target.group, point, *in, static_cast<std::size_t>(len), nullptr)) {
        if (point != target.publicKey) delete point;
        PushError(ErrLibEc, EcFunctionO2iEcPublicKey, EcReasonAsn1Error);
        return nullptr;
    }
    target.publicKey = point;
    *in += len;
    return &target;
}

}
