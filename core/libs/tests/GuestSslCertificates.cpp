#include "prx/libSceSsl/SslTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "SslCertificateFixture.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

extern "C" {
int APS5_VABI sceSslInit_nid_postfix(std::uint64_t);
int APS5_VABI sceSslTerm_nid_postfix(int);
int APS5_VABI sceSslGetMemoryPoolStats(int, SceSsl::MemoryPoolStats*);
int APS5_VABI sceSslLoadCert(int, int, SceSsl::Data**, SceSsl::Data*, SceSsl::Data*);
int APS5_VABI sceSslUnloadCert(int);
int APS5_VABI sceSslGetCaList(int, SceSsl::CaList*);
int APS5_VABI sceSslFreeCaList(int, SceSsl::CaList*);
int APS5_VABI sceSslGetCaCerts(int, SceSsl::CaCerts*);
int APS5_VABI sceSslFreeCaCerts(int, SceSsl::CaCerts*);
void* APS5_VABI sceSslGetSubjectName(int, void*);
void* APS5_VABI sceSslGetIssuerName(int, void*);
int APS5_VABI sceSslFreeSslCertName(int, void*);
int APS5_VABI sceSslGetNameEntryCount(int, void*);
int APS5_VABI sceSslGetNameEntryInfo(int, void*, int, char*, std::size_t, std::uint8_t*, std::size_t, std::size_t*);
int APS5_VABI sceSslGetSerialNumber(int, void*, std::uint8_t*, std::size_t*);
int APS5_VABI sceSslGetNotBefore(int, void*, SceSsl::RtcTick*);
int APS5_VABI sceSslGetNotAfter(int, void*, SceSsl::RtcTick*);
int APS5_VABI sceSslGetFingerprint(int, void*, int, std::uint8_t*);
int APS5_VABI sceSslGetPem(int, void*, char*, std::size_t*);
}

namespace {

constexpr int NotFound = static_cast<int>(0x8095f004);
constexpr int InvalidId = static_cast<int>(0x8095f006);
constexpr int InvalidValue = static_cast<int>(0x8095f007);
constexpr int InvalidArg = static_cast<int>(0x8095177a);

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Error, typename Action>
void Throws(Action action, const char* message) {
    bool caught = false;
    try { action(); } catch (const Error&) { caught = true; }
    Check(caught, message);
}

SceSsl::Data Data(const char* text) {
    return {reinterpret_cast<std::uint8_t*>(const_cast<char*>(text)), std::strlen(text)};
}

SceSsl::Data Der() {
    return {const_cast<std::uint8_t*>(RootDer.data()), RootDer.size()};
}

SceSsl::MemoryPoolStats Stats(int context) {
    SceSsl::MemoryPoolStats stats{};
    Check(sceSslGetMemoryPoolStats(context, &stats) == 0, "pool stats failed");
    Check(stats.currentInuseSize <= stats.maxInuseSize && stats.maxInuseSize <= stats.poolSize && !stats.reserved,
        "pool accounting invariant broken");
    return stats;
}

void LoadRoot(int context) {
    auto data = Data(RootPem);
    auto* pointer = &data;
    Check(sceSslLoadCert(context, 1, &pointer, nullptr, nullptr) == 0, "root import failed");
}

std::pair<std::string, std::string> Entry(int context, void* name, int index) {
    std::array<char, 64> oid{};
    std::array<std::uint8_t, 128> value{};
    std::size_t length = 999;
    Check(sceSslGetNameEntryInfo(context, name, index, oid.data(), oid.size(), value.data(), value.size(), &length) == 0,
        "name entry failed");
    return {oid.data(), std::string(reinterpret_cast<char*>(value.data()), length)};
}

void TestCertificateReaders() {
    const int context = sceSslInit_nid_postfix(1024 * 1024);
    const auto before = Stats(context);
    Check(before.poolSize == 1024 * 1024 && !before.currentInuseSize && !before.maxInuseSize, "initial pool stats wrong");
    SceSsl::CaList empty{};
    Check(sceSslGetCaList(context, &empty) == NotFound && !empty.certs && !empty.num, "empty CA list wrong");
    LoadRoot(context);
    const auto loaded = Stats(context);
    Check(loaded.currentInuseSize > RootDer.size(), "certificate backend allocations not counted");
    SceSsl::CaList list{};
    Check(sceSslGetCaList(context, &list) == 0 && list.num == 1 && list.certs[0], "CA handles missing");
    auto* certificate = list.certs[0];
    auto* subject = sceSslGetSubjectName(context, certificate);
    auto* issuer = sceSslGetIssuerName(context, certificate);
    Check(subject && issuer && subject != issuer, "independent name handles missing");
    Check(sceSslGetNameEntryCount(context, subject) == 3 && sceSslGetNameEntryCount(context, issuer) == 3,
        "root distinguished name count wrong");
    const std::array<std::pair<std::string, std::string>, 3> expected{{{"2.5.4.6", "GB"},
        {"2.5.4.10", "AnyPS5 Tests"}, {"2.5.4.3", "AnyPS5 fixture root"}}};
    for (int index = 0; index < 3; ++index) {
        Check(Entry(context, subject, index) == expected[index], "subject OID/value mismatch");
        Check(Entry(context, issuer, index) == expected[index], "issuer OID/value mismatch");
    }
    std::size_t serialSize = 0;
    Check(sceSslGetSerialNumber(context, certificate, nullptr, &serialSize) == 0 && serialSize == 3,
        "serial required length wrong");
    std::array<std::uint8_t, 5> serial{0x55, 0x55, 0x55, 0x55, 0x55};
    Check(sceSslGetSerialNumber(context, certificate, serial.data() + 1, &serialSize) == 0 &&
        serial == std::array<std::uint8_t, 5>{0x55, 0, 0x80, 0xa1, 0x55}, "serial DER integer content/canaries wrong");
    SceSsl::CaCerts encoded{};
    Check(sceSslGetCaCerts(context, &encoded) == 0 && encoded.num == 1 && encoded.certs[0].size == RootDer.size() &&
        std::memcmp(encoded.certs[0].ptr, RootDer.data(), RootDer.size()) == 0, "CA DER roundtrip failed");
    const auto peak = Stats(context).maxInuseSize;
    Check(sceSslUnloadCert(context) == 0, "unload failed");
    Check(sceSslGetCaList(context, &empty) == NotFound, "unloaded CA remained in store");
    Check(Entry(context, subject, 2) == expected[2], "copied name expired on unload");
    serialSize = 3;
    Check(sceSslGetSerialNumber(context, certificate, serial.data() + 1, &serialSize) == 0, "CA list did not retain certificate");
    Check(std::memcmp(encoded.certs[0].ptr, RootDer.data(), RootDer.size()) == 0, "DER snapshot expired on unload");
    auto savedList = list;
    Check(sceSslFreeCaList(context, &list) == 0 && !list.certs && !list.num, "list free did not clear descriptor");
    Throws<std::invalid_argument>([&] { sceSslFreeCaList(context, &savedList); }, "duplicate list free accepted");
    Throws<std::invalid_argument>([&] { sceSslGetSubjectName(context, certificate); }, "expired certificate handle accepted");
    Check(sceSslFreeSslCertName(context, subject) == 0 && sceSslFreeSslCertName(context, issuer) == 0, "name free failed");
    Throws<std::invalid_argument>([&] { sceSslGetNameEntryCount(context, subject); }, "freed name handle accepted");
    Check(sceSslFreeCaCerts(context, &encoded) == 0 && !encoded.certs && !encoded.num && !encoded.pool, "DER free failed");
    const auto released = Stats(context);
    Check(released.currentInuseSize == 0 && released.maxInuseSize == peak, "pool usage not released or peak forgotten");
    Check(sceSslTerm_nid_postfix(context) == 0, "context term failed");
    SceSsl::MemoryPoolStats terminated{1, 1, 1, 1};
    Check(sceSslGetMemoryPoolStats(context, &terminated) == InvalidId && !terminated.poolSize && !terminated.maxInuseSize &&
        !terminated.currentInuseSize && !terminated.reserved, "terminated context accepted");
}

bool Digest(int context, void* certificate, int type, const std::vector<std::uint8_t>& expected, std::size_t size) {
    std::array<std::uint8_t, 66> output;
    output.fill(0x55);
    if (sceSslGetFingerprint(context, certificate, type, output.data() + 1) != 0) return false;
    if (output[0] != 0x55 || output[size + 1] != 0x55) return false;
    return expected.empty() || std::memcmp(output.data() + 1, expected.data(), expected.size()) == 0;
}

void TestValidityFingerprintAndPem() {
    const int context = sceSslInit_nid_postfix(1024 * 1024);
    const int other = sceSslInit_nid_postfix(1024 * 1024);
    LoadRoot(context);
    SceSsl::CaList list{};
    Check(sceSslGetCaList(context, &list) == 0 && list.num == 1 && list.certs[0], "CA handle missing");
    auto* certificate = list.certs[0];
    SceSsl::RtcTick begin{}, limit{};
    Check(sceSslGetNotBefore(context, certificate, &begin) == 0 && begin.tick == 63927094442000000ULL, "notBefore tick wrong");
    Check(sceSslGetNotAfter(context, certificate, &limit) == 0 && limit.tick == 64242454442000000ULL, "notAfter tick wrong");
    Check(sceSslGetNotBefore(context, certificate, nullptr) == InvalidArg && sceSslGetNotAfter(context, certificate, nullptr) == InvalidArg,
        "null validity output accepted");
    Throws<std::invalid_argument>([&] { sceSslGetNotBefore(other, certificate, &begin); }, "foreign certificate validity read");
    Throws<std::invalid_argument>([&] { sceSslGetNotAfter(context, nullptr, &limit); }, "null certificate validity read");
    Check(Digest(context, certificate, 0, {0x71, 0x3c, 0xe5, 0x20, 0x88, 0x2b, 0x6b, 0xd0, 0x17, 0x85, 0x07, 0xde, 0xbd, 0xdc, 0xa9, 0x4f}, 16),
        "MD5 fingerprint wrong");
    Check(Digest(context, certificate, 1, {0x47, 0xe0, 0x89, 0xab, 0xb7, 0x02, 0xee, 0xb6, 0x2f, 0x8b, 0x5b, 0x7a, 0x13, 0xfe, 0x82, 0x6d,
        0x2b, 0xe8, 0xee, 0xa7}, 20), "SHA-1 fingerprint wrong");
    Check(Digest(context, certificate, 2, {}, 28), "SHA-224 fingerprint size wrong");
    Check(Digest(context, certificate, 3, {0x3d, 0x7d, 0xac, 0xd3, 0x04, 0x90, 0x42, 0xe1, 0x57, 0x19, 0x87, 0x27, 0x54, 0x45, 0xae, 0xed,
        0xc4, 0xea, 0xd6, 0xd4, 0x20, 0xa0, 0x7f, 0xc4, 0xf5, 0x26, 0xc1, 0xa0, 0x85, 0x87, 0x5a, 0x4c}, 32), "SHA-256 fingerprint wrong");
    Check(Digest(context, certificate, 4, {}, 48) && Digest(context, certificate, 5, {}, 64), "SHA-384/512 fingerprint size wrong");
    std::array<std::uint8_t, 64> digest{};
    Check(sceSslGetFingerprint(context, certificate, -1, digest.data()) == InvalidArg &&
        sceSslGetFingerprint(context, certificate, 6, digest.data()) == InvalidArg, "unknown hash type accepted");
    Check(sceSslGetFingerprint(context, certificate, 3, nullptr) == InvalidArg, "null fingerprint output accepted");
    Throws<std::invalid_argument>([&] { sceSslGetFingerprint(other, certificate, 3, digest.data()); }, "foreign certificate hashed");
    const auto pemSize = std::strlen(RootPem) + 1;
    std::size_t length = ~std::size_t{0};
    Check(sceSslGetPem(context, certificate, nullptr, &length) == 0 && length == pemSize, "PEM size query wrong");
    std::vector<char> pem(length + 2, 'Z');
    Check(sceSslGetPem(context, certificate, pem.data(), &length) == 0 && length == pemSize, "PEM copy length wrong");
    Check(std::strcmp(pem.data(), RootPem) == 0 && pem[length] == 'Z' && pem[length + 1] == 'Z', "PEM bytes or guards wrong");
    std::array<char, 4> small{'x', 'x', 'x', 'x'};
    length = 2;
    Throws<std::length_error>([&] { sceSslGetPem(context, certificate, small.data(), &length); }, "short PEM buffer accepted");
    Check(length == pemSize && small == std::array<char, 4>{'x', 'x', 'x', 'x'}, "short PEM buffer written or length not reported");
    Check(sceSslGetPem(context, certificate, pem.data(), nullptr) == InvalidArg, "null PEM length accepted");
    Throws<std::invalid_argument>([&] { sceSslGetPem(other, certificate, nullptr, &length); }, "foreign certificate PEM read");
    Check(sceSslUnloadCert(context) == 0 && sceSslFreeCaList(context, &list) == 0, "unload or list free failed");
    Throws<std::invalid_argument>([&] { sceSslGetNotBefore(context, certificate, &begin); }, "expired certificate validity read");
    Throws<std::invalid_argument>([&] { sceSslGetFingerprint(context, certificate, 3, digest.data()); }, "expired certificate hashed");
    Throws<std::invalid_argument>([&] { sceSslGetPem(context, certificate, nullptr, &length); }, "expired certificate PEM read");
    Check(Stats(context).currentInuseSize == 0, "certificate queries leaked pool memory");
    sceSslTerm_nid_postfix(context);
    sceSslTerm_nid_postfix(other);
}

void TestBuffersAndIsolation() {
    const int context = sceSslInit_nid_postfix(1024 * 1024);
    const int other = sceSslInit_nid_postfix(1024 * 1024);
    LoadRoot(context);
    SceSsl::CaList list{};
    sceSslGetCaList(context, &list);
    auto* name = sceSslGetSubjectName(context, list.certs[0]);
    std::array<char, 12> oid;
    std::array<std::uint8_t, 12> value;
    oid.fill('!'); value.fill(0x55);
    std::size_t length = 0;
    Throws<std::length_error>([&] { sceSslGetNameEntryInfo(context, name, 0, oid.data(), 1, value.data(), value.size(), &length); },
        "short OID accepted");
    Check(length == 2 && oid[0] == '!' && value[0] == 0x55, "short OID partially wrote outputs");
    Throws<std::length_error>([&] { sceSslGetNameEntryInfo(context, name, 0, oid.data(), oid.size(), value.data(), 1, &length); },
        "short value accepted");
    Check(oid[0] == '!' && value[0] == 0x55, "short value partially wrote outputs");
    Check(sceSslGetNameEntryInfo(context, name, 0, oid.data() + 1, 8, value.data() + 1, 2, &length) == 0 &&
        oid[0] == '!' && oid[9] == '!' && value[0] == 0x55 && value[3] == 0x55 && value[1] == 'G' && value[2] == 'B',
        "name output boundary wrong");
    length = 2;
    Throws<std::length_error>([&] { sceSslGetSerialNumber(context, list.certs[0], value.data(), &length); }, "short serial accepted");
    Check(length == 3 && value[0] == 0x55, "short serial did not report length or wrote output");
    Check(sceSslGetNameEntryInfo(context, name, -1, nullptr, 0, nullptr, 0, &length) == InvalidArg, "negative entry index accepted");
    Check(sceSslGetNameEntryInfo(context, name, 3, nullptr, 0, nullptr, 0, &length) == InvalidArg, "past-end entry index accepted");
    Check(sceSslGetNameEntryInfo(context, name, 0, nullptr, 1, nullptr, 0, &length) == InvalidArg, "null OID buffer accepted");
    Check(sceSslGetSerialNumber(context, list.certs[0], nullptr, nullptr) == InvalidArg, "null serial length accepted");
    Check(sceSslGetMemoryPoolStats(context, nullptr) == InvalidValue, "null stats accepted");
    Throws<std::invalid_argument>([&] { sceSslGetIssuerName(other, list.certs[0]); }, "foreign certificate accepted");
    Throws<std::invalid_argument>([&] { sceSslFreeSslCertName(other, name); }, "foreign name freed");
    Throws<std::invalid_argument>([&] { sceSslFreeCaList(other, &list); }, "foreign list freed");
    SceSsl::CaCerts foreign{reinterpret_cast<SceSsl::Data*>(&length), 3, &length};
    Check(sceSslFreeCaCerts(context, &foreign) == 0 && !foreign.certs && !foreign.num && !foreign.pool, "foreign CA bytes descriptor not cleared");
    Check(Stats(other).currentInuseSize == 0, "other context pool polluted");
    sceSslTerm_nid_postfix(context);
    sceSslTerm_nid_postfix(other);
}

void TestLoadRollbackAndKeys() {
    const int context = sceSslInit_nid_postfix(1024 * 1024);
    LoadRoot(context);
    const auto original = Stats(context).currentInuseSize;
    auto root = Der();
    auto malformed = Data("invalid certificate");
    std::array<SceSsl::Data*, 2> badList{&root, &malformed};
    Throws<std::invalid_argument>([&] { sceSslLoadCert(context, 2, badList.data(), nullptr, nullptr); }, "partially invalid CA list accepted");
    Check(Stats(context).currentInuseSize == original, "failed CA import leaked pool allocations");
    const auto partialPem = std::string(RootPem) + "-----BEGIN CERTIFICATE-----\n!!!!\n-----END CERTIFICATE-----\n";
    auto partial = Data(partialPem.c_str());
    auto* partialPointer = &partial;
    Throws<std::invalid_argument>([&] { sceSslLoadCert(context, 1, &partialPointer, nullptr, nullptr); }, "partially parsed PEM chain accepted");
    Check(Stats(context).currentInuseSize == original, "partial PEM parse leaked allocations");
    auto client = Data(ClientPem);
    auto wrongKey = Data(RootKey);
    auto key = Data(ClientKey);
    auto* rootPointer = &root;
    Throws<std::invalid_argument>([&] { sceSslLoadCert(context, 1, &rootPointer, &client, &wrongKey); }, "mismatched private key accepted");
    Check(Stats(context).currentInuseSize == original, "failed key import changed old pool state");
    SceSsl::CaList list{};
    Check(sceSslGetCaList(context, &list) == 0 && list.num == 1, "failed import committed new CA");
    sceSslFreeCaList(context, &list);
    Throws<std::invalid_argument>([&] { sceSslLoadCert(context, 0, nullptr, &client, nullptr); }, "identity without key accepted");
    const auto chain = std::string(ClientPem) + RootPem;
    auto clientChain = Data(chain.c_str());
    Check(sceSslLoadCert(context, 0, nullptr, &clientChain, &key) == 0, "valid client chain/key rejected");
    Check(Stats(context).currentInuseSize > original, "client identity allocations not counted");
    auto* chainPointer = &clientChain;
    Check(sceSslLoadCert(context, 1, &chainPointer, nullptr, nullptr) == 0, "PEM chain import failed");
    Check(sceSslGetCaList(context, &list) == 0 && list.num == 3, "PEM chain members not enumerated");
    auto* subject = sceSslGetSubjectName(context, list.certs[1]);
    auto* issuer = sceSslGetIssuerName(context, list.certs[1]);
    Check(sceSslGetNameEntryCount(context, subject) == 5 && sceSslGetNameEntryCount(context, issuer) == 3,
        "multi-valued name entries lost");
    Check(Entry(context, subject, 4).second == "AnyPS5 fixture client" && Entry(context, issuer, 2).second == "AnyPS5 fixture root",
        "client issuer/subject confused");
    sceSslFreeSslCertName(context, subject);
    sceSslFreeSslCertName(context, issuer);
    sceSslFreeCaList(context, &list);
    std::vector<std::uint8_t> trailing(RootDer.begin(), RootDer.end());
    trailing.push_back(0);
    SceSsl::Data trailingData{trailing.data(), trailing.size()};
    auto* trailingPointer = &trailingData;
    Throws<std::invalid_argument>([&] { sceSslLoadCert(context, 1, &trailingPointer, nullptr, nullptr); }, "trailing DER accepted");
    sceSslUnloadCert(context);
    Check(Stats(context).currentInuseSize == 0, "identity/chain unload leaked allocations");
    sceSslTerm_nid_postfix(context);
}

void TestPoolExhaustion() {
    const int context = sceSslInit_nid_postfix(RootDer.size() + 50);
    auto data = Der();
    auto* pointer = &data;
    Throws<std::bad_alloc>([&] { sceSslLoadCert(context, 1, &pointer, nullptr, nullptr); }, "pool exhaustion was ignored");
    const auto stats = Stats(context);
    Check(stats.currentInuseSize == 0 && stats.maxInuseSize > 0, "partial parse allocation rollback failed");
    SceSsl::CaList list{};
    Check(sceSslGetCaList(context, &list) == NotFound, "failed allocation added authority");
    sceSslTerm_nid_postfix(context);
}

void TestConcurrentContexts() {
    std::array<std::exception_ptr, 4> errors{};
    std::vector<std::thread> threads;
    for (unsigned index = 0; index < errors.size(); ++index) {
        threads.emplace_back([&, index] {
            try {
                const int context = sceSslInit_nid_postfix(1024 * 1024);
                for (unsigned iteration = 0; iteration < 8; ++iteration) {
                    LoadRoot(context);
                    SceSsl::CaList list{};
                    sceSslGetCaList(context, &list);
                    auto* name = sceSslGetSubjectName(context, list.certs[0]);
                    Check(Entry(context, name, 0).second == "GB", "concurrent name data mixed contexts");
                    sceSslFreeSslCertName(context, name);
                    sceSslFreeCaList(context, &list);
                    sceSslUnloadCert(context);
                    Check(Stats(context).currentInuseSize == 0, "concurrent context leaked allocations");
                }
                sceSslTerm_nid_postfix(context);
            } catch (...) { errors[index] = std::current_exception(); }
        });
    }
    for (auto& thread : threads) thread.join();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
}

}

int main() {
    try {
        TestCertificateReaders();
        TestValidityFingerprintAndPem();
        TestBuffersAndIsolation();
        TestLoadRollbackAndKeys();
        TestPoolExhaustion();
        TestConcurrentContexts();
        std::puts("SSL certificate/context tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "SSL certificate/context test failed: %s\n", error.what());
        return 1;
    }
}
