#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/include/Keys.hpp"
#include "prx/libSceLibreSsl/include/X509Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <mbedtls/asn1.h>

#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern "C" void APS5_VABI X509_free_nid_postfix(LibreSsl::X509Object* a);

namespace LibreSsl {

namespace {

constexpr int TypeBoolean = 1;
constexpr int TypeInteger = 2;
constexpr int TypeBitString = 3;
constexpr int TypeOctetString = 4;
constexpr int TypeNull = 5;
constexpr int TypeObject = 6;
constexpr int TypeEnumerated = 10;
constexpr int TypeSequence = 16;
constexpr int TypeSet = 17;
constexpr int TypeIa5String = 22;
constexpr std::int64_t FlagBitsLeft = 0x08;
constexpr int ObjectFlagDynamic = 0x01 | 0x04 | 0x08;

std::mutex& Mutex() {
    static std::mutex mutex;
    return mutex;
}

struct HeapStack {
    Stack stack{};
    std::vector<char*> items;
};

std::unordered_map<const Stack*, std::unique_ptr<HeapStack>>& HeapStacks() {
    static std::unordered_map<const Stack*, std::unique_ptr<HeapStack>> stacks;
    return stacks;
}

std::unordered_set<const X509Crl*>& Crls() {
    static std::unordered_set<const X509Crl*> crls;
    return crls;
}

bool Element(unsigned char*& cursor, const unsigned char* end, int tag, std::size_t& length) {
    return mbedtls_asn1_get_tag(&cursor, end, &length, tag) == 0;
}

void Malformed(const char* what) {
    throw std::runtime_error(std::string("X509: malformed ") + what);
}

}

void X509UpRef(X509Object* x) {
    LibreSslX509UpRef_nid_no_patch(x);
}

void FreeX509(X509Object* x) {
    X509_free_nid_postfix(x);
}

Asn1Object* OidObject(Arena& arena, const unsigned char* oid, std::size_t length) {
    if (const auto* known = TableObjectByOid(oid, length, false)) return const_cast<Asn1Object*>(known);
    arena.objects.push_back(std::make_unique<Asn1Object>(Asn1Object{nullptr, nullptr, NidUndef, static_cast<int>(length), arena.Copy(oid, length), ObjectFlagDynamic}));
    return arena.objects.back().get();
}

Asn1String* BitString(Arena& arena, const unsigned char* content, std::size_t length) {
    if (!length) Malformed("BIT STRING");
    return arena.String(TypeBitString, content + 1, length - 1, FlagBitsLeft | (content[0] & 0x07));
}

Asn1Type* ParseAsn1Type(Arena& arena, unsigned char*& cursor, const unsigned char* end) {
    if (cursor >= end) Malformed("ASN1_TYPE");
    auto* start = cursor;
    const int tag = *cursor++;
    std::size_t length = 0;
    if (mbedtls_asn1_get_len(&cursor, end, &length) != 0) Malformed("ASN1_TYPE");
    Asn1Type type{};
    switch (tag) {
    case MBEDTLS_ASN1_BOOLEAN:
        if (length != 1) Malformed("BOOLEAN");
        type.type = TypeBoolean;
        type.value.boolean = cursor[0];
        break;
    case MBEDTLS_ASN1_NULL:
        if (length) Malformed("NULL");
        type.type = TypeNull;
        break;
    case MBEDTLS_ASN1_OID:
        type.type = TypeObject;
        type.value.ptr = OidObject(arena, cursor, length);
        break;
    case MBEDTLS_ASN1_INTEGER:
    case MBEDTLS_ASN1_ENUMERATED: {
        if (!length) Malformed("INTEGER");
        auto* integer = NewInteger(arena, cursor, length);
        if (tag == MBEDTLS_ASN1_ENUMERATED) integer->type = TypeEnumerated | (integer->type & 0x100);
        type.type = tag;
        type.value.ptr = integer;
        break;
    }
    case MBEDTLS_ASN1_BIT_STRING:
        type.type = TypeBitString;
        type.value.ptr = BitString(arena, cursor, length);
        break;
    case MBEDTLS_ASN1_OCTET_STRING:
    case MBEDTLS_ASN1_UTF8_STRING:
    case MBEDTLS_ASN1_PRINTABLE_STRING:
    case MBEDTLS_ASN1_T61_STRING:
    case MBEDTLS_ASN1_IA5_STRING:
    case MBEDTLS_ASN1_UTC_TIME:
    case MBEDTLS_ASN1_GENERALIZED_TIME:
    case MBEDTLS_ASN1_UNIVERSAL_STRING:
    case MBEDTLS_ASN1_BMP_STRING:
    case 18:
    case 26:
    case 27:
        type.type = tag;
        type.value.ptr = arena.String(tag, cursor, length);
        break;
    case MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE:
    case MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET:
        type.type = tag == (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) ? TypeSequence : TypeSet;
        type.value.ptr = arena.String(type.type, start, static_cast<std::size_t>(cursor - start) + length);
        break;
    default:
        throw std::invalid_argument("ASN1_TYPE: a value with tag " + std::to_string(tag) + " is not supported");
    }
    cursor += length;
    return arena.Make(type);
}

Algor* ParseAlgor(Arena& arena, unsigned char*& cursor, const unsigned char* end) {
    std::size_t length = 0;
    if (!Element(cursor, end, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE, length)) Malformed("AlgorithmIdentifier");
    const auto* stop = cursor + length;
    std::size_t oidLength = 0;
    if (!Element(cursor, stop, MBEDTLS_ASN1_OID, oidLength)) Malformed("AlgorithmIdentifier");
    auto* algorithm = OidObject(arena, cursor, oidLength);
    cursor += oidLength;
    Asn1Type* parameter = nullptr;
    if (cursor < stop) parameter = ParseAsn1Type(arena, cursor, stop);
    if (cursor != stop) Malformed("AlgorithmIdentifier");
    return arena.Make(Algor{algorithm, parameter});
}

std::size_t DerElementLength(const unsigned char* data, std::size_t available) {
    if (available < 2) return 0;
    auto* cursor = const_cast<unsigned char*>(data) + 1;
    std::size_t length = 0;
    if (mbedtls_asn1_get_len(&cursor, data + available, &length) != 0) return 0;
    return static_cast<std::size_t>(cursor - data) + length;
}

Stack* NewHeapStack() {
    auto heap = std::make_unique<HeapStack>();
    auto* stack = &heap->stack;
    std::lock_guard lock(Mutex());
    HeapStacks().emplace(stack, std::move(heap));
    return stack;
}

void HeapStackPush(Stack* stack, void* item) {
    std::lock_guard lock(Mutex());
    const auto found = HeapStacks().find(stack);
    if (found == HeapStacks().end()) throw std::invalid_argument("STACK push: the stack was not created by this library");
    auto& heap = *found->second;
    heap.items.resize(static_cast<std::size_t>(heap.stack.num));
    heap.items.push_back(static_cast<char*>(item));
    heap.stack.data = heap.items.data();
    heap.stack.num = static_cast<int>(heap.items.size());
    heap.stack.numAlloc = static_cast<int>(heap.items.capacity());
}

bool IsHeapStack(const Stack* stack) {
    std::lock_guard lock(Mutex());
    return HeapStacks().count(stack) != 0;
}

void FreeHeapStack(Stack* stack) {
    if (!stack) return;
    std::lock_guard lock(Mutex());
    if (!HeapStacks().erase(stack)) throw std::invalid_argument("STACK free: the stack was not created by this library");
}

X509Crl* NewCrl(const unsigned char* der, std::size_t length) {
    auto crl = std::make_unique<X509Crl>();
    if (mbedtls_x509_crl_parse_der(&crl->parsed, der, length) != 0 || crl->parsed.next) return nullptr;
    std::lock_guard lock(Mutex());
    Crls().insert(crl.get());
    return crl.release();
}

void CrlUpRef(X509Crl* crl) {
    if (!crl) throw std::invalid_argument("X509_CRL up_ref: null CRL");
    crl->references = crl->count.fetch_add(1) + 1;
}

void FreeCrl(X509Crl* crl) {
    if (!crl) return;
    {
        std::lock_guard lock(Mutex());
        if (!Crls().count(crl)) throw std::invalid_argument("X509_CRL free: the CRL was not created by this library");
    }
    const int left = crl->count.fetch_sub(1) - 1;
    crl->references = left;
    if (left) return;
    {
        std::lock_guard lock(Mutex());
        Crls().erase(crl);
    }
    delete crl;
}

}
