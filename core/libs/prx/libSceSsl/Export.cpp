#include "SslTypes.hpp"
#include "prx/libc/include/General.hpp"

#include <mbedtls/asn1.h>
#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/oid.h>
#include <mbedtls/pem.h>
#include <mbedtls/pk.h>
#include <mbedtls/platform.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/x509_crt.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

extern "C" int APS5_VABI sceNetGetpeername(int s, void* addr, std::uint32_t* addrlen);

namespace {

constexpr int ErrorNotFound = static_cast<int>(0x8095f004);
constexpr int ErrorInvalidId = static_cast<int>(0x8095f006);
constexpr int ErrorInvalidValue = static_cast<int>(0x8095f007);
constexpr int ErrorInvalidArg = static_cast<int>(0x8095177a);
constexpr int SslVersionTls12 = 5;
constexpr std::size_t MaxAlpnSize = 0xffff;

constexpr std::int64_t TicksPerSecond = 1000000;
constexpr std::int64_t TicksPerMinute = 60 * TicksPerSecond;
constexpr std::int64_t TicksPerHour = 60 * TicksPerMinute;
constexpr std::int64_t TicksPerDay = 24 * TicksPerHour;
constexpr std::int64_t UnixEpochDays = 719162;

std::int64_t DaysFromCivil(std::int64_t year, unsigned month, unsigned day) {
    year -= month <= 2;
    const std::int64_t era = (year >= 0 ? year : year - 399) / 400;
    const auto yearOfEra = static_cast<unsigned>(year - era * 400);
    const unsigned dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + static_cast<std::int64_t>(dayOfEra) - 719468 + UnixEpochDays;
}

std::uint64_t TimeToTick(const mbedtls_x509_time& time) {
    return static_cast<std::uint64_t>(DaysFromCivil(time.year, static_cast<unsigned>(time.mon), static_cast<unsigned>(time.day)) * TicksPerDay +
        time.hour * TicksPerHour + time.min * TicksPerMinute + time.sec * TicksPerSecond);
}

struct Pool {
    std::size_t size, current = 0, peak = 0;
    bool exhausted = false;
};

struct alignas(std::max_align_t) Allocation {
    Pool* pool;
    std::size_t size;
};

thread_local Pool* currentPool = nullptr;

void* PoolCalloc(std::size_t count, std::size_t size) noexcept {
    if (!currentPool) return nullptr;
    if (size && count > std::numeric_limits<std::size_t>::max() / size) {
        currentPool->exhausted = true;
        return nullptr;
    }
    const auto bytes = count * size;
    if (bytes > currentPool->size - currentPool->current || bytes > std::numeric_limits<std::size_t>::max() - sizeof(Allocation)) {
        currentPool->exhausted = true;
        return nullptr;
    }
    auto* allocation = static_cast<Allocation*>(std::calloc(1, sizeof(Allocation) + bytes));
    if (!allocation) {
        currentPool->exhausted = true;
        return nullptr;
    }
    allocation->pool = currentPool;
    allocation->size = bytes;
    currentPool->current += bytes;
    currentPool->peak = std::max(currentPool->peak, currentPool->current);
    return allocation + 1;
}

void PoolFree(void* pointer) noexcept {
    if (!pointer) return;
    auto* allocation = static_cast<Allocation*>(pointer) - 1;
    allocation->pool->current -= allocation->size;
    mbedtls_platform_zeroize(pointer, allocation->size);
    std::free(allocation);
}

struct PoolScope {
    Pool* previous;
    explicit PoolScope(Pool& pool) : previous(std::exchange(currentPool, &pool)) { pool.exhausted = false; }
    ~PoolScope() { currentPool = previous; }
};

struct PoolDeleter {
    void operator()(std::uint8_t* bytes) const { PoolFree(bytes); }
};

using Block = std::unique_ptr<std::uint8_t, PoolDeleter>;

Block Allocate(std::size_t size) {
    Block block(static_cast<std::uint8_t*>(PoolCalloc(1, std::max<std::size_t>(1, size))));
    if (!block) throw std::bad_alloc();
    return block;
}

std::size_t Add(std::size_t left, std::size_t right) {
    if (right > std::numeric_limits<std::size_t>::max() - left) throw std::length_error("SSL allocation size overflow");
    return left + right;
}

void CheckBackend(int error, const char* message) {
    if (error == 0) return;
    if (currentPool && currentPool->exhausted) throw std::bad_alloc();
    throw std::invalid_argument(message);
}

struct CertificateChain;

struct CertificateHandle {
    std::weak_ptr<CertificateChain> owner;
    const mbedtls_x509_crt* certificate;
};

struct CertificateChain {
    mbedtls_x509_crt root;
    std::vector<CertificateHandle*> handles;
    CertificateChain() { mbedtls_x509_crt_init(&root); }
    ~CertificateChain() { mbedtls_x509_crt_free(&root); }
};

struct PrivateKey {
    mbedtls_pk_context key;
    PrivateKey() { mbedtls_pk_init(&key); }
    ~PrivateKey() { mbedtls_pk_free(&key); }
};

struct Random {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context drbg;
    Random() {
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&drbg);
    }
    ~Random() {
        mbedtls_ctr_drbg_free(&drbg);
        mbedtls_entropy_free(&entropy);
    }
    void Seed() {
        CheckBackend(mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, nullptr, 0), "SSL random generator initialization failed");
    }
};

struct Identity {
    std::shared_ptr<CertificateChain> chain;
    PrivateKey key;
};

struct Snapshot {
    Block data;
    std::vector<std::shared_ptr<CertificateChain>> certificates;
    std::size_t count;
};

struct NameEntry {
    char* oid;
    std::size_t oidSize;
    std::uint8_t* value;
    std::size_t valueSize;
};

struct NameData {
    Block data;
    std::size_t count;
};

struct NameHandle {
    std::unique_ptr<NameData> name;
};

struct Context {
    Pool pool;
    std::vector<std::shared_ptr<CertificateChain>> authorities;
    std::shared_ptr<Identity> identity;
    std::vector<std::unique_ptr<CertificateHandle>> certificateHandles;
    std::vector<std::unique_ptr<NameHandle>> names;
    std::unordered_map<void*, Snapshot> caLists;
    std::unordered_map<void*, Snapshot> caCerts;
    explicit Context(std::size_t size) : pool{size} {}
};

using VerifyCallback = int(APS5_VABI*)(int, unsigned int, void**, int, void*);

struct Connection {
    int context;
    int socket;
    std::string host;
    VerifyCallback verifyCallback;
    void* verifyArgument;
    std::uint32_t enabledVerifyOptions = 0;
    std::uint32_t disabledVerifyOptions = 0;
    int minimumVersion = 0;
    std::string alpn;
    bool closed = false;
};

struct Runtime {
    std::mutex mutex;
    std::unordered_map<int, std::unique_ptr<Context>> contexts;
    std::unordered_map<int, Connection> connections;
    unsigned nextHandle = 1;
    Runtime() {
        if (mbedtls_platform_set_calloc_free(PoolCalloc, PoolFree) != 0)
            throw std::runtime_error("SSL allocator initialization failed");
    }
};

Runtime& State() {
    static auto* runtime = new Runtime;
    return *runtime;
}

Context& GetContext(int handle) {
    const auto found = State().contexts.find(handle);
    if (found == State().contexts.end()) throw std::invalid_argument("Invalid SSL context");
    return *found->second;
}

int AllocateHandle(Runtime& state) {
    if (state.nextHandle > static_cast<unsigned>(std::numeric_limits<int>::max())) throw std::overflow_error("SSL handles exhausted");
    return static_cast<int>(state.nextHandle++);
}

Connection& RequireConnection(const char* function, int connectionId) {
    auto& state = State();
    const auto connection = state.connections.find(connectionId);
    if (connection == state.connections.end()) throw std::invalid_argument(std::string(function) + ": unknown connection");
    if (!state.contexts.contains(connection->second.context))
        throw std::invalid_argument(std::string(function) + ": the connection's context was terminated");
    return connection->second;
}

std::string Endpoint(const Connection& connection) {
    std::uint8_t address[28]{};
    std::uint32_t length = sizeof(address);
    if (sceNetGetpeername(connection.socket, address, &length) != 0 || length < 4)
        return connection.host + " (socket " + std::to_string(connection.socket) + " has no peer)";
    return connection.host + ":" + std::to_string((address[2] << 8) | address[3]);
}

bool ValidAlpn(const char* protocols) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(protocols);
    const auto size = std::strlen(protocols);
    if (size == 0 || size > MaxAlpnSize) return false;
    for (std::size_t offset = 0; offset < size; offset += 1 + bytes[offset])
        if (bytes[offset] > size - offset - 1) return false;
    return true;
}

std::string AlpnNames(const std::string& protocols) {
    if (protocols.empty()) return "none";
    std::string names;
    for (std::size_t offset = 0; offset < protocols.size(); offset += 1 + static_cast<unsigned char>(protocols[offset])) {
        if (!names.empty()) names += ',';
        names.append(protocols, offset + 1, static_cast<unsigned char>(protocols[offset]));
    }
    return names;
}

std::string Settings(const Connection& connection) {
    char options[96];
    std::snprintf(options, sizeof(options), "verify options enabled 0x%x, disabled 0x%x, minimum version %d",
        connection.enabledVerifyOptions, connection.disabledVerifyOptions, connection.minimumVersion);
    return std::string(options) + ", ALPN " + AlpnNames(connection.alpn);
}

[[noreturn]] void ThrowNoTransport(const char* function, int connectionId, const char* operation) {
    Connection connection;
    {
        std::lock_guard lock(State().mutex);
        connection = RequireConnection(function, connectionId);
    }
    if (connection.closed)
        throw std::runtime_error(std::string(function) + ": the connection with " + Endpoint(connection) + " was closed with sceSslClose");
    throw std::runtime_error(std::string(function) + ": no TLS transport for " + operation + " with " + Endpoint(connection) + " (" +
        Settings(connection) + ")");
}

std::pair<std::shared_ptr<CertificateChain>, const mbedtls_x509_crt*> GetCertificate(Context& context, void* handle) {
    const auto found = std::find_if(context.certificateHandles.begin(), context.certificateHandles.end(),
        [&](const auto& entry) { return entry.get() == handle; });
    if (found == context.certificateHandles.end()) throw std::invalid_argument("Invalid SSL certificate handle");
    auto owner = (*found)->owner.lock();
    if (!owner) throw std::invalid_argument("Expired SSL certificate handle");
    return {std::move(owner), (*found)->certificate};
}

NameHandle& GetName(Context& context, void* handle) {
    const auto found = std::find_if(context.names.begin(), context.names.end(),
        [&](const auto& entry) { return entry.get() == handle; });
    if (found == context.names.end() || !(*found)->name) throw std::invalid_argument("Invalid SSL name handle");
    return **found;
}

struct Input {
    Block bytes;
    std::size_t size;
};

Input CopyInput(const SceSsl::Data& data) {
    if (!data.ptr || !data.size) throw std::invalid_argument("Empty SSL certificate/key data");
    auto bytes = Allocate(Add(data.size, 1));
    std::memcpy(bytes.get(), data.ptr, data.size);
    auto size = data.size;
    std::size_t first = 0;
    while (first < size && (bytes.get()[first] == ' ' || bytes.get()[first] == '\r' || bytes.get()[first] == '\n' || bytes.get()[first] == '\t')) ++first;
    const bool pem = first < size && bytes.get()[first] == '-';
    if (pem) {
        if (bytes.get()[size - 1] != 0) ++size;
        if (std::memchr(bytes.get(), 0, size - 1)) throw std::invalid_argument("Embedded null in SSL PEM data");
    } else {
        auto* position = bytes.get();
        std::size_t length = 0;
        CheckBackend(mbedtls_asn1_get_tag(&position, bytes.get() + size, &length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE),
            "Invalid SSL DER sequence");
        if (length != static_cast<std::size_t>(bytes.get() + size - position)) throw std::invalid_argument("Trailing SSL DER data");
    }
    return {std::move(bytes), size};
}

std::shared_ptr<CertificateChain> ParseCertificate(const SceSsl::Data& data) {
    auto chain = std::make_shared<CertificateChain>();
    const auto input = CopyInput(data);
    CheckBackend(mbedtls_x509_crt_parse(&chain->root, input.bytes.get(), input.size), "Invalid SSL certificate chain");
    return chain;
}

std::size_t CertificateCount(const Context& context) {
    std::size_t count = 0;
    for (const auto& chain : context.authorities) count = Add(count, chain->handles.size());
    return count;
}

void* CopyName(Context& context, const mbedtls_x509_name& name) {
    struct Entry {
        std::string oid;
        const mbedtls_x509_name* name;
    };
    std::vector<Entry> entries;
    std::size_t total = 0;
    for (auto* entry = &name; entry && entry->oid.p; entry = entry->next) {
        if (entry->oid.len > (std::numeric_limits<std::size_t>::max() - 32) / 4) throw std::length_error("SSL name OID size overflow");
        std::string oid(entry->oid.len * 4 + 32, '\0');
        const auto length = mbedtls_oid_get_numeric_string(oid.data(), oid.size(), &entry->oid);
        if (length < 0) throw std::runtime_error("Unsupported SSL name OID");
        oid.resize(static_cast<std::size_t>(length));
        total = Add(total, Add(sizeof(NameEntry), Add(oid.size() + 1, entry->val.len)));
        entries.push_back({std::move(oid), entry});
    }
    auto handle = std::make_unique<NameHandle>();
    handle->name = std::make_unique<NameData>();
    handle->name->data = Allocate(total);
    handle->name->count = entries.size();
    auto* output = reinterpret_cast<NameEntry*>(handle->name->data.get());
    auto* next = handle->name->data.get() + entries.size() * sizeof(NameEntry);
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto& entry = entries[index];
        output[index].oid = reinterpret_cast<char*>(next);
        output[index].oidSize = entry.oid.size();
        std::memcpy(next, entry.oid.c_str(), entry.oid.size() + 1);
        next += entry.oid.size() + 1;
        output[index].value = next;
        output[index].valueSize = entry.name->val.len;
        std::memcpy(next, entry.name->val.p, entry.name->val.len);
        next += entry.name->val.len;
    }
    auto* result = handle.get();
    context.names.push_back(std::move(handle));
    return result;
}

}

extern "C" {

int APS5_VABI sceSslInit_nid_postfix(std::uint64_t poolSize) {
    auto& state = State();
    std::lock_guard lock(state.mutex);
    const auto handle = AllocateHandle(state);
    state.contexts.emplace(handle, std::make_unique<Context>(poolSize));
    return handle;
}

int APS5_VABI sceSslTerm_nid_postfix(int sslCtxId) {
    auto& state = State();
    std::lock_guard lock(state.mutex);
    state.contexts.erase(sslCtxId);
    return 0;
}

int APS5_VABI sceSslCreateConnection(int sslCtxId, int sockId, const char* host) {
    if (!host) throw std::invalid_argument("sceSslCreateConnection: null host");
    auto& state = State();
    std::lock_guard lock(state.mutex);
    if (!state.contexts.contains(sslCtxId)) return ErrorInvalidId;
    const auto id = AllocateHandle(state);
    state.connections[id] = {sslCtxId, sockId, host, nullptr, nullptr};
    return id;
}

int APS5_VABI sceSslDeleteConnection(int sslConnectionId) {
    auto& state = State();
    std::lock_guard lock(state.mutex);
    if (!state.connections.erase(sslConnectionId)) throw std::invalid_argument("sceSslDeleteConnection: unknown connection");
    return 0;
}

int APS5_VABI sceSslSetVerifyCallback(int sslConnectionId, VerifyCallback cbfunc, void* userArg) {
    std::lock_guard lock(State().mutex);
    auto& connection = RequireConnection(__func__, sslConnectionId);
    connection.verifyCallback = cbfunc;
    connection.verifyArgument = userArg;
    return 0;
}

int APS5_VABI sceSslConnect(int sslConnectionId) {
    ThrowNoTransport(__func__, sslConnectionId, "the handshake");
}

int APS5_VABI sceSslRead(int sslConnectionId, void* buf, std::size_t len, int flags) {
    (void)buf;
    (void)len;
    (void)flags;
    ThrowNoTransport(__func__, sslConnectionId, "reading");
}

int APS5_VABI sceSslWrite(int sslConnectionId, void* buf, std::size_t len, int flags) {
    (void)buf;
    (void)len;
    (void)flags;
    ThrowNoTransport(__func__, sslConnectionId, "writing");
}

int APS5_VABI sceSslGetMemoryPoolStats(int sslCtxId, SceSsl::MemoryPoolStats* stats) {
    if (!stats) return ErrorInvalidValue;
    auto& state = State();
    std::lock_guard lock(state.mutex);
    const auto found = state.contexts.find(sslCtxId);
    if (found == state.contexts.end()) {
        std::memset(stats, 0, sizeof(*stats));
        return ErrorInvalidId;
    }
    const auto& pool = found->second->pool;
    *stats = {pool.size, pool.peak, pool.current, 0};
    return 0;
}

int APS5_VABI sceSslLoadCert(int sslCtxId, int caCertNum, SceSsl::Data** caList, SceSsl::Data* cert, SceSsl::Data* privateKey) {
    if (caCertNum < 0 || (caCertNum && !caList) || (!!cert != !!privateKey))
        throw std::invalid_argument("Invalid SSL certificate load arguments");
    auto& state = State();
    std::lock_guard lock(state.mutex);
    auto& context = GetContext(sslCtxId);
    PoolScope scope(context.pool);
    std::vector<std::shared_ptr<CertificateChain>> authorities;
    std::vector<std::unique_ptr<CertificateHandle>> handles;
    for (int index = 0; index < caCertNum; ++index) {
        if (!caList[index]) throw std::invalid_argument("Missing SSL certificate descriptor");
        auto chain = ParseCertificate(*caList[index]);
        for (auto* entry = &chain->root; entry; entry = entry->next) {
            auto handle = std::make_unique<CertificateHandle>(CertificateHandle{chain, entry});
            chain->handles.push_back(handle.get());
            handles.push_back(std::move(handle));
        }
        authorities.push_back(std::move(chain));
    }
    std::shared_ptr<Identity> identity;
    if (cert) {
        identity = std::make_shared<Identity>();
        identity->chain = ParseCertificate(*cert);
        const auto key = CopyInput(*privateKey);
        Random random;
        random.Seed();
        CheckBackend(mbedtls_pk_parse_key(&identity->key.key, key.bytes.get(), key.size, nullptr, 0,
            mbedtls_ctr_drbg_random, &random.drbg), "Invalid SSL private key");
        CheckBackend(mbedtls_pk_check_pair(&identity->chain->root.pk, &identity->key.key, mbedtls_ctr_drbg_random, &random.drbg),
            "SSL client certificate and private key do not match");
    }
    context.authorities.reserve(Add(context.authorities.size(), authorities.size()));
    context.certificateHandles.reserve(Add(context.certificateHandles.size(), handles.size()));
    for (auto& chain : authorities) context.authorities.push_back(std::move(chain));
    for (auto& handle : handles) context.certificateHandles.push_back(std::move(handle));
    if (identity) context.identity = std::move(identity);
    return 0;
}

int APS5_VABI sceSslUnloadCert(int sslCtxId) {
    auto& state = State();
    std::lock_guard lock(state.mutex);
    auto& context = GetContext(sslCtxId);
    context.authorities.clear();
    context.identity.reset();
    return 0;
}

int APS5_VABI sceSslGetCaList(int sslCtxId, SceSsl::CaList* output) {
    if (!output) return ErrorInvalidArg;
    auto& state = State();
    std::lock_guard lock(state.mutex);
    auto& context = GetContext(sslCtxId);
    PoolScope scope(context.pool);
    const auto count = CertificateCount(context);
    if (!count) {
        *output = {};
        return ErrorNotFound;
    }
    if (count > static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("Too many SSL authorities");
    Snapshot snapshot{Allocate(count * sizeof(void*)), context.authorities, count};
    auto** pointers = reinterpret_cast<void**>(snapshot.data.get());
    std::size_t index = 0;
    for (const auto& chain : snapshot.certificates) for (auto* handle : chain->handles) pointers[index++] = handle;
    context.caLists.emplace(pointers, std::move(snapshot));
    *output = {pointers, static_cast<int>(count)};
    return 0;
}

int APS5_VABI sceSslFreeCaList(int sslCtxId, SceSsl::CaList* list) {
    if (!list) return ErrorInvalidArg;
    auto& state = State();
    std::lock_guard lock(state.mutex);
    auto& context = GetContext(sslCtxId);
    if (!list->certs && list->num == 0) return 0;
    const auto found = context.caLists.find(list->certs);
    if (found == context.caLists.end() || list->num < 0 || static_cast<std::size_t>(list->num) != found->second.count)
        throw std::invalid_argument("Invalid SSL CA list allocation");
    context.caLists.erase(found);
    *list = {};
    return 0;
}

int APS5_VABI sceSslGetCaCerts(int sslCtxId, SceSsl::CaCerts* output) {
    if (!output) return ErrorInvalidArg;
    auto& state = State();
    std::lock_guard lock(state.mutex);
    const auto found = state.contexts.find(sslCtxId);
    if (found == state.contexts.end()) {
        *output = {};
        return ErrorNotFound;
    }
    auto& context = *found->second;
    PoolScope scope(context.pool);
    const auto count = CertificateCount(context);
    if (!count) {
        *output = {};
        return ErrorNotFound;
    }
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(SceSsl::Data)) throw std::length_error("Too many SSL authorities");
    auto total = count * sizeof(SceSsl::Data);
    for (const auto& chain : context.authorities)
        for (auto* entry = &chain->root; entry; entry = entry->next) total = Add(total, entry->raw.len);
    Snapshot snapshot{Allocate(total), {}, count};
    auto* data = reinterpret_cast<SceSsl::Data*>(snapshot.data.get());
    auto* next = snapshot.data.get() + count * sizeof(SceSsl::Data);
    std::size_t index = 0;
    for (const auto& chain : context.authorities) {
        for (auto* entry = &chain->root; entry; entry = entry->next) {
            data[index++] = {next, entry->raw.len};
            std::memcpy(next, entry->raw.p, entry->raw.len);
            next += entry->raw.len;
        }
    }
    context.caCerts.emplace(data, std::move(snapshot));
    *output = {data, count, data};
    return 0;
}

int APS5_VABI sceSslFreeCaCerts(int sslCtxId, SceSsl::CaCerts* certificates) {
    if (!certificates) return ErrorInvalidArg;
    auto& state = State();
    std::lock_guard lock(state.mutex);
    const auto found = state.contexts.find(sslCtxId);
    if (found != state.contexts.end()) {
        auto& snapshots = found->second->caCerts;
        const auto allocation = snapshots.find(certificates->pool);
        if (allocation != snapshots.end()) {
            if (certificates->certs != certificates->pool || certificates->num != allocation->second.count)
                throw std::invalid_argument("Invalid SSL CA certificate allocation");
            snapshots.erase(allocation);
        }
    }
    *certificates = {};
    return 0;
}

void* APS5_VABI sceSslGetSubjectName(int sslCtxId, void* certificate) {
    auto& state = State();
    std::lock_guard lock(state.mutex);
    auto& context = GetContext(sslCtxId);
    const auto [owner, parsed] = GetCertificate(context, certificate);
    PoolScope scope(context.pool);
    return CopyName(context, parsed->subject);
}

void* APS5_VABI sceSslGetIssuerName(int sslCtxId, void* certificate) {
    auto& state = State();
    std::lock_guard lock(state.mutex);
    auto& context = GetContext(sslCtxId);
    const auto [owner, parsed] = GetCertificate(context, certificate);
    PoolScope scope(context.pool);
    return CopyName(context, parsed->issuer);
}

int APS5_VABI sceSslFreeSslCertName(int sslCtxId, void* name) {
    auto& state = State();
    std::lock_guard lock(state.mutex);
    GetName(GetContext(sslCtxId), name).name.reset();
    return 0;
}

int APS5_VABI sceSslGetNameEntryCount(int sslCtxId, void* name) {
    auto& state = State();
    std::lock_guard lock(state.mutex);
    const auto count = GetName(GetContext(sslCtxId), name).name->count;
    if (count > static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("Too many SSL name entries");
    return static_cast<int>(count);
}

int APS5_VABI sceSslGetNameEntryInfo(int sslCtxId, void* name, int index, char* oid, std::size_t oidCapacity,
    std::uint8_t* value, std::size_t valueCapacity, std::size_t* valueLength) {
    if (!valueLength || (!oid && oidCapacity) || (!value && valueCapacity)) return ErrorInvalidArg;
    auto& state = State();
    std::lock_guard lock(state.mutex);
    const auto& parsed = *GetName(GetContext(sslCtxId), name).name;
    if (index < 0 || static_cast<std::size_t>(index) >= parsed.count) return ErrorInvalidArg;
    const auto& entry = reinterpret_cast<const NameEntry*>(parsed.data.get())[index];
    *valueLength = entry.valueSize;
    if ((oid && oidCapacity <= entry.oidSize) || (value && valueCapacity < entry.valueSize))
        throw std::length_error("SSL name output buffer is too small");
    if (oid) std::memcpy(oid, entry.oid, entry.oidSize + 1);
    if (value) {
        std::memcpy(value, entry.value, entry.valueSize);
        if (valueCapacity > entry.valueSize) value[entry.valueSize] = 0;
    }
    return 0;
}

int APS5_VABI sceSslGetSerialNumber(int sslCtxId, void* certificate, std::uint8_t* bytes, std::size_t* length) {
    if (!length) return ErrorInvalidArg;
    auto& state = State();
    std::lock_guard lock(state.mutex);
    const auto [owner, parsed] = GetCertificate(GetContext(sslCtxId), certificate);
    const auto capacity = *length;
    *length = parsed->serial.len;
    if (bytes && capacity < parsed->serial.len) throw std::length_error("SSL serial output buffer is too small");
    if (bytes) std::memcpy(bytes, parsed->serial.p, parsed->serial.len);
    return 0;
}

int APS5_VABI sceSslClose(int sslConnectionId) {
    std::lock_guard lock(State().mutex);
    RequireConnection(__func__, sslConnectionId).closed = true;
    return 0;
}

int APS5_VABI sceSslEnableVerifyOption(int sslConnectionId, std::uint32_t option) {
    std::lock_guard lock(State().mutex);
    auto& connection = RequireConnection(__func__, sslConnectionId);
    connection.enabledVerifyOptions |= option;
    connection.disabledVerifyOptions &= ~option;
    return 0;
}

int APS5_VABI sceSslDisableVerifyOption(int sslConnectionId, std::uint32_t option) {
    std::lock_guard lock(State().mutex);
    auto& connection = RequireConnection(__func__, sslConnectionId);
    connection.disabledVerifyOptions |= option;
    connection.enabledVerifyOptions &= ~option;
    return 0;
}

const std::uint8_t* APS5_VABI sceSslGetAlpnSelected(int sslConnectionId) {
    std::lock_guard lock(State().mutex);
    RequireConnection(__func__, sslConnectionId);
    return nullptr;
}

int APS5_VABI sceSslSetAlpn(int sslConnectionId, const char* protocols) {
    std::lock_guard lock(State().mutex);
    auto& connection = RequireConnection(__func__, sslConnectionId);
    if (!protocols || !ValidAlpn(protocols)) return ErrorInvalidValue;
    connection.alpn = protocols;
    return 0;
}

int APS5_VABI sceSslSetMinSslVersion(int sslConnectionId, int version) {
    std::lock_guard lock(State().mutex);
    auto& connection = RequireConnection(__func__, sslConnectionId);
    if (version < 0 || version > SslVersionTls12) return ErrorInvalidValue;
    connection.minimumVersion = version;
    return 0;
}

int APS5_VABI sceSslGetNotBefore(int sslCtxId, void* certificate, SceSsl::RtcTick* begin) {
    if (!begin) return ErrorInvalidArg;
    auto& state = State();
    std::lock_guard lock(state.mutex);
    const auto [owner, parsed] = GetCertificate(GetContext(sslCtxId), certificate);
    begin->tick = TimeToTick(parsed->valid_from);
    return 0;
}

int APS5_VABI sceSslGetNotAfter(int sslCtxId, void* certificate, SceSsl::RtcTick* limit) {
    if (!limit) return ErrorInvalidArg;
    auto& state = State();
    std::lock_guard lock(state.mutex);
    const auto [owner, parsed] = GetCertificate(GetContext(sslCtxId), certificate);
    limit->tick = TimeToTick(parsed->valid_to);
    return 0;
}

int APS5_VABI sceSslGetFingerprint(int sslCtxId, void* certificate, int hashType, std::uint8_t* fingerprint) {
    if (!fingerprint) return ErrorInvalidArg;
    mbedtls_md_type_t type;
    switch (hashType) {
    case 0: type = MBEDTLS_MD_MD5; break;
    case 1: type = MBEDTLS_MD_SHA1; break;
    case 2: type = MBEDTLS_MD_SHA224; break;
    case 3: type = MBEDTLS_MD_SHA256; break;
    case 4: type = MBEDTLS_MD_SHA384; break;
    case 5: type = MBEDTLS_MD_SHA512; break;
    default: return ErrorInvalidArg;
    }
    const auto* info = mbedtls_md_info_from_type(type);
    if (!info) throw std::runtime_error("sceSslGetFingerprint: digest is not built into the SSL backend");
    auto& state = State();
    std::lock_guard lock(state.mutex);
    auto& context = GetContext(sslCtxId);
    const auto [owner, parsed] = GetCertificate(context, certificate);
    PoolScope scope(context.pool);
    CheckBackend(mbedtls_md(info, parsed->raw.p, parsed->raw.len, fingerprint), "SSL certificate fingerprint failed");
    return 0;
}

int APS5_VABI sceSslGetPem(int sslCtxId, void* certificate, char* buffer, std::size_t* length) {
    if (!length) return ErrorInvalidArg;
    auto& state = State();
    std::lock_guard lock(state.mutex);
    auto& context = GetContext(sslCtxId);
    const auto retained = GetCertificate(context, certificate);
    const auto* parsed = retained.second;
    PoolScope scope(context.pool);
    const auto encode = [&](unsigned char* output, std::size_t capacity, std::size_t* written) {
        return mbedtls_pem_write_buffer("-----BEGIN CERTIFICATE-----\n", "-----END CERTIFICATE-----\n", parsed->raw.p, parsed->raw.len,
            output, capacity, written);
    };
    std::size_t required = 0;
    const auto sizing = encode(nullptr, 0, &required);
    if (sizing != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) CheckBackend(sizing, "SSL PEM sizing failed");
    const auto capacity = *length;
    *length = required;
    if (!buffer) return 0;
    if (capacity < required) throw std::length_error("SSL PEM output buffer is too small");
    std::size_t written = 0;
    CheckBackend(encode(reinterpret_cast<unsigned char*>(buffer), required, &written), "SSL PEM encoding failed");
    *length = written;
    return 0;
}

}
