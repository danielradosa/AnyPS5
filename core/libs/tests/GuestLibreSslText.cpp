#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <source_location>
#include <string>
#include <vector>

struct GuestAsn1String {
    int length;
    int type;
    unsigned char* data;
    std::int64_t flags;
};

struct GuestAsn1Object {
    const char* sn;
    const char* ln;
    int nid;
    int length;
    const unsigned char* data;
    int flags;
};

struct GuestStack {
    int num;
    char** data;
    int sorted;
    int numAlloc;
    void* comp;
};

struct GuestNameEntry {
    GuestAsn1Object* object;
    GuestAsn1String* value;
    int set;
    int size;
};

struct GuestName {
    GuestStack* entries;
    int modified;
    void* bytes;
    unsigned char* canonEnc;
    int canonEnclen;
};

struct GuestThreadId {
    void* ptr;
    std::uint64_t val;
};

struct GuestBio;
struct GuestBioMethod;

extern "C" {
std::uint64_t APS5_VABI SSLeay_nid_postfix();
void APS5_VABI CRYPTO_THREADID_set_numeric_nid_postfix(GuestThreadId*, std::uint64_t);
int APS5_VABI ASN1_STRING_print_nid_postfix(GuestBio*, const GuestAsn1String*);
int APS5_VABI ASN1_TIME_print_nid_postfix(GuestBio*, const GuestAsn1String*);
int APS5_VABI X509_NAME_print_ex_nid_postfix(GuestBio*, const GuestName*, int, std::uint64_t);
int APS5_VABI CONF_modules_load_file_nid_postfix(const char*, const char*, std::uint64_t);
const GuestBioMethod* APS5_VABI BIO_s_mem_nid_postfix();
GuestBio* APS5_VABI BIO_new_nid_postfix(const GuestBioMethod*);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
}

namespace {

constexpr int CtrlReset = 1;
constexpr int CtrlInfo = 3;
constexpr std::uint64_t EscCtrl = 0x2, EscMsb = 0x4, Utf8Convert = 0x10, ShowType = 0x40, DumpAll = 0x80, DumpUnknown = 0x100, DumpDer = 0x200;
constexpr std::uint64_t Rfc2253Strings = 0x1 | EscCtrl | EscMsb | Utf8Convert | DumpUnknown | DumpDer;
constexpr std::uint64_t SepCommaPlus = 1ull << 16, SepCplusSpc = 2ull << 16, SepSplusSpc = 3ull << 16, SepMultiline = 4ull << 16;
constexpr std::uint64_t DnRev = 1ull << 20, FnLn = 1ull << 21, FnOid = 2ull << 21, FnNone = 3ull << 21, SpcEq = 1ull << 23;
constexpr std::uint64_t DumpUnknownFields = 1ull << 24, FnAlign = 1ull << 25;
constexpr std::uint64_t FlagRfc2253 = Rfc2253Strings | SepCommaPlus | DnRev | DumpUnknownFields;
constexpr std::uint64_t FlagOneline = Rfc2253Strings | 0x8 | SepCplusSpc | SpcEq;
constexpr std::uint64_t FlagMultiline = EscCtrl | EscMsb | SepMultiline | SpcEq | FnLn | FnAlign;

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "LibreSSL text check failed at line %u\n", location.line());
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

std::string Take(GuestBio* bio) {
    char* data = nullptr;
    const auto length = BIO_ctrl_nid_postfix(bio, CtrlInfo, 0, &data);
    std::string text = length > 0 ? std::string(data, static_cast<std::size_t>(length)) : std::string();
    BIO_ctrl_nid_postfix(bio, CtrlReset, 0, nullptr);
    return text;
}

struct NameBuilder {
    std::vector<std::string> bytes;
    std::vector<GuestAsn1String> strings;
    std::vector<GuestNameEntry> entries;
    std::vector<char*> pointers;
    GuestStack stack{};
    GuestName name{};

    NameBuilder() {
        bytes.reserve(16);
        strings.reserve(16);
        entries.reserve(16);
    }

    void Add(GuestAsn1Object* object, int type, std::string value, int set) {
        bytes.push_back(std::move(value));
        strings.push_back({static_cast<int>(bytes.back().size()), type, reinterpret_cast<unsigned char*>(bytes.back().data()), 0});
        entries.push_back({object, &strings.back(), set, 0});
    }

    const GuestName* Build() {
        pointers.clear();
        for (auto& entry : entries) pointers.push_back(reinterpret_cast<char*>(&entry));
        stack = {static_cast<int>(pointers.size()), pointers.data(), 0, static_cast<int>(pointers.size()), nullptr};
        name = {&stack, 0, nullptr, nullptr, 0};
        return &name;
    }
};

}

int main(int argc, char** argv) {
    Require(SSLeay_nid_postfix() == 0x20000000);
    GuestThreadId id{&id, 7};
    CRYPTO_THREADID_set_numeric_nid_postfix(&id, 0x123456789abcdefull);
    Require(id.ptr == nullptr && id.val == 0x123456789abcdefull);
    Require(Throws([] { CRYPTO_THREADID_set_numeric_nid_postfix(nullptr, 1); }));

    auto* bio = BIO_new_nid_postfix(BIO_s_mem_nid_postfix());
    Require(bio != nullptr);

    unsigned char raw[] = {'a', 0x01, 'b', '\n', 'c', '\r', 0x7f, 0xe9, '~', ' '};
    GuestAsn1String octets{sizeof(raw), 4, raw, 0};
    Require(ASN1_STRING_print_nid_postfix(bio, &octets) == 1 && Take(bio) == "a.b\nc\r..~ ");
    GuestAsn1String nothing{0, 4, nullptr, 0};
    Require(ASN1_STRING_print_nid_postfix(bio, &nothing) == 1 && Take(bio).empty());
    Require(Throws([&] { ASN1_STRING_print_nid_postfix(bio, nullptr); }));

    const auto time = [](int type, const char* text) {
        return GuestAsn1String{static_cast<int>(std::strlen(text)), type, reinterpret_cast<unsigned char*>(const_cast<char*>(text)), 0};
    };
    auto utc = time(23, "150213005552Z");
    Require(ASN1_TIME_print_nid_postfix(bio, &utc) == 1 && Take(bio) == "Feb 13 00:55:52 2015 GMT");
    auto utc1999 = time(23, "991231235959Z");
    Require(ASN1_TIME_print_nid_postfix(bio, &utc1999) == 1 && Take(bio) == "Dec 31 23:59:59 1999 GMT");
    auto utc2049 = time(23, "491130120000Z");
    Require(ASN1_TIME_print_nid_postfix(bio, &utc2049) == 1 && Take(bio) == "Nov 30 12:00:00 2049 GMT");
    auto local = time(23, "150213005552");
    Require(ASN1_TIME_print_nid_postfix(bio, &local) == 1 && Take(bio) == "Feb 13 00:55:52 2015");
    auto generalized = time(24, "20500710235901Z");
    Require(ASN1_TIME_print_nid_postfix(bio, &generalized) == 1 && Take(bio) == "Jul 10 23:59:01 2050 GMT");
    auto badMonth = time(23, "151313005552Z");
    Require(ASN1_TIME_print_nid_postfix(bio, &badMonth) == 0 && Take(bio) == "Bad time value");
    auto letters = time(23, "15021x005552Z");
    Require(ASN1_TIME_print_nid_postfix(bio, &letters) == 0 && Take(bio) == "Bad time value");
    auto shortTime = time(24, "2050071023Z");
    Require(ASN1_TIME_print_nid_postfix(bio, &shortTime) == 0 && Take(bio) == "Bad time value");
    auto wrongType = time(4, "150213005552Z");
    Require(ASN1_TIME_print_nid_postfix(bio, &wrongType) == 0 && Take(bio) == "Bad time value");
    auto example = time(23, "150203005552Z");
    Require(Throws([&] { ASN1_TIME_print_nid_postfix(bio, &example); }));
    auto noSeconds = time(23, "1502130055Z");
    Require(Throws([&] { ASN1_TIME_print_nid_postfix(bio, &noSeconds); }));
    auto offset = time(23, "150213005552+0100");
    Require(Throws([&] { ASN1_TIME_print_nid_postfix(bio, &offset); }));
    auto fraction = time(24, "20150213005552.5Z");
    Require(Throws([&] { ASN1_TIME_print_nid_postfix(bio, &fraction); }));
    auto hour = time(23, "150213245552Z");
    Require(Throws([&] { ASN1_TIME_print_nid_postfix(bio, &hour); }));
    Require(Take(bio).empty());

    static const unsigned char countryOid[] = {0x55, 0x04, 0x06};
    static const unsigned char organizationOid[] = {0x55, 0x04, 0x0a};
    static const unsigned char unitOid[] = {0x55, 0x04, 0x0b};
    static const unsigned char commonNameOid[] = {0x55, 0x04, 0x03};
    static const unsigned char userIdOid[] = {0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x01};
    static const unsigned char privateOid[] = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x86, 0x8d, 0x1f, 0x01};
    static const unsigned char emailOid[] = {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01};
    GuestAsn1Object country{"C", "countryName", 14, 3, countryOid, 0};
    GuestAsn1Object organization{"O", "organizationName", 17, 3, organizationOid, 0};
    GuestAsn1Object unit{nullptr, nullptr, 0, 3, unitOid, 0};
    GuestAsn1Object commonName{"CN", "commonName", 13, 3, commonNameOid, 0};
    GuestAsn1Object userId{"UID", "userId", 458, sizeof(userIdOid), userIdOid, 0};
    GuestAsn1Object unknown{nullptr, nullptr, 0, sizeof(privateOid), privateOid, 0x0d};
    GuestAsn1Object email{"emailAddress", "emailAddress", 48, sizeof(emailOid), emailOid, 0};

    NameBuilder subject;
    subject.Add(&country, 19, "US", 0);
    subject.Add(&organization, 12, "Example, Inc.", 1);
    subject.Add(&unit, 12, "#hash", 2);
    subject.Add(&commonName, 12, "host", 3);
    subject.Add(&userId, 12, "42", 3);
    const auto* name = subject.Build();

    std::string expected = "UID=42+CN=host,OU=\\#hash,O=Example\\, Inc.,C=US";
    Require(X509_NAME_print_ex_nid_postfix(bio, name, 0, FlagRfc2253) == static_cast<int>(expected.size()) && Take(bio) == expected);
    expected = "C = US, O = \"Example, Inc.\", OU = \"#hash\", CN = host + UID = 42";
    Require(X509_NAME_print_ex_nid_postfix(bio, name, 0, FlagOneline) == static_cast<int>(expected.size()) && Take(bio) == expected);
    expected = "  countryName = US\n  organizationName = Example, Inc.\n  organizationalUnitName = #hash\n  commonName = host + userId = 42";
    Require(X509_NAME_print_ex_nid_postfix(bio, name, 2, SepMultiline | SpcEq | FnLn) == static_cast<int>(expected.size()) && Take(bio) == expected);
    expected = "2.5.4.6=US,2.5.4.10=Example, Inc.,2.5.4.11=#hash,2.5.4.3=host+0.9.2342.19200300.100.1.1=42";
    Require(X509_NAME_print_ex_nid_postfix(bio, name, 0, SepCommaPlus | FnOid) == static_cast<int>(expected.size()) && Take(bio) == expected);
    expected = "42 + host; #hash; Example, Inc.; US";
    Require(X509_NAME_print_ex_nid_postfix(bio, name, 0, SepSplusSpc | FnNone | DnRev) == static_cast<int>(expected.size()) && Take(bio) == expected);

    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, name, 0, 0); }));
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, name, 0, FlagMultiline); }));
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, name, 4, FlagOneline); }));
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, name, 0, SepCommaPlus | ShowType); }));
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, name, 0, SepCommaPlus | DumpAll); }));
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, nullptr, 0, FlagOneline); }));
    Require(Take(bio).empty());

    NameBuilder special;
    special.Add(&commonName, 12, " a\"b\\c<d>e;f+g ", 0);
    special.Add(&unknown, 12, "x", 1);
    const auto* specials = special.Build();
    expected = "CN=\\ a\\\"b\\\\c\\<d\\>e\\;f\\+g\\ ,1.3.6.1.4.1.99999.1=x";
    Require(X509_NAME_print_ex_nid_postfix(bio, specials, 0, Rfc2253Strings | SepCommaPlus) == static_cast<int>(expected.size()) && Take(bio) == expected);
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, specials, 0, FlagRfc2253); }));
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, specials, 0, FlagOneline); }));

    NameBuilder wide;
    wide.Add(&commonName, 30, std::string("\x00\x63\x00\xe9", 4), 0);
    const auto* wideName = wide.Build();
    expected = "CN=c\xc3\xa9";
    Require(X509_NAME_print_ex_nid_postfix(bio, wideName, 0, Utf8Convert | SepCommaPlus) == static_cast<int>(expected.size()) && Take(bio) == expected);
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, wideName, 0, SepCommaPlus); }));
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, wideName, 0, FlagOneline); }));

    NameBuilder control;
    control.Add(&commonName, 12, std::string("a\tb"), 0);
    const auto* controlName = control.Build();
    Require(X509_NAME_print_ex_nid_postfix(bio, controlName, 0, SepCommaPlus) == 6 && Take(bio) == "CN=a\tb");
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, controlName, 0, SepCommaPlus | EscCtrl); }));

    NameBuilder mail;
    mail.Add(&email, 22, "a@b", 0);
    const auto* mailName = mail.Build();
    Require(X509_NAME_print_ex_nid_postfix(bio, mailName, 0, SepCommaPlus | FnLn) == 16 && Take(bio) == "emailAddress=a@b");
    Require(Throws([&] { X509_NAME_print_ex_nid_postfix(bio, mailName, 0, SepCommaPlus); }));

    NameBuilder empty;
    Require(X509_NAME_print_ex_nid_postfix(bio, empty.Build(), 0, FlagOneline) == 0 && Take(bio).empty());

    Require(CONF_modules_load_file_nid_postfix("guest_libressl_no_such.cnf", nullptr, 0x10) == 1);
    Require(CONF_modules_load_file_nid_postfix("guest_libressl_no_such.cnf", "app", 0x10 | 0x20) == 1);
    Require(Throws([] { CONF_modules_load_file_nid_postfix("guest_libressl_no_such.cnf", nullptr, 0); }));
    if (argc > 1) {
        std::ofstream(argv[1], std::ios::binary) << "openssl_conf = openssl_init" << std::endl;
        Require(Throws([&] { CONF_modules_load_file_nid_postfix(argv[1], nullptr, 0x10); }));
        std::remove(argv[1]);
    }

    Require(BIO_free_nid_postfix(bio) == 1);
    return 0;
}
