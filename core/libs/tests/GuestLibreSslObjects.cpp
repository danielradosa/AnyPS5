#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <source_location>
#include <string>

struct GuestAsn1Object {
    const char* sn;
    const char* ln;
    int nid;
    int length;
    const unsigned char* data;
    int flags;
};

struct GuestBio;
struct GuestBioMethod;

extern "C" {
const char* APS5_VABI OBJ_nid2sn_nid_postfix(int);
int APS5_VABI OBJ_sn2nid_nid_postfix(const char*);
int APS5_VABI OBJ_obj2nid_nid_postfix(const GuestAsn1Object*);
int APS5_VABI OBJ_create_nid_postfix(const char*, const char*, const char*);
void APS5_VABI OBJ_cleanup_nid_postfix();
int APS5_VABI i2a_ASN1_OBJECT_nid_postfix(GuestBio*, const GuestAsn1Object*);
const GuestBioMethod* APS5_VABI BIO_s_mem_nid_postfix();
GuestBio* APS5_VABI BIO_new_nid_postfix(const GuestBioMethod*);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
}

namespace {

constexpr int CtrlReset = 1;
constexpr int CtrlInfo = 3;

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "LibreSSL objects check failed at line %u\n", location.line());
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

bool Same(const char* value, const char* expected) {
    return value && std::strcmp(value, expected) == 0;
}

std::string Take(GuestBio* bio) {
    char* data = nullptr;
    const auto length = BIO_ctrl_nid_postfix(bio, CtrlInfo, 0, &data);
    std::string text = length > 0 ? std::string(data, static_cast<std::size_t>(length)) : std::string();
    BIO_ctrl_nid_postfix(bio, CtrlReset, 0, nullptr);
    return text;
}

}

int main() {
    Require(Same(OBJ_nid2sn_nid_postfix(13), "CN"));
    Require(Same(OBJ_nid2sn_nid_postfix(0), "UNDEF"));
    Require(Same(OBJ_nid2sn_nid_postfix(672), "SHA256"));
    Require(Same(OBJ_nid2sn_nid_postfix(14), "C"));
    Require(OBJ_nid2sn_nid_postfix(968) == nullptr);
    Require(ERR_get_error_nid_postfix() == 0x08068065);
    Require(ERR_get_error_nid_postfix() == 0);
    Require(OBJ_nid2sn_nid_postfix(102) == nullptr);
    Require(ERR_get_error_nid_postfix() == 0x08068065);
    Require(OBJ_nid2sn_nid_postfix(959) == nullptr && OBJ_nid2sn_nid_postfix(-1) == nullptr);
    Require(ERR_get_error_nid_postfix() == 0x08068065 && ERR_get_error_nid_postfix() == 0x08068065);
    Require(Throws([] { OBJ_nid2sn_nid_postfix(6); }));
    Require(Throws([] { OBJ_nid2sn_nid_postfix(48); }));

    Require(OBJ_sn2nid_nid_postfix("CN") == 13);
    Require(OBJ_sn2nid_nid_postfix("SHA512") == 674);
    Require(OBJ_sn2nid_nid_postfix("commonName") == 0);
    Require(OBJ_sn2nid_nid_postfix("no such object") == 0);
    Require(Throws([] { OBJ_sn2nid_nid_postfix("rsaEncryption"); }));
    Require(Throws([] { OBJ_sn2nid_nid_postfix("emailAddress"); }));
    Require(Throws([] { OBJ_sn2nid_nid_postfix(nullptr); }));

    static const unsigned char commonName[] = {0x55, 0x04, 0x03};
    static const unsigned char newOid[] = {0x2a, 0x03, 0x04};
    static const unsigned char privateOid[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x86, 0x8d, 0x1f, 0x01};
    static const unsigned char badOid[] = {0x2b, 0x80, 0x01};
    static const unsigned char truncatedOid[] = {0x2b, 0x86};
    GuestAsn1Object byOid{nullptr, nullptr, 0, 3, commonName, 0};
    GuestAsn1Object ownNid{nullptr, nullptr, 4242, 3, commonName, 0};
    GuestAsn1Object fresh{nullptr, nullptr, 0, 3, newOid, 0};
    GuestAsn1Object unnamed{nullptr, nullptr, 0, sizeof(privateOid), privateOid, 0};
    GuestAsn1Object named{"CN", "commonName", 13, 3, commonName, 0};
    GuestAsn1Object shortOnly{"CN", nullptr, 13, 3, commonName, 0};
    GuestAsn1Object invalid{nullptr, nullptr, 0, sizeof(badOid), badOid, 0};
    GuestAsn1Object truncated{nullptr, nullptr, 0, sizeof(truncatedOid), truncatedOid, 0};
    GuestAsn1Object empty{nullptr, nullptr, 0, 0, nullptr, 0};
    Require(OBJ_obj2nid_nid_postfix(&byOid) == 13);
    Require(OBJ_obj2nid_nid_postfix(&ownNid) == 4242);
    Require(OBJ_obj2nid_nid_postfix(&fresh) == 0);
    Require(OBJ_obj2nid_nid_postfix(&empty) == 0);
    Require(OBJ_obj2nid_nid_postfix(nullptr) == 0);

    auto* bio = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    Require(bio != nullptr);
    Require(i2a_ASN1_OBJECT_nid_postfix(bio, &named) == 10 && Take(bio) == "commonName");
    Require(i2a_ASN1_OBJECT_nid_postfix(bio, &shortOnly) == 2 && Take(bio) == "CN");
    Require(i2a_ASN1_OBJECT_nid_postfix(bio, &unnamed) == 19 && Take(bio) == "1.3.6.1.4.1.99999.1");
    Require(i2a_ASN1_OBJECT_nid_postfix(bio, &fresh) == 7 && Take(bio) == "1.2.3.4");
    Require(i2a_ASN1_OBJECT_nid_postfix(bio, nullptr) == 4 && Take(bio) == "NULL");
    Require(i2a_ASN1_OBJECT_nid_postfix(bio, &empty) == 4 && Take(bio) == "NULL");
    Require(i2a_ASN1_OBJECT_nid_postfix(bio, &invalid) == 9 && Take(bio) == "<INVALID>");
    Require(i2a_ASN1_OBJECT_nid_postfix(bio, &truncated) == 9 && Take(bio) == "<INVALID>");
    Require(Throws([&] { i2a_ASN1_OBJECT_nid_postfix(bio, &byOid); }));
    Require(Throws([&] { i2a_ASN1_OBJECT_nid_postfix(nullptr, &named); }));

    Require(OBJ_create_nid_postfix("1.2.3.4", "NewOID", "New Object Identifier") == 959);
    Require(OBJ_obj2nid_nid_postfix(&fresh) == 959);
    Require(Same(OBJ_nid2sn_nid_postfix(959), "NewOID"));
    Require(OBJ_sn2nid_nid_postfix("NewOID") == 959);
    Require(OBJ_create_nid_postfix("2.999.1", "Second", "Second Object") == 960);
    Require(OBJ_create_nid_postfix("3.1", "Bad", "Bad Object") == 0);
    Require(OBJ_create_nid_postfix("1.40", "Bad", "Bad Object") == 0);
    Require(OBJ_create_nid_postfix("1", "Bad", "Bad Object") == 0);
    Require(OBJ_create_nid_postfix("1.2.x", "Bad", "Bad Object") == 0);
    Require(OBJ_create_nid_postfix("", "Bad", "Bad Object") == 0);
    Require(Throws([] { OBJ_create_nid_postfix("1.2.3.4", "Other", "Other Object"); }));
    Require(Throws([] { OBJ_create_nid_postfix("1.2.3.5", "NewOID", "Other Object"); }));
    Require(Throws([] { OBJ_create_nid_postfix("1.2.3.6", "CN", "Other Object"); }));
    Require(Throws([] { OBJ_create_nid_postfix("2.5.4.3", "Other", "Other Object"); }));
    Require(Throws([] { OBJ_create_nid_postfix("1.02.3", "Other", "Other Object"); }));
    Require(Throws([] { OBJ_create_nid_postfix("1.2.3.99999999999999999999", "Other", "Other Object"); }));
    Require(Throws([] { OBJ_create_nid_postfix("1.2.3.7", nullptr, "Other Object"); }));
    Require(Throws([&] { i2a_ASN1_OBJECT_nid_postfix(bio, &fresh); }));

    OBJ_cleanup_nid_postfix();
    Require(OBJ_nid2sn_nid_postfix(959) == nullptr && ERR_get_error_nid_postfix() == 0x08068065);
    Require(OBJ_sn2nid_nid_postfix("NewOID") == 0);
    Require(OBJ_obj2nid_nid_postfix(&fresh) == 0);
    Require(Throws([] { OBJ_create_nid_postfix("1.2.3.8", "Third", "Third Object"); }));
    OBJ_cleanup_nid_postfix();
    Require(BIO_free_nid_postfix(bio) == 1);
    return 0;
}
