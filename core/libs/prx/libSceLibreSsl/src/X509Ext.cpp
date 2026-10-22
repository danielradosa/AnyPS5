#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/include/Keys.hpp"
#include "prx/libSceLibreSsl/include/X509Internal.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestHeap.hpp"

#include <mbedtls/asn1.h>

#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace LibreSsl {

namespace {

constexpr int TypeOctetString = 4;
constexpr int TypeIa5String = 22;

constexpr int GenOtherName = 0;
constexpr int GenEmail = 1;
constexpr int GenDns = 2;
constexpr int GenDirName = 4;
constexpr int GenUri = 6;
constexpr int GenIpAddress = 7;
constexpr int GenRid = 8;

constexpr int NidSubjectAltName = 85;
constexpr int NidIssuerAltName = 86;

std::mutex& Mutex() {
    static std::mutex mutex;
    return mutex;
}

struct GeneralNameObject {
    GeneralName name{};
    Arena arena;
};

std::unordered_map<const GeneralName*, std::unique_ptr<GeneralNameObject>>& GeneralNames() {
    static std::unordered_map<const GeneralName*, std::unique_ptr<GeneralNameObject>> names;
    return names;
}

X509Object& RequireX509(X509Object* x, const char* function) {
    if (!x) throw std::invalid_argument(std::string(function) + ": null certificate");
    return *x;
}

const Extension& RequireExtension(const Extension* ex, const char* function) {
    if (!ex) throw std::invalid_argument(std::string(function) + ": null X509_EXTENSION");
    return *ex;
}

bool Element(unsigned char*& cursor, const unsigned char* end, int tag, std::size_t& length) {
    return mbedtls_asn1_get_tag(&cursor, end, &length, tag) == 0;
}

void Malformed(const char* what) {
    throw std::runtime_error(std::string("X509: malformed ") + what);
}

Stack* ParseExtensions(Arena& arena, unsigned char*& cursor, const unsigned char* end) {
    std::size_t sequence = 0;
    if (!Element(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, sequence)) Malformed("extensions");
    const auto* stop = cursor + sequence;
    std::vector<char*> items;
    while (cursor < stop) {
        std::size_t length = 0;
        if (!Element(cursor, stop, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, length)) Malformed("extension");
        const auto* extensionEnd = cursor + length;
        std::size_t oidLength = 0;
        if (!Element(cursor, extensionEnd, MBEDTLS_ASN1_OID, oidLength)) Malformed("extension");
        auto* object = OidObject(arena, cursor, oidLength);
        cursor += oidLength;
        int critical = 0;
        if (cursor < extensionEnd && *cursor == MBEDTLS_ASN1_BOOLEAN) {
            std::size_t boolLength = 0;
            if (!Element(cursor, extensionEnd, MBEDTLS_ASN1_BOOLEAN, boolLength) || boolLength != 1) Malformed("extension");
            critical = *cursor++;
        }
        std::size_t valueLength = 0;
        if (!Element(cursor, extensionEnd, MBEDTLS_ASN1_OCTET_STRING, valueLength) || cursor + valueLength != extensionEnd) Malformed("extension");
        auto* value = arena.String(TypeOctetString, cursor, valueLength);
        cursor += valueLength;
        items.push_back(reinterpret_cast<char*>(arena.Make(Extension{object, critical, value})));
    }
    return arena.NewStack(items);
}

GeneralNameObject* ParseGeneralName(unsigned char*& cursor, const unsigned char* end, const char* function) {
    if (cursor >= end) Malformed("GeneralName");
    const int tag = *cursor++;
    std::size_t length = 0;
    if (mbedtls_asn1_get_len(&cursor, end, &length) != 0) Malformed("GeneralName");
    auto object = std::make_unique<GeneralNameObject>();
    auto& arena = object->arena;
    auto& name = object->name;
    switch (tag) {
    case MBEDTLS_ASN1_CONTEXT_SPECIFIC | GenEmail:
    case MBEDTLS_ASN1_CONTEXT_SPECIFIC | GenDns:
    case MBEDTLS_ASN1_CONTEXT_SPECIFIC | GenUri:
        name.type = tag & 0x1f;
        name.d = arena.String(TypeIa5String, cursor, length);
        break;
    case MBEDTLS_ASN1_CONTEXT_SPECIFIC | GenIpAddress:
        name.type = GenIpAddress;
        name.d = arena.String(TypeOctetString, cursor, length);
        break;
    case MBEDTLS_ASN1_CONTEXT_SPECIFIC | GenRid:
        name.type = GenRid;
        name.d = OidObject(arena, cursor, length);
        break;
    case MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | GenDirName: {
        auto* inner = cursor;
        const auto* innerEnd = cursor + length;
        std::size_t nameLength = 0;
        auto* nameStart = inner;
        if (!Element(inner, innerEnd, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, nameLength) || inner + nameLength != innerEnd) Malformed("directoryName");
        mbedtls_x509_buf raw{};
        raw.tag = MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE;
        raw.p = nameStart;
        raw.len = static_cast<std::size_t>(innerEnd - nameStart);
        name.type = GenDirName;
        name.d = NewName(arena, raw);
        break;
    }
    case MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | GenOtherName: {
        auto* inner = cursor;
        const auto* innerEnd = cursor + length;
        std::size_t oidLength = 0;
        if (!Element(inner, innerEnd, MBEDTLS_ASN1_OID, oidLength)) Malformed("otherName");
        auto* typeId = OidObject(arena, inner, oidLength);
        inner += oidLength;
        std::size_t explicitLength = 0;
        if (!Element(inner, innerEnd, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0, explicitLength) || inner + explicitLength != innerEnd) Malformed("otherName");
        auto* value = ParseAsn1Type(arena, inner, innerEnd);
        if (inner != innerEnd) Malformed("otherName");
        name.type = GenOtherName;
        name.d = arena.Make(OtherName{typeId, value});
        break;
    }
    default:
        throw std::invalid_argument(std::string(function) + ": x400Address and ediPartyName general names are not supported");
    }
    cursor += length;
    return object.release();
}

void FreeGeneralName(GeneralName* name) {
    if (!name) return;
    std::lock_guard lock(Mutex());
    if (!GeneralNames().erase(name)) throw std::invalid_argument("GENERAL_NAME_free: the GENERAL_NAME was not created by X509_get_ext_d2i");
}

Stack* DecodeGeneralNames(const Asn1String& value, const char* function) {
    auto* cursor = value.data;
    const auto* end = value.data + value.length;
    std::size_t sequence = 0;
    if (!Element(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, sequence) || cursor + sequence != end) return nullptr;
    std::vector<std::unique_ptr<GeneralNameObject>> names;
    try {
        while (cursor < end) names.emplace_back(ParseGeneralName(cursor, end, function));
    } catch (const std::runtime_error&) {
        return nullptr;
    }
    auto* stack = NewHeapStack();
    for (auto& name : names) {
        auto* pointer = &name->name;
        {
            std::lock_guard lock(Mutex());
            GeneralNames().emplace(pointer, std::move(name));
        }
        HeapStackPush(stack, pointer);
    }
    return stack;
}

}

void CompleteX509(X509Object& x) {
    auto& arena = x.arena;
    auto* cursor = x.crt.raw.p;
    const auto* end = x.crt.raw.p + x.crt.raw.len;
    std::size_t length = 0;
    if (!Element(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, length)) Malformed("certificate");
    std::size_t tbsLength = 0;
    if (!Element(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, tbsLength)) Malformed("TBSCertificate");
    auto* tbs = cursor;
    const auto* tbsEnd = cursor + tbsLength;
    cursor = const_cast<unsigned char*>(tbsEnd);
    x.sigAlg = ParseAlgor(arena, cursor, end);
    std::size_t signatureLength = 0;
    if (!Element(cursor, end, MBEDTLS_ASN1_BIT_STRING, signatureLength)) Malformed("signature");
    x.signature = BitString(arena, cursor, signatureLength);
    const auto skip = [&](int tag) {
        std::size_t size = 0;
        if (!Element(tbs, tbsEnd, tag, size)) Malformed("TBSCertificate");
        tbs += size;
    };
    if (*tbs == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0)) skip(MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0);
    skip(MBEDTLS_ASN1_INTEGER);
    x.info.signature = ParseAlgor(arena, tbs, tbsEnd);
    skip(MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    skip(MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    skip(MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    auto* spki = tbs;
    std::size_t spkiLength = 0;
    if (!Element(tbs, tbsEnd, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, spkiLength)) Malformed("SubjectPublicKeyInfo");
    const auto* spkiEnd = tbs + spkiLength;
    auto* algor = ParseAlgor(arena, tbs, spkiEnd);
    std::size_t keyLength = 0;
    if (!Element(tbs, spkiEnd, MBEDTLS_ASN1_BIT_STRING, keyLength)) Malformed("SubjectPublicKeyInfo");
    auto* key = BitString(arena, tbs, keyLength);
    tbs += keyLength;
    x.info.key = arena.Make(Pubkey{algor, key, nullptr, spki, static_cast<std::size_t>(spkiEnd - spki)});
    while (tbs < tbsEnd) {
        const int tag = *tbs;
        if (tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 3)) {
            std::size_t wrapper = 0;
            if (!Element(tbs, tbsEnd, tag, wrapper)) Malformed("extensions");
            x.info.extensions = ParseExtensions(arena, tbs, tbs + wrapper);
        } else if (tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 1) || tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 2)) {
            std::size_t size = 0;
            if (!Element(tbs, tbsEnd, tag, size)) Malformed("unique identifier");
            (tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 1) ? x.info.issuerUid : x.info.subjectUid) = BitString(arena, tbs, size);
            tbs += size;
        } else {
            Malformed("TBSCertificate");
        }
    }
}

void ReleaseX509Extras(X509Object& x) {
    FreePkey(static_cast<EvpPkey*>(x.publicKey));
    x.publicKey = nullptr;
}

EvpPkey* X509PublicKey(X509Object& x, const char* function) {
    std::lock_guard lock(Mutex());
    if (!x.publicKey) {
        auto* pkey = PkeyFromMbed(x.crt.pk, function);
        x.publicKey = pkey;
        static_cast<Pubkey*>(x.info.key)->pkey = pkey;
    }
    return static_cast<EvpPkey*>(x.publicKey);
}

}

using namespace LibreSsl;

extern "C" {

EvpPkey* APS5_VABI X509_get_pubkey_nid_postfix(X509Object* x) {
    auto& cert = RequireX509(x, __func__);
    auto* pkey = X509PublicKey(cert, __func__);
    PkeyUpRef(pkey);
    return pkey;
}

Asn1Object* APS5_VABI X509_EXTENSION_get_object_nid_postfix(Extension* ex) {
    return RequireExtension(ex, __func__).object;
}

int APS5_VABI X509_EXTENSION_get_critical_nid_postfix(const Extension* ex) {
    return RequireExtension(ex, __func__).critical > 0 ? 1 : 0;
}

Asn1String* APS5_VABI X509_EXTENSION_get_data_nid_postfix(Extension* ex) {
    return RequireExtension(ex, __func__).value;
}

void* APS5_VABI X509_get_ext_d2i_nid_postfix(X509Object* x, int nid, int* crit, int* idx) {
    auto& cert = RequireX509(x, __func__);
    const Stack* extensions = cert.info.extensions;
    const int count = extensions ? extensions->num : 0;
    const Extension* found = nullptr;
    int foundIndex = -1;
    for (int index = idx ? (*idx < 0 ? 0 : *idx + 1) : 0; index < count; ++index) {
        const auto* extension = reinterpret_cast<const Extension*>(extensions->data[index]);
        if (extension->object->nid != nid || nid == NidUndef) continue;
        if (idx) {
            found = extension;
            foundIndex = index;
            break;
        }
        if (found) {
            if (crit) *crit = -2;
            return nullptr;
        }
        found = extension;
        foundIndex = index;
    }
    if (idx) *idx = foundIndex;
    if (!found) {
        if (crit) *crit = -1;
        return nullptr;
    }
    if (crit) *crit = found->critical > 0 ? 1 : 0;
    if (nid != NidSubjectAltName && nid != NidIssuerAltName)
        throw std::invalid_argument("X509_get_ext_d2i: decoding extension NID " + std::to_string(nid) + " is not supported (only subjectAltName and issuerAltName)");
    return DecodeGeneralNames(*found->value, __func__);
}

void APS5_VABI GENERAL_NAME_free_nid_postfix(GeneralName* a) {
    FreeGeneralName(a);
}

void APS5_VABI GENERAL_NAMES_free_nid_postfix(Stack* a) {
    if (!a) return;
    if (!IsHeapStack(a)) throw std::invalid_argument("GENERAL_NAMES_free: the stack was not created by X509_get_ext_d2i");
    for (int index = 0; index < a->num; ++index) FreeGeneralName(reinterpret_cast<GeneralName*>(a->data[index]));
    FreeHeapStack(a);
}

void APS5_VABI sk_pop_free_nid_postfix(Stack* st, void (APS5_VABI* func)(void*)) {
    if (!st) return;
    if (!IsHeapStack(st)) throw std::invalid_argument("sk_pop_free: the stack belongs to another object");
    if (!func) throw std::invalid_argument("sk_pop_free: null free function");
    for (int index = 0; index < st->num; ++index) {
        if (st->data[index]) func(st->data[index]);
    }
    FreeHeapStack(st);
}

int APS5_VABI i2d_X509_PUBKEY_nid_postfix(const Pubkey* a, unsigned char** pp) {
    if (!a) throw std::invalid_argument("i2d_X509_PUBKEY: null X509_PUBKEY");
    const int length = static_cast<int>(a->derLength);
    if (!pp) return length;
    if (!*pp) {
        auto* buffer = static_cast<unsigned char*>(GuestHeap::GuestHeapAllocate_nid_postfix(a->derLength));
        if (!buffer) throw std::bad_alloc();
        std::memcpy(buffer, a->der, a->derLength);
        *pp = buffer;
        return length;
    }
    std::memcpy(*pp, a->der, a->derLength);
    *pp += length;
    return length;
}

Extension* LibreSslX509GetExt_nid_no_patch(const X509Object* x, int loc) {
    if (!x) throw std::invalid_argument("X509_get_ext: null certificate");
    const Stack* extensions = x->info.extensions;
    if (!extensions || loc < 0 || loc >= extensions->num) return nullptr;
    return reinterpret_cast<Extension*>(extensions->data[loc]);
}

const Stack* LibreSslX509Get0Extensions_nid_no_patch(const X509Object* x) {
    if (!x) throw std::invalid_argument("X509_get0_extensions: null certificate");
    return x->info.extensions;
}

void LibreSslX509Get0Signature_nid_no_patch(const Asn1String** psig, const Algor** palg, const X509Object* x) {
    if (!x) throw std::invalid_argument("X509_get0_signature: null certificate");
    if (psig) *psig = x->signature;
    if (palg) *palg = static_cast<const Algor*>(x->sigAlg);
}

int LibreSslX509PubkeyGet0Param_nid_no_patch(const Asn1Object** ppkalg, const unsigned char** pk, int* ppklen, Algor** pa, const Pubkey* pub) {
    if (!pub) throw std::invalid_argument("X509_PUBKEY_get0_param: null X509_PUBKEY");
    if (ppkalg) *ppkalg = pub->algor->algorithm;
    if (pk) *pk = pub->publicKey->data;
    if (ppklen) *ppklen = pub->publicKey->length;
    if (pa) *pa = pub->algor;
    return 1;
}

}
