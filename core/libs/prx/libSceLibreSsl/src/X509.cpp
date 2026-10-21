#include "prx/libSceLibreSsl/include/Asn1.hpp"
#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestHeap.hpp"

#include <mbedtls/asn1.h>
#include <mbedtls/base64.h>
#include <mbedtls/x509_crt.h>

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

struct FileStream;

namespace LibreSsl {

namespace {

constexpr int ErrLibPem = 9;
constexpr int PemFunctionReadBio = 109;
constexpr int PemReasonNoStartLine = 108;
constexpr int PemReasonBadBase64Decode = 100;

constexpr int TypeInteger = 2;
constexpr int TypeNegative = 0x100;
constexpr int TypeBitString = 3;
constexpr int TypeUtf8String = 12;
constexpr int TypeNumericString = 18;
constexpr int TypePrintableString = 19;
constexpr int TypeIa5String = 22;
constexpr int TypeVisibleString = 26;
constexpr int TypeUniversalString = 28;
constexpr int TypeBmpString = 30;
constexpr std::int64_t FlagBitsLeft = 0x08;
constexpr int ObjectFlagDynamic = 0x01 | 0x04 | 0x08;

struct AttributeType {
    const char* oid;
    int nid;
    const char* sn;
    const char* ln;
};

constexpr AttributeType AttributeTypes[] = {
    {"2.5.4.3", 13, "CN", "commonName"},
    {"2.5.4.4", 100, "SN", "surname"},
    {"2.5.4.5", 105, "serialNumber", "serialNumber"},
    {"2.5.4.6", 14, "C", "countryName"},
    {"2.5.4.7", 15, "L", "localityName"},
    {"2.5.4.8", 16, "ST", "stateOrProvinceName"},
    {"2.5.4.9", 660, "street", "streetAddress"},
    {"2.5.4.10", 17, "O", "organizationName"},
    {"2.5.4.11", 18, "OU", "organizationalUnitName"},
    {"2.5.4.12", 106, "title", "title"},
    {"2.5.4.13", 107, "description", "description"},
    {"2.5.4.14", 859, "searchGuide", "searchGuide"},
    {"2.5.4.15", 860, "businessCategory", "businessCategory"},
    {"2.5.4.16", 861, "postalAddress", "postalAddress"},
    {"2.5.4.17", 661, "postalCode", "postalCode"},
    {"2.5.4.18", 862, "postOfficeBox", "postOfficeBox"},
    {"2.5.4.19", 863, "physicalDeliveryOfficeName", "physicalDeliveryOfficeName"},
    {"2.5.4.20", 864, "telephoneNumber", "telephoneNumber"},
    {"2.5.4.21", 865, "telexNumber", "telexNumber"},
    {"2.5.4.22", 866, "teletexTerminalIdentifier", "teletexTerminalIdentifier"},
    {"2.5.4.23", 867, "facsimileTelephoneNumber", "facsimileTelephoneNumber"},
    {"2.5.4.24", 868, "x121Address", "x121Address"},
    {"2.5.4.25", 869, "internationaliSDNNumber", "internationaliSDNNumber"},
    {"2.5.4.26", 870, "registeredAddress", "registeredAddress"},
    {"2.5.4.27", 871, "destinationIndicator", "destinationIndicator"},
    {"2.5.4.28", 872, "preferredDeliveryMethod", "preferredDeliveryMethod"},
    {"2.5.4.29", 873, "presentationAddress", "presentationAddress"},
    {"2.5.4.30", 874, "supportedApplicationContext", "supportedApplicationContext"},
    {"2.5.4.33", 877, "roleOccupant", "roleOccupant"},
    {"2.5.4.35", 879, "userPassword", "userPassword"},
    {"2.5.4.36", 880, "userCertificate", "userCertificate"},
    {"2.5.4.37", 881, "cACertificate", "cACertificate"},
    {"2.5.4.38", 882, "authorityRevocationList", "authorityRevocationList"},
    {"2.5.4.39", 883, "certificateRevocationList", "certificateRevocationList"},
    {"2.5.4.40", 884, "crossCertificatePair", "crossCertificatePair"},
    {"2.5.4.41", 173, "name", "name"},
    {"2.5.4.42", 99, "GN", "givenName"},
    {"2.5.4.43", 101, "initials", "initials"},
    {"2.5.4.44", 509, "generationQualifier", "generationQualifier"},
    {"2.5.4.45", 503, "x500UniqueIdentifier", "x500UniqueIdentifier"},
    {"2.5.4.46", 174, "dnQualifier", "dnQualifier"},
    {"2.5.4.47", 885, "enhancedSearchGuide", "enhancedSearchGuide"},
    {"2.5.4.48", 886, "protocolInformation", "protocolInformation"},
    {"2.5.4.49", 887, "distinguishedName", "distinguishedName"},
    {"2.5.4.50", 888, "uniqueMember", "uniqueMember"},
    {"2.5.4.51", 889, "houseIdentifier", "houseIdentifier"},
    {"2.5.4.52", 890, "supportedAlgorithms", "supportedAlgorithms"},
    {"2.5.4.53", 891, "deltaRevocationList", "deltaRevocationList"},
    {"2.5.4.65", 510, "pseudonym", "pseudonym"},
    {"2.5.4.72", 400, "role", "role"},
    {"1.2.840.113549.1.9.1", 48, "emailAddress", "emailAddress"},
    {"1.2.840.113549.1.9.2", 49, "unstructuredName", "unstructuredName"},
    {"1.2.840.113549.1.9.8", 55, "unstructuredAddress", "unstructuredAddress"},
    {"0.9.2342.19200300.100.1.1", 458, "UID", "userId"},
    {"0.9.2342.19200300.100.1.25", 391, "DC", "domainComponent"},
    {"1.3.6.1.4.1.311.60.2.1.1", 956, "jurisdictionLocalityName", "jurisdictionLocalityName"},
    {"1.3.6.1.4.1.311.60.2.1.2", 957, "jurisdictionStateOrProvinceName", "jurisdictionStateOrProvinceName"},
    {"1.3.6.1.4.1.311.60.2.1.3", 958, "jurisdictionCountryName", "jurisdictionCountryName"},
};

std::vector<unsigned char> EncodeOid(const char* dotted) {
    std::vector<std::uint64_t> arcs;
    std::uint64_t value = 0;
    for (const char* cursor = dotted;; ++cursor) {
        if (*cursor == '.' || *cursor == '\0') {
            arcs.push_back(value);
            value = 0;
            if (*cursor == '\0') break;
        } else {
            value = value * 10 + static_cast<std::uint64_t>(*cursor - '0');
        }
    }
    std::vector<std::uint64_t> encoded{arcs[0] * 40 + arcs[1]};
    encoded.insert(encoded.end(), arcs.begin() + 2, arcs.end());
    std::vector<unsigned char> bytes;
    for (auto arc : encoded) {
        unsigned char groups[10];
        int count = 0;
        do {
            groups[count++] = static_cast<unsigned char>(arc & 0x7f);
            arc >>= 7;
        } while (arc);
        while (count--) bytes.push_back(static_cast<unsigned char>(groups[count] | (count ? 0x80 : 0)));
    }
    return bytes;
}

struct ObjectTable {
    std::vector<std::vector<unsigned char>> encodings;
    std::vector<Asn1Object> objects;

    ObjectTable() {
        encodings.reserve(std::size(AttributeTypes));
        for (const auto& type : AttributeTypes) encodings.push_back(EncodeOid(type.oid));
        for (std::size_t index = 0; index < std::size(AttributeTypes); ++index) {
            const auto& type = AttributeTypes[index];
            objects.push_back({type.sn, type.ln, type.nid, static_cast<int>(encodings[index].size()), encodings[index].data(), 0});
        }
    }
};

const ObjectTable& Objects() {
    static const ObjectTable table;
    return table;
}

const Asn1Object* ObjectByNid(int nid) {
    for (const auto& object : Objects().objects) {
        if (object.nid == nid) return &object;
    }
    return nullptr;
}

struct Arena {
    std::vector<std::unique_ptr<Asn1String>> strings;
    std::vector<std::unique_ptr<Asn1Object>> objects;
    std::vector<std::unique_ptr<NameEntry>> entries;
    std::vector<std::unique_ptr<Name>> names;
    std::vector<std::unique_ptr<Stack>> stacks;
    std::vector<std::unique_ptr<std::vector<char*>>> stackData;
    std::vector<std::unique_ptr<unsigned char[]>> buffers;

    unsigned char* Copy(const unsigned char* data, std::size_t length) {
        buffers.push_back(std::make_unique<unsigned char[]>(length + 1));
        if (length) std::memcpy(buffers.back().get(), data, length);
        buffers.back()[length] = 0;
        return buffers.back().get();
    }

    Asn1String* String(int type, const unsigned char* data, std::size_t length, std::int64_t flags = 0) {
        strings.push_back(std::make_unique<Asn1String>(Asn1String{static_cast<int>(length), type, Copy(data, length), flags}));
        return strings.back().get();
    }

    Stack* NewStack(const std::vector<char*>& items) {
        stackData.push_back(std::make_unique<std::vector<char*>>(items));
        auto& data = *stackData.back();
        stacks.push_back(std::make_unique<Stack>(Stack{static_cast<int>(data.size()), data.data(), 0, static_cast<int>(data.size()), nullptr}));
        return stacks.back().get();
    }
};

std::mutex& StackMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_set<const Stack*>& OwnedStacks() {
    static std::unordered_set<const Stack*> stacks;
    return stacks;
}

std::mutex& IntegerMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_set<const Asn1String*>& StandaloneIntegers() {
    static std::unordered_set<const Asn1String*> integers;
    return integers;
}

}

struct X509Object {
    CertInfo* certInfo = nullptr;
    void* sigAlg = nullptr;
    Asn1String* signature = nullptr;
    int valid = 0;
    int references = 1;
    char* name = nullptr;
    std::atomic<int> count{1};
    mbedtls_x509_crt crt;
    Arena arena;
    CertInfo info{};
    Validity validity{};
    int extensionCount = 0;

    X509Object() {
        mbedtls_x509_crt_init(&crt);
    }

    ~X509Object() {
        std::lock_guard lock(StackMutex());
        for (const auto& stack : arena.stacks) OwnedStacks().erase(stack.get());
        mbedtls_x509_crt_free(&crt);
    }

    X509Object(const X509Object&) = delete;
    X509Object& operator=(const X509Object&) = delete;
};

static_assert(offsetof(X509Object, signature) == 16 && offsetof(X509Object, references) == 28);

namespace {

bool Element(unsigned char*& cursor, const unsigned char* end, int tag, std::size_t& length) {
    return mbedtls_asn1_get_tag(&cursor, end, &length, tag) == 0;
}

Asn1String* IntegerFromContent(Arena& arena, const unsigned char* data, std::size_t length) {
    if (length && (data[0] & 0x80)) {
        std::vector<unsigned char> magnitude(data, data + length);
        bool carry = true;
        for (std::size_t index = magnitude.size(); index-- > 0;) {
            magnitude[index] = static_cast<unsigned char>(~magnitude[index]);
            if (carry) carry = ++magnitude[index] == 0;
        }
        std::size_t skip = 0;
        while (skip + 1 < magnitude.size() && magnitude[skip] == 0) ++skip;
        return arena.String(TypeInteger | TypeNegative, magnitude.data() + skip, magnitude.size() - skip);
    }
    std::size_t skip = 0;
    while (skip + 1 < length && data[skip] == 0) ++skip;
    return arena.String(TypeInteger, data + skip, length - skip);
}

Name* ParseName(Arena& arena, const mbedtls_x509_buf& raw) {
    auto* cursor = raw.p;
    const auto* end = raw.p + raw.len;
    std::size_t length = 0;
    if (!Element(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, length)) throw std::runtime_error("X509: malformed name");
    std::vector<char*> items;
    int set = 0;
    while (cursor < end) {
        std::size_t setLength = 0;
        if (!Element(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET, setLength)) throw std::runtime_error("X509: malformed name");
        const auto* setEnd = cursor + setLength;
        while (cursor < setEnd) {
            std::size_t pairLength = 0;
            if (!Element(cursor, setEnd, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, pairLength)) throw std::runtime_error("X509: malformed name");
            const auto* pairEnd = cursor + pairLength;
            std::size_t oidLength = 0;
            if (!Element(cursor, pairEnd, MBEDTLS_ASN1_OID, oidLength)) throw std::runtime_error("X509: malformed name");
            const auto* oid = cursor;
            cursor += oidLength;
            if (cursor >= pairEnd) throw std::runtime_error("X509: malformed name");
            const int tag = *cursor++;
            std::size_t valueLength = 0;
            if (mbedtls_asn1_get_len(&cursor, pairEnd, &valueLength) != 0) throw std::runtime_error("X509: malformed name");
            const Asn1Object* object = nullptr;
            for (const auto& known : Objects().objects) {
                if (static_cast<std::size_t>(known.length) == oidLength && std::memcmp(known.data, oid, oidLength) == 0) object = &known;
            }
            if (!object) {
                arena.objects.push_back(std::make_unique<Asn1Object>(Asn1Object{nullptr, nullptr, NidUndef, static_cast<int>(oidLength), arena.Copy(oid, oidLength), ObjectFlagDynamic}));
                object = arena.objects.back().get();
            }
            arena.entries.push_back(std::make_unique<NameEntry>(NameEntry{const_cast<Asn1Object*>(object), arena.String(tag, cursor, valueLength), set, 0}));
            items.push_back(reinterpret_cast<char*>(arena.entries.back().get()));
            cursor += valueLength;
        }
        ++set;
    }
    arena.names.push_back(std::make_unique<Name>(Name{arena.NewStack(items), 0, nullptr, nullptr, 0}));
    return arena.names.back().get();
}

void ParseTbs(X509Object& x) {
    auto* cursor = x.crt.tbs.p;
    const auto* end = x.crt.tbs.p + x.crt.tbs.len;
    std::size_t length = 0;
    if (!Element(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, length)) throw std::runtime_error("X509: malformed TBSCertificate");
    const auto skip = [&](int tag) {
        std::size_t size = 0;
        if (!Element(cursor, end, tag, size)) return false;
        cursor += size;
        return true;
    };
    if (*cursor == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0)) skip(MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0);
    if (!skip(MBEDTLS_ASN1_INTEGER) || !skip(MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) || !skip(MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        throw std::runtime_error("X509: malformed TBSCertificate");
    std::size_t validityLength = 0;
    if (!Element(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, validityLength)) throw std::runtime_error("X509: malformed validity");
    auto* validityEnd = cursor + validityLength;
    Asn1String* times[2] = {};
    for (auto& time : times) {
        if (cursor >= validityEnd) throw std::runtime_error("X509: malformed validity");
        const int tag = *cursor++;
        std::size_t timeLength = 0;
        if ((tag != MBEDTLS_ASN1_UTC_TIME && tag != MBEDTLS_ASN1_GENERALIZED_TIME) || mbedtls_asn1_get_len(&cursor, validityEnd, &timeLength) != 0)
            throw std::runtime_error("X509: malformed validity");
        time = x.arena.String(tag, cursor, timeLength);
        cursor += timeLength;
    }
    x.validity = {times[0], times[1]};
    cursor = validityEnd;
    if (!skip(MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) || !skip(MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
        throw std::runtime_error("X509: malformed TBSCertificate");
    while (cursor < end) {
        const int tag = *cursor;
        if (tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 3)) {
            std::size_t wrapper = 0, sequence = 0;
            if (!Element(cursor, end, tag, wrapper) || !Element(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, sequence))
                throw std::runtime_error("X509: malformed extensions");
            const auto* extensionsEnd = cursor + sequence;
            while (cursor < extensionsEnd) {
                std::size_t extension = 0;
                if (!Element(cursor, extensionsEnd, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, extension)) throw std::runtime_error("X509: malformed extensions");
                cursor += extension;
                ++x.extensionCount;
            }
        } else {
            ++cursor;
            std::size_t size = 0;
            if (mbedtls_asn1_get_len(&cursor, end, &size) != 0) throw std::runtime_error("X509: malformed TBSCertificate");
            cursor += size;
        }
    }
}

X509Object* Build(const unsigned char* der, std::size_t length) {
    auto x = std::make_unique<X509Object>();
    if (mbedtls_x509_crt_parse_der(&x->crt, der, length) != 0) return nullptr;
    auto& arena = x->arena;
    const auto versionByte = static_cast<unsigned char>(x->crt.version - 1);
    x->info.version = x->crt.version > 1 ? IntegerFromContent(arena, &versionByte, 1) : nullptr;
    x->info.serialNumber = IntegerFromContent(arena, x->crt.serial.p, x->crt.serial.len);
    x->info.issuer = ParseName(arena, x->crt.issuer_raw);
    x->info.subject = ParseName(arena, x->crt.subject_raw);
    ParseTbs(*x);
    x->info.validity = &x->validity;
    x->certInfo = &x->info;
    {
        std::lock_guard lock(StackMutex());
        for (const auto& stack : arena.stacks) OwnedStacks().insert(stack.get());
    }
    return x.release();
}

std::size_t DerLength(const unsigned char* data, std::size_t available) {
    auto* cursor = const_cast<unsigned char*>(data);
    std::size_t length = 0;
    if (mbedtls_asn1_get_tag(&cursor, data + available, &length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) return 0;
    return static_cast<std::size_t>(cursor - data) + length;
}

X509Object& RequireX509(X509Object* x, const char* function) {
    if (!x) throw std::invalid_argument(std::string(function) + ": null certificate");
    return *x;
}

const Asn1String& RequireString(const Asn1String* string, const char* function) {
    if (!string) throw std::invalid_argument(std::string(function) + ": null ASN1_STRING");
    return *string;
}

const Name& RequireName(const Name* name, const char* function) {
    if (!name) throw std::invalid_argument(std::string(function) + ": null X509_NAME");
    return *name;
}

void AppendUtf8(std::string& out, std::uint32_t code) {
    if (code < 0x80) {
        out += static_cast<char>(code);
    } else if (code < 0x800) {
        out += static_cast<char>(0xc0 | (code >> 6));
        out += static_cast<char>(0x80 | (code & 0x3f));
    } else if (code < 0x10000) {
        out += static_cast<char>(0xe0 | (code >> 12));
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
        out += static_cast<char>(0x80 | (code & 0x3f));
    } else {
        out += static_cast<char>(0xf0 | (code >> 18));
        out += static_cast<char>(0x80 | ((code >> 12) & 0x3f));
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
        out += static_cast<char>(0x80 | (code & 0x3f));
    }
}

bool ToUtf8(const Asn1String& in, std::string& out) {
    const auto* data = in.data;
    const auto length = static_cast<std::size_t>(in.length);
    switch (in.type) {
    case TypeUtf8String:
    case TypeNumericString:
    case TypePrintableString:
    case TypeIa5String:
    case TypeVisibleString:
        out.assign(reinterpret_cast<const char*>(data), length);
        return true;
    case TypeBmpString:
        if (length % 2) return false;
        for (std::size_t index = 0; index < length; index += 2) AppendUtf8(out, static_cast<std::uint32_t>(data[index]) << 8 | data[index + 1]);
        return true;
    case TypeUniversalString:
        if (length % 4) return false;
        for (std::size_t index = 0; index < length; index += 4) {
            const std::uint32_t code = static_cast<std::uint32_t>(data[index]) << 24 | static_cast<std::uint32_t>(data[index + 1]) << 16 |
                static_cast<std::uint32_t>(data[index + 2]) << 8 | data[index + 3];
            if (code > 0x10ffff) return false;
            AppendUtf8(out, code);
        }
        return true;
    default:
        throw std::invalid_argument("ASN1_STRING_to_UTF8: unsupported string type " + std::to_string(in.type));
    }
}

int NameIndex(const Name& name, int nid, int lastpos, const char* function) {
    if (!ObjectByNid(nid)) throw std::invalid_argument(std::string(function) + ": NID " + std::to_string(nid) + " is not a name attribute type of this build");
    const int count = name.entries->num;
    for (int index = lastpos < 0 ? 0 : lastpos + 1; index < count; ++index) {
        const auto* entry = reinterpret_cast<const NameEntry*>(name.entries->data[index]);
        if (entry->object->nid == nid) return index;
    }
    return -1;
}

X509Object* ReadPem(Bio* bio, const char* function) {
    const auto bytes = BioReadAll(bio, function);
    const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    constexpr std::string_view begin = "-----BEGIN ";
    const auto start = text.find(begin);
    if (start == std::string_view::npos) {
        PushError(ErrLibPem, PemFunctionReadBio, PemReasonNoStartLine);
        return nullptr;
    }
    const auto labelEnd = text.find("-----", start + begin.size());
    if (labelEnd == std::string_view::npos) {
        PushError(ErrLibPem, PemFunctionReadBio, PemReasonNoStartLine);
        return nullptr;
    }
    const auto label = text.substr(start + begin.size(), labelEnd - start - begin.size());
    if (label != "CERTIFICATE" && label != "X509 CERTIFICATE")
        throw std::invalid_argument(std::string(function) + ": the next PEM object is a " + std::string(label) + ", not a certificate");
    const std::string footer = "-----END " + std::string(label) + "-----";
    const auto stop = text.find(footer, labelEnd);
    if (stop == std::string_view::npos) throw std::invalid_argument(std::string(function) + ": PEM object without an end line");
    auto bodyStart = text.find('\n', labelEnd);
    if (bodyStart == std::string_view::npos || bodyStart > stop) throw std::invalid_argument(std::string(function) + ": malformed PEM object");
    std::string base64;
    for (auto index = bodyStart; index < stop; ++index) {
        const char character = text[index];
        if (character != '\r' && character != '\n' && character != ' ' && character != '\t') base64 += character;
    }
    if (base64.find(':') != std::string::npos) throw std::invalid_argument(std::string(function) + ": PEM headers are not supported");
    std::size_t size = 0;
    mbedtls_base64_decode(nullptr, 0, &size, reinterpret_cast<const unsigned char*>(base64.data()), base64.size());
    std::vector<unsigned char> der(size);
    if (mbedtls_base64_decode(der.data(), der.size(), &size, reinterpret_cast<const unsigned char*>(base64.data()), base64.size()) != 0) {
        PushError(ErrLibPem, PemFunctionReadBio, PemReasonBadBase64Decode);
        return nullptr;
    }
    auto consumed = stop + footer.size();
    if (consumed < text.size() && text[consumed] == '\r') ++consumed;
    if (consumed < text.size() && text[consumed] == '\n') ++consumed;
    BioPutBack(bio, bytes.data() + consumed, bytes.size() - consumed);
    if (DerLength(der.data(), size) != size) return nullptr;
    return Build(der.data(), size);
}

}

}

using namespace LibreSsl;

extern "C" {
Bio* APS5_VABI BIO_new_fp_nid_postfix(FileStream* stream, int close_flag);
int APS5_VABI BIO_free_nid_postfix(Bio* a);

X509Object* APS5_VABI d2i_X509_nid_postfix(X509Object** a, const unsigned char** in, std::int64_t len) {
    if (a && *a) throw std::invalid_argument("d2i_X509: reusing an existing X509 is not supported");
    if (!in || !*in) throw std::invalid_argument("d2i_X509: null input");
    if (len <= 0) return nullptr;
    const std::size_t length = DerLength(*in, static_cast<std::size_t>(len));
    if (!length) return nullptr;
    auto* x = Build(*in, length);
    if (!x) return nullptr;
    *in += length;
    if (a) *a = x;
    return x;
}

X509Object* APS5_VABI PEM_read_bio_X509_nid_postfix(Bio* bp, X509Object** x, void*, void*) {
    if (x && *x) throw std::invalid_argument("PEM_read_bio_X509: reusing an existing X509 is not supported");
    auto* result = ReadPem(bp, __func__);
    if (result && x) *x = result;
    return result;
}

X509Object* APS5_VABI PEM_read_X509_nid_postfix(FileStream* fp, X509Object** x, void* cb, void* u) {
    if (!fp) throw std::invalid_argument("PEM_read_X509: null stream");
    auto* bio = BIO_new_fp_nid_postfix(fp, 0);
    if (!bio) return nullptr;
    X509Object* result = nullptr;
    try {
        result = PEM_read_bio_X509_nid_postfix(bio, x, cb, u);
    } catch (...) {
        BIO_free_nid_postfix(bio);
        throw;
    }
    BIO_free_nid_postfix(bio);
    return result;
}

void APS5_VABI X509_free_nid_postfix(X509Object* a) {
    if (!a) return;
    const int left = a->count.fetch_sub(1) - 1;
    a->references = left;
    if (left == 0) delete a;
}

Name* APS5_VABI X509_get_subject_name_nid_postfix(X509Object* a) {
    return RequireX509(a, __func__).info.subject;
}

Name* APS5_VABI X509_get_issuer_name_nid_postfix(X509Object* a) {
    return RequireX509(a, __func__).info.issuer;
}

Asn1String* APS5_VABI X509_get_serialNumber_nid_postfix(X509Object* x) {
    return RequireX509(x, __func__).info.serialNumber;
}

int APS5_VABI X509_NAME_get_index_by_NID_nid_postfix(Name* name, int nid, int lastpos) {
    return NameIndex(RequireName(name, __func__), nid, lastpos, __func__);
}

NameEntry* APS5_VABI X509_NAME_get_entry_nid_postfix(Name* name, int loc) {
    const auto& names = RequireName(name, __func__);
    if (loc < 0 || loc >= names.entries->num) return nullptr;
    return reinterpret_cast<NameEntry*>(names.entries->data[loc]);
}

Asn1String* APS5_VABI X509_NAME_ENTRY_get_data_nid_postfix(NameEntry* ne) {
    if (!ne) throw std::invalid_argument("X509_NAME_ENTRY_get_data: null entry");
    return ne->value;
}

int APS5_VABI X509_NAME_get_text_by_NID_nid_postfix(Name* name, int nid, char* buf, int len) {
    const auto& names = RequireName(name, __func__);
    const int index = NameIndex(names, nid, -1, __func__);
    if (index < 0) return -1;
    const auto& value = *reinterpret_cast<const NameEntry*>(names.entries->data[index])->value;
    switch (value.type) {
    case TypeUtf8String:
    case TypeNumericString:
    case TypePrintableString:
    case TypeIa5String:
    case TypeVisibleString:
        break;
    default:
        throw std::invalid_argument("X509_NAME_get_text_by_NID: the entry's string type " + std::to_string(value.type) + " converts differently between LibreSSL versions");
    }
    if (std::memchr(value.data, 0, static_cast<std::size_t>(value.length)))
        throw std::invalid_argument("X509_NAME_get_text_by_NID: an entry with an embedded NUL is handled differently between LibreSSL versions");
    if (!buf) return value.length;
    if (len <= value.length) throw std::invalid_argument("X509_NAME_get_text_by_NID: a buffer too small is handled differently between LibreSSL versions");
    std::memcpy(buf, value.data, static_cast<std::size_t>(value.length));
    buf[value.length] = '\0';
    return value.length;
}

unsigned char* APS5_VABI ASN1_STRING_data_nid_postfix(Asn1String* x) {
    return RequireString(x, __func__).data;
}

int APS5_VABI ASN1_STRING_length_nid_postfix(const Asn1String* x) {
    return RequireString(x, __func__).length;
}

int APS5_VABI ASN1_STRING_type_nid_postfix(Asn1String* x) {
    return RequireString(x, __func__).type;
}

int APS5_VABI ASN1_STRING_to_UTF8_nid_postfix(unsigned char** out, Asn1String* in) {
    if (!out) throw std::invalid_argument("ASN1_STRING_to_UTF8: null output");
    std::string text;
    if (!ToUtf8(RequireString(in, __func__), text)) return -1;
    auto* copy = static_cast<unsigned char*>(GuestHeap::GuestHeapAllocate_nid_postfix(text.size() + 1));
    if (!copy) return -1;
    std::memcpy(copy, text.data(), text.size());
    copy[text.size()] = 0;
    *out = copy;
    return static_cast<int>(text.size());
}

std::int64_t APS5_VABI ASN1_INTEGER_get_nid_postfix(const Asn1String* a) {
    if (!a) return 0;
    if (a->type != TypeInteger && a->type != (TypeInteger | TypeNegative)) return -1;
    if (a->length > 8) return -1;
    std::uint64_t value = 0;
    for (int index = 0; index < a->length; ++index) value = value << 8 | a->data[index];
    if (value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) return -1;
    return a->type & TypeNegative ? -static_cast<std::int64_t>(value) : static_cast<std::int64_t>(value);
}

int APS5_VABI sk_num_nid_postfix(const Stack* st) {
    return st ? st->num : -1;
}

void* APS5_VABI sk_value_nid_postfix(const Stack* st, int i) {
    if (!st || i < 0 || i >= st->num) return nullptr;
    return st->data[i];
}

void* APS5_VABI sk_pop_nid_postfix(Stack* st) {
    if (!st || st->num == 0) return nullptr;
    return st->data[--st->num];
}

X509Object* LibreSslD2iX509Bio_nid_no_patch(Bio* bp, X509Object** x509) {
    if (x509 && *x509) throw std::invalid_argument("d2i_X509_bio: reusing an existing X509 is not supported");
    const auto bytes = BioReadAll(bp, "d2i_X509_bio");
    const std::size_t length = DerLength(bytes.data(), bytes.size());
    if (!length) {
        BioPutBack(bp, bytes.data(), bytes.size());
        return nullptr;
    }
    BioPutBack(bp, bytes.data() + length, bytes.size() - length);
    auto* result = Build(bytes.data(), length);
    if (result && x509) *x509 = result;
    return result;
}

int LibreSslX509UpRef_nid_no_patch(X509Object* x) {
    auto& object = RequireX509(x, "X509_up_ref");
    object.references = object.count.fetch_add(1) + 1;
    return 1;
}

std::int64_t LibreSslX509Version_nid_no_patch(const X509Object* x) {
    if (!x) throw std::invalid_argument("X509_get_version: null certificate");
    return x->crt.version - 1;
}

Asn1String* LibreSslX509Time_nid_no_patch(const X509Object* x, int which) {
    if (!x) throw std::invalid_argument("X509_get0_notBefore/notAfter: null certificate");
    return which ? x->validity.notAfter : x->validity.notBefore;
}

int LibreSslX509ExtensionCount_nid_no_patch(const X509Object* x) {
    if (!x) throw std::invalid_argument("X509_get_ext_count: null certificate");
    return x->extensionCount;
}

Asn1String* LibreSslD2iAsn1Integer_nid_no_patch(Asn1String** a, const unsigned char** pp, std::int64_t length) {
    if (a && *a) throw std::invalid_argument("d2i_ASN1_INTEGER: reusing an existing ASN1_INTEGER is not supported");
    if (!pp || !*pp) throw std::invalid_argument("d2i_ASN1_INTEGER: null input");
    if (length <= 0) return nullptr;
    auto* cursor = const_cast<unsigned char*>(*pp);
    const auto* end = cursor + length;
    std::size_t size = 0;
    if (mbedtls_asn1_get_tag(&cursor, end, &size, MBEDTLS_ASN1_INTEGER) != 0 || size == 0) return nullptr;
    Arena scratch;
    const auto* parsed = IntegerFromContent(scratch, cursor, size);
    auto* result = new Asn1String{parsed->length, parsed->type, static_cast<unsigned char*>(std::malloc(static_cast<std::size_t>(parsed->length) + 1)), 0};
    std::memcpy(result->data, parsed->data, static_cast<std::size_t>(parsed->length) + 1);
    {
        std::lock_guard lock(IntegerMutex());
        StandaloneIntegers().insert(result);
    }
    *pp = cursor + size;
    if (a) *a = result;
    return result;
}

void LibreSslAsn1IntegerFree_nid_no_patch(Asn1String* a) {
    if (!a) return;
    {
        std::lock_guard lock(IntegerMutex());
        if (!StandaloneIntegers().erase(a)) throw std::invalid_argument("ASN1_INTEGER_free: the integer belongs to another object");
    }
    std::free(a->data);
    delete a;
}

void LibreSslAsn1ObjectFree_nid_no_patch(Asn1Object* a) {
    if (!a) return;
    if (ReleaseCreatedObject(a)) return;
    if (a->flags & 0x01) throw std::invalid_argument("ASN1_OBJECT_free: the object belongs to a certificate");
}

}
