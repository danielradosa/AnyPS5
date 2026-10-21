#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/include/Keys.hpp"
#include "prx/libSceLibreSsl/include/Pem.hpp"
#include "prx/libSceLibreSsl/include/X509Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <mbedtls/asn1.h>
#include <mbedtls/base64.h>
#include <mbedtls/pk.h>
#include <mbedtls/platform_util.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct FileStream;

extern "C" {
LibreSsl::Bio* APS5_VABI BIO_new_fp_nid_postfix(FileStream* stream, int close_flag);
int APS5_VABI BIO_free_nid_postfix(LibreSsl::Bio* a);
int APS5_VABI BIO_write_nid_postfix(LibreSsl::Bio* b, const void* data, int len);
}

namespace LibreSsl {

struct Pkcs8 {
    Asn1String* version;
    Algor* pkeyalg;
    Asn1String* pkey;
    void* attributes;
};

static_assert(sizeof(Pkcs8) == 32);

namespace {

constexpr int PemBufferSize = 1024;

constexpr int PemFunctionReadBio = 109;
constexpr int PemFunctionReadBioPrivateKey = 123;
constexpr int PemReasonBadBase64Decode = 100;
constexpr int PemReasonBadDecrypt = 101;
constexpr int PemReasonBadPasswordRead = 104;
constexpr int PemReasonNoStartLine = 108;

constexpr int EvpFunctionPkeyGet1Rsa = 121;
constexpr int EvpFunctionPkcs82Pkey = 111;
constexpr int EvpReasonExpectingAnRsaKey = 127;
constexpr int EvpReasonDecodeError = 114;

using PasswordCallback = int(APS5_VABI*)(char* buf, int size, int rwflag, void* userdata);

bool IsPrivateKeyLabel(std::string_view label) {
    return label == "PRIVATE KEY" || label == "ENCRYPTED PRIVATE KEY" || label == "RSA PRIVATE KEY" || label == "EC PRIVATE KEY" || label == "DSA PRIVATE KEY";
}

bool LegacyEncrypted(const PemObject& object, const char* function) {
    if (object.headers.empty()) return false;
    if (object.headers.rfind("Proc-Type: 4,ENCRYPTED\n", 0) != 0) throw std::invalid_argument(std::string(function) + ": PEM headers other than legacy encryption are not supported");
    const auto info = object.headers.find("DEK-Info: ");
    if (info == std::string::npos) throw std::invalid_argument(std::string(function) + ": encrypted PEM object without DEK-Info");
    const auto name = object.headers.substr(info + 10, object.headers.find(',', info) - info - 10);
    if (name != "DES-CBC" && name != "DES-EDE3-CBC" && name != "AES-128-CBC" && name != "AES-192-CBC" && name != "AES-256-CBC")
        throw std::invalid_argument(std::string(function) + ": PEM encryption " + name + " is not supported");
    return true;
}

EvpPkey* ReadPrivateKey(Bio* bio, const PemPassword& password, const char* function) {
    PemObject object;
    if (!ReadPemFromBio(bio, object, IsPrivateKeyLabel, function)) return nullptr;
    if (object.label == "DSA PRIVATE KEY") throw std::invalid_argument(std::string(function) + ": DSA keys are not supported");
    std::string passphrase;
    std::vector<unsigned char> input = object.der;
    bool encrypted = false;
    if (object.label == "ENCRYPTED PRIVATE KEY") {
        if (!object.headers.empty()) throw std::invalid_argument(std::string(function) + ": PEM headers on a PKCS #8 key are not supported");
        encrypted = true;
    } else if (object.label == "PRIVATE KEY") {
        if (!object.headers.empty()) throw std::invalid_argument(std::string(function) + ": PEM headers on a PKCS #8 key are not supported");
    } else if (LegacyEncrypted(object, function)) {
        encrypted = true;
        input.assign(object.text.begin(), object.text.end());
        input.push_back(0);
    }
    if (encrypted && !PemPassphrase(password, passphrase, function)) {
        PushError(ErrLibPem, PemFunctionReadBioPrivateKey, PemReasonBadPasswordRead);
        return nullptr;
    }
    EvpPkey* pkey = nullptr;
    const auto status = ParsePrivateKey(input.data(), input.size(), encrypted ? &passphrase : nullptr, pkey, function);
    mbedtls_platform_zeroize(passphrase.data(), passphrase.size());
    if (status == KeyParse::WrongPassword) {
        PushError(ErrLibPem, PemFunctionReadBioPrivateKey, PemReasonBadDecrypt);
        return nullptr;
    }
    if (status == KeyParse::Malformed) {
        PushError(ErrLibPem, PemFunctionReadBioPrivateKey, ErrReasonAsn1Lib);
        return nullptr;
    }
    if ((object.label == "RSA PRIVATE KEY" && pkey->type != NidRsaEncryption) || (object.label == "EC PRIVATE KEY" && pkey->type != NidEcPublicKey)) {
        FreePkey(pkey);
        PushError(ErrLibPem, PemFunctionReadBioPrivateKey, ErrReasonAsn1Lib);
        return nullptr;
    }
    return pkey;
}

template <typename TCall>
auto WithFileBio(FileStream* fp, const char* function, TCall call) {
    if (!fp) throw std::invalid_argument(std::string(function) + ": null stream");
    auto* bio = BIO_new_fp_nid_postfix(fp, 0);
    if (!bio) throw std::bad_alloc();
    try {
        auto result = call(bio);
        BIO_free_nid_postfix(bio);
        return result;
    } catch (...) {
        BIO_free_nid_postfix(bio);
        throw;
    }
}

Rsa* RsaFromSpki(const unsigned char* data, std::size_t length, std::size_t& consumed, const char* function) {
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    auto* cursor = const_cast<unsigned char*>(data);
    const int status = mbedtls_pk_parse_subpubkey(&cursor, data + length, &pk);
    if (status != 0) {
        mbedtls_pk_free(&pk);
        if (status == MBEDTLS_ERR_PK_ALLOC_FAILED) throw std::bad_alloc();
        return nullptr;
    }
    consumed = static_cast<std::size_t>(cursor - data);
    if (mbedtls_pk_get_type(&pk) != MBEDTLS_PK_RSA) {
        mbedtls_pk_free(&pk);
        PushError(ErrLibEvp, EvpFunctionPkeyGet1Rsa, EvpReasonExpectingAnRsaKey);
        return nullptr;
    }
    auto* rsa = NewRsa();
    try {
        if (!rsa) throw std::bad_alloc();
        FillRsa(*rsa, *mbedtls_pk_rsa(pk), function);
    } catch (...) {
        FreeRsa(rsa);
        mbedtls_pk_free(&pk);
        throw;
    }
    mbedtls_pk_free(&pk);
    return rsa;
}

std::vector<unsigned char> ReadDerFromBio(Bio* bio, const char* function, bool& ok) {
    auto bytes = BioReadAll(bio, function);
    const std::size_t length = DerElementLength(bytes.data(), bytes.size());
    if (!length || length > bytes.size()) {
        BioPutBack(bio, bytes.data(), bytes.size());
        ok = false;
        return {};
    }
    BioPutBack(bio, bytes.data() + length, bytes.size() - length);
    bytes.resize(length);
    ok = true;
    return bytes;
}

struct Pkcs8Object {
    Pkcs8 info{};
    Arena arena;
    std::vector<unsigned char> der;
};

std::mutex& Pkcs8Mutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<const Pkcs8*, std::unique_ptr<Pkcs8Object>>& Pkcs8Objects() {
    static std::unordered_map<const Pkcs8*, std::unique_ptr<Pkcs8Object>> objects;
    return objects;
}

bool ParsePkcs8(Pkcs8Object& object) {
    auto& arena = object.arena;
    auto* cursor = object.der.data();
    const auto* end = cursor + object.der.size();
    std::size_t length = 0;
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0 || cursor + length != end) return false;
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_INTEGER) != 0 || !length) return false;
    object.info.version = NewInteger(arena, cursor, length);
    cursor += length;
    try {
        object.info.pkeyalg = ParseAlgor(arena, cursor, end);
    } catch (const std::runtime_error&) {
        return false;
    }
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_OCTET_STRING) != 0) return false;
    object.info.pkey = arena.String(4, cursor, length);
    cursor += length;
    while (cursor < end) {
        const int tag = *cursor;
        if (tag != (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0) && tag != (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 1)) return false;
        if (mbedtls_asn1_get_tag(&cursor, end, &length, tag) != 0) return false;
        cursor += length;
    }
    return true;
}

}

PemStatus NextPem(std::string_view text, std::size_t& offset, PemObject& out, const LabelFilter& wanted) {
    constexpr std::string_view begin = "-----BEGIN ";
    while (true) {
        auto start = text.find(begin, offset);
        if (start == std::string_view::npos) {
            offset = text.size();
            return PemStatus::NoStartLine;
        }
        if (start != 0 && text[start - 1] != '\n') {
            offset = start + 1;
            continue;
        }
        const auto labelEnd = text.find("-----", start + begin.size());
        const auto lineEnd = text.find('\n', start);
        if (labelEnd == std::string_view::npos || (lineEnd != std::string_view::npos && labelEnd > lineEnd)) {
            offset = lineEnd == std::string_view::npos ? text.size() : lineEnd + 1;
            continue;
        }
        const std::string label(text.substr(start + begin.size(), labelEnd - start - begin.size()));
        const std::string footer = "-----END " + label + "-----";
        const auto stop = text.find(footer, labelEnd);
        if (stop == std::string_view::npos || lineEnd == std::string_view::npos || lineEnd > stop)
            throw std::invalid_argument("PEM: object \"" + label + "\" without an end line");
        auto consumed = stop + footer.size();
        if (consumed < text.size() && text[consumed] == '\r') ++consumed;
        if (consumed < text.size() && text[consumed] == '\n') ++consumed;
        offset = consumed;
        if (!wanted(label)) continue;
        std::vector<std::string> lines;
        std::size_t cursor = lineEnd + 1;
        while (cursor < stop) {
            auto next = text.find('\n', cursor);
            if (next == std::string_view::npos || next > stop) next = stop;
            std::string line(text.substr(cursor, next - cursor));
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
            lines.push_back(line);
            cursor = next + 1;
        }
        std::size_t index = 0;
        std::string headers;
        if (!lines.empty() && lines[0].find(':') != std::string::npos) {
            while (index < lines.size() && !lines[index].empty()) headers += lines[index++] + "\n";
        }
        std::string base64;
        for (; index < lines.size(); ++index) {
            for (const char character : lines[index]) {
                if (character != ' ' && character != '\t') base64 += character;
            }
        }
        std::size_t size = 0;
        mbedtls_base64_decode(nullptr, 0, &size, reinterpret_cast<const unsigned char*>(base64.data()), base64.size());
        std::vector<unsigned char> der(size);
        if (mbedtls_base64_decode(der.data(), der.size(), &size, reinterpret_cast<const unsigned char*>(base64.data()), base64.size()) != 0)
            return PemStatus::BadBase64;
        der.resize(size);
        out.label = label;
        out.headers = headers;
        out.der = std::move(der);
        out.text = std::string(text.substr(start, consumed - start));
        return PemStatus::Ok;
    }
}

bool ReadPemFromBio(Bio* bio, PemObject& object, const LabelFilter& wanted, const char* function) {
    const auto bytes = BioReadAll(bio, function);
    const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    std::size_t offset = 0;
    PemStatus status = PemStatus::NoStartLine;
    try {
        status = NextPem(text, offset, object, wanted);
    } catch (...) {
        BioPutBack(bio, bytes.data(), bytes.size());
        throw;
    }
    BioPutBack(bio, bytes.data() + offset, bytes.size() - offset);
    if (status == PemStatus::NoStartLine) {
        PushError(ErrLibPem, PemFunctionReadBio, PemReasonNoStartLine);
        return false;
    }
    if (status == PemStatus::BadBase64) {
        PushError(ErrLibPem, PemFunctionReadBio, PemReasonBadBase64Decode);
        return false;
    }
    return true;
}

bool PemPassphrase(const PemPassword& password, std::string& out, const char* function) {
    if (password.callback) {
        char buffer[PemBufferSize] = {};
        const int length = reinterpret_cast<PasswordCallback>(password.callback)(buffer, PemBufferSize, 0, password.userdata);
        if (length <= 0) return false;
        out.assign(buffer, static_cast<std::size_t>(length > PemBufferSize ? PemBufferSize : length));
        mbedtls_platform_zeroize(buffer, sizeof(buffer));
        return true;
    }
    if (password.userdata) {
        out = static_cast<const char*>(password.userdata);
        return true;
    }
    throw std::invalid_argument(std::string(function) + ": the default passphrase prompt is not supported (pass a callback or a passphrase)");
}

KeyParse ParsePrivateKey(const unsigned char* data, std::size_t length, const std::string* password, EvpPkey*& out, const char* function) {
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    const auto* pwd = password ? reinterpret_cast<const unsigned char*>(password->data()) : nullptr;
    const int status = mbedtls_pk_parse_key(&pk, data, length, pwd, password ? password->size() : 0, RandomBytes, nullptr);
    if (status != 0) {
        mbedtls_pk_free(&pk);
        if (status == MBEDTLS_ERR_PK_ALLOC_FAILED) throw std::bad_alloc();
        if (status == MBEDTLS_ERR_PK_PASSWORD_MISMATCH) return KeyParse::WrongPassword;
        if (status == MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE || status == MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE)
            throw std::invalid_argument(std::string(function) + ": the key algorithm or curve is not supported");
        return KeyParse::Malformed;
    }
    try {
        out = PkeyFromMbed(pk, function);
    } catch (...) {
        mbedtls_pk_free(&pk);
        throw;
    }
    mbedtls_pk_free(&pk);
    return KeyParse::Ok;
}

}

using namespace LibreSsl;

extern "C" {

EvpPkey* APS5_VABI PEM_read_bio_PrivateKey_nid_postfix(Bio* bp, EvpPkey** x, void* cb, void* u) {
    if (x && *x) throw std::invalid_argument("PEM_read_bio_PrivateKey: reusing an existing EVP_PKEY is not supported");
    auto* pkey = ReadPrivateKey(bp, PemPassword{cb, u}, __func__);
    if (pkey && x) *x = pkey;
    return pkey;
}

Rsa* APS5_VABI PEM_read_RSAPrivateKey_nid_postfix(FileStream* fp, Rsa** x, void* cb, void* u) {
    if (x && *x) throw std::invalid_argument("PEM_read_RSAPrivateKey: reusing an existing RSA is not supported");
    auto* pkey = WithFileBio(fp, __func__, [&](Bio* bio) { return ReadPrivateKey(bio, PemPassword{cb, u}, "PEM_read_RSAPrivateKey"); });
    if (!pkey) return nullptr;
    if (pkey->type != NidRsaEncryption) {
        FreePkey(pkey);
        PushError(ErrLibEvp, EvpFunctionPkeyGet1Rsa, EvpReasonExpectingAnRsaKey);
        return nullptr;
    }
    auto* rsa = static_cast<Rsa*>(pkey->pkey);
    RsaUpRef(rsa);
    FreePkey(pkey);
    if (x) *x = rsa;
    return rsa;
}

Rsa* APS5_VABI d2i_RSA_PUBKEY_nid_postfix(Rsa** a, const unsigned char** pp, std::int64_t length) {
    if (a && *a) throw std::invalid_argument("d2i_RSA_PUBKEY: reusing an existing RSA is not supported");
    if (!pp || !*pp) throw std::invalid_argument("d2i_RSA_PUBKEY: null input");
    if (length <= 0) return nullptr;
    std::size_t consumed = 0;
    auto* rsa = RsaFromSpki(*pp, static_cast<std::size_t>(length), consumed, __func__);
    if (!rsa) return nullptr;
    *pp += consumed;
    if (a) *a = rsa;
    return rsa;
}

EvpPkey* LibreSslD2iPrivateKeyBio_nid_no_patch(Bio* bp, EvpPkey** a) {
    constexpr const char* function = "d2i_PrivateKey_bio";
    if (a && *a) throw std::invalid_argument("d2i_PrivateKey_bio: reusing an existing EVP_PKEY is not supported");
    bool ok = false;
    const auto der = ReadDerFromBio(bp, function, ok);
    if (!ok) return nullptr;
    EvpPkey* pkey = nullptr;
    if (ParsePrivateKey(der.data(), der.size(), nullptr, pkey, function) != KeyParse::Ok) return nullptr;
    if (a) *a = pkey;
    return pkey;
}

Rsa* LibreSslD2iRsaPublicKeyBio_nid_no_patch(Bio* bp, Rsa** a) {
    constexpr const char* function = "d2i_RSAPublicKey_bio";
    if (a && *a) throw std::invalid_argument("d2i_RSAPublicKey_bio: reusing an existing RSA is not supported");
    auto bytes = BioReadAll(bp, function);
    auto* cursor = bytes.data();
    const auto* end = bytes.data() + bytes.size();
    std::size_t length = 0;
    Mpi n, e;
    const auto restore = [&]() -> Rsa* {
        BioPutBack(bp, bytes.data(), bytes.size());
        return nullptr;
    };
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) return restore();
    const auto* stop = cursor + length;
    for (auto* value : {&n.value, &e.value}) {
        auto* integer = cursor;
        std::size_t size = 0;
        if (mbedtls_asn1_get_tag(&integer, stop, &size, MBEDTLS_ASN1_INTEGER) != 0 || !size) return restore();
        if (integer[0] & 0x80) throw std::invalid_argument("d2i_RSAPublicKey_bio: negative key components are not supported");
        if (mbedtls_asn1_get_mpi(&cursor, stop, value) != 0) return restore();
    }
    if (cursor != stop) return restore();
    BioPutBack(bp, stop, static_cast<std::size_t>(end - stop));
    auto* rsa = NewRsa();
    if (!rsa) throw std::bad_alloc();
    rsa->n = MpiBignum(n.value, function);
    rsa->e = MpiBignum(e.value, function);
    if (a) *a = rsa;
    return rsa;
}

Pkcs8* LibreSslD2iPkcs8Bio_nid_no_patch(Bio* bp, Pkcs8** p8) {
    if (p8 && *p8) throw std::invalid_argument("d2i_PKCS8_PRIV_KEY_INFO_bio: reusing an existing PKCS8_PRIV_KEY_INFO is not supported");
    auto bytes = BioReadAll(bp, "d2i_PKCS8_PRIV_KEY_INFO_bio");
    const std::size_t length = DerElementLength(bytes.data(), bytes.size());
    auto object = std::make_unique<Pkcs8Object>();
    if (length && length <= bytes.size()) object->der.assign(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(length));
    if (!length || length > bytes.size() || !ParsePkcs8(*object)) {
        BioPutBack(bp, bytes.data(), bytes.size());
        return nullptr;
    }
    BioPutBack(bp, bytes.data() + length, bytes.size() - length);
    auto* info = &object->info;
    {
        std::lock_guard lock(Pkcs8Mutex());
        Pkcs8Objects().emplace(info, std::move(object));
    }
    if (p8) *p8 = info;
    return info;
}

void LibreSslPkcs8Free_nid_no_patch(Pkcs8* a) {
    if (!a) return;
    std::unique_ptr<Pkcs8Object> owned;
    {
        std::lock_guard lock(Pkcs8Mutex());
        const auto found = Pkcs8Objects().find(a);
        if (found == Pkcs8Objects().end()) throw std::invalid_argument("PKCS8_PRIV_KEY_INFO_free: the object was not created by d2i_PKCS8_PRIV_KEY_INFO_bio");
        owned = std::move(found->second);
        Pkcs8Objects().erase(found);
    }
    mbedtls_platform_zeroize(owned->der.data(), owned->der.size());
}

EvpPkey* LibreSslPkcs82Pkey_nid_no_patch(const Pkcs8* p8) {
    if (!p8) throw std::invalid_argument("EVP_PKCS82PKEY: null PKCS8_PRIV_KEY_INFO");
    const Pkcs8Object* object = nullptr;
    {
        std::lock_guard lock(Pkcs8Mutex());
        const auto found = Pkcs8Objects().find(p8);
        if (found == Pkcs8Objects().end()) throw std::invalid_argument("EVP_PKCS82PKEY: the object was not created by d2i_PKCS8_PRIV_KEY_INFO_bio");
        object = found->second.get();
    }
    const int nid = p8->pkeyalg->algorithm->nid;
    if (nid != NidRsaEncryption && nid != NidEcPublicKey) throw std::invalid_argument("EVP_PKCS82PKEY: only RSA and EC keys are supported");
    EvpPkey* pkey = nullptr;
    if (ParsePrivateKey(object->der.data(), object->der.size(), nullptr, pkey, "EVP_PKCS82PKEY") != KeyParse::Ok) {
        PushError(ErrLibEvp, EvpFunctionPkcs82Pkey, EvpReasonDecodeError);
        return nullptr;
    }
    return pkey;
}

}
