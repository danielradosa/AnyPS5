#ifndef CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_ASN1_HPP
#define CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_ASN1_HPP

#include <cstddef>
#include <cstdint>

namespace LibreSsl {

struct Asn1String {
    int length;
    int type;
    unsigned char* data;
    std::int64_t flags;
};

struct Asn1Object {
    const char* sn;
    const char* ln;
    int nid;
    int length;
    const unsigned char* data;
    int flags;
};

struct Stack {
    int num;
    char** data;
    int sorted;
    int numAlloc;
    void* comp;
};

struct NameEntry {
    Asn1Object* object;
    Asn1String* value;
    int set;
    int size;
};

struct Name {
    Stack* entries;
    int modified;
    void* bytes;
    unsigned char* canonEnc;
    int canonEnclen;
};

struct Validity {
    Asn1String* notBefore;
    Asn1String* notAfter;
};

struct CertInfo {
    Asn1String* version;
    Asn1String* serialNumber;
    void* signature;
    Name* issuer;
    Validity* validity;
    Name* subject;
    void* key;
    Asn1String* issuerUid;
    Asn1String* subjectUid;
    Stack* extensions;
    unsigned char* enc;
    std::int64_t encLength;
    int encModified;
};

static_assert(sizeof(Asn1String) == 24 && offsetof(Asn1String, data) == 8);
static_assert(sizeof(Asn1Object) == 40 && offsetof(Asn1Object, nid) == 16 && offsetof(Asn1Object, data) == 24);
static_assert(sizeof(Stack) == 32 && offsetof(Stack, data) == 8);
static_assert(sizeof(NameEntry) == 24);
static_assert(offsetof(CertInfo, validity) == 32 && offsetof(CertInfo, subject) == 40);

}

#endif
