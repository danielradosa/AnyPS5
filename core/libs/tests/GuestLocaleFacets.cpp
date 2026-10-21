#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/GuestLocale.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

extern "C" {
extern GuestLocale::Implementation* _ZSt21_sceLibcClassicLocale_nid_postfix;
std::size_t APS5_VABI _ZNSt7codecvtIcc9_MbstatetE7_GetcatEPPKNSt6locale5facetEPKS2__nid_postfix(GuestLocale::Facet** facet, const GuestLocale::Implementation* const* locale);
std::size_t APS5_VABI _ZNSt8numpunctIcE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(GuestLocale::Facet** facet, const GuestLocale::Implementation* const* locale);
std::size_t APS5_VABI _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5__nid_postfix(GuestLocale::Facet** facet, const GuestLocale::Implementation* const* locale);
std::size_t APS5_VABI _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5__nid_postfix(GuestLocale::Facet** facet, const GuestLocale::Implementation* const* locale);
}

namespace {

std::size_t allocations = 0;
std::size_t lastSize = 0;
void* lastAllocation = nullptr;

void require(bool condition) {
    if (!condition) std::abort();
}

template<typename TAction>
void reject(TAction action) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    require(rejected);
}

void* APS5_VABI allocate(std::size_t bytes) {
    ++allocations;
    lastSize = bytes;
    lastAllocation = std::malloc(bytes);
    return lastAllocation;
}

void APS5_VABI release(void* pointer) {
    std::free(pointer);
}

void* APS5_VABI allocateZeroed(std::size_t, std::size_t) { std::abort(); }
void* APS5_VABI reallocate(void*, std::size_t) { std::abort(); }
void* APS5_VABI align(std::size_t, std::size_t) { std::abort(); }
void* APS5_VABI realign(void*, std::size_t, std::size_t) { std::abort(); }
int APS5_VABI posixAlign(void**, std::size_t, std::size_t) { std::abort(); }

void CheckCodecvtGetcat(const GuestLocale::Implementation* const* classic) {
    require(_ZNSt7codecvtIcc9_MbstatetE7_GetcatEPPKNSt6locale5facetEPKS2__nid_postfix(nullptr, classic) == 2);

    GuestLocale::Facet existing{};
    GuestLocale::Facet* facet = &existing;
    require(_ZNSt7codecvtIcc9_MbstatetE7_GetcatEPPKNSt6locale5facetEPKS2__nid_postfix(&facet, classic) == 2);
    require(facet == &existing && allocations == 0);

    GuestLocale::Implementation french = *_ZSt21_sceLibcClassicLocale_nid_postfix;
    french.name = "fr_FR";
    const GuestLocale::Implementation* frenchPointer = &french;
    GuestLocale::Facet* absent = nullptr;
    reject([&] { _ZNSt7codecvtIcc9_MbstatetE7_GetcatEPPKNSt6locale5facetEPKS2__nid_postfix(&absent, &frenchPointer); });
    require(absent == nullptr && allocations == 0);

    facet = nullptr;
    require(_ZNSt7codecvtIcc9_MbstatetE7_GetcatEPPKNSt6locale5facetEPKS2__nid_postfix(&facet, classic) == 2);
    require(facet != nullptr && allocations == 1 && lastSize == sizeof(GuestLocale::CodecvtIdentityFacet) && lastAllocation == facet);
    require(facet->references == 0);

    const auto& vtable = *reinterpret_cast<const GuestLocale::CodecvtIdentityVtable*>(facet->vtable);
    const auto* identity = reinterpret_cast<const GuestLocale::CodecvtIdentityFacet*>(facet);
    require(vtable.alwaysNoconv(identity));
    require(vtable.encoding(identity) == 1);
    require(vtable.maxLength(identity) == 1);

    const char from[] = "abc";
    char to[8]{};
    const char* fromNext = nullptr;
    char* toNext = nullptr;
    require(vtable.in(identity, nullptr, from, from + 3, &fromNext, to, to + 8, &toNext) == 3);
    require(fromNext == from && toNext == to);
    require(vtable.out(identity, nullptr, from, from + 3, &fromNext, to, to + 8, &toNext) == 3);
    require(fromNext == from && toNext == to);
    require(vtable.unshift(identity, nullptr, to, to + 8, &toNext) == 0);
    require(toNext == to);
    require(vtable.length(identity, nullptr, from, from + 3, 2) == 2);
    require(vtable.length(identity, nullptr, from, from + 3, 9) == 3);
    require(vtable.length(identity, nullptr, from, from, 9) == 0);
    reject([&] { vtable.in(identity, nullptr, from, from + 3, nullptr, to, to + 8, &toNext); });
    reject([&] { vtable.unshift(identity, nullptr, to, to + 8, nullptr); });
    reject([&] { vtable.length(identity, nullptr, from + 3, from, 9); });

    vtable.facet.retain(facet);
    require(facet->references == 1);
    require(vtable.facet.release(facet) == facet && facet->references == 0);
    vtable.facet.destroy(facet);
    reject([&] { vtable.facet.deleteObject(nullptr); });
    vtable.facet.deleteObject(facet);
}

void CheckNumpunctGetcat(const GuestLocale::Implementation* const* classic) {
    require(_ZNSt8numpunctIcE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(nullptr, classic) == 4);

    GuestLocale::Facet existing{};
    GuestLocale::Facet* facet = &existing;
    const std::size_t before = allocations;
    require(_ZNSt8numpunctIcE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(&facet, classic) == 4);
    require(facet == &existing && allocations == before);

    GuestLocale::Implementation french = *_ZSt21_sceLibcClassicLocale_nid_postfix;
    french.name = "de_DE";
    const GuestLocale::Implementation* frenchPointer = &french;
    GuestLocale::Facet* absent = nullptr;
    reject([&] { _ZNSt8numpunctIcE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(&absent, &frenchPointer); });
    require(absent == nullptr && allocations == before);

    facet = nullptr;
    require(_ZNSt8numpunctIcE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(&facet, classic) == 4);
    require(facet != nullptr && allocations == before + 1 && lastSize == sizeof(GuestLocale::NumpunctFacet) && lastAllocation == facet);
    require(facet->references == 0);

    const auto* numpunct = reinterpret_cast<const GuestLocale::NumpunctFacet*>(facet);
    const auto& vtable = *reinterpret_cast<const GuestLocale::NumpunctVtable*>(facet->vtable);
    require(vtable.decimalPoint(numpunct) == '.');
    require(vtable.thousandsSep(numpunct) == ',');
    require(numpunct->decimalPoint == '.' && numpunct->thousandsSep == ',');
    require(std::string(numpunct->falsename) == "false" && std::string(numpunct->truename) == "true" && std::string(numpunct->grouping) == "");

    GuestLocale::String result;
    std::memset(&result, 0xcd, sizeof(result));
    require(vtable.grouping(&result, numpunct) == &result);
    require(result.size == 0 && result.capacity == 15 && result.buffer[0] == 0);
    require(vtable.falsename(&result, numpunct) == &result);
    require(result.size == 5 && std::string(result.buffer) == "false");
    require(vtable.truename(&result, numpunct) == &result);
    require(result.size == 4 && std::string(result.buffer) == "true");
    reject([&] { vtable.grouping(nullptr, numpunct); });
    reject([&] { vtable.decimalPoint(nullptr); });

    vtable.facet.deleteObject(facet);
}

void CheckPutFacet(GuestLocale::Facet* facet, std::size_t before, std::size_t size) {
    require(facet != nullptr && allocations == before + 1 && lastSize == size && lastAllocation == facet && facet->references == 0);
    const auto& vtable = *reinterpret_cast<const GuestLocale::UnsupportedMethodsVtable*>(facet->vtable);
    reject([&] { vtable.methods[0](); });
    reject([&] { vtable.methods[7](); });
    vtable.facet.retain(facet);
    vtable.facet.retain(facet);
    require(facet->references == 2);
    vtable.facet.deleteObject(facet);
}

void CheckPutGetcats(const GuestLocale::Implementation* const* classic) {
    require(_ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5__nid_postfix(nullptr, classic) == 4);
    require(_ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5__nid_postfix(nullptr, classic) == 5);
    GuestLocale::Facet existing{};
    GuestLocale::Facet* facet = &existing;
    std::size_t before = allocations;
    require(_ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5__nid_postfix(&facet, classic) == 4 && facet == &existing);
    require(_ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5__nid_postfix(&facet, classic) == 5 && facet == &existing && allocations == before);

    facet = nullptr;
    require(_ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5__nid_postfix(&facet, classic) == 4);
    CheckPutFacet(facet, before, sizeof(GuestLocale::Facet));
    facet = nullptr;
    before = allocations;
    require(_ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5__nid_postfix(&facet, classic) == 5);
    CheckPutFacet(facet, before, 0xb8);
}

}

int main() {
    const std::array<void*, 10> api{reinterpret_cast<void*>(&allocate), reinterpret_cast<void*>(&release), reinterpret_cast<void*>(&allocateZeroed), reinterpret_cast<void*>(&reallocate), reinterpret_cast<void*>(&align), reinterpret_cast<void*>(&realign), reinterpret_cast<void*>(&posixAlign)};
    ApplicationHeapRegister_nid_no_patch(api.data());
    const GuestLocale::Implementation* const* classic = &_ZSt21_sceLibcClassicLocale_nid_postfix;

    CheckCodecvtGetcat(classic);
    CheckNumpunctGetcat(classic);
    CheckPutGetcats(classic);
    return 0;
}
