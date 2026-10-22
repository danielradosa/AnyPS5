#ifndef CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_X509INTERNAL_HPP
#define CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_X509INTERNAL_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include <mbedtls/x509_crl.h>
#include <mbedtls/x509_crt.h>

#include "Asn1.hpp"
#include "LibreSsl.hpp"

namespace LibreSsl {

struct EvpPkey;

struct Arena {
    std::vector<std::unique_ptr<Asn1String>> strings;
    std::vector<std::unique_ptr<Asn1Object>> objects;
    std::vector<std::unique_ptr<NameEntry>> entries;
    std::vector<std::unique_ptr<Name>> names;
    std::vector<std::unique_ptr<Stack>> stacks;
    std::vector<std::unique_ptr<std::vector<char*>>> stackData;
    std::vector<std::unique_ptr<unsigned char[]>> buffers;
    std::vector<std::shared_ptr<void>> others;

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

    template <typename T>
    T* Make(T value) {
        auto object = std::make_shared<T>(std::move(value));
        others.push_back(object);
        return object.get();
    }
};

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
    void* publicKey = nullptr;

    X509Object() {
        mbedtls_x509_crt_init(&crt);
    }

    ~X509Object();

    X509Object(const X509Object&) = delete;
    X509Object& operator=(const X509Object&) = delete;
};

static_assert(offsetof(X509Object, signature) == 16 && offsetof(X509Object, references) == 28);

struct Asn1Type {
    int type;
    union {
        void* ptr;
        int boolean;
    } value;
};

struct Algor {
    Asn1Object* algorithm;
    Asn1Type* parameter;
};

struct Pubkey {
    Algor* algor;
    Asn1String* publicKey;
    void* pkey;
    const unsigned char* der;
    std::size_t derLength;
};

struct Extension {
    Asn1Object* object;
    int critical;
    Asn1String* value;
};

struct GeneralName {
    int type;
    void* d;
};

struct OtherName {
    Asn1Object* typeId;
    Asn1Type* value;
};

static_assert(sizeof(Asn1Type) == 16 && sizeof(Algor) == 16 && offsetof(Pubkey, pkey) == 16);
static_assert(sizeof(Extension) == 24 && offsetof(Extension, value) == 16 && sizeof(GeneralName) == 16 && sizeof(OtherName) == 16);

struct X509Crl {
    void* crl = nullptr;
    void* sigAlg = nullptr;
    Asn1String* signature = nullptr;
    int references = 1;
    int flags = 0;
    std::atomic<int> count{1};
    mbedtls_x509_crl parsed;

    X509Crl() {
        mbedtls_x509_crl_init(&parsed);
    }

    ~X509Crl() {
        mbedtls_x509_crl_free(&parsed);
    }

    X509Crl(const X509Crl&) = delete;
    X509Crl& operator=(const X509Crl&) = delete;
};

static_assert(offsetof(X509Crl, references) == 24 && offsetof(X509Crl, flags) == 28);

Name* NewName(Arena& arena, const mbedtls_x509_buf& raw);
Asn1String* NewInteger(Arena& arena, const unsigned char* data, std::size_t length);
void CompleteX509(X509Object& x);
void ReleaseX509Extras(X509Object& x);
void X509UpRef(X509Object* x);
void FreeX509(X509Object* x);
X509Crl* NewCrl(const unsigned char* der, std::size_t length);
void CrlUpRef(X509Crl* crl);
void FreeCrl(X509Crl* crl);

Stack* NewHeapStack();
void HeapStackPush(Stack* stack, void* item);
bool IsHeapStack(const Stack* stack);
void FreeHeapStack(Stack* stack);

Asn1Object* OidObject(Arena& arena, const unsigned char* oid, std::size_t length);
Asn1Type* ParseAsn1Type(Arena& arena, unsigned char*& cursor, const unsigned char* end);
Algor* ParseAlgor(Arena& arena, unsigned char*& cursor, const unsigned char* end);
Asn1String* BitString(Arena& arena, const unsigned char* content, std::size_t length);
std::size_t DerElementLength(const unsigned char* data, std::size_t available);
EvpPkey* X509PublicKey(X509Object& x, const char* function);

}

#endif
