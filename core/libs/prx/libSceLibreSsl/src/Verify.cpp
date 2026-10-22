#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/include/Keys.hpp"
#include "prx/libSceLibreSsl/include/X509Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <mbedtls/asn1.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <ctime>
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
FileStream* APS5_VABI fopen_nid_postfix(const char* filename, const char* mode);
LibreSsl::Bio* APS5_VABI BIO_new_fp_nid_postfix(FileStream* stream, int close_flag);
int APS5_VABI BIO_free_nid_postfix(LibreSsl::Bio* a);
}

namespace LibreSsl {

struct VerifyParam {
    char* name;
    std::int64_t checkTime;
    std::uint64_t inhFlags;
    std::uint64_t flags;
    int purpose;
    int trust;
    int depth;
    void* policies;
    void* id;
};

struct Store {
    int cache;
    Stack* objs;
    Stack* getCertMethods;
    VerifyParam* param;
    void* verify;
    void* verifyCb;
    void* getIssuer;
    void* checkIssued;
    void* checkRevocation;
    void* getCrl;
    void* checkCrl;
    void* certCrl;
    void* lookupCerts;
    void* lookupCrls;
    void* cleanup;
    CryptoExData exData;
    int references;
};

struct StoreCtx {
    Store* ctx;
    int currentMethod;
    X509Object* cert;
    Stack* untrusted;
    Stack* crls;
    VerifyParam* param;
    void* otherCtx;
    void* verify;
    void* verifyCb;
    void* getIssuer;
    void* checkIssued;
    void* checkRevocation;
    void* getCrl;
    void* checkCrl;
    void* certCrl;
    void* checkPolicy;
    void* lookupCerts;
    void* lookupCrls;
    void* cleanup;
    int valid;
    int lastUntrusted;
    Stack* chain;
    void* tree;
    int explicitPolicy;
    int errorDepth;
    int error;
    X509Object* currentCert;
    X509Object* currentIssuer;
    X509Crl* currentCrl;
    int currentCrlScore;
    unsigned currentReasons;
    StoreCtx* parent;
    CryptoExData exData;
};

struct LookupMethod {
    const char* name;
};

struct Lookup {
    int init;
    int skip;
    const LookupMethod* method;
    char* methodData;
    Store* storeCtx;
};

struct StoreEntry {
    int type;
    void* data;
};

static_assert(sizeof(VerifyParam) == 64 && offsetof(VerifyParam, flags) == 24 && offsetof(VerifyParam, depth) == 40 && offsetof(VerifyParam, id) == 56);
static_assert(sizeof(Store) == 136 && offsetof(Store, param) == 24 && offsetof(Store, verifyCb) == 40 && offsetof(Store, references) == 128);
static_assert(sizeof(StoreCtx) == 240 && offsetof(StoreCtx, verifyCb) == 64 && offsetof(StoreCtx, valid) == 152 && offsetof(StoreCtx, chain) == 160);
static_assert(offsetof(StoreCtx, errorDepth) == 180 && offsetof(StoreCtx, error) == 184 && offsetof(StoreCtx, currentCert) == 192 && offsetof(StoreCtx, exData) == 232);
static_assert(sizeof(Lookup) == 32 && sizeof(StoreEntry) == 16);

namespace {

constexpr int VOk = 0;
constexpr int VErrUnableToGetIssuerCert = 2;
constexpr int VErrUnableToGetCrl = 3;
constexpr int VErrCertSignatureFailure = 7;
constexpr int VErrCrlSignatureFailure = 8;
constexpr int VErrCertNotYetValid = 9;
constexpr int VErrCertHasExpired = 10;
constexpr int VErrCrlNotYetValid = 11;
constexpr int VErrCrlHasExpired = 12;
constexpr int VErrDepthZeroSelfSignedCert = 18;
constexpr int VErrSelfSignedCertInChain = 19;
constexpr int VErrUnableToGetIssuerCertLocally = 20;
constexpr int VErrUnableToVerifyLeafSignature = 21;
constexpr int VErrCertChainTooLong = 22;
constexpr int VErrCertRevoked = 23;
constexpr int VErrInvalidCa = 24;
constexpr int VErrPathLengthExceeded = 25;
constexpr int VErrSubjectIssuerMismatch = 29;
constexpr int VErrAkidSkidMismatch = 30;
constexpr int VErrAkidIssuerSerialMismatch = 31;
constexpr int VErrKeyusageNoCertsign = 32;
constexpr int VErrUnhandledCriticalExtension = 34;
constexpr int VErrKeyusageNoCrlSign = 35;
constexpr int VErrHostnameMismatch = 62;
constexpr int VErrIpAddressMismatch = 64;

constexpr std::uint64_t FlagUseCheckTime = 0x2;
constexpr std::uint64_t FlagCrlCheck = 0x4;
constexpr std::uint64_t FlagCrlCheckAll = 0x8;
constexpr std::uint64_t FlagIgnoreCritical = 0x10;
constexpr std::uint64_t FlagX509Strict = 0x20;
constexpr std::uint64_t FlagAllowProxyCerts = 0x40;
constexpr std::uint64_t FlagPolicyCheck = 0x80;
constexpr std::uint64_t FlagExplicitPolicy = 0x100;
constexpr std::uint64_t FlagInhibitAny = 0x200;
constexpr std::uint64_t FlagInhibitMap = 0x400;
constexpr std::uint64_t FlagExtendedCrlSupport = 0x1000;
constexpr std::uint64_t FlagUseDeltas = 0x2000;
constexpr std::uint64_t FlagCheckSsSignature = 0x4000;
constexpr std::uint64_t FlagPartialChain = 0x80000;
constexpr std::uint64_t FlagNoCheckTime = 0x200000;

constexpr unsigned CheckFlagAlwaysCheckSubject = 0x1;
constexpr unsigned CheckFlagNoWildcards = 0x2;
constexpr unsigned CheckFlagNoPartialWildcards = 0x4;
constexpr unsigned CheckFlagMultiLabelWildcards = 0x8;
constexpr unsigned CheckFlagSingleLabelSubdomains = 0x10;

constexpr int LuX509 = 1;
constexpr int LuCrl = 2;
constexpr int FileTypePem = 1;
constexpr int FileTypeAsn1 = 2;
constexpr int DefaultDepth = 100;

constexpr int X509FunctionStoreAddCert = 124;
constexpr int X509FunctionStoreAddCrl = 125;
constexpr int X509FunctionVerifyCert = 127;
constexpr int X509FunctionLoadCrlFile = 112;
constexpr int X509ReasonCertAlreadyInHashTable = 101;
constexpr int X509ReasonNoCertSetForUsToVerify = 105;
constexpr int X509ReasonBadX509FileType = 100;
constexpr int ErrReasonAsn1Lib = 13;
constexpr int ErrReasonSysLib = 2;

constexpr int NidCommonName = 13;
constexpr int NidKeyUsage = 83;
constexpr int NidSubjectKeyIdentifier = 82;
constexpr int NidSubjectAltName = 85;
constexpr int NidBasicConstraints = 87;
constexpr int NidAuthorityKeyIdentifier = 90;
constexpr int NidNameConstraints = 666;
constexpr int NidNetscapeCertType = 71;

constexpr int HandledCriticalExtensions[] = {71, 82, 83, 85, 86, 87, 89, 90, 103, 126, 177, 401, 666, 747, 748};

constexpr LookupMethod FileMethod{"Load file into cache"};

using VerifyCallback = int(APS5_VABI*)(int ok, StoreCtx* ctx);
using VerifyFunction = int(APS5_VABI*)(StoreCtx* ctx);

struct ParamId {
    std::vector<std::string> hosts;
    unsigned hostFlags = 0;
    std::vector<unsigned char> ip;
};

struct StoreObject {
    Store store{};
    std::mutex mutex;
};

struct CtxState {
    VerifyParam* param = nullptr;
    std::unordered_map<int, void*> exData;
    Stack* chain = nullptr;
};

std::mutex& Mutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<const Store*, std::unique_ptr<StoreObject>>& Stores() {
    static std::unordered_map<const Store*, std::unique_ptr<StoreObject>> stores;
    return stores;
}

std::unordered_map<const StoreCtx*, CtxState>& CtxStates() {
    static std::unordered_map<const StoreCtx*, CtxState> states;
    return states;
}

std::unordered_map<const VerifyParam*, std::unique_ptr<ParamId>>& ParamIds() {
    static std::unordered_map<const VerifyParam*, std::unique_ptr<ParamId>> ids;
    return ids;
}

VerifyParam* NewParam() {
    auto* param = new VerifyParam{};
    param->depth = -1;
    auto id = std::make_unique<ParamId>();
    param->id = id.get();
    std::lock_guard lock(Mutex());
    ParamIds().emplace(param, std::move(id));
    return param;
}

ParamId& Id(VerifyParam* param, const char* function) {
    if (!param) throw std::invalid_argument(std::string(function) + ": null X509_VERIFY_PARAM");
    std::lock_guard lock(Mutex());
    const auto found = ParamIds().find(param);
    if (found == ParamIds().end()) throw std::invalid_argument(std::string(function) + ": X509_VERIFY_PARAM not created by this library");
    return *found->second;
}

void FreeParam(VerifyParam* param) {
    if (!param) return;
    {
        std::lock_guard lock(Mutex());
        ParamIds().erase(param);
    }
    delete param;
}

Store& RequireStore(Store* store, const char* function) {
    if (!store) throw std::invalid_argument(std::string(function) + ": null X509_STORE");
    std::lock_guard lock(Mutex());
    if (!Stores().count(store)) throw std::invalid_argument(std::string(function) + ": X509_STORE not created by X509_STORE_new");
    return *store;
}

std::mutex& StoreMutex(const Store* store) {
    std::lock_guard lock(Mutex());
    return Stores().at(store)->mutex;
}

StoreCtx& RequireCtx(StoreCtx* ctx, const char* function) {
    if (!ctx) throw std::invalid_argument(std::string(function) + ": null X509_STORE_CTX");
    return *ctx;
}

const StoreCtx& RequireCtx(const StoreCtx* ctx, const char* function) {
    if (!ctx) throw std::invalid_argument(std::string(function) + ": null X509_STORE_CTX");
    return *ctx;
}

void ReleaseChain(Stack* chain) {
    if (!chain) return;
    for (int index = 0; index < chain->num; ++index) FreeX509(reinterpret_cast<X509Object*>(chain->data[index]));
    FreeHeapStack(chain);
}

void ReleaseCtxState(const StoreCtx* ctx) {
    CtxState state;
    {
        std::lock_guard lock(Mutex());
        const auto found = CtxStates().find(ctx);
        if (found == CtxStates().end()) return;
        state = std::move(found->second);
        CtxStates().erase(found);
    }
    ReleaseChain(state.chain);
    FreeParam(state.param);
}

std::string Lower(std::string_view text) {
    std::string out(text);
    for (auto& character : out) {
        if (character >= 'A' && character <= 'Z') character = static_cast<char>(character - 'A' + 'a');
    }
    return out;
}

bool IsStringType(int type) {
    switch (type) {
    case 12:
    case 18:
    case 19:
    case 20:
    case 22:
    case 26:
    case 27:
    case 28:
    case 30:
        return true;
    default:
        return false;
    }
}

std::string Canonical(const Asn1String& value) {
    std::string text;
    if (value.type == 30) {
        for (int index = 0; index + 1 < value.length; index += 2) {
            const unsigned code = static_cast<unsigned>(value.data[index]) << 8 | value.data[index + 1];
            text += code < 0x80 ? static_cast<char>(code) : '\x80';
            if (code >= 0x80) text += std::to_string(code);
        }
    } else if (value.type == 28) {
        for (int index = 0; index + 3 < value.length; index += 4) {
            const unsigned code = static_cast<unsigned>(value.data[index + 2]) << 8 | value.data[index + 3];
            text += code < 0x80 && !value.data[index] && !value.data[index + 1] ? static_cast<char>(code) : '\x80';
        }
    } else {
        text.assign(reinterpret_cast<const char*>(value.data), static_cast<std::size_t>(value.length));
    }
    std::string out;
    bool space = false;
    for (const char character : text) {
        const bool blank = character == ' ' || character == '\t' || character == '\n' || character == '\r' || character == '\f' || character == '\v';
        if (blank) {
            space = !out.empty();
            continue;
        }
        if (space) out += ' ';
        space = false;
        out += character >= 'A' && character <= 'Z' ? static_cast<char>(character - 'A' + 'a') : character;
    }
    return out;
}

bool SameName(const Name* a, const Name* b) {
    if (!a || !b) return false;
    const Stack& first = *a->entries;
    const Stack& second = *b->entries;
    if (first.num != second.num) return false;
    for (int index = 0; index < first.num; ++index) {
        const auto& left = *reinterpret_cast<const NameEntry*>(first.data[index]);
        const auto& right = *reinterpret_cast<const NameEntry*>(second.data[index]);
        if (left.set != right.set) return false;
        if (left.object->length != right.object->length || std::memcmp(left.object->data, right.object->data, static_cast<std::size_t>(left.object->length)) != 0) return false;
        const auto& x = *left.value;
        const auto& y = *right.value;
        if (IsStringType(x.type) && IsStringType(y.type)) {
            if (Canonical(x) != Canonical(y)) return false;
        } else if (x.type != y.type || x.length != y.length || std::memcmp(x.data, y.data, static_cast<std::size_t>(x.length)) != 0) {
            return false;
        }
    }
    return true;
}

const Extension* FindExtension(const X509Object& x, int nid) {
    const Stack* extensions = x.info.extensions;
    if (!extensions) return nullptr;
    for (int index = 0; index < extensions->num; ++index) {
        const auto* extension = reinterpret_cast<const Extension*>(extensions->data[index]);
        if (extension->object->nid == nid) return extension;
    }
    return nullptr;
}

bool KeyUsage(const X509Object& x, int bit, bool& present) {
    present = false;
    const auto* extension = FindExtension(x, NidKeyUsage);
    if (!extension) return true;
    auto* cursor = extension->value->data;
    const auto* end = cursor + extension->value->length;
    std::size_t length = 0;
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_BIT_STRING) != 0 || length < 1) return false;
    present = true;
    const std::size_t byte = 1 + static_cast<std::size_t>(bit / 8);
    if (byte >= length) return false;
    return (cursor[byte] & (0x80 >> (bit % 8))) != 0;
}

struct BasicConstraints {
    bool present = false;
    bool ca = false;
    long long pathLength = -1;
};

bool NetscapeCa(const X509Object& x) {
    const auto* extension = FindExtension(x, NidNetscapeCertType);
    if (!extension) return false;
    auto* cursor = extension->value->data;
    const auto* end = cursor + extension->value->length;
    std::size_t length = 0;
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_BIT_STRING) != 0 || length < 2) return false;
    return (cursor[1] & 0x07) != 0;
}

BasicConstraints Constraints(const X509Object& x) {
    BasicConstraints result;
    const auto* extension = FindExtension(x, NidBasicConstraints);
    if (!extension) return result;
    result.present = true;
    auto* cursor = extension->value->data;
    const auto* end = cursor + extension->value->length;
    std::size_t length = 0;
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) return result;
    const auto* stop = cursor + length;
    if (cursor < stop && *cursor == MBEDTLS_ASN1_BOOLEAN) {
        int value = 0;
        if (mbedtls_asn1_get_bool(&cursor, stop, &value) != 0) return result;
        result.ca = value != 0;
    }
    if (cursor < stop) {
        int value = 0;
        if (mbedtls_asn1_get_int(&cursor, stop, &value) == 0 && value >= 0) result.pathLength = value;
    }
    return result;
}

bool OctetExtension(const X509Object& x, int nid, std::vector<unsigned char>& out) {
    const auto* extension = FindExtension(x, nid);
    if (!extension) return false;
    auto* cursor = extension->value->data;
    const auto* end = cursor + extension->value->length;
    std::size_t length = 0;
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_OCTET_STRING) != 0) return false;
    out.assign(cursor, cursor + length);
    return true;
}

int CheckIssued(const X509Object& issuer, const X509Object& subject) {
    if (!SameName(subject.info.issuer, issuer.info.subject)) return VErrSubjectIssuerMismatch;
    if (const auto* akid = FindExtension(subject, NidAuthorityKeyIdentifier)) {
        auto* cursor = akid->value->data;
        const auto* end = cursor + akid->value->length;
        std::size_t length = 0;
        if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) == 0) {
            const auto* stop = cursor + length;
            while (cursor < stop) {
                const int tag = *cursor;
                std::size_t size = 0;
                if (mbedtls_asn1_get_tag(&cursor, stop, &size, tag) != 0) break;
                if (tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0)) {
                    std::vector<unsigned char> skid;
                    if (OctetExtension(issuer, NidSubjectKeyIdentifier, skid) && (skid.size() != size || std::memcmp(skid.data(), cursor, size) != 0))
                        return VErrAkidSkidMismatch;
                } else if (tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 2)) {
                    std::size_t skip = 0;
                    while (skip + 1 < size && cursor[skip] == 0) ++skip;
                    const auto* serial = issuer.info.serialNumber;
                    if (static_cast<std::size_t>(serial->length) != size - skip || std::memcmp(serial->data, cursor + skip, size - skip) != 0)
                        return VErrAkidIssuerSerialMismatch;
                } else if (tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 1)) {
                    auto* names = cursor;
                    const auto* namesEnd = cursor + size;
                    bool matched = false;
                    bool sawDirectory = false;
                    while (names < namesEnd) {
                        const int nameTag = *names;
                        std::size_t nameSize = 0;
                        if (mbedtls_asn1_get_tag(&names, namesEnd, &nameSize, nameTag) != 0) break;
                        if (nameTag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 4)) {
                            sawDirectory = true;
                            Arena scratch;
                            mbedtls_x509_buf raw{};
                            raw.tag = MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE;
                            raw.p = names;
                            raw.len = nameSize;
                            try {
                                if (SameName(NewName(scratch, raw), issuer.info.issuer)) matched = true;
                            } catch (const std::runtime_error&) {
                            }
                        }
                        names += nameSize;
                    }
                    if (sawDirectory && !matched) return VErrAkidIssuerSerialMismatch;
                }
                cursor += size;
            }
        }
    }
    bool present = false;
    if (!KeyUsage(issuer, 5, present) && present) return VErrKeyusageNoCertsign;
    return VOk;
}

bool SelfSigned(const X509Object& x) {
    return CheckIssued(x, x) == VOk;
}

bool SignatureValid(const X509Object& subject, X509Object& issuer) {
    const auto md = subject.crt.MBEDTLS_PRIVATE(sig_md);
    const auto* info = mbedtls_md_info_from_type(md);
    if (!info) return false;
    unsigned char hash[MBEDTLS_MD_MAX_SIZE];
    if (mbedtls_md(info, subject.crt.tbs.p, subject.crt.tbs.len, hash) != 0) return false;
    return mbedtls_pk_verify_ext(subject.crt.MBEDTLS_PRIVATE(sig_pk), subject.crt.MBEDTLS_PRIVATE(sig_opts), &issuer.crt.pk, md, hash, mbedtls_md_get_size(info),
               subject.crt.MBEDTLS_PRIVATE(sig).p, subject.crt.MBEDTLS_PRIVATE(sig).len) == 0;
}

std::int64_t DaysFromCivil(std::int64_t year, unsigned month, unsigned day) {
    year -= month <= 2;
    const std::int64_t era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<std::int64_t>(doe) - 719468;
}

std::int64_t Seconds(const mbedtls_x509_time& time) {
    return DaysFromCivil(time.year, static_cast<unsigned>(time.mon), static_cast<unsigned>(time.day)) * 86400 + time.hour * 3600 + time.min * 60 + time.sec;
}

bool LabelMatch(std::string_view pattern, std::string_view label, bool allowPartial) {
    const auto star = pattern.find('*');
    if (star == std::string_view::npos) return pattern == label;
    if (pattern.size() == 1) return !label.empty();
    if (!allowPartial) return false;
    const auto prefix = pattern.substr(0, star);
    const auto suffix = pattern.substr(star + 1);
    if (suffix.find('*') != std::string_view::npos) return false;
    return label.size() >= prefix.size() + suffix.size() && label.substr(0, prefix.size()) == prefix && label.substr(label.size() - suffix.size()) == suffix;
}

std::vector<std::string_view> Labels(std::string_view name) {
    std::vector<std::string_view> labels;
    std::size_t start = 0;
    while (true) {
        const auto dot = name.find('.', start);
        labels.push_back(name.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start));
        if (dot == std::string_view::npos) break;
        start = dot + 1;
    }
    return labels;
}

bool HostMatches(const std::string& certName, const std::string& wanted, unsigned flags) {
    const std::string pattern = Lower(certName);
    const std::string host = Lower(wanted);
    if (pattern.empty() || host.empty()) return false;
    if (host[0] == '.') {
        if (pattern.size() <= host.size() || pattern.compare(pattern.size() - host.size(), host.size(), host) != 0) return false;
        if (flags & CheckFlagSingleLabelSubdomains) return pattern.substr(0, pattern.size() - host.size()).find('.') == std::string::npos;
        return true;
    }
    if (pattern.find('*') == std::string::npos || (flags & CheckFlagNoWildcards)) return pattern == host;
    const auto patternLabels = Labels(pattern);
    const auto hostLabels = Labels(host);
    if (patternLabels[0].find('*') == std::string_view::npos || patternLabels.size() < 3) return pattern == host;
    for (std::size_t index = 1; index < patternLabels.size(); ++index) {
        if (patternLabels[index].find('*') != std::string_view::npos || patternLabels[index].empty()) return false;
    }
    const bool whole = patternLabels[0] == "*";
    const bool multi = whole && (flags & CheckFlagMultiLabelWildcards);
    if (hostLabels.size() < patternLabels.size() || (!multi && hostLabels.size() != patternLabels.size())) return false;
    const std::size_t skip = hostLabels.size() - patternLabels.size();
    for (std::size_t index = 1; index < patternLabels.size(); ++index) {
        if (patternLabels[index] != hostLabels[index + skip]) return false;
    }
    if (multi) {
        for (std::size_t index = 0; index <= skip; ++index) {
            if (hostLabels[index].empty()) return false;
        }
        return true;
    }
    return LabelMatch(patternLabels[0], hostLabels[0], !(flags & CheckFlagNoPartialWildcards));
}

struct AltNames {
    std::vector<std::string> dns;
    std::vector<std::vector<unsigned char>> ips;
};

AltNames ReadAltNames(const X509Object& x) {
    AltNames names;
    const auto* extension = FindExtension(x, NidSubjectAltName);
    if (!extension) return names;
    auto* cursor = extension->value->data;
    const auto* end = cursor + extension->value->length;
    std::size_t length = 0;
    if (mbedtls_asn1_get_tag(&cursor, end, &length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) return names;
    while (cursor < end) {
        const int tag = *cursor;
        std::size_t size = 0;
        if (mbedtls_asn1_get_tag(&cursor, end, &size, tag) != 0) break;
        if (tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 2)) names.dns.emplace_back(reinterpret_cast<const char*>(cursor), size);
        if (tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 7)) names.ips.emplace_back(cursor, cursor + size);
        cursor += size;
    }
    return names;
}

bool CheckHost(const X509Object& x, const std::string& host, unsigned flags) {
    const auto names = ReadAltNames(x);
    for (const auto& dns : names.dns) {
        if (dns.find('\0') == std::string::npos && HostMatches(dns, host, flags)) return true;
    }
    if (!names.dns.empty() && !(flags & CheckFlagAlwaysCheckSubject)) return false;
    const Stack& entries = *x.info.subject->entries;
    for (int index = entries.num - 1; index >= 0; --index) {
        const auto& entry = *reinterpret_cast<const NameEntry*>(entries.data[index]);
        if (entry.object->nid != NidCommonName) continue;
        if (!IsStringType(entry.value->type) || entry.value->type == 30 || entry.value->type == 28)
            throw std::invalid_argument("X509_verify_cert: a BMP or Universal commonName is converted differently between LibreSSL versions");
        const std::string value(reinterpret_cast<const char*>(entry.value->data), static_cast<std::size_t>(entry.value->length));
        return value.find('\0') == std::string::npos && HostMatches(value, host, flags);
    }
    return false;
}

bool ParseIpv4(std::string_view text, unsigned char* out) {
    int parts = 0;
    std::size_t index = 0;
    while (parts < 4) {
        if (index >= text.size() || text[index] < '0' || text[index] > '9') return false;
        unsigned value = 0;
        std::size_t digits = 0;
        while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
            value = value * 10 + static_cast<unsigned>(text[index++] - '0');
            if (++digits > 3 || value > 255) return false;
        }
        out[parts++] = static_cast<unsigned char>(value);
        if (parts < 4) {
            if (index >= text.size() || text[index] != '.') return false;
            ++index;
        }
    }
    return index == text.size();
}

bool ParseIpv6(std::string_view text, unsigned char* out) {
    std::vector<std::uint16_t> head, tail;
    bool compressed = false;
    std::size_t index = 0;
    if (text.substr(0, 2) == "::") {
        compressed = true;
        index = 2;
    } else if (!text.empty() && text[0] == ':') {
        return false;
    }
    auto* current = compressed ? &tail : &head;
    while (index < text.size()) {
        const auto colon = text.find(':', index);
        const auto group = text.substr(index, colon == std::string_view::npos ? std::string_view::npos : colon - index);
        if (group.find('.') != std::string_view::npos && colon == std::string_view::npos) {
            unsigned char v4[4];
            if (!ParseIpv4(group, v4)) return false;
            current->push_back(static_cast<std::uint16_t>(v4[0] << 8 | v4[1]));
            current->push_back(static_cast<std::uint16_t>(v4[2] << 8 | v4[3]));
            index = text.size();
            break;
        }
        if (group.empty() || group.size() > 4) return false;
        unsigned value = 0;
        for (const char character : group) {
            int digit = -1;
            if (character >= '0' && character <= '9') digit = character - '0';
            else if (character >= 'a' && character <= 'f') digit = character - 'a' + 10;
            else if (character >= 'A' && character <= 'F') digit = character - 'A' + 10;
            if (digit < 0) return false;
            value = value << 4 | static_cast<unsigned>(digit);
        }
        current->push_back(static_cast<std::uint16_t>(value));
        if (colon == std::string_view::npos) {
            index = text.size();
            break;
        }
        index = colon + 1;
        if (index < text.size() && text[index] == ':') {
            if (compressed) return false;
            compressed = true;
            current = &tail;
            ++index;
        } else if (index == text.size()) {
            return false;
        }
    }
    const std::size_t total = head.size() + tail.size();
    if ((compressed && total > 7) || (!compressed && total != 8)) return false;
    std::vector<std::uint16_t> groups = head;
    groups.resize(8 - tail.size(), 0);
    groups.insert(groups.end(), tail.begin(), tail.end());
    for (std::size_t group = 0; group < 8; ++group) {
        out[group * 2] = static_cast<unsigned char>(groups[group] >> 8);
        out[group * 2 + 1] = static_cast<unsigned char>(groups[group]);
    }
    return true;
}

bool InList(const Stack* stack, const X509Object* x) {
    if (!stack) return false;
    for (int index = 0; index < stack->num; ++index) {
        if (reinterpret_cast<const X509Object*>(stack->data[index]) == x) return true;
    }
    return false;
}

bool SameCertificate(const X509Object& a, const X509Object& b) {
    return a.crt.raw.len == b.crt.raw.len && std::memcmp(a.crt.raw.p, b.crt.raw.p, a.crt.raw.len) == 0;
}

class Verifier {
public:
    explicit Verifier(StoreCtx& ctx) : ctx_(ctx), store_(ctx.ctx), param_(*ctx.param) {
        if (store_) {
            std::lock_guard lock(StoreMutex(store_));
            const Stack* objects = store_->objs;
            for (int index = 0; index < objects->num; ++index) {
                const auto* entry = reinterpret_cast<const StoreEntry*>(objects->data[index]);
                if (entry->type == LuX509) {
                    auto* cert = static_cast<X509Object*>(entry->data);
                    X509UpRef(cert);
                    trusted_.push_back(cert);
                } else {
                    auto* crl = static_cast<X509Crl*>(entry->data);
                    CrlUpRef(crl);
                    crls_.push_back(crl);
                }
            }
        }
        if (ctx.crls) {
            for (int index = 0; index < ctx.crls->num; ++index) {
                auto* crl = reinterpret_cast<X509Crl*>(ctx.crls->data[index]);
                CrlUpRef(crl);
                crls_.push_back(crl);
            }
        }
        now_ = (param_.flags & FlagUseCheckTime) ? param_.checkTime : static_cast<std::int64_t>(std::time(nullptr));
    }

    ~Verifier() {
        for (auto* cert : trusted_) FreeX509(cert);
        for (auto* crl : crls_) FreeCrl(crl);
    }

    Verifier(const Verifier&) = delete;
    Verifier& operator=(const Verifier&) = delete;

    int Run() {
        if (param_.flags & (FlagPolicyCheck | FlagExplicitPolicy | FlagInhibitAny | FlagInhibitMap))
            throw std::invalid_argument("X509_verify_cert: policy checking is not supported");
        if (param_.flags & (FlagAllowProxyCerts | FlagExtendedCrlSupport | FlagUseDeltas))
            throw std::invalid_argument("X509_verify_cert: proxy certificates and extended or delta CRLs are not supported");
        if (param_.purpose || param_.trust) throw std::invalid_argument("X509_verify_cert: purpose and trust settings are not supported");
        BuildChain();
        PublishChain();
        if (!CheckTrust()) return 0;
        if (!CheckExtensions()) return 0;
        if (!CheckIdentity()) return 0;
        if (!CheckRevocation()) return 0;
        return CheckSignatures();
    }

private:
    bool Fail(int error, int depth, X509Object* cert) {
        ctx_.error = error;
        ctx_.errorDepth = depth;
        ctx_.currentCert = cert;
        return Callback(0) != 0;
    }

    int Callback(int ok) {
        if (ctx_.verifyCb) return reinterpret_cast<VerifyCallback>(ctx_.verifyCb)(ok, &ctx_);
        return ok;
    }

    X509Object* TrustedIssuer(const X509Object& x) {
        for (auto* candidate : trusted_) {
            if (CheckIssued(*candidate, x) == VOk && !InChain(candidate)) return candidate;
        }
        return nullptr;
    }

    X509Object* UntrustedIssuer(const X509Object& x) {
        if (!ctx_.untrusted) return nullptr;
        for (int index = 0; index < ctx_.untrusted->num; ++index) {
            auto* candidate = reinterpret_cast<X509Object*>(ctx_.untrusted->data[index]);
            if (candidate && CheckIssued(*candidate, x) == VOk && !InChain(candidate)) return candidate;
        }
        return nullptr;
    }

    bool InChain(const X509Object* x) const {
        return std::find(chain_.begin(), chain_.end(), x) != chain_.end();
    }

    bool IsTrusted(const X509Object& x) const {
        for (const auto* candidate : trusted_) {
            if (candidate == &x || SameCertificate(*candidate, x)) return true;
        }
        return false;
    }

    void BuildChain() {
        const int depth = param_.depth < 0 ? DefaultDepth : param_.depth;
        chain_.push_back(ctx_.cert);
        untrustedCount_ = IsTrusted(*ctx_.cert) ? 0 : 1;
        bool reachedTrusted = untrustedCount_ == 0;
        while (true) {
            auto& top = *chain_.back();
            if (SelfSigned(top)) break;
            if (static_cast<int>(chain_.size()) > depth + 1) {
                tooLong_ = true;
                break;
            }
            X509Object* issuer = TrustedIssuer(top);
            if (!issuer && !reachedTrusted) issuer = UntrustedIssuer(top);
            if (!issuer) break;
            chain_.push_back(issuer);
            if (IsTrusted(*issuer)) reachedTrusted = true;
            else untrustedCount_ = static_cast<int>(chain_.size());
        }
    }

    void PublishChain() {
        auto* stack = NewHeapStack();
        for (auto* cert : chain_) {
            X509UpRef(cert);
            HeapStackPush(stack, cert);
        }
        Stack* old = nullptr;
        {
            std::lock_guard lock(Mutex());
            auto& state = CtxStates()[&ctx_];
            old = state.chain;
            state.chain = stack;
        }
        ReleaseChain(old);
        ctx_.chain = stack;
        ctx_.lastUntrusted = untrustedCount_;
    }

    bool CheckTrust() {
        const int last = static_cast<int>(chain_.size()) - 1;
        auto* top = chain_.back();
        if (tooLong_ && !Fail(VErrCertChainTooLong, last, top)) return false;
        const bool trusted = untrustedCount_ < static_cast<int>(chain_.size());
        if (trusted) {
            if (!SelfSigned(*top) && !(param_.flags & FlagPartialChain)) return Fail(VErrUnableToGetIssuerCert, last, top);
            return true;
        }
        if (SelfSigned(*top)) return Fail(last == 0 ? VErrDepthZeroSelfSignedCert : VErrSelfSignedCertInChain, last, top);
        return Fail(VErrUnableToGetIssuerCertLocally, last, top);
    }

    bool CheckExtensions() {
        long long nonSelfIssued = 0;
        for (std::size_t index = 0; index < chain_.size(); ++index) {
            auto* x = chain_[index];
            const int depth = static_cast<int>(index);
            if (FindExtension(*x, NidNameConstraints)) throw std::invalid_argument("X509_verify_cert: name constraints are not supported");
            if (!(param_.flags & FlagIgnoreCritical) && x->info.extensions) {
                const Stack* extensions = x->info.extensions;
                for (int item = 0; item < extensions->num; ++item) {
                    const auto* extension = reinterpret_cast<const Extension*>(extensions->data[item]);
                    if (extension->critical <= 0) continue;
                    const int nid = extension->object->nid;
                    if (std::find(std::begin(HandledCriticalExtensions), std::end(HandledCriticalExtensions), nid) == std::end(HandledCriticalExtensions)) {
                        if (!Fail(VErrUnhandledCriticalExtension, depth, x)) return false;
                        break;
                    }
                }
            }
            if (index > 0) {
                const auto constraints = Constraints(*x);
                bool present = false;
                const bool certSign = KeyUsage(*x, 5, present);
                int caKind = 0;
                if (constraints.present) caKind = constraints.ca ? 1 : 0;
                else if (x->crt.version == 1 && SelfSigned(*x)) caKind = 3;
                else if (present && certSign) caKind = 4;
                else if (NetscapeCa(*x)) caKind = 5;
                if (caKind == 0 || ((param_.flags & FlagX509Strict) && caKind != 1)) {
                    if (!Fail(VErrInvalidCa, depth, x)) return false;
                } else if (index > 1 && constraints.pathLength >= 0 && nonSelfIssued > constraints.pathLength + 1) {
                    if (!Fail(VErrPathLengthExceeded, depth, x)) return false;
                }
            }
            if (!SameName(x->info.subject, x->info.issuer)) ++nonSelfIssued;
        }
        return true;
    }

    bool CheckIdentity() {
        auto& id = Id(&param_, "X509_verify_cert");
        auto* leaf = chain_.front();
        if (!id.hosts.empty()) {
            bool matched = false;
            for (const auto& host : id.hosts) {
                if (CheckHost(*leaf, host, id.hostFlags)) matched = true;
            }
            if (!matched && !Fail(VErrHostnameMismatch, 0, leaf)) return false;
        }
        if (!id.ip.empty()) {
            bool matched = false;
            for (const auto& ip : ReadAltNames(*leaf).ips) {
                if (ip == id.ip) matched = true;
            }
            if (!matched && !Fail(VErrIpAddressMismatch, 0, leaf)) return false;
        }
        return true;
    }

    bool CheckRevocation() {
        if (!(param_.flags & (FlagCrlCheck | FlagCrlCheckAll))) return true;
        const std::size_t count = (param_.flags & FlagCrlCheckAll) ? chain_.size() : 1;
        for (std::size_t index = 0; index < count; ++index) {
            auto* x = chain_[index];
            if (index + 1 == chain_.size() && index > 0 && SelfSigned(*x)) break;
            X509Object* issuer = index + 1 < chain_.size() ? chain_[index + 1] : (SelfSigned(*x) ? x : nullptr);
            if (!CheckCrl(*x, issuer, static_cast<int>(index))) return false;
        }
        return true;
    }

    bool CheckCrl(X509Object& x, X509Object* issuer, int depth) {
        X509Crl* found = nullptr;
        for (auto* crl : crls_) {
            Arena scratch;
            Name* name = nullptr;
            try {
                name = NewName(scratch, crl->parsed.issuer_raw);
            } catch (const std::runtime_error&) {
                continue;
            }
            if (SameName(name, x.info.issuer)) {
                found = crl;
                break;
            }
        }
        if (!found) return Fail(VErrUnableToGetCrl, depth, &x);
        ctx_.currentCrl = found;
        const auto& crl = found->parsed;
        if (!issuer) {
            if (!Fail(VErrUnableToGetCrl, depth, &x)) return false;
        } else {
            bool present = false;
            if (!KeyUsage(*issuer, 6, present) && present && !Fail(VErrKeyusageNoCrlSign, depth, &x)) return false;
            const auto* info = mbedtls_md_info_from_type(crl.MBEDTLS_PRIVATE(sig_md));
            unsigned char hash[MBEDTLS_MD_MAX_SIZE];
            const bool valid = info && mbedtls_md(info, crl.tbs.p, crl.tbs.len, hash) == 0 &&
                mbedtls_pk_verify_ext(crl.MBEDTLS_PRIVATE(sig_pk), crl.MBEDTLS_PRIVATE(sig_opts), &issuer->crt.pk, crl.MBEDTLS_PRIVATE(sig_md), hash,
                    mbedtls_md_get_size(info), crl.MBEDTLS_PRIVATE(sig).p, crl.MBEDTLS_PRIVATE(sig).len) == 0;
            if (!valid && !Fail(VErrCrlSignatureFailure, depth, &x)) return false;
        }
        if (!(param_.flags & FlagNoCheckTime)) {
            if (Seconds(crl.this_update) > now_ && !Fail(VErrCrlNotYetValid, depth, &x)) return false;
            if (crl.next_update.year && Seconds(crl.next_update) < now_ && !Fail(VErrCrlHasExpired, depth, &x)) return false;
        }
        for (const auto* entry = &crl.entry; entry && entry->serial.len; entry = entry->next) {
            std::size_t skip = 0;
            while (skip + 1 < entry->serial.len && entry->serial.p[skip] == 0) ++skip;
            std::size_t own = 0;
            while (own + 1 < x.crt.serial.len && x.crt.serial.p[own] == 0) ++own;
            if (entry->serial.len - skip == x.crt.serial.len - own && std::memcmp(entry->serial.p + skip, x.crt.serial.p + own, entry->serial.len - skip) == 0) {
                if (!Fail(VErrCertRevoked, depth, &x)) return false;
                break;
            }
        }
        ctx_.currentCrl = nullptr;
        return true;
    }

    int CheckSignatures() {
        int n = static_cast<int>(chain_.size()) - 1;
        X509Object* issuer = chain_[static_cast<std::size_t>(n)];
        X509Object* subject = nullptr;
        if (SelfSigned(*issuer)) {
            subject = issuer;
        } else {
            if (n == 0) {
                ctx_.currentIssuer = nullptr;
                if (!Fail(VErrUnableToVerifyLeafSignature, 0, issuer)) return 0;
                subject = issuer;
            } else {
                --n;
                subject = chain_[static_cast<std::size_t>(n)];
            }
        }
        while (n >= 0) {
            ctx_.errorDepth = n;
            ctx_.currentIssuer = issuer;
            if (subject != issuer || (param_.flags & FlagCheckSsSignature)) {
                if (!SignatureValid(*subject, *issuer) && !Fail(VErrCertSignatureFailure, n, subject)) return 0;
            }
            if (!(param_.flags & FlagNoCheckTime)) {
                if (Seconds(subject->crt.valid_from) > now_ && !Fail(VErrCertNotYetValid, n, subject)) return 0;
                if (Seconds(subject->crt.valid_to) < now_ && !Fail(VErrCertHasExpired, n, subject)) return 0;
            }
            ctx_.currentCert = subject;
            ctx_.errorDepth = n;
            if (!Callback(1)) return 0;
            --n;
            if (n >= 0) {
                issuer = subject;
                subject = chain_[static_cast<std::size_t>(n)];
            }
        }
        return 1;
    }

    StoreCtx& ctx_;
    Store* store_;
    VerifyParam& param_;
    std::vector<X509Object*> trusted_;
    std::vector<X509Crl*> crls_;
    std::vector<X509Object*> chain_;
    int untrustedCount_ = 0;
    bool tooLong_ = false;
    std::int64_t now_ = 0;
};

int AddEntry(Store* store, int type, void* data, int function) {
    auto& target = RequireStore(store, function == X509FunctionStoreAddCert ? "X509_STORE_add_cert" : "X509_STORE_add_crl");
    if (!data) return 0;
    std::lock_guard lock(StoreMutex(&target));
    Stack* objects = target.objs;
    for (int index = 0; index < objects->num; ++index) {
        const auto* entry = reinterpret_cast<const StoreEntry*>(objects->data[index]);
        if (entry->type != type) continue;
        if (type == LuX509) {
            const auto& existing = *static_cast<const X509Object*>(entry->data);
            const auto& added = *static_cast<const X509Object*>(data);
            if (SameCertificate(existing, added)) {
                PushError(ErrLibX509, function, X509ReasonCertAlreadyInHashTable);
                return 0;
            }
            if (SameName(existing.info.subject, added.info.subject))
                throw std::invalid_argument("X509_STORE_add_cert: a second certificate with the same subject is handled differently between LibreSSL versions");
        } else {
            const auto& existing = *static_cast<const X509Crl*>(entry->data);
            const auto& added = *static_cast<const X509Crl*>(data);
            if (existing.parsed.raw.len == added.parsed.raw.len && std::memcmp(existing.parsed.raw.p, added.parsed.raw.p, added.parsed.raw.len) == 0) {
                PushError(ErrLibX509, function, X509ReasonCertAlreadyInHashTable);
                return 0;
            }
            if (existing.parsed.issuer_raw.len == added.parsed.issuer_raw.len &&
                std::memcmp(existing.parsed.issuer_raw.p, added.parsed.issuer_raw.p, added.parsed.issuer_raw.len) == 0)
                throw std::invalid_argument("X509_STORE_add_crl: a second CRL from the same issuer is handled differently between LibreSSL versions");
        }
    }
    if (type == LuX509) X509UpRef(static_cast<X509Object*>(data));
    else CrlUpRef(static_cast<X509Crl*>(data));
    HeapStackPush(objects, new StoreEntry{type, data});
    return 1;
}

}

}

using namespace LibreSsl;

extern "C" {

Store* APS5_VABI X509_STORE_new_nid_postfix() {
    auto object = std::make_unique<StoreObject>();
    auto& store = object->store;
    store.objs = NewHeapStack();
    store.getCertMethods = NewHeapStack();
    store.param = NewParam();
    store.references = 1;
    std::lock_guard lock(Mutex());
    Stores().emplace(&store, std::move(object));
    return &store;
}

void APS5_VABI X509_STORE_free_nid_postfix(Store* v) {
    if (!v) return;
    auto& store = RequireStore(v, __func__);
    if (std::atomic_ref<int>(store.references).fetch_sub(1) > 1) return;
    for (int index = 0; index < store.objs->num; ++index) {
        auto* entry = reinterpret_cast<StoreEntry*>(store.objs->data[index]);
        if (entry->type == LuX509) FreeX509(static_cast<X509Object*>(entry->data));
        else FreeCrl(static_cast<X509Crl*>(entry->data));
        delete entry;
    }
    FreeHeapStack(store.objs);
    for (int index = 0; index < store.getCertMethods->num; ++index) delete reinterpret_cast<Lookup*>(store.getCertMethods->data[index]);
    FreeHeapStack(store.getCertMethods);
    FreeParam(store.param);
    std::unique_ptr<StoreObject> owned;
    {
        std::lock_guard lock(Mutex());
        const auto found = Stores().find(v);
        owned = std::move(found->second);
        Stores().erase(found);
    }
}

int APS5_VABI X509_STORE_add_cert_nid_postfix(Store* ctx, X509Object* x) {
    return AddEntry(ctx, LuX509, x, X509FunctionStoreAddCert);
}

int APS5_VABI X509_STORE_add_crl_nid_postfix(Store* ctx, X509Crl* x) {
    return AddEntry(ctx, LuCrl, x, X509FunctionStoreAddCrl);
}

int APS5_VABI X509_STORE_set_flags_nid_postfix(Store* ctx, std::uint64_t flags) {
    auto& store = RequireStore(ctx, __func__);
    store.param->flags |= flags;
    return 1;
}

const LookupMethod* APS5_VABI X509_LOOKUP_file_nid_postfix() {
    return &FileMethod;
}

Lookup* APS5_VABI X509_STORE_add_lookup_nid_postfix(Store* v, const LookupMethod* m) {
    auto& store = RequireStore(v, __func__);
    if (m != &FileMethod) throw std::invalid_argument("X509_STORE_add_lookup: only X509_LOOKUP_file is supported");
    std::lock_guard lock(StoreMutex(&store));
    for (int index = 0; index < store.getCertMethods->num; ++index) {
        auto* lookup = reinterpret_cast<Lookup*>(store.getCertMethods->data[index]);
        if (lookup->method == m) return lookup;
    }
    auto* lookup = new Lookup{1, 0, m, nullptr, &store};
    HeapStackPush(store.getCertMethods, lookup);
    return lookup;
}

int APS5_VABI X509_load_crl_file_nid_postfix(Lookup* ctx, const char* file, int type) {
    if (!ctx || !ctx->storeCtx) throw std::invalid_argument("X509_load_crl_file: null lookup");
    if (!file) throw std::invalid_argument("X509_load_crl_file: null file name");
    if (type != FileTypePem && type != FileTypeAsn1) {
        PushError(ErrLibX509, X509FunctionLoadCrlFile, X509ReasonBadX509FileType);
        return 0;
    }
    auto* stream = fopen_nid_postfix(file, "rb");
    if (!stream) {
        PushError(ErrLibX509, X509FunctionLoadCrlFile, ErrReasonSysLib);
        return 0;
    }
    auto* bio = BIO_new_fp_nid_postfix(stream, 1);
    std::vector<unsigned char> bytes;
    try {
        bytes = BioReadAll(bio, __func__);
    } catch (...) {
        BIO_free_nid_postfix(bio);
        throw;
    }
    BIO_free_nid_postfix(bio);
    mbedtls_x509_crl parsed;
    mbedtls_x509_crl_init(&parsed);
    int status = 0;
    if (type == FileTypeAsn1) {
        const std::size_t length = DerElementLength(bytes.data(), bytes.size());
        status = length && length <= bytes.size() ? mbedtls_x509_crl_parse_der(&parsed, bytes.data(), length) : MBEDTLS_ERR_X509_INVALID_FORMAT;
    } else {
        bytes.push_back(0);
        status = mbedtls_x509_crl_parse(&parsed, bytes.data(), bytes.size());
    }
    std::vector<std::vector<unsigned char>> ders;
    if (status == 0) {
        for (const auto* crl = &parsed; crl && crl->raw.len; crl = crl->next) ders.emplace_back(crl->raw.p, crl->raw.p + crl->raw.len);
    }
    mbedtls_x509_crl_free(&parsed);
    if (status != 0 || ders.empty()) {
        PushError(ErrLibX509, X509FunctionLoadCrlFile, ErrReasonAsn1Lib);
        return 0;
    }
    int count = 0;
    for (const auto& der : ders) {
        auto* crl = NewCrl(der.data(), der.size());
        if (!crl) {
            PushError(ErrLibX509, X509FunctionLoadCrlFile, ErrReasonAsn1Lib);
            return 0;
        }
        const int added = X509_STORE_add_crl_nid_postfix(ctx->storeCtx, crl);
        FreeCrl(crl);
        if (!added) return 0;
        ++count;
    }
    return count;
}

int APS5_VABI X509_check_issued_nid_postfix(X509Object* issuer, X509Object* subject) {
    if (!issuer || !subject) throw std::invalid_argument("X509_check_issued: null certificate");
    return CheckIssued(*issuer, *subject);
}

int APS5_VABI X509_verify_cert_nid_postfix(StoreCtx* ctx) {
    auto& context = RequireCtx(ctx, __func__);
    if (!context.cert) {
        PushError(ErrLibX509, X509FunctionVerifyCert, X509ReasonNoCertSetForUsToVerify);
        return -1;
    }
    if (!context.param) throw std::logic_error("X509_verify_cert: the context was not initialized with X509_STORE_CTX_init");
    if (context.verify) return reinterpret_cast<VerifyFunction>(context.verify)(&context);
    if (context.getIssuer || context.checkIssued || context.checkRevocation || context.getCrl || context.checkCrl || context.certCrl || context.checkPolicy ||
        context.lookupCerts || context.lookupCrls || context.cleanup)
        throw std::invalid_argument("X509_verify_cert: custom store or context callbacks other than verify and verify_cb are not supported");
    context.error = VOk;
    context.errorDepth = 0;
    context.currentCert = nullptr;
    context.currentIssuer = nullptr;
    context.currentCrl = nullptr;
    Verifier verifier(context);
    const int result = verifier.Run();
    context.valid = result == 1;
    return result;
}

const char* APS5_VABI X509_verify_cert_error_string_nid_postfix(std::int64_t n) {
    switch (n) {
    case 0: return "ok";
    case 2: return "unable to get issuer certificate";
    case 3: return "unable to get certificate CRL";
    case 4: return "unable to decrypt certificate's signature";
    case 5: return "unable to decrypt CRL's signature";
    case 6: return "unable to decode issuer public key";
    case 7: return "certificate signature failure";
    case 8: return "CRL signature failure";
    case 9: return "certificate is not yet valid";
    case 10: return "certificate has expired";
    case 11: return "CRL is not yet valid";
    case 12: return "CRL has expired";
    case 13: return "format error in certificate's notBefore field";
    case 14: return "format error in certificate's notAfter field";
    case 15: return "format error in CRL's lastUpdate field";
    case 16: return "format error in CRL's nextUpdate field";
    case 17: return "out of memory";
    case 18: return "self signed certificate";
    case 19: return "self signed certificate in certificate chain";
    case 20: return "unable to get local issuer certificate";
    case 21: return "unable to verify the first certificate";
    case 22: return "certificate chain too long";
    case 23: return "certificate revoked";
    case 24: return "invalid CA certificate";
    case 25: return "path length constraint exceeded";
    case 26: return "unsupported certificate purpose";
    case 27: return "certificate not trusted";
    case 28: return "certificate rejected";
    case 29: return "subject issuer mismatch";
    case 30: return "authority and subject key identifier mismatch";
    case 31: return "authority and issuer serial number mismatch";
    case 32: return "key usage does not include certificate signing";
    case 41: return "invalid or inconsistent certificate extension";
    case 42: return "invalid or inconsistent certificate policy extension";
    case 43: return "no explicit policy";
    case 44: return "different CRL scope";
    case 45: return "unsupported extension feature";
    case 47: return "permitted subtree violation";
    case 48: return "excluded subtree violation";
    case 49: return "name constraints minimum and maximum not supported";
    case 50: return "application verification failure";
    case 51: return "unsupported name constraint type";
    case 52: return "unsupported or invalid name constraint syntax";
    case 54: return "CRL path validation error";
    default:
        throw std::invalid_argument("X509_verify_cert_error_string: the text for code " + std::to_string(n) + " is not documented");
    }
}

int APS5_VABI X509_STORE_CTX_get_error_nid_postfix(const StoreCtx* ctx) {
    return RequireCtx(ctx, __func__).error;
}

void APS5_VABI X509_STORE_CTX_set_error_nid_postfix(StoreCtx* ctx, int s) {
    RequireCtx(ctx, __func__).error = s;
}

int APS5_VABI X509_STORE_CTX_get_error_depth_nid_postfix(const StoreCtx* ctx) {
    return RequireCtx(ctx, __func__).errorDepth;
}

X509Object* APS5_VABI X509_STORE_CTX_get_current_cert_nid_postfix(const StoreCtx* ctx) {
    return RequireCtx(ctx, __func__).currentCert;
}

Stack* APS5_VABI X509_STORE_CTX_get_chain_nid_postfix(const StoreCtx* ctx) {
    return RequireCtx(ctx, __func__).chain;
}

Stack* APS5_VABI X509_STORE_CTX_get1_chain_nid_postfix(const StoreCtx* ctx) {
    const auto& context = RequireCtx(ctx, __func__);
    if (!context.chain) return nullptr;
    auto* copy = NewHeapStack();
    for (int index = 0; index < context.chain->num; ++index) {
        auto* cert = reinterpret_cast<X509Object*>(context.chain->data[index]);
        X509UpRef(cert);
        HeapStackPush(copy, cert);
    }
    return copy;
}

void* APS5_VABI X509_STORE_CTX_get_ex_data_nid_postfix(const StoreCtx* ctx, int idx) {
    RequireCtx(ctx, __func__);
    std::lock_guard lock(Mutex());
    const auto state = CtxStates().find(ctx);
    if (state == CtxStates().end()) return nullptr;
    const auto found = state->second.exData.find(idx);
    return found == state->second.exData.end() ? nullptr : found->second;
}

int APS5_VABI X509_VERIFY_PARAM_set1_host_nid_postfix(VerifyParam* param, const char* name, std::size_t namelen) {
    auto& id = Id(param, __func__);
    if (name && !namelen) namelen = std::strlen(name);
    if (name && std::memchr(name, 0, namelen)) return 0;
    id.hosts.clear();
    if (name && namelen) id.hosts.emplace_back(name, namelen);
    return 1;
}

void APS5_VABI X509_VERIFY_PARAM_set_hostflags_nid_postfix(VerifyParam* param, unsigned int flags) {
    Id(param, __func__).hostFlags = flags;
}

int APS5_VABI X509_VERIFY_PARAM_set1_ip_asc_nid_postfix(VerifyParam* param, const char* ipasc) {
    auto& id = Id(param, __func__);
    if (!ipasc) throw std::invalid_argument("X509_VERIFY_PARAM_set1_ip_asc: null address");
    const std::string_view text(ipasc);
    unsigned char address[16];
    if (text.find(':') != std::string_view::npos) {
        if (!ParseIpv6(text, address)) return 0;
        id.ip.assign(address, address + 16);
    } else {
        if (!ParseIpv4(text, address)) return 0;
        id.ip.assign(address, address + 4);
    }
    return 1;
}

int LibreSslX509StoreUpRef_nid_no_patch(Store* store) {
    std::atomic_ref<int>(RequireStore(store, "X509_STORE_up_ref").references).fetch_add(1);
    return 1;
}

StoreCtx* LibreSslX509StoreCtxNew_nid_no_patch() {
    return new (std::nothrow) StoreCtx{};
}

void LibreSslX509StoreCtxFree_nid_no_patch(StoreCtx* ctx) {
    if (!ctx) return;
    ReleaseCtxState(ctx);
    delete ctx;
}

int LibreSslX509StoreCtxInit_nid_no_patch(StoreCtx* ctx, Store* store, X509Object* x509, Stack* chain) {
    auto& context = RequireCtx(ctx, "X509_STORE_CTX_init");
    if (store) RequireStore(store, "X509_STORE_CTX_init");
    ReleaseCtxState(ctx);
    context = StoreCtx{};
    context.ctx = store;
    context.cert = x509;
    context.untrusted = chain;
    auto* param = NewParam();
    if (store) {
        const auto* source = store->param;
        param->flags = source->flags;
        param->depth = source->depth;
        param->checkTime = source->checkTime;
        param->purpose = source->purpose;
        param->trust = source->trust;
        auto& from = Id(const_cast<VerifyParam*>(source), "X509_STORE_CTX_init");
        auto& to = Id(param, "X509_STORE_CTX_init");
        to = from;
        context.verify = store->verify;
        context.verifyCb = store->verifyCb;
        context.getIssuer = store->getIssuer;
        context.checkIssued = store->checkIssued;
        context.checkRevocation = store->checkRevocation;
        context.getCrl = store->getCrl;
        context.checkCrl = store->checkCrl;
        context.certCrl = store->certCrl;
        context.lookupCerts = store->lookupCerts;
        context.lookupCrls = store->lookupCrls;
        context.cleanup = store->cleanup;
    }
    context.param = param;
    std::lock_guard lock(Mutex());
    CtxStates()[ctx].param = param;
    return 1;
}

void LibreSslX509StoreCtxSetExData_nid_no_patch(StoreCtx* ctx, int idx, void* data) {
    if (!ctx) throw std::invalid_argument("X509_STORE_CTX_set_ex_data: null X509_STORE_CTX");
    std::lock_guard lock(Mutex());
    CtxStates()[ctx].exData[idx] = data;
}

}

namespace LibreSsl {

VerifyParam* NewVerifyParam() {
    return NewParam();
}

void FreeVerifyParam(VerifyParam* param) {
    FreeParam(param);
}

int AddTrustedCertificate(Store* store, X509Object* x, const char* function) {
    auto& target = RequireStore(store, function);
    {
        std::lock_guard lock(StoreMutex(&target));
        const Stack* objects = target.objs;
        for (int index = 0; index < objects->num; ++index) {
            const auto* entry = reinterpret_cast<const StoreEntry*>(objects->data[index]);
            if (entry->type == LuX509 && SameCertificate(*static_cast<const X509Object*>(entry->data), *x)) return 0;
        }
    }
    return AddEntry(store, LuX509, x, X509FunctionStoreAddCert);
}

int VerifyPeerChain(Store* store, VerifyParam* overrides, X509Object* leaf, Stack* untrusted, void* verifyCallback, int sslIndex, void* ssl,
    CertVerifyCallback callback, void* argument, int& error) {
    std::unique_ptr<StoreCtx, void (*)(StoreCtx*)> ctx(LibreSslX509StoreCtxNew_nid_no_patch(), LibreSslX509StoreCtxFree_nid_no_patch);
    if (!ctx) throw std::bad_alloc();
    LibreSslX509StoreCtxInit_nid_no_patch(ctx.get(), store, leaf, untrusted);
    auto& param = *ctx->param;
    param.flags |= overrides->flags;
    if (overrides->depth >= 0) param.depth = overrides->depth;
    if (overrides->flags & FlagUseCheckTime) param.checkTime = overrides->checkTime;
    if (overrides->purpose) param.purpose = overrides->purpose;
    if (overrides->trust) param.trust = overrides->trust;
    const auto& from = Id(overrides, "SSL certificate verification");
    auto& to = Id(&param, "SSL certificate verification");
    if (!from.hosts.empty()) to.hosts = from.hosts;
    if (from.hostFlags) to.hostFlags = from.hostFlags;
    if (!from.ip.empty()) to.ip = from.ip;
    if (verifyCallback) ctx->verifyCb = verifyCallback;
    LibreSslX509StoreCtxSetExData_nid_no_patch(ctx.get(), sslIndex, ssl);
    const int result = callback ? callback(ctx.get(), argument) : X509_verify_cert_nid_postfix(ctx.get());
    error = ctx->error;
    return result;
}

}
