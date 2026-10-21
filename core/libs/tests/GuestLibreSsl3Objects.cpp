#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <source_location>

struct GuestAsn1Object {
    const char* sn;
    const char* ln;
    int nid;
    int length;
    const unsigned char* data;
    int flags;
};

extern "C" {
int APS5_VABI OBJ_cmp_nid_postfix(const GuestAsn1Object*, const GuestAsn1Object*);
GuestAsn1Object* APS5_VABI OBJ_txt2obj_nid_postfix(const char*, int);
void APS5_VABI ASN1_OBJECT_free_nid_postfix(GuestAsn1Object*);
}

namespace {

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "LibreSSL 3 objects check failed at line %u\n", location.line());
    std::abort();
}

template <typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

bool Encoded(const GuestAsn1Object* object, const unsigned char* bytes, int length) {
    return object && object->length == length && std::memcmp(object->data, bytes, static_cast<std::size_t>(length)) == 0;
}

}

int main() {
    static const unsigned char commonName[] = {0x55, 0x04, 0x03};
    static const unsigned char rsadsi[] = {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d};
    static const unsigned char sm3[] = {0x2a, 0x81, 0x1c, 0xcf, 0x55, 0x01, 0x83, 0x11};

    auto* byShort = OBJ_txt2obj_nid_postfix("CN", 0);
    auto* byLong = OBJ_txt2obj_nid_postfix("commonName", 0);
    Require(byShort && byShort == byLong && byShort->nid == 13 && byShort->flags == 0);
    Require(std::strcmp(byShort->sn, "CN") == 0 && std::strcmp(byShort->ln, "commonName") == 0 && Encoded(byShort, commonName, 3));
    auto* sm3Object = OBJ_txt2obj_nid_postfix("SM3", 0);
    Require(sm3Object && sm3Object->nid == 968 && Encoded(sm3Object, sm3, sizeof(sm3)));
    auto* withoutShortName = OBJ_txt2obj_nid_postfix("rsaEncryption", 0);
    Require(withoutShortName && withoutShortName->nid == 6 && withoutShortName->sn == nullptr);

    auto* numeric = OBJ_txt2obj_nid_postfix("2.5.4.3", 1);
    Require(numeric && numeric != byShort && numeric->nid == 0 && numeric->sn == nullptr && numeric->ln == nullptr);
    Require(numeric->flags == 0x0d && Encoded(numeric, commonName, 3));
    Require(OBJ_cmp_nid_postfix(numeric, byShort) == 0);
    auto* numericNamed = OBJ_txt2obj_nid_postfix("2.5.4.3", 0);
    Require(numericNamed && numericNamed != byShort && numericNamed->nid == 0 && OBJ_cmp_nid_postfix(numericNamed, numeric) == 0);
    auto* arc = OBJ_txt2obj_nid_postfix("1.2.840.113549", 1);
    Require(arc && Encoded(arc, rsadsi, sizeof(rsadsi)));
    Require(OBJ_cmp_nid_postfix(arc, numeric) != 0 && OBJ_cmp_nid_postfix(numeric, arc) != 0);
    GuestAsn1Object none{nullptr, nullptr, 0, 0, nullptr, 0};
    GuestAsn1Object alsoNone{"X", "Y", 1, 0, nullptr, 0};
    Require(OBJ_cmp_nid_postfix(&none, &alsoNone) == 0 && OBJ_cmp_nid_postfix(&none, numeric) != 0);

    Require(OBJ_txt2obj_nid_postfix("CN", 1) == nullptr);
    Require(OBJ_txt2obj_nid_postfix("no such object", 0) == nullptr);
    Require(OBJ_txt2obj_nid_postfix("3.1", 1) == nullptr);
    Require(OBJ_txt2obj_nid_postfix("0.40", 1) == nullptr);
    Require(OBJ_txt2obj_nid_postfix("2", 1) == nullptr);
    Require(OBJ_txt2obj_nid_postfix("1..2", 1) == nullptr);
    Require(OBJ_txt2obj_nid_postfix("", 0) == nullptr);
    Require(Throws([] { OBJ_txt2obj_nid_postfix("1.02", 1); }));
    Require(Throws([] { OBJ_txt2obj_nid_postfix("1.2.99999999999999999999", 1); }));
    Require(Throws([] { OBJ_txt2obj_nid_postfix(nullptr, 0); }));
    Require(Throws([&] { OBJ_cmp_nid_postfix(nullptr, numeric); }));

    ASN1_OBJECT_free_nid_postfix(byShort);
    ASN1_OBJECT_free_nid_postfix(sm3Object);
    ASN1_OBJECT_free_nid_postfix(numeric);
    ASN1_OBJECT_free_nid_postfix(numericNamed);
    ASN1_OBJECT_free_nid_postfix(arc);
    ASN1_OBJECT_free_nid_postfix(nullptr);
    Require(OBJ_txt2obj_nid_postfix("CN", 0) == byShort && byShort->nid == 13);
    return 0;
}
