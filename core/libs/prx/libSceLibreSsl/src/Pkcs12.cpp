#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/include/Keys.hpp"
#include "prx/libSceLibreSsl/include/X509Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <mbedtls/asn1.h>
#include <mbedtls/md.h>
#include <mbedtls/oid.h>
#include <mbedtls/pkcs12.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/platform_util.h>

#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

struct FileStream;

extern "C" {
LibreSsl::Bio* APS5_VABI BIO_new_fp_nid_postfix(FileStream* stream, int close_flag);
int APS5_VABI BIO_free_nid_postfix(LibreSsl::Bio* a);
}

namespace LibreSsl {

struct Pkcs12 {
    Asn1String* version;
    void* mac;
    void* authsafes;
};

static_assert(sizeof(Pkcs12) == 24);

namespace {

constexpr int Pkcs12FunctionParse = 118;
constexpr int Pkcs12FunctionVerifyMac = 126;
constexpr int Pkcs12ReasonMacAbsent = 108;
constexpr int Pkcs12ReasonMacVerifyFailure = 113;
constexpr int Pkcs12ReasonParseError = 114;
constexpr int Pkcs12ReasonPbeCryptError = 117;
constexpr std::size_t MaxPasswordLength = 31;

constexpr unsigned char OidData[] = {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x01};
constexpr unsigned char OidEncryptedData[] = {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x06};
constexpr unsigned char OidBagPrefix[] = {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x0c, 0x0a, 0x01};
constexpr unsigned char OidX509Certificate[] = {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x16, 0x01};
constexpr unsigned char OidPbes2[] = {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x05, 0x0d};
constexpr unsigned char OidPkcs12PbePrefix[] = {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x0c, 0x01};

struct Pkcs12Object {
    Pkcs12 info{};
    Arena arena;
    std::vector<unsigned char> der;
};

std::mutex& Mutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<const Pkcs12*, std::unique_ptr<Pkcs12Object>>& Objects() {
    static std::unordered_map<const Pkcs12*, std::unique_ptr<Pkcs12Object>> objects;
    return objects;
}

template <std::size_t N>
bool Is(const mbedtls_asn1_buf& oid, const unsigned char (&expected)[N]) {
    return oid.len == N && std::memcmp(oid.p, expected, N) == 0;
}

template <std::size_t N>
bool Starts(const mbedtls_asn1_buf& oid, const unsigned char (&prefix)[N]) {
    return oid.len == N + 1 && std::memcmp(oid.p, prefix, N) == 0;
}

struct ParseError {};

void Need(bool condition) {
    if (!condition) throw ParseError{};
}

std::size_t Tag(unsigned char*& cursor, const unsigned char* end, int tag) {
    std::size_t length = 0;
    Need(mbedtls_asn1_get_tag(&cursor, end, &length, tag) == 0);
    return length;
}

const unsigned char* Enter(unsigned char*& cursor, const unsigned char* end, int tag) {
    const std::size_t length = Tag(cursor, end, tag);
    return cursor + length;
}

struct Password {
    const char* text;
    std::size_t length;
};

std::vector<unsigned char> BmpPassword(const Password& password) {
    std::vector<unsigned char> out;
    if (!password.text) return out;
    for (std::size_t index = 0; index < password.length; ++index) {
        out.push_back(0);
        out.push_back(static_cast<unsigned char>(password.text[index]));
    }
    out.push_back(0);
    out.push_back(0);
    return out;
}

struct Pfx {
    unsigned char* authSafe = nullptr;
    std::size_t authSafeLength = 0;
    bool hasMac = false;
    mbedtls_md_type_t macMd = MBEDTLS_MD_NONE;
    const unsigned char* macDigest = nullptr;
    std::size_t macDigestLength = 0;
    const unsigned char* salt = nullptr;
    std::size_t saltLength = 0;
    int iterations = 1;
};

Pfx ParsePfx(std::vector<unsigned char>& der) {
    Pfx pfx;
    auto* cursor = der.data();
    const auto* end = der.data() + der.size();
    const auto* stop = Enter(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    int version = 0;
    Need(mbedtls_asn1_get_int(&cursor, stop, &version) == 0);
    const auto* contentEnd = Enter(cursor, stop, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    mbedtls_asn1_buf oid{};
    oid.len = Tag(cursor, contentEnd, MBEDTLS_ASN1_OID);
    oid.p = cursor;
    cursor += oid.len;
    if (!Is(oid, OidData)) throw std::invalid_argument("PKCS12_parse: authSafe content types other than data (public-key integrity) are not supported");
    Tag(cursor, contentEnd, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0);
    pfx.authSafeLength = Tag(cursor, contentEnd, MBEDTLS_ASN1_OCTET_STRING);
    pfx.authSafe = cursor;
    cursor += pfx.authSafeLength;
    Need(cursor == contentEnd);
    if (cursor < stop) {
        const auto* macEnd = Enter(cursor, stop, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
        const auto* digestInfoEnd = Enter(cursor, macEnd, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
        mbedtls_asn1_buf algorithm{}, params{};
        Need(mbedtls_asn1_get_alg(&cursor, digestInfoEnd, &algorithm, &params) == 0);
        if (mbedtls_oid_get_md_alg(&algorithm, &pfx.macMd) != 0) throw std::invalid_argument("PKCS12_parse: the MAC digest is not supported");
        pfx.macDigestLength = Tag(cursor, digestInfoEnd, MBEDTLS_ASN1_OCTET_STRING);
        pfx.macDigest = cursor;
        cursor += pfx.macDigestLength;
        Need(cursor == digestInfoEnd);
        pfx.saltLength = Tag(cursor, macEnd, MBEDTLS_ASN1_OCTET_STRING);
        pfx.salt = cursor;
        cursor += pfx.saltLength;
        if (cursor < macEnd) Need(mbedtls_asn1_get_int(&cursor, macEnd, &pfx.iterations) == 0 && pfx.iterations > 0);
        Need(cursor == macEnd);
        pfx.hasMac = true;
    }
    Need(cursor == stop);
    return pfx;
}

bool MacMatches(const Pfx& pfx, const Password& password) {
    const auto bmp = BmpPassword(password);
    const auto* info = mbedtls_md_info_from_type(pfx.macMd);
    const std::size_t size = mbedtls_md_get_size(info);
    std::vector<unsigned char> key(size), mac(size);
    if (mbedtls_pkcs12_derivation(key.data(), key.size(), bmp.empty() ? nullptr : bmp.data(), bmp.size(), pfx.salt, pfx.saltLength, pfx.macMd,
            MBEDTLS_PKCS12_DERIVE_MAC_KEY, pfx.iterations) != 0)
        throw std::invalid_argument("PKCS12_parse: the MAC parameters are not supported");
    if (mbedtls_md_hmac(info, key.data(), key.size(), pfx.authSafe, pfx.authSafeLength, mac.data()) != 0) throw std::runtime_error("PKCS12_parse: Mbed TLS failed");
    return pfx.macDigestLength == size && std::memcmp(mac.data(), pfx.macDigest, size) == 0;
}

std::vector<unsigned char> Decrypt(const mbedtls_asn1_buf& algorithm, mbedtls_asn1_buf& params, const unsigned char* data, std::size_t length, const Password& password) {
    std::vector<unsigned char> out(length + 16);
    std::size_t written = 0;
    int status = 0;
    const auto* pwd = reinterpret_cast<const unsigned char*>(password.text ? password.text : "");
    if (Is(algorithm, OidPbes2)) {
        status = mbedtls_pkcs5_pbes2_ext(&params, MBEDTLS_PKCS5_DECRYPT, pwd, password.length, data, length, out.data(), out.size(), &written);
    } else if (Starts(algorithm, OidPkcs12PbePrefix)) {
        mbedtls_md_type_t md = MBEDTLS_MD_NONE;
        mbedtls_cipher_type_t cipher = MBEDTLS_CIPHER_NONE;
        if (mbedtls_oid_get_pkcs12_pbe_alg(&algorithm, &md, &cipher) != 0) throw std::invalid_argument("PKCS12_parse: RC2 and RC4 encryption are not supported");
        if (!password.text) throw std::invalid_argument("PKCS12_parse: PKCS #12 encryption with an absent password is not supported");
        status = mbedtls_pkcs12_pbe_ext(&params, MBEDTLS_PKCS12_PBE_DECRYPT, cipher, md, pwd, password.length, data, length, out.data(), out.size(), &written);
    } else {
        throw std::invalid_argument("PKCS12_parse: the encryption algorithm is not supported");
    }
    if (status == MBEDTLS_ERR_PKCS5_FEATURE_UNAVAILABLE || status == MBEDTLS_ERR_PKCS12_FEATURE_UNAVAILABLE)
        throw std::invalid_argument("PKCS12_parse: the encryption parameters are not supported");
    Need(status == 0);
    out.resize(written);
    return out;
}

struct Contents {
    std::vector<EvpPkey*> keys;
    std::vector<X509Object*> certs;

    ~Contents() {
        for (auto* key : keys) FreePkey(key);
        for (auto* cert : certs) FreeX509(cert);
    }
};

void ParseSafeContents(unsigned char* cursor, const unsigned char* end, const Password& password, Contents& contents, int depth) {
    Need(depth < 4);
    const auto* stop = Enter(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    Need(stop == end);
    while (cursor < stop) {
        const auto* bagEnd = Enter(cursor, stop, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
        mbedtls_asn1_buf oid{};
        oid.len = Tag(cursor, bagEnd, MBEDTLS_ASN1_OID);
        oid.p = cursor;
        cursor += oid.len;
        const auto* valueEnd = Enter(cursor, bagEnd, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0);
        Need(Starts(oid, OidBagPrefix));
        const int bag = oid.p[oid.len - 1];
        if (bag == 1 || bag == 2) {
            std::vector<unsigned char> keyInfo(static_cast<const unsigned char*>(cursor), valueEnd);
            if (bag == 2) {
                auto* inner = cursor;
                const auto* innerEnd = Enter(inner, valueEnd, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
                mbedtls_asn1_buf algorithm{}, params{};
                Need(mbedtls_asn1_get_alg(&inner, innerEnd, &algorithm, &params) == 0);
                const std::size_t length = Tag(inner, innerEnd, MBEDTLS_ASN1_OCTET_STRING);
                keyInfo = Decrypt(algorithm, params, inner, length, password);
            }
            EvpPkey* pkey = nullptr;
            if (ParsePrivateKey(keyInfo.data(), keyInfo.size(), nullptr, pkey, "PKCS12_parse") != KeyParse::Ok) {
                mbedtls_platform_zeroize(keyInfo.data(), keyInfo.size());
                throw ParseError{};
            }
            mbedtls_platform_zeroize(keyInfo.data(), keyInfo.size());
            contents.keys.push_back(pkey);
        } else if (bag == 3) {
            auto* inner = cursor;
            const auto* innerEnd = Enter(inner, valueEnd, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
            mbedtls_asn1_buf certType{};
            certType.len = Tag(inner, innerEnd, MBEDTLS_ASN1_OID);
            certType.p = inner;
            inner += certType.len;
            Tag(inner, innerEnd, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0);
            const std::size_t length = Tag(inner, innerEnd, MBEDTLS_ASN1_OCTET_STRING);
            if (Is(certType, OidX509Certificate)) {
                Need(DerElementLength(inner, length) == length);
                auto* cert = X509FromDer(inner, length);
                Need(cert != nullptr);
                contents.certs.push_back(cert);
            }
        } else if (bag == 6) {
            ParseSafeContents(cursor, valueEnd, password, contents, depth + 1);
        }
        cursor = const_cast<unsigned char*>(bagEnd);
    }
}

void ParseAuthSafe(const Pfx& pfx, const Password& password, Contents& contents) {
    auto* cursor = pfx.authSafe;
    const auto* end = pfx.authSafe + pfx.authSafeLength;
    const auto* stop = Enter(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    Need(stop == end);
    while (cursor < stop) {
        const auto* infoEnd = Enter(cursor, stop, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
        mbedtls_asn1_buf oid{};
        oid.len = Tag(cursor, infoEnd, MBEDTLS_ASN1_OID);
        oid.p = cursor;
        cursor += oid.len;
        const auto* contentEnd = Enter(cursor, infoEnd, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0);
        if (Is(oid, OidData)) {
            const std::size_t length = Tag(cursor, contentEnd, MBEDTLS_ASN1_OCTET_STRING);
            ParseSafeContents(cursor, cursor + length, password, contents, 0);
        } else if (Is(oid, OidEncryptedData)) {
            const auto* dataEnd = Enter(cursor, contentEnd, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
            int version = 0;
            Need(mbedtls_asn1_get_int(&cursor, dataEnd, &version) == 0);
            const auto* encryptedEnd = Enter(cursor, dataEnd, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
            mbedtls_asn1_buf type{};
            type.len = Tag(cursor, encryptedEnd, MBEDTLS_ASN1_OID);
            type.p = cursor;
            cursor += type.len;
            Need(Is(type, OidData));
            mbedtls_asn1_buf algorithm{}, params{};
            Need(mbedtls_asn1_get_alg(&cursor, encryptedEnd, &algorithm, &params) == 0);
            const std::size_t length = Tag(cursor, encryptedEnd, MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0);
            auto plain = Decrypt(algorithm, params, cursor, length, password);
            ParseSafeContents(plain.data(), plain.data() + plain.size(), password, contents, 0);
        } else {
            throw std::invalid_argument("PKCS12_parse: enveloped (public-key privacy) safe contents are not supported");
        }
        cursor = const_cast<unsigned char*>(infoEnd);
    }
}

bool Matches(EvpPkey& key, X509Object& cert) {
    auto* pub = X509PublicKey(cert, "PKCS12_parse");
    if (pub->type != key.type) return false;
    return EncodeSubjectPublicKey(*pub, "PKCS12_parse") == EncodeSubjectPublicKey(key, "PKCS12_parse");
}

Pkcs12Object* FindObject(const Pkcs12* p12, const char* function) {
    if (!p12) throw std::invalid_argument(std::string(function) + ": null PKCS12");
    std::lock_guard lock(Mutex());
    const auto found = Objects().find(p12);
    if (found == Objects().end()) throw std::invalid_argument(std::string(function) + ": the PKCS12 was not created by d2i_PKCS12_bio");
    return found->second.get();
}

Pkcs12* ReadPkcs12(Bio* bp, Pkcs12** p12, const char* function) {
    if (p12 && *p12) throw std::invalid_argument(std::string(function) + ": reusing an existing PKCS12 is not supported");
    auto bytes = BioReadAll(bp, function);
    const std::size_t length = DerElementLength(bytes.data(), bytes.size());
    auto object = std::make_unique<Pkcs12Object>();
    bool valid = length && length <= bytes.size();
    if (valid) {
        object->der.assign(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(length));
        try {
            ParsePfx(object->der);
        } catch (const ParseError&) {
            valid = false;
        }
    }
    if (!valid) {
        BioPutBack(bp, bytes.data(), bytes.size());
        return nullptr;
    }
    BioPutBack(bp, bytes.data() + length, bytes.size() - length);
    unsigned char three = 3;
    object->info.version = NewInteger(object->arena, &three, 1);
    auto* info = &object->info;
    {
        std::lock_guard lock(Mutex());
        Objects().emplace(info, std::move(object));
    }
    if (p12) *p12 = info;
    return info;
}

}

}

using namespace LibreSsl;

extern "C" {

Pkcs12* APS5_VABI d2i_PKCS12_bio_nid_postfix(Bio* bp, Pkcs12** p12) {
    return ReadPkcs12(bp, p12, __func__);
}

Pkcs12* APS5_VABI d2i_PKCS12_fp_nid_postfix(FileStream* fp, Pkcs12** p12) {
    if (!fp) throw std::invalid_argument("d2i_PKCS12_fp: null stream");
    auto* bio = BIO_new_fp_nid_postfix(fp, 0);
    if (!bio) throw std::bad_alloc();
    Pkcs12* result = nullptr;
    try {
        result = ReadPkcs12(bio, p12, "d2i_PKCS12_fp");
    } catch (...) {
        BIO_free_nid_postfix(bio);
        throw;
    }
    BIO_free_nid_postfix(bio);
    return result;
}

void APS5_VABI PKCS12_free_nid_postfix(Pkcs12* a) {
    if (!a) return;
    FindObject(a, "PKCS12_free");
    std::unique_ptr<Pkcs12Object> owned;
    std::lock_guard lock(Mutex());
    const auto found = Objects().find(a);
    owned = std::move(found->second);
    Objects().erase(found);
}

void APS5_VABI PKCS12_PBE_add_nid_postfix() {
}

int APS5_VABI PKCS12_parse_nid_postfix(Pkcs12* p12, const char* pass, EvpPkey** pkey, X509Object** cert, Stack** ca) {
    auto* object = FindObject(p12, "PKCS12_parse");
    if (!pkey || !cert) throw std::invalid_argument("PKCS12_parse: null key or certificate output");
    if (ca && *ca && !IsHeapStack(*ca)) throw std::invalid_argument("PKCS12_parse: the CA stack was not created by this library");
    *pkey = nullptr;
    *cert = nullptr;
    auto der = object->der;
    Pfx pfx;
    try {
        pfx = ParsePfx(der);
    } catch (const ParseError&) {
        PushError(ErrLibPkcs12, Pkcs12FunctionParse, Pkcs12ReasonParseError);
        return 0;
    }
    if (!pfx.hasMac) {
        PushError(ErrLibPkcs12, Pkcs12FunctionVerifyMac, Pkcs12ReasonMacAbsent);
        PushError(ErrLibPkcs12, Pkcs12FunctionParse, Pkcs12ReasonMacVerifyFailure);
        return 0;
    }
    const std::size_t length = pass ? std::strlen(pass) : 0;
    if (length > MaxPasswordLength) throw std::invalid_argument("PKCS12_parse: passwords longer than 31 characters are not supported");
    Password password{pass, length};
    bool verified = false;
    if (!length) {
        for (const Password candidate : {Password{nullptr, 0}, Password{"", 0}}) {
            if (MacMatches(pfx, candidate)) {
                password = candidate;
                verified = true;
                break;
            }
        }
    } else {
        verified = MacMatches(pfx, password);
    }
    if (!verified) {
        PushError(ErrLibPkcs12, Pkcs12FunctionParse, Pkcs12ReasonMacVerifyFailure);
        return 0;
    }
    Contents contents;
    try {
        ParseAuthSafe(pfx, password, contents);
    } catch (const ParseError&) {
        PushError(ErrLibPkcs12, Pkcs12FunctionParse, Pkcs12ReasonPbeCryptError);
        return 0;
    }
    if (contents.keys.size() > 1) throw std::invalid_argument("PKCS12_parse: files with more than one private key are not supported");
    EvpPkey* key = contents.keys.empty() ? nullptr : contents.keys.front();
    X509Object* match = nullptr;
    std::vector<X509Object*> others;
    for (auto* candidate : contents.certs) {
        if (key && !match && Matches(*key, *candidate)) match = candidate;
        else others.push_back(candidate);
    }
    Stack* created = nullptr;
    if (ca && !*ca) created = NewHeapStack();
    if (ca) {
        Stack* target = *ca ? *ca : created;
        for (auto* other : others) {
            X509UpRef(other);
            HeapStackPush(target, other);
        }
        if (created) *ca = created;
    }
    if (key) PkeyUpRef(key);
    if (match) X509UpRef(match);
    *pkey = key;
    *cert = match;
    return 1;
}

}
