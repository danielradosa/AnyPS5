#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_GUESTLOCALE_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_GUESTLOCALE_HPP

#include <cstddef>
#include <cstdint>
#include "general/VabiMacros.hpp"

namespace GuestLocale {

struct LocinfoStorage {
    alignas(std::uint64_t) unsigned char bytes[64];
};

struct Facet;

struct FacetVtable {
    void (APS5_VABI *destroy)(Facet* self);
    void (APS5_VABI *deleteObject)(Facet* self);
    void (APS5_VABI *retain)(Facet* self);
    Facet* (APS5_VABI *release)(Facet* self);
};

struct Facet {
    const FacetVtable* vtable;
    std::uint32_t references;
    std::uint32_t reserved;
};

struct Implementation {
    Facet base;
    Facet** facets;
    std::uint64_t facetCount;
    std::uint32_t category;
    bool transparent;
    const char* name;
};

struct String {
    std::uint64_t reserved;
    union {
        char buffer[16];
        char* pointer;
    };
    std::uint64_t size;
    std::uint64_t capacity;
};

struct CollateFacet {
    Facet base;
    const void* collation;
    const void* wideCollation;
};

struct CollateVtable {
    FacetVtable facet;
    int (APS5_VABI *compare)(const CollateFacet* self, const char* first1, const char* last1, const char* first2, const char* last2);
    String* (APS5_VABI *transform)(String* result, const CollateFacet* self, const char* first, const char* last);
    std::int64_t (APS5_VABI *hash)(const CollateFacet* self, const char* first, const char* last);
};

struct WideString {
    std::uint64_t reserved;
    union {
        std::uint16_t buffer[8];
        std::uint16_t* pointer;
    };
    std::uint64_t size;
    std::uint64_t capacity;
};

struct WideCollateVtable {
    FacetVtable facet;
    int (APS5_VABI *compare)(const CollateFacet* self, const std::uint16_t* first1, const std::uint16_t* last1, const std::uint16_t* first2, const std::uint16_t* last2);
    WideString* (APS5_VABI *transform)(WideString* result, const CollateFacet* self, const std::uint16_t* first, const std::uint16_t* last);
    std::int64_t (APS5_VABI *hash)(const CollateFacet* self, const std::uint16_t* first, const std::uint16_t* last);
};

inline constexpr std::size_t StandardStreamCount = 8;

struct CodecvtIdentityFacet {
    Facet base;
};

struct CodecvtIdentityVtable {
    FacetVtable facet;
    bool (APS5_VABI *alwaysNoconv)(const CodecvtIdentityFacet* self);
    int (APS5_VABI *encoding)(const CodecvtIdentityFacet* self);
    int (APS5_VABI *maxLength)(const CodecvtIdentityFacet* self);
    int (APS5_VABI *in)(const CodecvtIdentityFacet* self, void* state, const char* from, const char* fromEnd, const char** fromNext, char* to, char* toEnd, char** toNext);
    int (APS5_VABI *out)(const CodecvtIdentityFacet* self, void* state, const char* from, const char* fromEnd, const char** fromNext, char* to, char* toEnd, char** toNext);
    int (APS5_VABI *unshift)(const CodecvtIdentityFacet* self, void* state, char* to, char* toEnd, char** toNext);
    std::size_t (APS5_VABI *length)(const CodecvtIdentityFacet* self, void* state, const char* from, const char* fromEnd, std::size_t max);
};

struct NumpunctFacet {
    Facet base;
    const char* grouping;
    char decimalPoint;
    char thousandsSep;
    const char* falsename;
    const char* truename;
};

struct NumpunctVtable {
    FacetVtable facet;
    char (APS5_VABI *decimalPoint)(const NumpunctFacet* self);
    char (APS5_VABI *thousandsSep)(const NumpunctFacet* self);
    String* (APS5_VABI *grouping)(String* result, const NumpunctFacet* self);
    String* (APS5_VABI *falsename)(String* result, const NumpunctFacet* self);
    String* (APS5_VABI *truename)(String* result, const NumpunctFacet* self);
};

struct IosBase {
    const void* vtable;
    std::int64_t standardStream;
    std::int32_t state;
    std::int32_t exceptions;
    std::int32_t flags;
    std::int32_t precision;
    std::int32_t width;
    std::uint32_t reserved;
    void* storage;
    void* callbacks;
    Implementation** locale;
};

static_assert(sizeof(Facet) == 0x10);
static_assert(offsetof(Implementation, facets) == 0x10);
static_assert(offsetof(Implementation, facetCount) == 0x18);
static_assert(offsetof(Implementation, transparent) == 0x24);
static_assert(offsetof(Implementation, name) == 0x28);
static_assert(offsetof(IosBase, state) == 0x10);
static_assert(offsetof(IosBase, storage) == 0x28);
static_assert(offsetof(IosBase, callbacks) == 0x30);
static_assert(offsetof(IosBase, locale) == 0x38);
static_assert(sizeof(IosBase) == 0x40);
static_assert(offsetof(String, buffer) == 0x8);
static_assert(offsetof(String, size) == 0x18);
static_assert(sizeof(String) == 0x28);
static_assert(offsetof(CollateFacet, collation) == 0x10);
static_assert(sizeof(CollateFacet) == 0x20);
static_assert(offsetof(CollateVtable, compare) == 0x20);
static_assert(offsetof(CollateVtable, hash) == 0x30);
static_assert(offsetof(WideString, buffer) == 0x8);
static_assert(offsetof(WideString, size) == 0x18);
static_assert(sizeof(WideString) == 0x28);
static_assert(offsetof(WideCollateVtable, compare) == 0x20);
static_assert(offsetof(WideCollateVtable, hash) == 0x30);
static_assert(sizeof(CodecvtIdentityFacet) == 0x10);
static_assert(offsetof(CodecvtIdentityVtable, alwaysNoconv) == 0x20);
static_assert(offsetof(CodecvtIdentityVtable, length) == 0x50);
static_assert(offsetof(NumpunctFacet, grouping) == 0x10);
static_assert(offsetof(NumpunctFacet, decimalPoint) == 0x18);
static_assert(offsetof(NumpunctFacet, falsename) == 0x20);
static_assert(sizeof(NumpunctFacet) == 0x30);
static_assert(offsetof(NumpunctVtable, decimalPoint) == 0x20);
static_assert(offsetof(NumpunctVtable, truename) == 0x40);
static_assert(alignof(LocinfoStorage) == 8);

}

#endif
