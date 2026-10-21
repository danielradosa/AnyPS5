#define MBEDTLS_ALLOW_PRIVATE_ACCESS

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <cerrno>
#endif

#include "prx/libSceLibreSsl/include/Asn1.hpp"
#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/src/SslCipherTable.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/HostEntropy.hpp"
#include "prx/libkernel/Socket/include/SocketPoll.hpp"

#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/pem.h>
#include <mbedtls/pk.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cache.h>
#include <mbedtls/ssl_ciphersuites.h>
#include <mbedtls/x509_crt.h>

#include <algorithm>
#include <atomic>
#include <climits>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct FileStream;

extern "C" {
FileStream* APS5_VABI fopen_nid_postfix(const char* filename, const char* mode);
int APS5_VABI fclose_nid_postfix(FileStream* stream);
std::size_t APS5_VABI fread_nid_postfix(void* buffer, std::size_t size, std::size_t count, FileStream* stream);
int APS5_VABI close_nid_postfix(int descriptor);
LibreSsl::Bio* APS5_VABI BIO_new_nid_postfix(const LibreSsl::BioMethod* type);
int APS5_VABI BIO_free_nid_postfix(LibreSsl::Bio* a);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(LibreSsl::Bio* bp, int cmd, std::int64_t larg, void* parg);
void APS5_VABI X509_free_nid_postfix(LibreSsl::X509Object* a);
LibreSsl::Store* APS5_VABI X509_STORE_new_nid_postfix();
void APS5_VABI X509_STORE_free_nid_postfix(LibreSsl::Store* v);
void APS5_VABI EVP_PKEY_free_nid_postfix(LibreSsl::EvpPkey* pkey);
}

namespace LibreSsl {

struct SslCtxState;
struct SslState;
struct SessionState;
struct SslMethodObject;
struct SslCipherObject;
struct SslSessionObject;

struct SslCtxObject {
    const SslMethodObject* method;
    void* cipherList;
    void* certStore;
    std::int64_t sessionTimeout;
    int references;
    void* extraCerts;
    int verifyMode;
    unsigned int sidCtxLength;
    unsigned char sidCtx[32];
    void* param;
    void* defaultPasswdCallback;
    void* defaultPasswdCallbackUserdata;
    SslCtxState* internal;
};

struct SslObject {
    int version;
    const SslMethodObject* method;
    Bio* rbio;
    Bio* wbio;
    Bio* bbio;
    int server;
    void* s3;
    void* d1;
    void* param;
    void* cipherList;
    void* cert;
    unsigned int sidCtxLength;
    unsigned char sidCtx[32];
    SslSessionObject* session;
    int verifyMode;
    int error;
    int errorCode;
    SslCtxObject* ctx;
    std::int64_t verifyResult;
    int references;
    int clientVersion;
    unsigned int maxSendFragment;
    char* tlsextHostname;
    int tlsextStatusType;
    SslCtxObject* initialCtx;
    void* encReadCtx;
    void* readHash;
    SslState* internal;
};

struct SslSessionObject {
    int sslVersion;
    int masterKeyLength;
    unsigned char masterKey[48];
    unsigned int sessionIdLength;
    unsigned char sessionId[32];
    unsigned int sidCtxLength;
    unsigned char sidCtx[32];
    X509Object* peer;
    std::int64_t verifyResult;
    std::int64_t timeout;
    std::int64_t time;
    int references;
    const SslCipherObject* cipher;
    std::uint64_t cipherId;
    void* ciphers;
    char* tlsextHostname;
    unsigned char* tlsextTick;
    std::size_t tlsextTicklen;
    std::int64_t tlsextTickLifetimeHint;
    SessionState* internal;
};

struct SslMethodObject {
    void* functions[5];
    const void* internal;
};

struct SslCipherObject {
    int valid;
    const char* name;
    std::uint64_t id;
    std::uint64_t algorithmMkey;
    std::uint64_t algorithmAuth;
    std::uint64_t algorithmEnc;
    std::uint64_t algorithmMac;
    std::uint64_t algorithmSsl;
    std::uint64_t algoStrength;
    std::uint64_t algorithm2;
    int strengthBits;
    int algBits;
};

static_assert(sizeof(SslCtxObject) == 120 && offsetof(SslCtxObject, verifyMode) == 48 && offsetof(SslCtxObject, sidCtx) == 56);
static_assert(offsetof(SslCtxObject, param) == 88 && offsetof(SslCtxObject, internal) == 112);
static_assert(sizeof(SslObject) == 232 && offsetof(SslObject, server) == 40 && offsetof(SslObject, sidCtx) == 92);
static_assert(offsetof(SslObject, session) == 128 && offsetof(SslObject, ctx) == 152 && offsetof(SslObject, verifyResult) == 160);
static_assert(offsetof(SslObject, tlsextHostname) == 184 && offsetof(SslObject, initialCtx) == 200 && offsetof(SslObject, internal) == 224);
static_assert(sizeof(SslSessionObject) == 232 && offsetof(SslSessionObject, sessionId) == 60 && offsetof(SslSessionObject, peer) == 128);
static_assert(offsetof(SslSessionObject, references) == 160 && offsetof(SslSessionObject, cipher) == 168 && offsetof(SslSessionObject, internal) == 224);
static_assert(sizeof(SslMethodObject) == 48 && sizeof(SslCipherObject) == 88 && offsetof(SslCipherObject, strengthBits) == 80);

using VerifyCallback = int (APS5_VABI*)(int, void*);
using ServernameCallback = int (APS5_VABI*)(SslObject*, int*, void*);
using NewSessionCallback = int (APS5_VABI*)(SslObject*, SslSessionObject*);
using ClientCertCallback = int (APS5_VABI*)(SslObject*, X509Object**, EvpPkey**);

namespace {

constexpr int ErrLibSsl = 20;
constexpr int ErrLibX509 = 11;
constexpr int ErrReasonSysLib = 2;
constexpr int ErrReasonPemLib = 9;
constexpr int ErrReasonAsn1Lib = 13;

constexpr int SslFunctionCtxNew = 169;
constexpr int SslFunctionCtxUseCertificate = 171;
constexpr int SslFunctionCtxUseCertificateAsn1 = 172;
constexpr int SslFunctionCtxUseCertificateChainFile = 220;
constexpr int SslFunctionCtxUseCertificateFile = 173;
constexpr int SslFunctionCtxUsePrivateKey = 174;
constexpr int SslFunctionCtxUsePrivateKeyAsn1 = 175;
constexpr int SslFunctionCtxUsePrivateKeyFile = 176;
constexpr int SslFunctionCtxCheckPrivateKey = 168;
constexpr int SslFunctionCheckPrivateKey = 163;
constexpr int SslFunctionCtxSetCipherList = 269;
constexpr int SslFunctionCtxSetSessionIdContext = 219;
constexpr int SslFunctionSsl3Connect = 132;
constexpr int SslFunctionSsl3Accept = 128;
constexpr int SslFunctionSsl3ReadBytes = 148;
constexpr int SslFunctionSsl3WriteBytes = 158;
constexpr int SslFunctionSsl3Ctrl = 213;
constexpr int SslFunctionUseCertificateAsn1 = 199;
constexpr int SslFunctionUsePrivateKeyAsn1 = 202;
constexpr int X509FunctionCheckPrivateKey = 128;

constexpr int SslReasonCertificateVerifyFailed = 134;
constexpr int SslReasonNoCertificateAssigned = 177;
constexpr int SslReasonNoCipherMatch = 185;
constexpr int SslReasonNoPrivateKeyAssigned = 190;
constexpr int SslReasonNoProtocolsAvailable = 191;
constexpr int SslReasonNoSharedCipher = 193;
constexpr int SslReasonNullSslMethodPassed = 196;
constexpr int SslReasonPeerDidNotReturnCertificate = 199;
constexpr int SslReasonProtocolIsShutdown = 207;
constexpr int SslReasonSessionIdContextTooLong = 273;
constexpr int SslReasonSsl3ExtInvalidServername = 319;
constexpr int SslReasonDecryptionFailedOrBadRecordMac = 281;
constexpr int SslReasonWrongVersionNumber = 267;
constexpr int SslReasonUnexpectedMessage = 244;
constexpr int SslAlertReasonOffset = 1000;
constexpr int X509ReasonKeyValuesMismatch = 116;

constexpr int VerifyNone = 0x00;
constexpr int VerifyPeer = 0x01;
constexpr int VerifyFailIfNoPeerCert = 0x02;
constexpr int VerifyClientOnce = 0x04;

constexpr int ErrorNone = 0;
constexpr int ErrorSsl = 1;
constexpr int ErrorWantRead = 2;
constexpr int ErrorWantWrite = 3;
constexpr int ErrorWantX509Lookup = 4;
constexpr int ErrorSyscall = 5;
constexpr int ErrorZeroReturn = 6;

constexpr int StateNothing = 1;
constexpr int StateWriting = 2;
constexpr int StateReading = 3;
constexpr int StateX509Lookup = 4;

constexpr int HandshakeWantX509Lookup = 1;
constexpr int StoreCtxSslIndex = 0;

constexpr int SentShutdown = 1;
constexpr int ReceivedShutdown = 2;

constexpr int FiletypePem = 1;
constexpr int FiletypeAsn1 = 2;

constexpr int PkeyRsa = 6;
constexpr int PkeyEc = 408;

constexpr std::uint64_t OptionNoTicket = 0x00004000;
constexpr std::uint64_t OptionNoTlsv1 = 0x04000000;
constexpr std::uint64_t OptionNoTlsv1_2 = 0x08000000;
constexpr std::uint64_t OptionNoTlsv1_1 = 0x10000000;
constexpr std::uint64_t ModeEnablePartialWrite = 0x00000001;

constexpr int SessCacheClient = 0x0001;
constexpr int SessCacheServer = 0x0002;

constexpr int Tls1Version = 0x0301;
constexpr int Tls1_1Version = 0x0302;
constexpr int Tls1_2Version = 0x0303;
constexpr int Tls1_3Version = 0x0304;

constexpr int CtrlGetSessionReused = 8;
constexpr int CtrlExtraChainCert = 14;
constexpr int CtrlSetMsgCallbackArg = 16;
constexpr int CtrlOptions = 32;
constexpr int CtrlMode = 33;
constexpr int CtrlGetReadAhead = 40;
constexpr int CtrlSetReadAhead = 41;
constexpr int CtrlSetSessCacheSize = 42;
constexpr int CtrlGetSessCacheSize = 43;
constexpr int CtrlSetSessCacheMode = 44;
constexpr int CtrlGetSessCacheMode = 45;
constexpr int CtrlGetMaxCertList = 50;
constexpr int CtrlSetMaxCertList = 51;
constexpr int CtrlSetTlsextServernameCb = 53;
constexpr int CtrlSetTlsextServernameArg = 54;
constexpr int CtrlSetTlsextHostname = 55;
constexpr int CtrlClearOptions = 77;
constexpr int CtrlClearMode = 78;
constexpr int CtrlGetExtraChainCerts = 82;
constexpr int CtrlClearExtraChainCerts = 83;
constexpr int CtrlSetGroupsList = 92;
constexpr int CtrlSetEcdhAuto = 94;
constexpr int CtrlSetDhAuto = 118;
constexpr int CtrlSetMinProtoVersion = 123;
constexpr int CtrlSetMaxProtoVersion = 124;

constexpr int TlsextNametypeHostName = 0;
constexpr int TlsextErrOk = 0;
constexpr int TlsextErrAlertWarning = 1;
constexpr int TlsextErrAlertFatal = 2;
constexpr int TlsextErrNoack = 3;

constexpr int NpnNegotiated = 1;
constexpr int NpnNoOverlap = 2;

constexpr std::int64_t X509VOk = 0;
constexpr int X509VErrUnableToGetIssuerCert = 2;
constexpr int X509VErrCertSignatureFailure = 7;
constexpr int X509VErrCertNotYetValid = 9;
constexpr int X509VErrCertHasExpired = 10;
constexpr int X509VErrDepthZeroSelfSigned = 18;
constexpr int X509VErrSelfSignedInChain = 19;
constexpr int X509VErrUnableToGetIssuerLocally = 20;
constexpr int X509VErrUnableToVerifyLeafSignature = 21;
constexpr int X509VErrCertRevoked = 23;
constexpr int X509VErrInvalidCa = 24;
constexpr int X509VErrPathLengthExceeded = 25;
constexpr int X509VErrCertUntrusted = 27;
constexpr int X509VErrHostnameMismatch = 62;
constexpr int X509VErrIpAddressMismatch = 64;

constexpr int TypeSocket = 5 | 0x0400 | 0x0100;
constexpr int TypeSsl = 7 | 0x0200;
constexpr int BioCtrlPending = 10;
constexpr int BioCtrlFlush = 11;
constexpr int BioCtrlDup = 12;
constexpr int BioCtrlSetClose = 9;
constexpr int BioCtrlGetClose = 8;
constexpr int BioDoStateMachine = 101;
constexpr int BioSetFd = 104;
constexpr int BioGetFd = 105;
constexpr int BioSetSsl = 109;
constexpr int BioGetSsl = 110;
constexpr int BioSslMode = 119;

constexpr std::int64_t DefaultSessionTimeout = 300;
constexpr std::int64_t DefaultSessCacheSize = 1024 * 20;
constexpr std::int64_t DefaultMaxCertList = 1024 * 100;

using Der = std::vector<unsigned char>;

struct Identity {
    Der certificate;
    std::vector<Der> chain;
    Der key;
    int keyType = 0;
};

struct Cipher {
    SslCipherObject object;
    std::string name;
    int bits;
    bool kRsa, kDh, kEcdh, aRsa, aEcdsa, aes, aesGcm, camellia, chacha, sha1, sha256, sha384, aead, tls12;
};

struct CipherTable {
    std::vector<std::unique_ptr<Cipher>> ciphers;

    CipherTable() {
        for (const int* id = mbedtls_ssl_list_ciphersuites(); *id; ++id) {
            const auto found = std::find_if(std::begin(CipherNames), std::end(CipherNames), [&](const CipherName& entry) { return entry.id == *id; });
            if (found == std::end(CipherNames)) continue;
            const std::string name = found->name;
            if (name.find("PSK") != std::string::npos || name.find("SRP") != std::string::npos || name.rfind("ECDH-", 0) == 0 ||
                name.rfind("DH-", 0) == 0 || name.rfind("ADH", 0) == 0 || name.rfind("AECDH", 0) == 0 || name.find("NULL") != std::string::npos)
                continue;
            const auto* info = mbedtls_ssl_ciphersuite_from_id(*id);
            if (!info) continue;
            auto cipher = std::make_unique<Cipher>();
            cipher->name = name;
            cipher->bits = static_cast<int>(mbedtls_ssl_ciphersuite_get_cipher_key_bitlen(info));
            const bool ecdhe = name.rfind("ECDHE-", 0) == 0;
            const bool dhe = name.rfind("DHE-", 0) == 0 || name.rfind("EDH-", 0) == 0;
            cipher->kEcdh = ecdhe;
            cipher->kDh = dhe;
            cipher->kRsa = !ecdhe && !dhe;
            cipher->aEcdsa = name.find("ECDSA") != std::string::npos;
            cipher->aRsa = !cipher->aEcdsa;
            cipher->aesGcm = name.find("AES") != std::string::npos && name.find("GCM") != std::string::npos;
            cipher->aes = name.find("AES") != std::string::npos;
            cipher->camellia = name.find("CAMELLIA") != std::string::npos;
            cipher->chacha = name.find("CHACHA20") != std::string::npos;
            cipher->aead = name.find("GCM") != std::string::npos || cipher->chacha;
            const auto ends = [&](const char* suffix) {
                const std::string tail(suffix);
                return name.size() >= tail.size() && name.compare(name.size() - tail.size(), tail.size(), tail) == 0;
            };
            cipher->sha1 = !cipher->aead && ends("-SHA");
            cipher->sha256 = !cipher->aead && ends("-SHA256");
            cipher->sha384 = !cipher->aead && ends("-SHA384");
            cipher->tls12 = cipher->aead || cipher->sha256 || cipher->sha384;
            cipher->object = {1, nullptr, 0x03000000u | static_cast<std::uint64_t>(*id), 0, 0, 0, 0, 0, 0, 0, cipher->bits, cipher->bits};
            ciphers.push_back(std::move(cipher));
        }
        for (auto& cipher : ciphers) cipher->object.name = cipher->name.c_str();
    }

    const Cipher* ById(int id) const {
        for (const auto& cipher : ciphers) {
            if ((cipher->object.id & 0xffff) == static_cast<std::uint64_t>(id)) return cipher.get();
        }
        return nullptr;
    }
};

const CipherTable& Ciphers() {
    static const CipherTable table;
    return table;
}

struct MethodInfo {
    bool client;
    bool server;
};

constexpr MethodInfo ClientInfo{true, false};
constexpr MethodInfo ServerInfo{false, true};
constexpr SslMethodObject Sslv23ClientMethod{{}, &ClientInfo};
constexpr SslMethodObject Sslv23ServerMethod{{}, &ServerInfo};
constexpr SslMethodObject TlsClientMethod{{}, &ClientInfo};

const MethodInfo& Info(const SslMethodObject* method) {
    return *static_cast<const MethodInfo*>(method->internal);
}

struct ExIndices {
    std::mutex mutex;
    int nextSsl = 1;
    int nextCtx = 1;
};

ExIndices& Indices() {
    static ExIndices indices;
    return indices;
}

struct PeerChain {
    X509Object* leaf = nullptr;
    std::vector<X509Object*> certificates;
    std::vector<char*> pointers;
    Stack stack{};

    ~PeerChain() {
        if (leaf) X509_free_nid_postfix(leaf);
        for (auto* certificate : certificates) X509_free_nid_postfix(certificate);
    }
};

}

struct SessionState {
    std::atomic<int> references{1};
    mbedtls_ssl_session session;
    std::string hostname;
    std::shared_ptr<PeerChain> peer;
    bool resumable = false;

    SessionState() {
        mbedtls_ssl_session_init(&session);
    }

    ~SessionState() {
        mbedtls_ssl_session_free(&session);
    }
};

struct SslCtxState {
    std::mutex mutex;
    std::atomic<int> references{1};
    std::uint64_t options = 0;
    std::uint64_t mode = 0;
    int quietShutdown = 0;
    std::int64_t readAhead = 0;
    std::int64_t sessCacheMode = SessCacheServer;
    std::int64_t sessCacheSize = DefaultSessCacheSize;
    std::int64_t maxCertList = DefaultMaxCertList;
    VerifyCallback verifyCallback = nullptr;
    std::vector<int> ciphers;
    std::vector<std::uint16_t> groups;
    Der alpn;
    int minVersion = 0;
    int maxVersion = 0;
    std::shared_ptr<const Identity> identity = std::make_shared<Identity>();
    std::vector<Der> clientAuthorities;
    ClientCertCallback clientCertCallback = nullptr;
    CertVerifyCallback certVerifyCallback = nullptr;
    void* certVerifyArgument = nullptr;
    std::vector<X509Object*> extraCertificates;
    std::map<int, void*> exData;
    ServernameCallback servernameCallback = nullptr;
    void* servernameArgument = nullptr;
    NewSessionCallback newSessionCallback = nullptr;
    mbedtls_ssl_cache_context cache;

    SslCtxState() {
        mbedtls_ssl_cache_init(&cache);
    }

    ~SslCtxState() {
        for (auto* certificate : extraCertificates) X509_free_nid_postfix(certificate);
        mbedtls_ssl_cache_free(&cache);
    }
};

struct SslState {
    std::atomic<int> references{1};
    int role = 0;
    std::uint64_t options = 0;
    std::uint64_t mode = 0;
    int quietShutdown = 0;
    VerifyCallback verifyCallback = nullptr;
    std::vector<int> ciphers;
    std::vector<std::uint16_t> groups;
    Der alpn;
    int minVersion = 0;
    int maxVersion = 0;
    std::shared_ptr<const Identity> identity;
    std::string hostname;
    std::map<int, void*> exData;
    bool configured = false;
    bool handshaken = false;
    bool failed = false;
    int shutdown = 0;
    int want = StateNothing;
    bool syscallFailure = false;
    std::size_t writeProgress = 0;
    std::vector<unsigned char> peek;
    std::string servername;
    unsigned char clientRandom[32]{};
    unsigned char serverRandom[32]{};
    unsigned char masterSecret[48]{};
    bool haveRandoms = false;
    bool reused = false;
    std::int64_t verifyResult = X509VOk;
    CertVerifyCallback certVerifyCallback = nullptr;
    void* certVerifyArgument = nullptr;
    bool certificateRequested = false;
    std::shared_ptr<const Identity> privateKeyIdentity;
    EvpPkey* privateKey = nullptr;
    std::exception_ptr pending;
    std::vector<Der> clientAuthorities;
    std::vector<int> suiteList;
    std::vector<std::uint16_t> groupList;
    std::vector<std::string> alpnStrings;
    std::vector<const char*> alpnList;
    X509Object* ownCertificate = nullptr;
    mbedtls_x509_crt ownChain;
    mbedtls_pk_context ownKey;
    mbedtls_x509_crt switchedChain;
    mbedtls_pk_context switchedKey;
    mbedtls_x509_crt caChain;
    mbedtls_x509_crt dnHints;
    mbedtls_ssl_config config;
    mbedtls_ssl_context ssl;

    SslState() {
        mbedtls_x509_crt_init(&ownChain);
        mbedtls_pk_init(&ownKey);
        mbedtls_x509_crt_init(&switchedChain);
        mbedtls_pk_init(&switchedKey);
        mbedtls_x509_crt_init(&caChain);
        mbedtls_x509_crt_init(&dnHints);
        mbedtls_ssl_config_init(&config);
        mbedtls_ssl_init(&ssl);
    }

    ~SslState() {
        mbedtls_ssl_free(&ssl);
        mbedtls_ssl_config_free(&config);
        mbedtls_x509_crt_free(&dnHints);
        mbedtls_x509_crt_free(&caChain);
        mbedtls_pk_free(&switchedKey);
        mbedtls_x509_crt_free(&switchedChain);
        mbedtls_pk_free(&ownKey);
        mbedtls_x509_crt_free(&ownChain);
        if (ownCertificate) X509_free_nid_postfix(ownCertificate);
        if (privateKey) EVP_PKEY_free_nid_postfix(privateKey);
    }
};

namespace {

SslCtxObject& RequireCtx(SslCtxObject* ctx, const char* function) {
    if (!ctx || !ctx->internal) throw std::invalid_argument(std::string(function) + ": null SSL_CTX");
    return *ctx;
}

SslObject& RequireSsl(SslObject* ssl, const char* function) {
    if (!ssl || !ssl->internal) throw std::invalid_argument(std::string(function) + ": null SSL");
    return *ssl;
}

int HostRandom(void*, unsigned char* output, std::size_t length) {
    HostEntropy_nid_no_patch(output, length);
    return 0;
}

int HostError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

bool HostWouldBlock(int error) {
#ifdef _WIN32
    return error == WSAEWOULDBLOCK;
#else
    return error == EAGAIN || error == EWOULDBLOCK;
#endif
}

bool HostInterrupted(int error) {
#ifdef _WIN32
    return error == WSAEINTR;
#else
    return error == EINTR;
#endif
}

struct SocketBio {
    int descriptor;
};

int SocketTransfer(Bio& bio, bool send, void* data, int length, bool& retry) {
    auto* socket = static_cast<SocketBio*>(BioState(bio));
    retry = false;
    std::shared_ptr<void> owner;
    std::uintptr_t native = 0;
    if (!KernelLookupSocketNative_nid_no_patch(socket->descriptor, &owner, &native))
        throw std::invalid_argument("socket BIO: descriptor " + std::to_string(socket->descriptor) + " is not a socket");
    for (;;) {
#ifdef _WIN32
        auto* transfer = static_cast<KernelSocketPoll::NativeOwner*>(owner.get());
        const int result = transfer && transfer->transfer ? transfer->transfer(transfer, send, data, length)
            : send ? ::send(static_cast<SOCKET>(native), static_cast<const char*>(data), length, 0)
                   : ::recv(static_cast<SOCKET>(native), static_cast<char*>(data), length, 0);
#else
        const auto result = send ? ::send(static_cast<int>(native), data, static_cast<std::size_t>(length), MSG_NOSIGNAL)
                                 : ::recv(static_cast<int>(native), data, static_cast<std::size_t>(length), 0);
#endif
        if (result >= 0) return static_cast<int>(result);
        const int error = HostError();
        if (HostInterrupted(error)) continue;
        retry = HostWouldBlock(error);
        return -1;
    }
}

int SocketWrite(Bio& bio, const void* data, int length, bool& retry) {
    return SocketTransfer(bio, true, const_cast<void*>(data), length, retry);
}

int SocketRead(Bio& bio, void* data, int length, bool& retry) {
    return SocketTransfer(bio, false, data, length, retry);
}

std::int64_t SocketCtrl(Bio& bio, int cmd, std::int64_t larg, void* parg) {
    auto* socket = static_cast<SocketBio*>(BioState(bio));
    switch (cmd) {
    case BioSetFd:
        if (!parg) throw std::invalid_argument("BIO_set_fd: null descriptor");
        if (socket->descriptor >= 0 && (BioCloseFlag(bio) & 1)) close_nid_postfix(socket->descriptor);
        socket->descriptor = *static_cast<int*>(parg);
        BIO_ctrl_nid_postfix(&bio, BioCtrlSetClose, larg, nullptr);
        return 1;
    case BioGetFd:
        if (parg) *static_cast<int*>(parg) = socket->descriptor;
        return socket->descriptor;
    case BioCtrlFlush:
    case BioCtrlDup:
        return 1;
    default:
        return 0;
    }
}

void SocketDestroy(Bio& bio) {
    auto* socket = static_cast<SocketBio*>(BioState(bio));
    if (socket && socket->descriptor >= 0 && (BioCloseFlag(bio) & 1)) close_nid_postfix(socket->descriptor);
    delete socket;
    BioState(bio) = nullptr;
}

constexpr BioHooks SocketHooks{SocketWrite, SocketRead, SocketCtrl, SocketDestroy};
constexpr BioMethod SocketMethod{TypeSocket, &SocketHooks};

int SslBioWrite(Bio& bio, const void* data, int length, bool& retry);
int SslBioRead(Bio& bio, void* data, int length, bool& retry);
std::int64_t SslBioCtrl(Bio& bio, int cmd, std::int64_t larg, void* parg);
void SslBioDestroy(Bio& bio);

constexpr BioHooks SslHooks{SslBioWrite, SslBioRead, SslBioCtrl, SslBioDestroy};
constexpr BioMethod SslBioMethod{TypeSsl, &SslHooks};

void FreeBios(SslObject& ssl) {
    auto* rbio = ssl.rbio;
    auto* wbio = ssl.wbio;
    ssl.rbio = nullptr;
    ssl.wbio = nullptr;
    if (rbio) BIO_free_nid_postfix(rbio);
    if (wbio && wbio != rbio) BIO_free_nid_postfix(wbio);
}

void ReleaseSession(SslSessionObject* session);

void Unref(SslCtxObject* ctx);

int SendCallback(void* argument, const unsigned char* data, std::size_t length) {
    auto& ssl = *static_cast<SslObject*>(argument);
    auto& state = *ssl.internal;
    try {
        if (!ssl.wbio) throw std::logic_error("SSL: no write BIO is set");
        bool retry = false;
        const int written = BioTransportWrite(ssl.wbio, data, static_cast<int>(std::min<std::size_t>(length, INT_MAX)), retry, "SSL");
        if (written > 0) return written;
        if (retry) return MBEDTLS_ERR_SSL_WANT_WRITE;
        state.syscallFailure = true;
        return MBEDTLS_ERR_NET_SEND_FAILED;
    } catch (...) {
        state.pending = std::current_exception();
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }
}

int ReceiveCallback(void* argument, unsigned char* data, std::size_t length) {
    auto& ssl = *static_cast<SslObject*>(argument);
    auto& state = *ssl.internal;
    try {
        if (!ssl.rbio) throw std::logic_error("SSL: no read BIO is set");
        bool retry = false;
        const int read = BioTransportRead(ssl.rbio, data, static_cast<int>(std::min<std::size_t>(length, INT_MAX)), retry, "SSL");
        if (read > 0) return read;
        if (read == 0) return 0;
        if (retry) return MBEDTLS_ERR_SSL_WANT_READ;
        state.syscallFailure = true;
        return MBEDTLS_ERR_NET_RECV_FAILED;
    } catch (...) {
        state.pending = std::current_exception();
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }
}

using StoreReference = std::unique_ptr<Store, void (APS5_VABI*)(Store*)>;

StoreReference AcquireStore(SslCtxObject& ctx, const char* function) {
    std::lock_guard lock(ctx.internal->mutex);
    auto* store = static_cast<Store*>(ctx.certStore);
    if (!store) throw std::invalid_argument(std::string(function) + ": the SSL_CTX has no X509_STORE (SSL_CTX_set_cert_store with NULL); what LibreSSL does then is not documented");
    LibreSslX509StoreUpRef_nid_no_patch(store);
    return StoreReference(store, X509_STORE_free_nid_postfix);
}

std::uint32_t AlertFlags(int error) {
    switch (error) {
    case X509VErrCertNotYetValid:
        return MBEDTLS_X509_BADCERT_FUTURE;
    case X509VErrCertHasExpired:
        return MBEDTLS_X509_BADCERT_EXPIRED;
    case X509VErrCertRevoked:
        return MBEDTLS_X509_BADCERT_REVOKED;
    case X509VErrHostnameMismatch:
    case X509VErrIpAddressMismatch:
        return MBEDTLS_X509_BADCERT_CN_MISMATCH;
    case X509VErrUnableToGetIssuerCert:
    case X509VErrCertSignatureFailure:
    case X509VErrDepthZeroSelfSigned:
    case X509VErrSelfSignedInChain:
    case X509VErrUnableToGetIssuerLocally:
    case X509VErrUnableToVerifyLeafSignature:
    case X509VErrInvalidCa:
    case X509VErrPathLengthExceeded:
    case X509VErrCertUntrusted:
        return MBEDTLS_X509_BADCERT_NOT_TRUSTED;
    default:
        return MBEDTLS_X509_BADCERT_OTHER;
    }
}

std::shared_ptr<PeerChain> BuildPeer(const mbedtls_x509_crt* certificate, bool includeLeaf);

int VerifyCallbackHook(void* argument, mbedtls_x509_crt* certificate, int depth, std::uint32_t* flags) {
    auto& ssl = *static_cast<SslObject*>(argument);
    auto& state = *ssl.internal;
    *flags = 0;
    if (depth != 0 || state.pending) return 0;
    try {
        const auto store = AcquireStore(*ssl.ctx, "SSL certificate verification");
        const auto peer = BuildPeer(certificate, false);
        if (!peer->leaf) throw std::runtime_error("SSL: the peer certificate Mbed TLS accepted does not convert to an X509 object");
        int error = 0;
        const int result = VerifyPeerChain(store.get(), static_cast<VerifyParam*>(ssl.param), peer->leaf, &peer->stack, reinterpret_cast<void*>(state.verifyCallback),
            StoreCtxSslIndex, &ssl, state.certVerifyCallback, state.certVerifyArgument, error);
        state.verifyResult = error;
        ssl.verifyResult = error;
        if (result > 0 || !(ssl.verifyMode & VerifyPeer)) return 0;
        if (state.role == 2 && !(ssl.verifyMode & VerifyFailIfNoPeerCert)) return MBEDTLS_ERR_X509_FATAL_ERROR;
        *flags = AlertFlags(error);
        return 0;
    } catch (...) {
        state.pending = std::current_exception();
        return MBEDTLS_ERR_X509_FATAL_ERROR;
    }
}

void ExportKeys(void* argument, mbedtls_ssl_key_export_type type, const unsigned char* secret, std::size_t secretLength,
    const unsigned char clientRandom[32], const unsigned char serverRandom[32], mbedtls_tls_prf_types) {
    auto& state = *static_cast<SslState*>(argument);
    if (type != MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET) return;
    std::memcpy(state.clientRandom, clientRandom, 32);
    std::memcpy(state.serverRandom, serverRandom, 32);
    std::memcpy(state.masterSecret, secret, std::min<std::size_t>(secretLength, sizeof(state.masterSecret)));
    state.haveRandoms = true;
}

int ParseIdentity(mbedtls_x509_crt& chain, mbedtls_pk_context& key, const Identity& identity) {
    mbedtls_x509_crt_free(&chain);
    mbedtls_x509_crt_init(&chain);
    mbedtls_pk_free(&key);
    mbedtls_pk_init(&key);
    if (identity.certificate.empty() || identity.key.empty()) return 0;
    if (mbedtls_x509_crt_parse_der(&chain, identity.certificate.data(), identity.certificate.size()) != 0) return -1;
    for (const auto& extra : identity.chain) {
        if (mbedtls_x509_crt_parse_der(&chain, extra.data(), extra.size()) != 0) return -1;
    }
    if (mbedtls_pk_parse_key(&key, identity.key.data(), identity.key.size(), nullptr, 0, HostRandom, nullptr) != 0) return -1;
    return 1;
}

int SniCallback(void* argument, mbedtls_ssl_context* context, const unsigned char* name, std::size_t length) {
    auto& ssl = *static_cast<SslObject*>(argument);
    auto& state = *ssl.internal;
    state.servername.assign(reinterpret_cast<const char*>(name), length);
    auto& ctx = *ssl.ctx;
    ServernameCallback callback = nullptr;
    void* callbackArgument = nullptr;
    {
        std::lock_guard lock(ctx.internal->mutex);
        callback = ctx.internal->servernameCallback;
        callbackArgument = ctx.internal->servernameArgument;
    }
    if (!callback) return 0;
    int alert = 112;
    int result = TlsextErrOk;
    auto* before = ssl.ctx;
    try {
        result = callback(&ssl, &alert, callbackArgument);
        if (ssl.ctx != before) {
            const auto identity = ssl.internal->identity;
            if (ParseIdentity(state.switchedChain, state.switchedKey, *identity) > 0)
                mbedtls_ssl_set_hs_own_cert(context, &state.switchedChain, &state.switchedKey);
        }
    } catch (...) {
        state.pending = std::current_exception();
        return -1;
    }
    if (result == TlsextErrAlertFatal) return -1;
    if (result != TlsextErrOk && result != TlsextErrNoack && result != TlsextErrAlertWarning) {
        state.pending = std::make_exception_ptr(std::invalid_argument("SSL servername callback: unknown result " + std::to_string(result)));
        return -1;
    }
    return 0;
}

std::vector<Der> PemObjects(const unsigned char* data, std::size_t length, const char* label, bool& malformed) {
    std::vector<Der> objects;
    malformed = false;
    const std::string_view text(reinterpret_cast<const char*>(data), length);
    const std::string begin = std::string("-----BEGIN ") + label + "-----";
    const std::string end = std::string("-----END ") + label + "-----";
    std::size_t cursor = 0;
    while (true) {
        const auto start = text.find(begin, cursor);
        if (start == std::string_view::npos) break;
        const auto stop = text.find(end, start);
        if (stop == std::string_view::npos) {
            malformed = true;
            break;
        }
        std::string block(text.substr(start, stop + end.size() - start));
        block.push_back('\n');
        mbedtls_pem_context pem;
        mbedtls_pem_init(&pem);
        std::size_t used = 0;
        const int result = mbedtls_pem_read_buffer(&pem, begin.c_str(), end.c_str(), reinterpret_cast<const unsigned char*>(block.c_str()), nullptr, 0, &used);
        if (result != 0) {
            mbedtls_pem_free(&pem);
            if (result == MBEDTLS_ERR_PEM_PASSWORD_REQUIRED) throw std::invalid_argument("SSL: encrypted PEM objects need the PEM password callback, which this build does not call");
            malformed = true;
            break;
        }
        std::size_t size = 0;
        const unsigned char* buffer = mbedtls_pem_get_buffer(&pem, &size);
        objects.emplace_back(buffer, buffer + size);
        mbedtls_pem_free(&pem);
        cursor = stop + end.size();
    }
    return objects;
}

bool ReadGuestFile(const char* path, Der& contents) {
    auto* stream = fopen_nid_postfix(path, "rb");
    if (!stream) return false;
    unsigned char chunk[4096];
    while (true) {
        const std::size_t got = fread_nid_postfix(chunk, 1, sizeof(chunk), stream);
        contents.insert(contents.end(), chunk, chunk + got);
        if (got < sizeof(chunk)) break;
    }
    fclose_nid_postfix(stream);
    return true;
}

bool ValidCertificate(const Der& der) {
    mbedtls_x509_crt parsed;
    mbedtls_x509_crt_init(&parsed);
    const bool valid = !der.empty() && mbedtls_x509_crt_parse_der(&parsed, der.data(), der.size()) == 0;
    mbedtls_x509_crt_free(&parsed);
    return valid;
}

int KeyMatches(const Der& certificate, const Der& key) {
    mbedtls_x509_crt parsed;
    mbedtls_x509_crt_init(&parsed);
    mbedtls_pk_context privateKey;
    mbedtls_pk_init(&privateKey);
    int result = -1;
    if (mbedtls_x509_crt_parse_der(&parsed, certificate.data(), certificate.size()) == 0 &&
        mbedtls_pk_parse_key(&privateKey, key.data(), key.size(), nullptr, 0, HostRandom, nullptr) == 0) {
        unsigned char first[2048];
        unsigned char second[2048];
        const int a = mbedtls_pk_write_pubkey_der(&parsed.pk, first, sizeof(first));
        const int b = mbedtls_pk_write_pubkey_der(&privateKey, second, sizeof(second));
        result = a > 0 && a == b && std::memcmp(first + sizeof(first) - a, second + sizeof(second) - b, static_cast<std::size_t>(a)) == 0 ? 1 : 0;
    }
    mbedtls_pk_free(&privateKey);
    mbedtls_x509_crt_free(&parsed);
    return result;
}

int ParsedKeyType(const Der& key) {
    mbedtls_pk_context parsed;
    mbedtls_pk_init(&parsed);
    int type = 0;
    if (mbedtls_pk_parse_key(&parsed, key.data(), key.size(), nullptr, 0, HostRandom, nullptr) == 0) {
        const auto kind = mbedtls_pk_get_type(&parsed);
        type = kind == MBEDTLS_PK_RSA ? PkeyRsa : (kind == MBEDTLS_PK_ECKEY || kind == MBEDTLS_PK_ECKEY_DH) ? PkeyEc : -1;
    }
    mbedtls_pk_free(&parsed);
    return type;
}

int SetCertificate(std::shared_ptr<const Identity>& identity, Der der, int function, bool keepChain) {
    if (!ValidCertificate(der)) {
        PushError(ErrLibSsl, function, ErrReasonAsn1Lib);
        return 0;
    }
    auto updated = std::make_shared<Identity>(*identity);
    updated->certificate = std::move(der);
    if (!keepChain) updated->chain.clear();
    identity = std::move(updated);
    return 1;
}

int SetKey(std::shared_ptr<const Identity>& identity, Der der, int expectedType, int function) {
    const int type = ParsedKeyType(der);
    if (type <= 0 || (expectedType && type != expectedType)) {
        PushError(ErrLibSsl, function, ErrReasonAsn1Lib);
        return 0;
    }
    if (!identity->certificate.empty() && KeyMatches(identity->certificate, der) != 1) {
        PushError(ErrLibX509, X509FunctionCheckPrivateKey, X509ReasonKeyValuesMismatch);
        return 0;
    }
    auto updated = std::make_shared<Identity>(*identity);
    updated->key = std::move(der);
    updated->keyType = type;
    identity = std::move(updated);
    return 1;
}

int CheckKey(const Identity& identity, int function) {
    if (identity.certificate.empty()) {
        PushError(ErrLibSsl, function, SslReasonNoCertificateAssigned);
        return 0;
    }
    if (identity.key.empty()) {
        PushError(ErrLibSsl, function, SslReasonNoPrivateKeyAssigned);
        return 0;
    }
    if (KeyMatches(identity.certificate, identity.key) != 1) {
        PushError(ErrLibX509, X509FunctionCheckPrivateKey, X509ReasonKeyValuesMismatch);
        return 0;
    }
    return 1;
}

Der Bytes(const unsigned char* data, std::size_t length) {
    return Der(data, data + length);
}

const X509Object& RequireX509Object(const X509Object* x, const char* function) {
    if (!x) throw std::invalid_argument(std::string(function) + ": null certificate");
    return *x;
}

Der CertificateDer(const X509Object* x, const char* function) {
    const auto& certificate = X509Certificate(RequireX509Object(x, function));
    return Bytes(certificate.raw.p, certificate.raw.len);
}

bool CipherSelected(const Cipher& cipher, std::string_view word, bool& known) {
    known = true;
    if (word == "ALL") return true;
    if (word == "eNULL" || word == "NULL" || word == "COMPLEMENTOFALL" || word == "aNULL" || word == "ADH" || word == "AECDH" ||
        word == "COMPLEMENTOFDEFAULT" || word == "3DES" || word == "DES" || word == "RC4" || word == "MD5" || word == "IDEA" || word == "LOW" ||
        word == "aGOST" || word == "aGOST01" || word == "kGOST" || word == "GOST89MAC" || word == "GOST94" || word == "STREEBOG256" ||
        word == "TLSv1.3" || word == "aDSS" || word == "DSS")
        return false;
    if (word == "kRSA") return cipher.kRsa;
    if (word == "RSA") return cipher.kRsa && cipher.aRsa;
    if (word == "aRSA") return cipher.aRsa;
    if (word == "aECDSA" || word == "ECDSA") return cipher.aEcdsa;
    if (word == "DH" || word == "DHE" || word == "EDH" || word == "kEDH") return cipher.kDh;
    if (word == "ECDH" || word == "ECDHE" || word == "EECDH" || word == "kEECDH") return cipher.kEcdh;
    if (word == "AES") return cipher.aes;
    if (word == "AES128") return cipher.aes && cipher.bits == 128;
    if (word == "AES256") return cipher.aes && cipher.bits == 256;
    if (word == "AESGCM") return cipher.aesGcm;
    if (word == "AEAD") return cipher.aead;
    if (word == "CAMELLIA") return cipher.camellia;
    if (word == "CAMELLIA128") return cipher.camellia && cipher.bits == 128;
    if (word == "CAMELLIA256") return cipher.camellia && cipher.bits == 256;
    if (word == "CHACHA20") return cipher.chacha;
    if (word == "SHA" || word == "SHA1") return cipher.sha1;
    if (word == "SHA256") return cipher.sha256;
    if (word == "SHA384") return cipher.sha384;
    if (word == "TLSv1" || word == "SSLv3") return true;
    if (word == "TLSv1.2") return cipher.tls12;
    if (word == "HIGH" || word == "MEDIUM" || word.rfind("@SECLEVEL", 0) == 0)
        throw std::invalid_argument("SSL_CTX_set_cipher_list: the cipher suites of '" + std::string(word) + "' are not documented");
    if (word == cipher.name) return true;
    known = false;
    return false;
}

int ParseCipherList(const char* text, std::vector<int>& result) {
    const auto& table = Ciphers();
    std::vector<const Cipher*> active;
    std::vector<const Cipher*> removed;
    std::vector<const Cipher*> deleted;
    const std::string_view control(text);
    std::size_t cursor = 0;
    bool first = true;
    while (cursor <= control.size()) {
        const auto end = control.find_first_of(":, ;", cursor);
        const auto word = control.substr(cursor, end == std::string_view::npos ? std::string_view::npos : end - cursor);
        cursor = end == std::string_view::npos ? control.size() + 1 : end + 1;
        if (word.empty()) continue;
        const bool wasFirst = first;
        first = false;
        if (word == "DEFAULT") {
            if (!wasFirst) throw std::invalid_argument("SSL_CTX_set_cipher_list: DEFAULT is only allowed as the first word");
            for (const auto& cipher : table.ciphers) active.push_back(cipher.get());
            continue;
        }
        if (word == "@STRENGTH") {
            std::stable_sort(active.begin(), active.end(), [](const Cipher* a, const Cipher* b) { return a->bits > b->bits; });
            continue;
        }
        char prefix = 0;
        auto body = word;
        if (body[0] == '!' || body[0] == '-' || body[0] == '+') {
            prefix = body[0];
            body.remove_prefix(1);
        }
        std::vector<const Cipher*> selected;
        bool unknown = false;
        std::size_t part = 0;
        while (part <= body.size()) {
            const auto plus = body.find('+', part);
            const auto term = body.substr(part, plus == std::string_view::npos ? std::string_view::npos : plus - part);
            part = plus == std::string_view::npos ? body.size() + 1 : plus + 1;
            bool anyKnown = false;
            for (const auto& cipher : table.ciphers) {
                bool known = true;
                CipherSelected(*cipher, term, known);
                anyKnown = anyKnown || known;
            }
            if (term.empty() || !anyKnown) unknown = true;
        }
        for (const auto& cipher : table.ciphers) {
            if (unknown) break;
            bool all = true;
            part = 0;
            while (part <= body.size()) {
                const auto plus = body.find('+', part);
                const auto term = body.substr(part, plus == std::string_view::npos ? std::string_view::npos : plus - part);
                part = plus == std::string_view::npos ? body.size() + 1 : plus + 1;
                bool known = true;
                if (!CipherSelected(*cipher, term, known)) all = false;
            }
            if (all) selected.push_back(cipher.get());
        }
        if (unknown) continue;
        const auto contains = [](const std::vector<const Cipher*>& list, const Cipher* cipher) { return std::find(list.begin(), list.end(), cipher) != list.end(); };
        for (const auto* cipher : selected) {
            if (prefix == '!') {
                if (!contains(deleted, cipher)) deleted.push_back(cipher);
                active.erase(std::remove(active.begin(), active.end(), cipher), active.end());
            } else if (prefix == '-') {
                if (contains(active, cipher)) {
                    active.erase(std::remove(active.begin(), active.end(), cipher), active.end());
                    removed.push_back(cipher);
                }
            } else if (prefix == '+') {
                if (contains(active, cipher)) {
                    active.erase(std::remove(active.begin(), active.end(), cipher), active.end());
                    active.push_back(cipher);
                }
            } else if (!contains(deleted, cipher) && !contains(active, cipher)) {
                active.push_back(cipher);
            }
        }
    }
    result.clear();
    for (const auto* cipher : active) result.push_back(static_cast<int>(cipher->object.id & 0xffff));
    return result.empty() ? 0 : 1;
}

int ParseGroups(const char* text, std::vector<std::uint16_t>& result) {
    struct Group {
        const char* nist;
        int nid;
        std::uint16_t iana;
    };
    static constexpr Group groups[] = {
        {"P-256", 415, MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1},
        {"P-384", 715, MBEDTLS_SSL_IANA_TLS_GROUP_SECP384R1},
        {"P-521", 716, MBEDTLS_SSL_IANA_TLS_GROUP_SECP521R1},
        {nullptr, 950, MBEDTLS_SSL_IANA_TLS_GROUP_X25519},
    };
    std::vector<std::uint16_t> parsed;
    const std::string_view list(text);
    std::size_t cursor = 0;
    while (cursor <= list.size()) {
        const auto end = list.find(':', cursor);
        const std::string name(list.substr(cursor, end == std::string_view::npos ? std::string_view::npos : end - cursor));
        cursor = end == std::string_view::npos ? list.size() + 1 : end + 1;
        const Group* found = nullptr;
        for (const auto& group : groups) {
            const auto* object = TableObject(group.nid, false);
            if ((group.nist && name == group.nist) || (object && object->sn && name == object->sn)) found = &group;
        }
        if (!found) return 0;
        if (std::find(parsed.begin(), parsed.end(), found->iana) == parsed.end()) parsed.push_back(found->iana);
    }
    if (parsed.empty()) return 0;
    result = std::move(parsed);
    return 1;
}

bool VersionAllowed(int version, std::uint64_t options, int minimum, int maximum) {
    if (version == Tls1Version && (options & OptionNoTlsv1)) return false;
    if (version == Tls1_1Version && (options & OptionNoTlsv1_1)) return false;
    if (version == Tls1_2Version && (options & OptionNoTlsv1_2)) return false;
    if (minimum && version < minimum) return false;
    if (maximum && version > maximum) return false;
    return true;
}

void PushHandshakeError(SslObject& ssl, int result, int function) {
    auto& state = *ssl.internal;
    switch (result) {
    case MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE:
        PushError(ErrLibSsl, function, SslAlertReasonOffset + state.ssl.MBEDTLS_PRIVATE(in_msg)[1]);
        return;
    case MBEDTLS_ERR_X509_CERT_VERIFY_FAILED:
    case MBEDTLS_ERR_X509_FATAL_ERROR:
        PushError(ErrLibSsl, function, SslReasonCertificateVerifyFailed);
        return;
    case MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE:
        if (state.role == 2) PushError(ErrLibSsl, function, SslReasonNoSharedCipher);
        else PushError(ErrLibSsl, function, SslAlertReasonOffset + 40);
        return;
    case MBEDTLS_ERR_SSL_BAD_PROTOCOL_VERSION:
        PushError(ErrLibSsl, function, SslReasonWrongVersionNumber);
        return;
    case MBEDTLS_ERR_SSL_INVALID_MAC:
        PushError(ErrLibSsl, function, SslReasonDecryptionFailedOrBadRecordMac);
        return;
    case MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE:
        PushError(ErrLibSsl, function, SslReasonUnexpectedMessage);
        return;
    case MBEDTLS_ERR_SSL_NO_CLIENT_CERTIFICATE:
        PushError(ErrLibSsl, function, SslReasonPeerDidNotReturnCertificate);
        return;
    default: {
        char text[160];
        mbedtls_strerror(result, text, sizeof(text));
        throw std::runtime_error(std::string("SSL: Mbed TLS failure without a known LibreSSL error code: ") + text);
    }
    }
}

int Configure(SslObject& ssl) {
    auto& state = *ssl.internal;
    if (state.configured) return 1;
    const bool server = state.role == 2;
    std::uint64_t options = state.options;
    if (!VersionAllowed(Tls1_2Version, options, state.minVersion, state.maxVersion)) {
        PushError(ErrLibSsl, server ? SslFunctionSsl3Accept : SslFunctionSsl3Connect, SslReasonNoProtocolsAvailable);
        return 0;
    }
    if (mbedtls_ssl_config_defaults(&state.config, server ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
            MBEDTLS_SSL_PRESET_DEFAULT) != 0)
        throw std::runtime_error("SSL: Mbed TLS configuration failed");
    mbedtls_ssl_conf_rng(&state.config, HostRandom, nullptr);
    mbedtls_ssl_conf_min_tls_version(&state.config, MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_max_tls_version(&state.config, MBEDTLS_SSL_VERSION_TLS1_2);
    {
        auto& ctx = *ssl.ctx->internal;
        std::lock_guard lock(ctx.mutex);
        state.clientAuthorities = ctx.clientAuthorities;
    }
    mbedtls_ssl_conf_ca_chain(&state.config, &state.caChain, nullptr);
    if (server) {
        const int mode = ssl.verifyMode;
        mbedtls_ssl_conf_authmode(&state.config, !(mode & VerifyPeer) ? MBEDTLS_SSL_VERIFY_NONE
            : (mode & VerifyFailIfNoPeerCert) ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_OPTIONAL);
        for (const auto& der : state.clientAuthorities) {
            if (mbedtls_x509_crt_parse_der(&state.dnHints, der.data(), der.size()) != 0) throw std::runtime_error("SSL: a client CA no longer parses");
        }
        mbedtls_ssl_conf_dn_hints(&state.config, state.dnHints.raw.p ? &state.dnHints : nullptr);
        mbedtls_ssl_conf_sni(&state.config, SniCallback, &ssl);
        std::int64_t cacheMode = 0;
        {
            auto& ctx = *ssl.ctx->internal;
            std::lock_guard lock(ctx.mutex);
            cacheMode = ctx.sessCacheMode;
        }
        if (cacheMode & SessCacheServer) mbedtls_ssl_conf_session_cache(&state.config, &ssl.ctx->internal->cache, mbedtls_ssl_cache_get, mbedtls_ssl_cache_set);
    } else {
        mbedtls_ssl_conf_authmode(&state.config, (ssl.verifyMode & VerifyPeer) ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_OPTIONAL);
        mbedtls_ssl_conf_session_tickets(&state.config, (options & OptionNoTicket) ? MBEDTLS_SSL_SESSION_TICKETS_DISABLED : MBEDTLS_SSL_SESSION_TICKETS_ENABLED);
    }
    mbedtls_ssl_conf_verify(&state.config, VerifyCallbackHook, &ssl);
    const int own = ParseIdentity(state.ownChain, state.ownKey, *state.identity);
    if (own < 0) throw std::runtime_error("SSL: the loaded certificate or key no longer parses");
    if (own > 0 && mbedtls_ssl_conf_own_cert(&state.config, &state.ownChain, &state.ownKey) != 0) throw std::runtime_error("SSL: Mbed TLS rejected the certificate");
    if (!state.ciphers.empty()) {
        state.suiteList = state.ciphers;
        state.suiteList.push_back(0);
        mbedtls_ssl_conf_ciphersuites(&state.config, state.suiteList.data());
    } else {
        for (const auto& cipher : Ciphers().ciphers) state.suiteList.push_back(static_cast<int>(cipher->object.id & 0xffff));
        state.suiteList.push_back(0);
        mbedtls_ssl_conf_ciphersuites(&state.config, state.suiteList.data());
    }
    if (!state.groups.empty()) {
        state.groupList = state.groups;
        state.groupList.push_back(MBEDTLS_SSL_IANA_TLS_GROUP_NONE);
        mbedtls_ssl_conf_groups(&state.config, state.groupList.data());
    }
    if (!state.alpn.empty() && !server) {
        for (std::size_t index = 0; index < state.alpn.size();) {
            const std::size_t length = state.alpn[index];
            state.alpnStrings.emplace_back(reinterpret_cast<const char*>(state.alpn.data() + index + 1), length);
            index += length + 1;
        }
        for (const auto& protocol : state.alpnStrings) state.alpnList.push_back(protocol.c_str());
        state.alpnList.push_back(nullptr);
        if (mbedtls_ssl_conf_alpn_protocols(&state.config, state.alpnList.data()) != 0) throw std::runtime_error("SSL: Mbed TLS rejected the ALPN list");
    }
    if (mbedtls_ssl_setup(&state.ssl, &state.config) != 0) throw std::runtime_error("SSL: Mbed TLS setup failed");
    mbedtls_ssl_set_export_keys_cb(&state.ssl, ExportKeys, &state);
    mbedtls_ssl_set_bio(&state.ssl, &ssl, SendCallback, ReceiveCallback, nullptr);
    if (!server) {
        if (mbedtls_ssl_set_hostname(&state.ssl, state.hostname.empty() ? nullptr : state.hostname.c_str()) != 0)
            throw std::runtime_error("SSL: Mbed TLS rejected the host name");
        if (ssl.session && ssl.session->internal->resumable) mbedtls_ssl_set_session(&state.ssl, &ssl.session->internal->session);
    }
    state.configured = true;
    return 1;
}

SslSessionObject* NewSession() {
    auto* session = new SslSessionObject{};
    session->internal = new SessionState();
    session->references = 1;
    return session;
}

void ReleaseSession(SslSessionObject* session) {
    if (!session) return;
    const int left = session->internal->references.fetch_sub(1) - 1;
    session->references = left;
    if (left > 0) return;
    delete session->internal;
    delete session;
}

std::shared_ptr<PeerChain> BuildPeer(const mbedtls_x509_crt* certificate, bool includeLeaf) {
    auto peer = std::make_shared<PeerChain>();
    if (!certificate) return peer;
    peer->leaf = X509FromDer(certificate->raw.p, certificate->raw.len);
    for (const auto* entry = includeLeaf ? certificate : certificate->next; entry; entry = entry->next) {
        if (auto* object = X509FromDer(entry->raw.p, entry->raw.len)) peer->certificates.push_back(object);
    }
    for (auto* object : peer->certificates) peer->pointers.push_back(reinterpret_cast<char*>(object));
    peer->stack = {static_cast<int>(peer->pointers.size()), peer->pointers.data(), 0, static_cast<int>(peer->pointers.size()), nullptr};
    return peer;
}

void Finish(SslObject& ssl) {
    auto& state = *ssl.internal;
    state.handshaken = true;
    ssl.version = Tls1_2Version;
    const bool server = state.role == 2;
    std::vector<unsigned char> offered;
    if (ssl.session && !server) offered.assign(ssl.session->sessionId, ssl.session->sessionId + ssl.session->sessionIdLength);
    auto* session = NewSession();
    auto& sessionState = *session->internal;
    if (mbedtls_ssl_get_session(&state.ssl, &sessionState.session) == 0) sessionState.resumable = true;
    const auto idLength = mbedtls_ssl_session_get_id_len(&sessionState.session);
    session->sessionIdLength = static_cast<unsigned int>(std::min<std::size_t>(idLength, 32));
    std::memcpy(session->sessionId, *mbedtls_ssl_session_get_id(&sessionState.session), session->sessionIdLength);
    session->sslVersion = Tls1_2Version;
    if (state.haveRandoms) {
        std::memcpy(session->masterKey, state.masterSecret, sizeof(session->masterKey));
        session->masterKeyLength = 48;
    }
    std::memcpy(session->sidCtx, ssl.sidCtx, sizeof(session->sidCtx));
    session->sidCtxLength = ssl.sidCtxLength;
    sessionState.peer = BuildPeer(mbedtls_ssl_get_peer_cert(&state.ssl), !server);
    session->peer = sessionState.peer->leaf;
    session->verifyResult = ssl.verifyResult;
    session->timeout = ssl.ctx->sessionTimeout;
    session->time = static_cast<std::int64_t>(std::time(nullptr));
    const auto* cipher = Ciphers().ById(mbedtls_ssl_get_ciphersuite_id_from_ssl(&state.ssl));
    session->cipher = cipher ? &cipher->object : nullptr;
    session->cipherId = cipher ? cipher->object.id : 0;
    sessionState.hostname = server ? state.servername : state.hostname;
    session->tlsextHostname = sessionState.hostname.empty() ? nullptr : sessionState.hostname.data();
    state.reused = !offered.empty() && offered.size() == session->sessionIdLength && std::memcmp(offered.data(), session->sessionId, offered.size()) == 0;
    ReleaseSession(ssl.session);
    ssl.session = session;
    NewSessionCallback callback = nullptr;
    std::int64_t cacheMode = 0;
    {
        auto& ctx = *ssl.ctx->internal;
        std::lock_guard lock(ctx.mutex);
        callback = ctx.newSessionCallback;
        cacheMode = ctx.sessCacheMode;
    }
    if (callback && !state.reused && (cacheMode & (server ? SessCacheServer : SessCacheClient))) {
        session->references = session->internal->references.fetch_add(1) + 1;
        if (callback(&ssl, session) == 0) {
            if (server) mbedtls_ssl_cache_remove(&ssl.ctx->internal->cache, session->sessionId, session->sessionIdLength);
            ReleaseSession(session);
        }
    }
}

int SelectClientCertificate(SslObject& ssl) {
    auto& state = *ssl.internal;
    ClientCertCallback callback = nullptr;
    {
        std::lock_guard lock(ssl.ctx->internal->mutex);
        callback = ssl.ctx->internal->clientCertCallback;
    }
    if (!callback || !state.identity->certificate.empty()) {
        state.certificateRequested = false;
        return 0;
    }
    X509Object* x509 = nullptr;
    EvpPkey* pkey = nullptr;
    const int result = callback(&ssl, &x509, &pkey);
    if (result < 0) return -1;
    state.certificateRequested = false;
    if (result == 0) return 0;
    if (result != 1 || !x509 || !pkey)
        throw std::invalid_argument("SSL client certificate callback: result " + std::to_string(result) + " without both a certificate and a key is not documented");
    auto identity = state.identity;
    if (!SetCertificate(identity, CertificateDer(x509, "SSL client certificate callback"), SslFunctionUseCertificateAsn1, true) ||
        !SetKey(identity, EncodePrivateKey(pkey, "SSL client certificate callback"), 0, SslFunctionUsePrivateKeyAsn1))
        throw std::invalid_argument("SSL client certificate callback: the returned certificate and key do not install; what LibreSSL does then is not documented");
    state.identity = std::move(identity);
    if (ParseIdentity(state.ownChain, state.ownKey, *state.identity) <= 0 || mbedtls_ssl_conf_own_cert(&state.config, &state.ownChain, &state.ownKey) != 0)
        throw std::runtime_error("SSL client certificate callback: Mbed TLS rejected the returned certificate and key");
    return 0;
}

int RunHandshake(SslObject& ssl) {
    auto& state = *ssl.internal;
    while (!mbedtls_ssl_is_handshake_over(&state.ssl)) {
        const int step = state.ssl.MBEDTLS_PRIVATE(state);
        if (step == MBEDTLS_SSL_CLIENT_CERTIFICATE && state.certificateRequested && SelectClientCertificate(ssl) < 0) return HandshakeWantX509Lookup;
        const int result = mbedtls_ssl_handshake_step(&state.ssl);
        if (result != 0) return result;
        if (step == MBEDTLS_SSL_CERTIFICATE_REQUEST && state.role == 1)
            state.certificateRequested = state.ssl.MBEDTLS_PRIVATE(in_msgtype) == MBEDTLS_SSL_MSG_HANDSHAKE &&
                state.ssl.MBEDTLS_PRIVATE(in_msg)[0] == MBEDTLS_SSL_HS_CERTIFICATE_REQUEST;
    }
    return 0;
}

int Handshake(SslObject& ssl, const char* function) {
    auto& state = *ssl.internal;
    if (state.handshaken) return 1;
    if (state.failed) throw std::logic_error(std::string(function) + ": the handshake already failed; continuing after a fatal error is not supported");
    const int errorFunction = state.role == 2 ? SslFunctionSsl3Accept : SslFunctionSsl3Connect;
    if (!Configure(ssl)) {
        state.failed = true;
        return -1;
    }
    state.want = StateNothing;
    state.syscallFailure = false;
    const int result = RunHandshake(ssl);
    if (state.pending) {
        auto pending = state.pending;
        state.pending = nullptr;
        state.failed = true;
        std::rethrow_exception(pending);
    }
    if (result == 0) {
        Finish(ssl);
        return 1;
    }
    if (result == HandshakeWantX509Lookup) {
        state.want = StateX509Lookup;
        return -1;
    }
    if (result == MBEDTLS_ERR_SSL_WANT_READ) {
        state.want = StateReading;
        return -1;
    }
    if (result == MBEDTLS_ERR_SSL_WANT_WRITE) {
        state.want = StateWriting;
        return -1;
    }
    state.failed = true;
    if (result == MBEDTLS_ERR_SSL_CONN_EOF) return 0;
    if (result == MBEDTLS_ERR_NET_SEND_FAILED || result == MBEDTLS_ERR_NET_RECV_FAILED) return -1;
    PushHandshakeError(ssl, result, errorFunction);
    return -1;
}

int ReadRecords(SslObject& ssl, unsigned char* buffer, int length, const char* function) {
    auto& state = *ssl.internal;
    if (state.role == 0) throw std::logic_error(std::string(function) + ": the connection has no client or server state yet");
    const int handshake = Handshake(ssl, function);
    if (handshake <= 0) return handshake;
    if (state.shutdown & ReceivedShutdown) return 0;
    state.want = StateNothing;
    state.syscallFailure = false;
    const int result = mbedtls_ssl_read(&state.ssl, buffer, static_cast<std::size_t>(length));
    if (state.pending) {
        auto pending = state.pending;
        state.pending = nullptr;
        std::rethrow_exception(pending);
    }
    if (result > 0) return result;
    switch (result) {
    case MBEDTLS_ERR_SSL_WANT_READ:
        state.want = StateReading;
        return -1;
    case MBEDTLS_ERR_SSL_WANT_WRITE:
        state.want = StateWriting;
        return -1;
    case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
        state.shutdown |= ReceivedShutdown;
        return 0;
    case 0:
    case MBEDTLS_ERR_SSL_CONN_EOF:
        return 0;
    case MBEDTLS_ERR_NET_RECV_FAILED:
    case MBEDTLS_ERR_NET_SEND_FAILED:
        return -1;
    case MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET:
        return ReadRecords(ssl, buffer, length, function);
    default:
        PushHandshakeError(ssl, result, SslFunctionSsl3ReadBytes);
        return -1;
    }
}

void Unref(SslCtxObject* ctx) {
    if (!ctx) return;
    const int left = ctx->internal->references.fetch_sub(1) - 1;
    ctx->references = left;
    if (left > 0) return;
    X509_STORE_free_nid_postfix(static_cast<Store*>(ctx->certStore));
    delete ctx->internal;
    delete ctx;
}

void CopyFromCtx(SslObject& ssl, SslCtxObject& ctx) {
    auto& state = *ssl.internal;
    auto& source = *ctx.internal;
    std::lock_guard lock(source.mutex);
    state.options = source.options;
    state.mode = source.mode;
    state.quietShutdown = source.quietShutdown;
    state.verifyCallback = source.verifyCallback;
    state.certVerifyCallback = source.certVerifyCallback;
    state.certVerifyArgument = source.certVerifyArgument;
    state.ciphers = source.ciphers;
    state.groups = source.groups;
    state.alpn = source.alpn;
    state.minVersion = source.minVersion;
    state.maxVersion = source.maxVersion;
    state.identity = source.identity;
    ssl.verifyMode = ctx.verifyMode;
    ssl.sidCtxLength = ctx.sidCtxLength;
    std::memcpy(ssl.sidCtx, ctx.sidCtx, sizeof(ssl.sidCtx));
}

int SetAlpn(Der& target, const unsigned char* protos, unsigned int length) {
    if (!protos || length == 0) {
        target.clear();
        return 0;
    }
    for (unsigned int index = 0; index < length;) {
        const unsigned int item = protos[index];
        if (item == 0 || index + 1 + item > length) return 1;
        index += item + 1;
    }
    target.assign(protos, protos + length);
    return 0;
}

std::int64_t CommonCtrl(std::uint64_t& options, std::uint64_t& mode, int cmd, std::int64_t larg, bool& handled) {
    handled = true;
    switch (cmd) {
    case CtrlOptions:
        options |= static_cast<std::uint64_t>(larg);
        return static_cast<std::int64_t>(options);
    case CtrlClearOptions:
        options &= ~static_cast<std::uint64_t>(larg);
        return static_cast<std::int64_t>(options);
    case CtrlMode:
        mode |= static_cast<std::uint64_t>(larg);
        return static_cast<std::int64_t>(mode);
    case CtrlClearMode:
        mode &= ~static_cast<std::uint64_t>(larg);
        return static_cast<std::int64_t>(mode);
    case CtrlSetEcdhAuto:
        return 1;
    default:
        handled = false;
        return 0;
    }
}

int SslBioWrite(Bio& bio, const void* data, int length, bool& retry);
int SslBioRead(Bio& bio, void* data, int length, bool& retry);

}

}

using namespace LibreSsl;

extern "C" {
int APS5_VABI SSL_write_nid_postfix(SslObject* ssl, const void* buf, int num);
int APS5_VABI SSL_read_nid_postfix(SslObject* ssl, void* buf, int num);
int APS5_VABI SSL_connect_nid_postfix(SslObject* ssl);
int APS5_VABI SSL_accept_nid_postfix(SslObject* ssl);
int APS5_VABI SSL_pending_nid_postfix(const SslObject* s);
void APS5_VABI SSL_free_nid_postfix(SslObject* ssl);
int APS5_VABI SSL_get_error_nid_postfix(const SslObject* s, int ret_code);
}

namespace LibreSsl {

namespace {

struct SslBio {
    SslObject* ssl = nullptr;
};

SslObject& BioSsl(Bio& bio) {
    auto* state = static_cast<SslBio*>(BioState(bio));
    if (!state || !state->ssl) throw std::logic_error("SSL BIO: no SSL is set");
    return *state->ssl;
}

int SslBioWrite(Bio& bio, const void* data, int length, bool& retry) {
    auto& ssl = BioSsl(bio);
    const int result = SSL_write_nid_postfix(&ssl, data, length);
    if (result <= 0) {
        const int error = SSL_get_error_nid_postfix(&ssl, result);
        retry = error == ErrorWantRead || error == ErrorWantWrite;
    }
    return result;
}

int SslBioRead(Bio& bio, void* data, int length, bool& retry) {
    auto& ssl = BioSsl(bio);
    const int result = SSL_read_nid_postfix(&ssl, data, length);
    if (result <= 0) {
        const int error = SSL_get_error_nid_postfix(&ssl, result);
        retry = error == ErrorWantRead || error == ErrorWantWrite;
    }
    return result;
}

std::int64_t SslBioCtrl(Bio& bio, int cmd, std::int64_t larg, void* parg) {
    auto*& raw = BioState(bio);
    if (!raw) raw = new SslBio();
    auto& state = *static_cast<SslBio*>(raw);
    switch (cmd) {
    case BioSetSsl:
        if (state.ssl && (BioCloseFlag(bio) & 1)) SSL_free_nid_postfix(state.ssl);
        state.ssl = static_cast<SslObject*>(parg);
        BIO_ctrl_nid_postfix(&bio, BioCtrlSetClose, larg, nullptr);
        return 1;
    case BioGetSsl:
        if (parg) *static_cast<SslObject**>(parg) = state.ssl;
        return 1;
    case BioSslMode: {
        auto& ssl = BioSsl(bio);
        ssl.internal->role = larg ? 1 : 2;
        ssl.server = larg ? 0 : 1;
        return 1;
    }
    case BioDoStateMachine: {
        auto& ssl = BioSsl(bio);
        if (ssl.internal->role == 0) throw std::logic_error("BIO_do_handshake: the SSL has no client or server state yet");
        return ssl.internal->role == 2 ? SSL_accept_nid_postfix(&ssl) : SSL_connect_nid_postfix(&ssl);
    }
    case BioCtrlPending:
        return SSL_pending_nid_postfix(&BioSsl(bio));
    case BioCtrlFlush:
        return 1;
    case BioCtrlGetClose:
        return BioCloseFlag(bio);
    default:
        throw std::invalid_argument("SSL BIO: control " + std::to_string(cmd) + " goes to the next BIO in the chain, which this build has no way to set");
    }
}

void SslBioDestroy(Bio& bio) {
    auto* state = static_cast<SslBio*>(BioState(bio));
    if (state && state->ssl && (BioCloseFlag(bio) & 1)) SSL_free_nid_postfix(state->ssl);
    delete state;
    BioState(bio) = nullptr;
}

}

}

extern "C" {

const SslMethodObject* APS5_VABI SSLv23_client_method_nid_postfix() {
    return &Sslv23ClientMethod;
}

const SslMethodObject* APS5_VABI SSLv23_server_method_nid_postfix() {
    return &Sslv23ServerMethod;
}

const SslMethodObject* APS5_VABI TLS_client_method_nid_postfix() {
    return &TlsClientMethod;
}

Bio* APS5_VABI BIO_new_socket_nid_postfix(int sock, int close_flag) {
    auto* bio = BIO_new_nid_postfix(&SocketMethod);
    if (!bio) return nullptr;
    BioState(*bio) = new SocketBio{sock};
    BIO_ctrl_nid_postfix(bio, BioCtrlSetClose, close_flag & 1, nullptr);
    return bio;
}

const BioMethod* APS5_VABI BIO_f_ssl_nid_postfix() {
    return &SslBioMethod;
}

SslCtxObject* APS5_VABI SSL_CTX_new_nid_postfix(const SslMethodObject* meth) {
    if (meth != &Sslv23ClientMethod && meth != &Sslv23ServerMethod && meth != &TlsClientMethod) {
        if (!meth) {
            PushError(ErrLibSsl, SslFunctionCtxNew, SslReasonNullSslMethodPassed);
            return nullptr;
        }
        throw std::invalid_argument("SSL_CTX_new: unknown SSL_METHOD");
    }
    auto* ctx = new SslCtxObject{};
    ctx->method = meth;
    ctx->sessionTimeout = DefaultSessionTimeout;
    ctx->references = 1;
    ctx->verifyMode = VerifyNone;
    ctx->internal = new SslCtxState();
    ctx->certStore = X509_STORE_new_nid_postfix();
    return ctx;
}

void APS5_VABI SSL_CTX_free_nid_postfix(SslCtxObject* ctx) {
    if (!ctx) return;
    Unref(ctx);
}

int LibreSslSslCtxUpRef_nid_no_patch(SslCtxObject* ctx) {
    auto& context = RequireCtx(ctx, "SSL_CTX_up_ref");
    context.references = context.internal->references.fetch_add(1) + 1;
    return 1;
}

std::int64_t APS5_VABI SSL_CTX_ctrl_nid_postfix(SslCtxObject* ctx, int cmd, std::int64_t larg, void* parg) {
    auto& context = RequireCtx(ctx, __func__);
    auto& state = *context.internal;
    std::lock_guard lock(state.mutex);
    bool handled = false;
    const auto common = CommonCtrl(state.options, state.mode, cmd, larg, handled);
    if (handled) return common;
    switch (cmd) {
    case CtrlGetReadAhead:
        return state.readAhead;
    case CtrlSetReadAhead: {
        const auto previous = state.readAhead;
        state.readAhead = larg;
        return previous;
    }
    case CtrlSetSessCacheSize: {
        const auto previous = state.sessCacheSize;
        state.sessCacheSize = larg;
        mbedtls_ssl_cache_set_max_entries(&state.cache, larg > INT_MAX || larg <= 0 ? INT_MAX : static_cast<int>(larg));
        return previous;
    }
    case CtrlGetSessCacheSize:
        return state.sessCacheSize;
    case CtrlSetSessCacheMode: {
        const auto previous = state.sessCacheMode;
        if (larg & ~static_cast<std::int64_t>(SessCacheClient | SessCacheServer))
            throw std::invalid_argument("SSL_CTX_set_session_cache_mode: only SSL_SESS_CACHE_CLIENT and SSL_SESS_CACHE_SERVER are supported");
        state.sessCacheMode = larg;
        return previous;
    }
    case CtrlGetSessCacheMode:
        return state.sessCacheMode;
    case CtrlGetMaxCertList:
        return state.maxCertList;
    case CtrlSetMaxCertList: {
        const auto previous = state.maxCertList;
        state.maxCertList = larg;
        return previous;
    }
    case CtrlSetTlsextServernameArg:
        state.servernameArgument = parg;
        return 1;
    case CtrlExtraChainCert: {
        auto* certificate = static_cast<X509Object*>(parg);
        auto der = CertificateDer(certificate, "SSL_CTX_add_extra_chain_cert");
        auto updated = std::make_shared<Identity>(*state.identity);
        updated->chain.push_back(std::move(der));
        state.identity = std::move(updated);
        state.extraCertificates.push_back(certificate);
        return 1;
    }
    case CtrlClearExtraChainCerts: {
        auto updated = std::make_shared<Identity>(*state.identity);
        updated->chain.clear();
        state.identity = std::move(updated);
        for (auto* certificate : state.extraCertificates) X509_free_nid_postfix(certificate);
        state.extraCertificates.clear();
        return 1;
    }
    case CtrlSetGroupsList:
        if (!parg) throw std::invalid_argument("SSL_CTX_set1_groups_list: null list");
        return ParseGroups(static_cast<const char*>(parg), state.groups);
    case CtrlSetMinProtoVersion:
    case CtrlSetMaxProtoVersion: {
        const int version = static_cast<int>(larg);
        if (version != 0 && version != Tls1Version && version != Tls1_1Version && version != Tls1_2Version) return 0;
        (cmd == CtrlSetMinProtoVersion ? state.minVersion : state.maxVersion) = version;
        return 1;
    }
    default:
        throw std::invalid_argument("SSL_CTX_ctrl: command " + std::to_string(cmd) + " is not supported");
    }
}

std::int64_t APS5_VABI SSL_CTX_callback_ctrl_nid_postfix(SslCtxObject* ctx, int cmd, void (*fp)()) {
    auto& context = RequireCtx(ctx, __func__);
    if (cmd != CtrlSetTlsextServernameCb) throw std::invalid_argument("SSL_CTX_callback_ctrl: command " + std::to_string(cmd) + " is not supported");
    std::lock_guard lock(context.internal->mutex);
    context.internal->servernameCallback = reinterpret_cast<ServernameCallback>(fp);
    return 1;
}

int LibreSslSslCtxSetProtoVersion_nid_no_patch(SslCtxObject* ctx, std::uint16_t version, int maximum) {
    auto& context = RequireCtx(ctx, maximum ? "SSL_CTX_set_max_proto_version" : "SSL_CTX_set_min_proto_version");
    if (version != 0 && version != Tls1Version && version != Tls1_1Version && version != Tls1_2Version && version != Tls1_3Version) return 0;
    std::lock_guard lock(context.internal->mutex);
    (maximum ? context.internal->maxVersion : context.internal->minVersion) = version == Tls1_3Version && !maximum ? 0x7fffffff : version;
    return 1;
}

void APS5_VABI SSL_CTX_set_verify_nid_postfix(SslCtxObject* ctx, int mode, VerifyCallback callback) {
    auto& context = RequireCtx(ctx, __func__);
    if (mode & VerifyClientOnce) throw std::invalid_argument("SSL_CTX_set_verify: SSL_VERIFY_CLIENT_ONCE needs renegotiation, which this build does not do");
    std::lock_guard lock(context.internal->mutex);
    context.verifyMode = mode;
    context.internal->verifyCallback = callback;
}

int APS5_VABI SSL_CTX_get_verify_mode_nid_postfix(const SslCtxObject* ctx) {
    return RequireCtx(const_cast<SslCtxObject*>(ctx), __func__).verifyMode;
}

void APS5_VABI SSL_CTX_set_quiet_shutdown_nid_postfix(SslCtxObject* ctx, int mode) {
    auto& context = RequireCtx(ctx, __func__);
    std::lock_guard lock(context.internal->mutex);
    context.internal->quietShutdown = mode;
}

int APS5_VABI SSL_CTX_set_session_id_context_nid_postfix(SslCtxObject* ctx, const unsigned char* sid_ctx, unsigned int sid_ctx_len) {
    auto& context = RequireCtx(ctx, __func__);
    if (sid_ctx_len > sizeof(context.sidCtx)) {
        PushError(ErrLibSsl, SslFunctionCtxSetSessionIdContext, SslReasonSessionIdContextTooLong);
        return 0;
    }
    if (sid_ctx_len && !sid_ctx) throw std::invalid_argument("SSL_CTX_set_session_id_context: null context");
    std::lock_guard lock(context.internal->mutex);
    context.sidCtxLength = sid_ctx_len;
    std::memset(context.sidCtx, 0, sizeof(context.sidCtx));
    if (sid_ctx_len) std::memcpy(context.sidCtx, sid_ctx, sid_ctx_len);
    return 1;
}

void APS5_VABI SSL_CTX_set_default_passwd_cb_nid_postfix(SslCtxObject* ctx, void* cb) {
    RequireCtx(ctx, __func__).defaultPasswdCallback = cb;
}

void APS5_VABI SSL_CTX_set_default_passwd_cb_userdata_nid_postfix(SslCtxObject* ctx, void* u) {
    RequireCtx(ctx, __func__).defaultPasswdCallbackUserdata = u;
}

void APS5_VABI SSL_CTX_sess_set_new_cb_nid_postfix(SslCtxObject* ctx, NewSessionCallback callback) {
    auto& context = RequireCtx(ctx, __func__);
    std::lock_guard lock(context.internal->mutex);
    context.internal->newSessionCallback = callback;
}

int APS5_VABI SSL_CTX_set_cipher_list_nid_postfix(SslCtxObject* ctx, const char* str) {
    auto& context = RequireCtx(ctx, __func__);
    if (!str) throw std::invalid_argument("SSL_CTX_set_cipher_list: null string");
    std::vector<int> list;
    if (!ParseCipherList(str, list)) {
        PushError(ErrLibSsl, SslFunctionCtxSetCipherList, SslReasonNoCipherMatch);
        return 0;
    }
    std::lock_guard lock(context.internal->mutex);
    context.internal->ciphers = std::move(list);
    return 1;
}

int APS5_VABI SSL_CTX_set1_groups_list_nid_postfix(SslCtxObject* ctx, const char* groups) {
    return static_cast<int>(SSL_CTX_ctrl_nid_postfix(ctx, CtrlSetGroupsList, 0, const_cast<char*>(groups)));
}

int APS5_VABI SSL_CTX_set_alpn_protos_nid_postfix(SslCtxObject* ctx, const unsigned char* protos, unsigned int protos_len) {
    auto& context = RequireCtx(ctx, __func__);
    std::lock_guard lock(context.internal->mutex);
    return SetAlpn(context.internal->alpn, protos, protos_len);
}

int APS5_VABI SSL_CTX_get_ex_new_index_nid_postfix(std::int64_t, void*, void* new_func, void* dup_func, void* free_func) {
    if (new_func || dup_func || free_func) throw std::invalid_argument("SSL_CTX_get_ex_new_index: ex_data callbacks are not supported");
    auto& indices = Indices();
    std::lock_guard lock(indices.mutex);
    return indices.nextCtx++;
}

int APS5_VABI SSL_CTX_set_ex_data_nid_postfix(SslCtxObject* ctx, int idx, void* data) {
    auto& context = RequireCtx(ctx, __func__);
    if (idx < 0) throw std::invalid_argument("SSL_CTX_set_ex_data: negative index");
    std::lock_guard lock(context.internal->mutex);
    context.internal->exData[idx] = data;
    return 1;
}

void* APS5_VABI SSL_CTX_get_ex_data_nid_postfix(const SslCtxObject* ctx, int idx) {
    auto& context = RequireCtx(const_cast<SslCtxObject*>(ctx), __func__);
    std::lock_guard lock(context.internal->mutex);
    const auto found = context.internal->exData.find(idx);
    return found == context.internal->exData.end() ? nullptr : found->second;
}

int APS5_VABI SSL_CTX_use_certificate_nid_postfix(SslCtxObject* ctx, X509Object* x) {
    auto& context = RequireCtx(ctx, __func__);
    auto der = CertificateDer(x, __func__);
    std::lock_guard lock(context.internal->mutex);
    return SetCertificate(context.internal->identity, std::move(der), SslFunctionCtxUseCertificate, true);
}

int APS5_VABI SSL_CTX_use_certificate_ASN1_nid_postfix(SslCtxObject* ctx, int len, const unsigned char* d) {
    auto& context = RequireCtx(ctx, __func__);
    if (len < 0 || (len && !d)) throw std::invalid_argument("SSL_CTX_use_certificate_ASN1: invalid buffer");
    std::lock_guard lock(context.internal->mutex);
    return SetCertificate(context.internal->identity, Bytes(d, static_cast<std::size_t>(len)), SslFunctionCtxUseCertificateAsn1, true);
}

int APS5_VABI SSL_CTX_use_certificate_file_nid_postfix(SslCtxObject* ctx, const char* file, int type) {
    auto& context = RequireCtx(ctx, __func__);
    if (!file) throw std::invalid_argument("SSL_CTX_use_certificate_file: null file name");
    if (type != FiletypePem && type != FiletypeAsn1) throw std::invalid_argument("SSL_CTX_use_certificate_file: unknown file type");
    Der contents;
    if (!ReadGuestFile(file, contents)) {
        PushError(ErrLibSsl, SslFunctionCtxUseCertificateFile, ErrReasonSysLib);
        return 0;
    }
    Der der = contents;
    if (type == FiletypePem) {
        bool malformed = false;
        auto objects = PemObjects(contents.data(), contents.size(), "CERTIFICATE", malformed);
        if (objects.empty()) {
            PushError(ErrLibSsl, SslFunctionCtxUseCertificateFile, ErrReasonPemLib);
            return 0;
        }
        der = std::move(objects.front());
    }
    std::lock_guard lock(context.internal->mutex);
    return SetCertificate(context.internal->identity, std::move(der), SslFunctionCtxUseCertificateFile, true);
}

int APS5_VABI SSL_CTX_use_certificate_chain_file_nid_postfix(SslCtxObject* ctx, const char* file) {
    auto& context = RequireCtx(ctx, __func__);
    if (!file) throw std::invalid_argument("SSL_CTX_use_certificate_chain_file: null file name");
    Der contents;
    if (!ReadGuestFile(file, contents)) {
        PushError(ErrLibSsl, SslFunctionCtxUseCertificateChainFile, ErrReasonSysLib);
        return 0;
    }
    bool malformed = false;
    auto objects = PemObjects(contents.data(), contents.size(), "CERTIFICATE", malformed);
    if (objects.empty() || malformed) {
        PushError(ErrLibSsl, SslFunctionCtxUseCertificateChainFile, ErrReasonPemLib);
        return 0;
    }
    for (const auto& object : objects) {
        if (!ValidCertificate(object)) {
            PushError(ErrLibSsl, SslFunctionCtxUseCertificateChainFile, ErrReasonPemLib);
            return 0;
        }
    }
    std::lock_guard lock(context.internal->mutex);
    if (!context.internal->identity->chain.empty())
        throw std::invalid_argument("SSL_CTX_use_certificate_chain_file: whether the file's chain replaces or extends the extra chain certificates is not documented");
    auto updated = std::make_shared<Identity>(*context.internal->identity);
    updated->certificate = objects.front();
    updated->chain.assign(objects.begin() + 1, objects.end());
    context.internal->identity = std::move(updated);
    return 1;
}

int APS5_VABI SSL_CTX_use_PrivateKey_ASN1_nid_postfix(int pk, SslCtxObject* ctx, const unsigned char* d, std::int64_t len) {
    auto& context = RequireCtx(ctx, __func__);
    if (len < 0 || (len && !d)) throw std::invalid_argument("SSL_CTX_use_PrivateKey_ASN1: invalid buffer");
    if (pk != PkeyRsa && pk != PkeyEc) throw std::invalid_argument("SSL_CTX_use_PrivateKey_ASN1: only EVP_PKEY_RSA and EVP_PKEY_EC keys are supported");
    std::lock_guard lock(context.internal->mutex);
    return SetKey(context.internal->identity, Bytes(d, static_cast<std::size_t>(len)), pk, SslFunctionCtxUsePrivateKeyAsn1);
}

int APS5_VABI SSL_CTX_use_PrivateKey_file_nid_postfix(SslCtxObject* ctx, const char* file, int type) {
    auto& context = RequireCtx(ctx, __func__);
    if (!file) throw std::invalid_argument("SSL_CTX_use_PrivateKey_file: null file name");
    if (type != FiletypePem && type != FiletypeAsn1) throw std::invalid_argument("SSL_CTX_use_PrivateKey_file: unknown file type");
    Der contents;
    if (!ReadGuestFile(file, contents)) {
        PushError(ErrLibSsl, SslFunctionCtxUsePrivateKeyFile, ErrReasonSysLib);
        return 0;
    }
    Der der = contents;
    if (type == FiletypePem) {
        const std::string_view text(reinterpret_cast<const char*>(contents.data()), contents.size());
        if (text.find("-----BEGIN ENCRYPTED PRIVATE KEY-----") != std::string_view::npos || text.find("Proc-Type: 4,ENCRYPTED") != std::string_view::npos)
            throw std::invalid_argument("SSL_CTX_use_PrivateKey_file: encrypted keys need the PEM password callback, which this build does not call");
        std::size_t first = std::string_view::npos;
        const char* label = nullptr;
        for (const char* candidate : {"PRIVATE KEY", "RSA PRIVATE KEY", "EC PRIVATE KEY"}) {
            const auto at = text.find(std::string("-----BEGIN ") + candidate + "-----");
            if (at < first) {
                first = at;
                label = candidate;
            }
        }
        bool malformed = false;
        auto objects = label ? PemObjects(contents.data() + first, contents.size() - first, label, malformed) : std::vector<Der>{};
        if (objects.empty()) {
            PushError(ErrLibSsl, SslFunctionCtxUsePrivateKeyFile, ErrReasonPemLib);
            return 0;
        }
        der = std::move(objects.front());
    }
    std::lock_guard lock(context.internal->mutex);
    return SetKey(context.internal->identity, std::move(der), 0, SslFunctionCtxUsePrivateKeyFile);
}

int APS5_VABI SSL_CTX_check_private_key_nid_postfix(const SslCtxObject* ctx) {
    auto& context = RequireCtx(const_cast<SslCtxObject*>(ctx), __func__);
    std::shared_ptr<const Identity> identity;
    {
        std::lock_guard lock(context.internal->mutex);
        identity = context.internal->identity;
    }
    return CheckKey(*identity, SslFunctionCtxCheckPrivateKey);
}

int APS5_VABI SSL_CTX_load_verify_mem_nid_postfix(SslCtxObject* ctx, void* buf, int len) {
    auto& context = RequireCtx(ctx, __func__);
    if (!buf || len <= 0) throw std::invalid_argument("SSL_CTX_load_verify_mem: invalid buffer");
    bool malformed = false;
    auto objects = PemObjects(static_cast<const unsigned char*>(buf), static_cast<std::size_t>(len), "CERTIFICATE", malformed);
    if (objects.empty() || malformed) return 0;
    std::vector<X509Object*> certificates;
    const auto release = [&] {
        for (auto* certificate : certificates) X509_free_nid_postfix(certificate);
    };
    for (const auto& object : objects) {
        auto* certificate = ValidCertificate(object) ? X509FromDer(object.data(), object.size()) : nullptr;
        if (!certificate) {
            release();
            return 0;
        }
        certificates.push_back(certificate);
    }
    try {
        const auto store = AcquireStore(context, __func__);
        for (auto* certificate : certificates) AddTrustedCertificate(store.get(), certificate, __func__);
    } catch (...) {
        release();
        throw;
    }
    release();
    return 1;
}

int APS5_VABI SSL_CTX_load_verify_locations_nid_postfix(SslCtxObject* ctx, const char* CAfile, const char* CApath) {
    RequireCtx(ctx, __func__);
    if (CApath) throw std::invalid_argument("SSL_CTX_load_verify_locations: hashed CA directories are not supported");
    if (!CAfile) return 0;
    Der contents;
    if (!ReadGuestFile(CAfile, contents) || contents.empty()) return 0;
    return SSL_CTX_load_verify_mem_nid_postfix(ctx, contents.data(), static_cast<int>(contents.size()));
}

int APS5_VABI SSL_CTX_add_client_CA_nid_postfix(SslCtxObject* ctx, X509Object* x) {
    auto& context = RequireCtx(ctx, __func__);
    auto der = CertificateDer(x, __func__);
    std::lock_guard lock(context.internal->mutex);
    context.internal->clientAuthorities.push_back(std::move(der));
    return 1;
}

int LibreSslSslCtxAdd0ChainCert_nid_no_patch(SslCtxObject* ctx, X509Object* x) {
    return static_cast<int>(SSL_CTX_ctrl_nid_postfix(ctx, CtrlExtraChainCert, 0, x));
}

int LibreSslSslCtxClearChainCerts_nid_no_patch(SslCtxObject* ctx) {
    return static_cast<int>(SSL_CTX_ctrl_nid_postfix(ctx, CtrlClearExtraChainCerts, 0, nullptr));
}

SslObject* APS5_VABI SSL_new_nid_postfix(SslCtxObject* ctx) {
    auto& context = RequireCtx(ctx, __func__);
    auto* ssl = new SslObject{};
    ssl->internal = new SslState();
    ssl->method = context.method;
    ssl->references = 1;
    ssl->ctx = ctx;
    ssl->initialCtx = ctx;
    ssl->verifyResult = X509VOk;
    ssl->tlsextStatusType = -1;
    ssl->server = Info(context.method).server ? 1 : 0;
    ssl->param = NewVerifyParam();
    context.references = context.internal->references.fetch_add(1) + 1;
    CopyFromCtx(*ssl, context);
    return ssl;
}

void APS5_VABI SSL_free_nid_postfix(SslObject* ssl) {
    if (!ssl) return;
    const int left = ssl->internal->references.fetch_sub(1) - 1;
    ssl->references = left;
    if (left > 0) return;
    FreeBios(*ssl);
    ReleaseSession(ssl->session);
    auto* ctx = ssl->ctx;
    delete ssl->internal;
    FreeVerifyParam(static_cast<VerifyParam*>(ssl->param));
    delete[] ssl->tlsextHostname;
    delete ssl;
    Unref(ctx);
}

int LibreSslSslUpRef_nid_no_patch(SslObject* ssl) {
    auto& object = RequireSsl(ssl, "SSL_up_ref");
    object.references = object.internal->references.fetch_add(1) + 1;
    return 1;
}

void APS5_VABI SSL_set_bio_nid_postfix(SslObject* s, Bio* rbio, Bio* wbio) {
    auto& ssl = RequireSsl(s, __func__);
    if (ssl.rbio && ssl.rbio != rbio && ssl.rbio != wbio) BIO_free_nid_postfix(ssl.rbio);
    if (ssl.wbio && ssl.wbio != ssl.rbio && ssl.wbio != wbio && ssl.wbio != rbio) BIO_free_nid_postfix(ssl.wbio);
    ssl.rbio = rbio;
    ssl.wbio = wbio;
}

Bio* APS5_VABI SSL_get_rbio_nid_postfix(const SslObject* s) {
    return RequireSsl(const_cast<SslObject*>(s), __func__).rbio;
}

Bio* APS5_VABI SSL_get_wbio_nid_postfix(const SslObject* s) {
    return RequireSsl(const_cast<SslObject*>(s), __func__).wbio;
}

int APS5_VABI SSL_set_fd_nid_postfix(SslObject* s, int fd) {
    auto& ssl = RequireSsl(s, __func__);
    auto* bio = BIO_new_socket_nid_postfix(fd, 0);
    if (!bio) return 0;
    SSL_set_bio_nid_postfix(&ssl, bio, bio);
    return 1;
}

int APS5_VABI SSL_get_fd_nid_postfix(const SslObject* s) {
    auto& ssl = RequireSsl(const_cast<SslObject*>(s), __func__);
    if (!ssl.rbio || BioMethodOf(*ssl.rbio) != &SocketMethod) return -1;
    return static_cast<SocketBio*>(BioState(*ssl.rbio))->descriptor;
}

SslCtxObject* APS5_VABI SSL_get_SSL_CTX_nid_postfix(const SslObject* ssl) {
    return RequireSsl(const_cast<SslObject*>(ssl), __func__).ctx;
}

SslCtxObject* APS5_VABI SSL_set_SSL_CTX_nid_postfix(SslObject* ssl, SslCtxObject* ctx) {
    auto& object = RequireSsl(ssl, __func__);
    if (!ctx) ctx = object.initialCtx;
    if (ctx == object.ctx) return ctx;
    auto& context = RequireCtx(ctx, __func__);
    context.references = context.internal->references.fetch_add(1) + 1;
    {
        std::lock_guard lock(context.internal->mutex);
        object.internal->identity = context.internal->identity;
    }
    auto* previous = object.ctx;
    object.ctx = ctx;
    Unref(previous);
    return ctx;
}

void APS5_VABI SSL_set_connect_state_nid_postfix(SslObject* s) {
    auto& ssl = RequireSsl(s, __func__);
    if (ssl.internal->configured) throw std::logic_error("SSL_set_connect_state: changing the state after the handshake started is not supported");
    ssl.internal->role = 1;
    ssl.server = 0;
}

int APS5_VABI SSL_connect_nid_postfix(SslObject* ssl) {
    auto& object = RequireSsl(ssl, __func__);
    if (!Info(object.method).client) throw std::logic_error("SSL_connect: the SSL_METHOD has no client side");
    if (object.internal->role == 2) throw std::logic_error("SSL_connect: the connection is in server state");
    object.internal->role = 1;
    object.server = 0;
    return Handshake(object, __func__);
}

int APS5_VABI SSL_accept_nid_postfix(SslObject* ssl) {
    auto& object = RequireSsl(ssl, __func__);
    if (!Info(object.method).server) throw std::logic_error("SSL_accept: the SSL_METHOD has no server side");
    if (object.internal->role == 1) throw std::logic_error("SSL_accept: the connection is in client state");
    object.internal->role = 2;
    object.server = 1;
    return Handshake(object, __func__);
}

int APS5_VABI SSL_read_nid_postfix(SslObject* ssl, void* buf, int num) {
    auto& object = RequireSsl(ssl, __func__);
    if (num <= 0 || !buf) throw std::invalid_argument("SSL_read: a zero or negative length or a null buffer is handled differently between LibreSSL versions");
    auto& state = *object.internal;
    if (!state.peek.empty()) {
        std::size_t count = std::min<std::size_t>(state.peek.size(), static_cast<std::size_t>(num));
        std::memcpy(buf, state.peek.data(), count);
        state.peek.erase(state.peek.begin(), state.peek.begin() + static_cast<std::ptrdiff_t>(count));
        state.want = StateNothing;
        const std::size_t available = mbedtls_ssl_get_bytes_avail(&state.ssl);
        if (state.peek.empty() && count < static_cast<std::size_t>(num) && available) {
            const int more = ReadRecords(object, static_cast<unsigned char*>(buf) + count,
                static_cast<int>(std::min<std::size_t>(available, static_cast<std::size_t>(num) - count)), __func__);
            if (more > 0) count += static_cast<std::size_t>(more);
        }
        return static_cast<int>(count);
    }
    return ReadRecords(object, static_cast<unsigned char*>(buf), num, __func__);
}

int APS5_VABI SSL_peek_nid_postfix(SslObject* ssl, void* buf, int num) {
    auto& object = RequireSsl(ssl, __func__);
    if (num <= 0 || !buf) throw std::invalid_argument("SSL_peek: a zero or negative length or a null buffer is handled differently between LibreSSL versions");
    auto& state = *object.internal;
    if (state.peek.size() < static_cast<std::size_t>(num) && (state.peek.empty() || mbedtls_ssl_get_bytes_avail(&state.ssl))) {
        const std::size_t wanted = static_cast<std::size_t>(num) - state.peek.size();
        std::vector<unsigned char> chunk(state.peek.empty() ? wanted : std::min(wanted, mbedtls_ssl_get_bytes_avail(&state.ssl)));
        const int result = ReadRecords(object, chunk.data(), static_cast<int>(chunk.size()), __func__);
        if (result <= 0 && state.peek.empty()) return result;
        if (result > 0) state.peek.insert(state.peek.end(), chunk.begin(), chunk.begin() + result);
    }
    const std::size_t count = std::min<std::size_t>(state.peek.size(), static_cast<std::size_t>(num));
    std::memcpy(buf, state.peek.data(), count);
    return static_cast<int>(count);
}

int APS5_VABI SSL_write_nid_postfix(SslObject* ssl, const void* buf, int num) {
    auto& object = RequireSsl(ssl, __func__);
    if (num <= 0 || !buf) throw std::invalid_argument("SSL_write: a zero or negative length or a null buffer is handled differently between LibreSSL versions");
    auto& state = *object.internal;
    if (state.role == 0) throw std::logic_error("SSL_write: the connection has no client or server state yet");
    if (state.shutdown & SentShutdown) {
        PushError(ErrLibSsl, SslFunctionSsl3WriteBytes, SslReasonProtocolIsShutdown);
        return -1;
    }
    const int handshake = Handshake(object, __func__);
    if (handshake <= 0) return handshake;
    const auto* data = static_cast<const unsigned char*>(buf);
    const bool partial = (state.mode & ModeEnablePartialWrite) != 0;
    if (state.writeProgress >= static_cast<std::size_t>(num)) state.writeProgress = 0;
    while (state.writeProgress < static_cast<std::size_t>(num)) {
        state.want = StateNothing;
        state.syscallFailure = false;
        const int result = mbedtls_ssl_write(&state.ssl, data + state.writeProgress, static_cast<std::size_t>(num) - state.writeProgress);
        if (state.pending) {
            auto pending = state.pending;
            state.pending = nullptr;
            std::rethrow_exception(pending);
        }
        if (result > 0) {
            state.writeProgress += static_cast<std::size_t>(result);
            if (partial) break;
            continue;
        }
        if (result == MBEDTLS_ERR_SSL_WANT_WRITE || result == MBEDTLS_ERR_SSL_WANT_READ) {
            state.want = result == MBEDTLS_ERR_SSL_WANT_WRITE ? StateWriting : StateReading;
            if (partial && state.writeProgress) break;
            return -1;
        }
        state.writeProgress = 0;
        if (result == MBEDTLS_ERR_NET_SEND_FAILED || result == MBEDTLS_ERR_NET_RECV_FAILED) return -1;
        PushHandshakeError(object, result, SslFunctionSsl3WriteBytes);
        return -1;
    }
    const int written = static_cast<int>(state.writeProgress);
    state.writeProgress = 0;
    return written;
}

int APS5_VABI SSL_shutdown_nid_postfix(SslObject* s) {
    auto& ssl = RequireSsl(s, __func__);
    auto& state = *ssl.internal;
    if (state.quietShutdown) {
        state.shutdown = SentShutdown | ReceivedShutdown;
        return 1;
    }
    if (!state.handshaken) throw std::logic_error("SSL_shutdown: shutting down a connection whose handshake did not finish is handled in a way the manuals do not document");
    if (!(state.shutdown & SentShutdown)) {
        state.want = StateNothing;
        const int result = mbedtls_ssl_close_notify(&state.ssl);
        if (state.pending) {
            auto pending = state.pending;
            state.pending = nullptr;
            std::rethrow_exception(pending);
        }
        if (result == MBEDTLS_ERR_SSL_WANT_WRITE || result == MBEDTLS_ERR_SSL_WANT_READ) {
            state.want = result == MBEDTLS_ERR_SSL_WANT_WRITE ? StateWriting : StateReading;
            return -1;
        }
        state.shutdown |= SentShutdown;
        if (result != 0) return -1;
        return (state.shutdown & ReceivedShutdown) ? 1 : 0;
    }
    if (state.shutdown & ReceivedShutdown) return 1;
    unsigned char scratch[512];
    for (;;) {
        state.want = StateNothing;
        const int result = mbedtls_ssl_read(&state.ssl, scratch, sizeof(scratch));
        if (state.pending) {
            auto pending = state.pending;
            state.pending = nullptr;
            std::rethrow_exception(pending);
        }
        if (result > 0) continue;
        if (result == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            state.shutdown |= ReceivedShutdown;
            return 1;
        }
        if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE) {
            state.want = result == MBEDTLS_ERR_SSL_WANT_READ ? StateReading : StateWriting;
            return -1;
        }
        return -1;
    }
}

int APS5_VABI SSL_get_shutdown_nid_postfix(const SslObject* ssl) {
    return RequireSsl(const_cast<SslObject*>(ssl), __func__).internal->shutdown;
}

int APS5_VABI SSL_want_nid_postfix(const SslObject* s) {
    return RequireSsl(const_cast<SslObject*>(s), __func__).internal->want;
}

int APS5_VABI SSL_pending_nid_postfix(const SslObject* s) {
    auto& ssl = RequireSsl(const_cast<SslObject*>(s), __func__);
    auto& state = *ssl.internal;
    const std::size_t buffered = state.configured ? mbedtls_ssl_get_bytes_avail(&state.ssl) : 0;
    return static_cast<int>(std::min<std::size_t>(buffered + state.peek.size(), INT_MAX));
}

int APS5_VABI SSL_get_error_nid_postfix(const SslObject* s, int ret_code) {
    auto& ssl = RequireSsl(const_cast<SslObject*>(s), __func__);
    auto& state = *ssl.internal;
    if (ret_code > 0) return ErrorNone;
    const auto error = PeekLastError();
    if (error) return ((error >> 24) & 0xff) == ErrReasonSysLib ? ErrorSyscall : ErrorSsl;
    if (state.want == StateReading) return ErrorWantRead;
    if (state.want == StateWriting) return ErrorWantWrite;
    if (state.want == StateX509Lookup) return ErrorWantX509Lookup;
    if (ret_code == 0 && (state.shutdown & ReceivedShutdown)) return ErrorZeroReturn;
    return ErrorSyscall;
}

std::int64_t APS5_VABI SSL_ctrl_nid_postfix(SslObject* ssl, int cmd, std::int64_t larg, void* parg) {
    auto& object = RequireSsl(ssl, __func__);
    auto& state = *object.internal;
    bool handled = false;
    const auto common = CommonCtrl(state.options, state.mode, cmd, larg, handled);
    if (handled) return common;
    switch (cmd) {
    case CtrlSetTlsextHostname: {
        if (larg != TlsextNametypeHostName) throw std::invalid_argument("SSL_set_tlsext_host_name: unknown name type");
        if (state.configured) throw std::logic_error("SSL_set_tlsext_host_name: setting the name after the handshake started is not supported");
        delete[] object.tlsextHostname;
        object.tlsextHostname = nullptr;
        state.hostname.clear();
        if (!parg) return 1;
        const std::string name(static_cast<const char*>(parg));
        if (name.empty() || name.size() > 255) {
            PushError(ErrLibSsl, SslFunctionSsl3Ctrl, SslReasonSsl3ExtInvalidServername);
            return 0;
        }
        state.hostname = name;
        object.tlsextHostname = new char[name.size() + 1];
        std::memcpy(object.tlsextHostname, name.c_str(), name.size() + 1);
        return 1;
    }
    case CtrlGetSessionReused:
        if (state.role == 2) throw std::invalid_argument("SSL_session_reused: Mbed TLS does not report whether a server resumed a session");
        return state.reused ? 1 : 0;
    case CtrlSetMinProtoVersion:
    case CtrlSetMaxProtoVersion: {
        const int version = static_cast<int>(larg);
        if (version != 0 && version != Tls1Version && version != Tls1_1Version && version != Tls1_2Version) return 0;
        (cmd == CtrlSetMinProtoVersion ? state.minVersion : state.maxVersion) = version;
        return 1;
    }
    case CtrlSetGroupsList:
        if (!parg) throw std::invalid_argument("SSL_set1_groups_list: null list");
        return ParseGroups(static_cast<const char*>(parg), state.groups);
    default:
        throw std::invalid_argument("SSL_ctrl: command " + std::to_string(cmd) + " is not supported");
    }
}

const char* APS5_VABI SSL_get_servername_nid_postfix(const SslObject* s, int type) {
    auto& ssl = RequireSsl(const_cast<SslObject*>(s), __func__);
    if (type != TlsextNametypeHostName) return nullptr;
    auto& state = *ssl.internal;
    if (state.role == 2) return state.servername.empty() ? nullptr : state.servername.c_str();
    return ssl.tlsextHostname;
}

void APS5_VABI SSL_set_verify_nid_postfix(SslObject* s, int mode, VerifyCallback callback) {
    auto& ssl = RequireSsl(s, __func__);
    if (mode & VerifyClientOnce) throw std::invalid_argument("SSL_set_verify: SSL_VERIFY_CLIENT_ONCE needs renegotiation, which this build does not do");
    if (ssl.internal->configured) throw std::logic_error("SSL_set_verify: changing the verify mode after the handshake started is not supported");
    ssl.verifyMode = mode;
    ssl.internal->verifyCallback = callback;
}

std::int64_t APS5_VABI SSL_get_verify_result_nid_postfix(const SslObject* ssl) {
    return RequireSsl(const_cast<SslObject*>(ssl), __func__).verifyResult;
}

X509Object* APS5_VABI SSL_get_peer_certificate_nid_postfix(const SslObject* s) {
    auto& ssl = RequireSsl(const_cast<SslObject*>(s), __func__);
    if (!ssl.session || !ssl.session->peer) return nullptr;
    LibreSslX509UpRef_nid_no_patch(ssl.session->peer);
    return ssl.session->peer;
}

Stack* APS5_VABI SSL_get_peer_cert_chain_nid_postfix(const SslObject* s) {
    auto& ssl = RequireSsl(const_cast<SslObject*>(s), __func__);
    if (!ssl.session || !ssl.session->internal->peer || !ssl.session->peer) return nullptr;
    return &ssl.session->internal->peer->stack;
}

X509Object* APS5_VABI SSL_get_certificate_nid_postfix(const SslObject* ssl) {
    auto& object = RequireSsl(const_cast<SslObject*>(ssl), __func__);
    auto& state = *object.internal;
    if (state.identity->certificate.empty()) return nullptr;
    if (!state.ownCertificate) state.ownCertificate = X509FromDer(state.identity->certificate.data(), state.identity->certificate.size());
    return state.ownCertificate;
}

int APS5_VABI SSL_check_private_key_nid_postfix(const SslObject* ctx) {
    auto& ssl = RequireSsl(const_cast<SslObject*>(ctx), __func__);
    return CheckKey(*ssl.internal->identity, SslFunctionCheckPrivateKey);
}

int APS5_VABI SSL_use_certificate_ASN1_nid_postfix(SslObject* ssl, const unsigned char* d, int len) {
    auto& object = RequireSsl(ssl, __func__);
    if (len < 0 || (len && !d)) throw std::invalid_argument("SSL_use_certificate_ASN1: invalid buffer");
    if (object.internal->configured) throw std::logic_error("SSL_use_certificate_ASN1: changing the certificate after the handshake started is not supported");
    if (object.internal->ownCertificate) {
        X509_free_nid_postfix(object.internal->ownCertificate);
        object.internal->ownCertificate = nullptr;
    }
    return SetCertificate(object.internal->identity, Bytes(d, static_cast<std::size_t>(len)), SslFunctionUseCertificateAsn1, true);
}

int APS5_VABI SSL_use_PrivateKey_ASN1_nid_postfix(int pk, SslObject* ssl, const unsigned char* d, std::int64_t len) {
    auto& object = RequireSsl(ssl, __func__);
    if (len < 0 || (len && !d)) throw std::invalid_argument("SSL_use_PrivateKey_ASN1: invalid buffer");
    if (pk != PkeyRsa && pk != PkeyEc) throw std::invalid_argument("SSL_use_PrivateKey_ASN1: only EVP_PKEY_RSA and EVP_PKEY_EC keys are supported");
    if (object.internal->configured) throw std::logic_error("SSL_use_PrivateKey_ASN1: changing the key after the handshake started is not supported");
    return SetKey(object.internal->identity, Bytes(d, static_cast<std::size_t>(len)), pk, SslFunctionUsePrivateKeyAsn1);
}

int APS5_VABI SSL_version_nid_postfix(const SslObject* ssl) {
    auto& object = RequireSsl(const_cast<SslObject*>(ssl), __func__);
    if (!object.internal->handshaken) throw std::logic_error("SSL_version: the value before a connection is established is not documented");
    return object.version;
}

const SslCipherObject* APS5_VABI SSL_get_current_cipher_nid_postfix(const SslObject* s) {
    auto& ssl = RequireSsl(const_cast<SslObject*>(s), __func__);
    if (!ssl.internal->handshaken || !ssl.session) return nullptr;
    return ssl.session->cipher;
}

const char* APS5_VABI SSL_CIPHER_get_name_nid_postfix(const SslCipherObject* c) {
    return c ? c->name : "(NONE)";
}

int APS5_VABI SSL_CIPHER_get_bits_nid_postfix(const SslCipherObject* c, int* alg_bits) {
    if (!c) return 0;
    if (alg_bits) *alg_bits = c->algBits;
    return c->strengthBits;
}

const char* APS5_VABI SSL_CIPHER_get_version_nid_postfix(const SslCipherObject* c) {
    return c ? "TLSv1/SSLv3" : "(NONE)";
}

void APS5_VABI SSL_get0_alpn_selected_nid_postfix(const SslObject* ssl, const unsigned char** data, unsigned int* len) {
    auto& object = RequireSsl(const_cast<SslObject*>(ssl), __func__);
    if (!data || !len) throw std::invalid_argument("SSL_get0_alpn_selected: null output");
    const char* selected = object.internal->configured ? mbedtls_ssl_get_alpn_protocol(&object.internal->ssl) : nullptr;
    *data = reinterpret_cast<const unsigned char*>(selected);
    *len = selected ? static_cast<unsigned int>(std::strlen(selected)) : 0;
}

int APS5_VABI SSL_select_next_proto_nid_postfix(unsigned char** out, unsigned char* outlen, const unsigned char* server, unsigned int server_len,
    const unsigned char* client, unsigned int client_len) {
    if (!out || !outlen) throw std::invalid_argument("SSL_select_next_proto: null output");
    const auto valid = [](const unsigned char* list, unsigned int length) {
        if (!list && length) return false;
        for (unsigned int index = 0; index < length;) {
            if (list[index] == 0 || index + 1 + list[index] > length) return false;
            index += list[index] + 1u;
        }
        return true;
    };
    if (!valid(server, server_len) || !valid(client, client_len) || client_len == 0)
        throw std::invalid_argument("SSL_select_next_proto: malformed or empty protocol lists are handled in a way the manual does not document");
    for (unsigned int i = 0; i < server_len; i += server[i] + 1u) {
        for (unsigned int j = 0; j < client_len; j += client[j] + 1u) {
            if (server[i] == client[j] && std::memcmp(server + i + 1, client + j + 1, server[i]) == 0) {
                *out = const_cast<unsigned char*>(server + i + 1);
                *outlen = server[i];
                return NpnNegotiated;
            }
        }
    }
    *out = const_cast<unsigned char*>(client + 1);
    *outlen = client[0];
    return NpnNoOverlap;
}

SslSessionObject* APS5_VABI SSL_get_session_nid_postfix(const SslObject* ssl) {
    return RequireSsl(const_cast<SslObject*>(ssl), __func__).session;
}

SslSessionObject* APS5_VABI SSL_get1_session_nid_postfix(SslObject* ssl) {
    auto* session = RequireSsl(ssl, __func__).session;
    if (session) session->references = session->internal->references.fetch_add(1) + 1;
    return session;
}

int APS5_VABI SSL_set_session_nid_postfix(SslObject* to, SslSessionObject* session) {
    auto& ssl = RequireSsl(to, __func__);
    if (ssl.internal->configured) throw std::logic_error("SSL_set_session: setting a session after the handshake started is not supported");
    if (session) session->references = session->internal->references.fetch_add(1) + 1;
    ReleaseSession(ssl.session);
    ssl.session = session;
    return 1;
}

void APS5_VABI SSL_SESSION_free_nid_postfix(SslSessionObject* ses) {
    ReleaseSession(ses);
}

int LibreSslSslSessionUpRef_nid_no_patch(SslSessionObject* session) {
    if (!session) throw std::invalid_argument("SSL_SESSION_up_ref: null session");
    session->references = session->internal->references.fetch_add(1) + 1;
    return 1;
}

std::size_t LibreSslSslGetClientRandom_nid_no_patch(const SslObject* ssl, unsigned char* out, std::size_t outlen) {
    auto& object = RequireSsl(const_cast<SslObject*>(ssl), "SSL_get_client_random");
    if (!object.internal->haveRandoms) throw std::logic_error("SSL_get_client_random: the client random is only known after the key exchange");
    if (outlen == 0) return sizeof(object.internal->clientRandom);
    if (!out) throw std::invalid_argument("SSL_get_client_random: null output");
    const std::size_t count = std::min(outlen, sizeof(object.internal->clientRandom));
    std::memcpy(out, object.internal->clientRandom, count);
    return count;
}

Store* APS5_VABI SSL_CTX_get_cert_store_nid_postfix(const SslCtxObject* ctx) {
    auto& context = RequireCtx(const_cast<SslCtxObject*>(ctx), __func__);
    std::lock_guard lock(context.internal->mutex);
    return static_cast<Store*>(context.certStore);
}

void APS5_VABI SSL_CTX_set_cert_store_nid_postfix(SslCtxObject* ctx, Store* store) {
    auto& context = RequireCtx(ctx, __func__);
    Store* previous = nullptr;
    {
        std::lock_guard lock(context.internal->mutex);
        previous = static_cast<Store*>(context.certStore);
        if (previous == store) return;
        context.certStore = store;
    }
    X509_STORE_free_nid_postfix(previous);
}

void APS5_VABI SSL_CTX_set_cert_verify_callback_nid_postfix(SslCtxObject* ctx, CertVerifyCallback cb, void* arg) {
    auto& context = RequireCtx(ctx, __func__);
    std::lock_guard lock(context.internal->mutex);
    context.internal->certVerifyCallback = cb;
    context.internal->certVerifyArgument = arg;
}

void APS5_VABI SSL_CTX_set_client_cert_cb_nid_postfix(SslCtxObject* ctx, ClientCertCallback client_cert_cb) {
    auto& context = RequireCtx(ctx, __func__);
    std::lock_guard lock(context.internal->mutex);
    context.internal->clientCertCallback = client_cert_cb;
}

int APS5_VABI SSL_CTX_use_PrivateKey_nid_postfix(SslCtxObject* ctx, EvpPkey* pkey) {
    auto& context = RequireCtx(ctx, __func__);
    if (!pkey) throw std::invalid_argument("SSL_CTX_use_PrivateKey: null key");
    auto der = EncodePrivateKey(pkey, __func__);
    std::lock_guard lock(context.internal->mutex);
    return SetKey(context.internal->identity, std::move(der), 0, SslFunctionCtxUsePrivateKey);
}

EvpPkey* APS5_VABI SSL_get_privatekey_nid_postfix(const SslObject* ssl) {
    auto& object = RequireSsl(const_cast<SslObject*>(ssl), __func__);
    auto& state = *object.internal;
    if (state.identity->key.empty()) return nullptr;
    if (state.privateKeyIdentity != state.identity) {
        auto* pkey = DecodePrivateKey(state.identity->key.data(), state.identity->key.size(), __func__);
        if (state.privateKey) EVP_PKEY_free_nid_postfix(state.privateKey);
        state.privateKey = pkey;
        state.privateKeyIdentity = state.identity;
    }
    return state.privateKey;
}

VerifyParam* APS5_VABI SSL_get0_param_nid_postfix(SslObject* ssl) {
    return static_cast<VerifyParam*>(RequireSsl(ssl, __func__).param);
}

int APS5_VABI SSL_get_ex_data_X509_STORE_CTX_idx_nid_postfix() {
    return StoreCtxSslIndex;
}

int APS5_VABI SSL_get_ex_new_index_nid_postfix(std::int64_t, void*, void* new_func, void* dup_func, void* free_func) {
    if (new_func || dup_func || free_func) throw std::invalid_argument("SSL_get_ex_new_index: ex_data callbacks are not supported");
    auto& indices = Indices();
    std::lock_guard lock(indices.mutex);
    return indices.nextSsl++;
}

int APS5_VABI SSL_set_ex_data_nid_postfix(SslObject* ssl, int idx, void* data) {
    auto& object = RequireSsl(ssl, __func__);
    if (idx < 0) throw std::invalid_argument("SSL_set_ex_data: negative index");
    object.internal->exData[idx] = data;
    return 1;
}

void* APS5_VABI SSL_get_ex_data_nid_postfix(const SslObject* ssl, int idx) {
    auto& object = RequireSsl(const_cast<SslObject*>(ssl), __func__);
    const auto found = object.internal->exData.find(idx);
    return found == object.internal->exData.end() ? nullptr : found->second;
}

const char* APS5_VABI SSL_alert_desc_string_long_nid_postfix(int value) {
    if (value < 0 || value > 0xffff) throw std::invalid_argument("SSL_alert_desc_string_long: the value is no alert");
    switch (value & 0xff) {
    case 0: return "close notify";
    case 10: return "unexpected message";
    case 20: return "bad record mac";
    case 21: return "decryption failed";
    case 22: return "record overflow";
    case 30: return "decompression failure";
    case 40: return "handshake failure";
    case 41: return "no certificate";
    case 42: return "bad certificate";
    case 43: return "unsupported certificate";
    case 44: return "certificate revoked";
    case 45: return "certificate expired";
    case 46: return "certificate unknown";
    case 47: return "illegal parameter";
    case 48: return "unknown CA";
    case 49: return "access denied";
    case 50: return "decode error";
    case 51: return "decrypt error";
    case 60: return "export restriction";
    case 70: return "protocol version";
    case 71: return "insufficient security";
    case 80: return "internal error";
    case 90: return "user canceled";
    case 100: return "no renegotiation";
    case 115: return "unknown PSK identity";
    case 86:
    case 110:
    case 111:
    case 112:
    case 113:
    case 114:
        throw std::invalid_argument("SSL_alert_desc_string_long: the text for alert " + std::to_string(value & 0xff) + " is not documented");
    default: return "unknown";
    }
}

}
