#ifndef CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_INTERNAL_HPP
#define CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_INTERNAL_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <mbedtls/md.h>

#include "LibreSsl.hpp"

namespace LibreSsl {

constexpr int NidUndef = 0;
constexpr int NidMd5 = 4;
constexpr int NidSha1 = 64;
constexpr int NidSha224 = 675;
constexpr int NidSha256 = 672;
constexpr int NidSha384 = 673;
constexpr int NidSha512 = 674;

constexpr int ErrLibBn = 3;
constexpr int ErrLibEvp = 6;
constexpr int ErrLibBio = 32;
constexpr int ErrLibRand = 36;

constexpr int ErrReasonMallocFailure = 1 | 64;
constexpr int ErrReasonPassedNullParameter = 3 | 64;

struct EvpMd {
    int type;
    int pkeyType;
    int mdSize;
    int blockSize;
    mbedtls_md_type_t md;
};

void PushError(int library, int function, int reason);
std::uint64_t PeekLastError();
const EvpMd* DigestByMbedType(mbedtls_md_type_t type);
std::vector<unsigned char> BioReadAll(Bio* bio, const char* function);
void BioPutBack(Bio* bio, const unsigned char* data, std::size_t length);
Bignum* NewBignum();
void FreeBignum(Bignum* bn);
void SetBignumBytes(Bignum& bn, const unsigned char* bigEndian, std::size_t length);
std::vector<unsigned char> BignumBytes(const Bignum& bn);
std::vector<unsigned char> EncodeObjectIdentifier(const char* dotted);
bool DecodeObjectIdentifier(const unsigned char* data, std::size_t length, std::string& out);
const Asn1Object* TableObject(int nid, bool version3);
int ObjectNid(const Asn1Object* object, bool version3);
const char* TableShortName(int nid, const char* function);
bool ObjectText(const Asn1Object* object, bool noName, std::string& out, const char* function);
bool ReleaseCreatedObject(Asn1Object* object);
void ObjectsCleanup();

}

#endif
