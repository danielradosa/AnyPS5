#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/include/Keys.hpp"
#include "prx/libSceLibreSsl/include/Pem.hpp"
#include "prx/libSceLibreSsl/include/X509Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <mbedtls/asn1.h>
#include <mbedtls/base64.h>
#include <mbedtls/pk.h>
#include <mbedtls/platform_util.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct FileStream;

extern "C" {
LibreSsl::Bio* APS5_VABI BIO_new_fp_nid_postfix(FileStream* stream, int close_flag);
int APS5_VABI BIO_free_nid_postfix(LibreSsl::Bio* a);
int APS5_VABI BIO_write_nid_postfix(LibreSsl::Bio* b, const void* data, int len);
}

namespace LibreSsl {

namespace {

constexpr int ErrReasonBufLib = 7;
constexpr int PemFunctionReadBio = 109;
constexpr int PemFunctionWriteBio = 114;
constexpr int PemFunctionX509InfoReadBio = 116;
constexpr int PemReasonBadBase64Decode = 100;

bool IsCertificateLabel(std::string_view label) {
    return label == "CERTIFICATE" || label == "X509 CERTIFICATE" || label == "TRUSTED CERTIFICATE";
}

struct CipherInfo {
    const void* cipher;
    unsigned char iv[16];
};

struct X509Pkey {
    int version;
    void* encAlgor;
    Asn1String* encPkey;
    EvpPkey* decPkey;
    int keyLength;
    char* keyData;
    int keyFree;
    CipherInfo cipher;
    int references;
};

struct X509Info {
    X509Object* x509;
    X509Crl* crl;
    X509Pkey* xPkey;
    CipherInfo encCipher;
    int encLen;
    char* encData;
    int references;
};

static_assert(sizeof(CipherInfo) == 24 && sizeof(X509Pkey) == 88 && offsetof(X509Pkey, decPkey) == 24 && offsetof(X509Pkey, references) == 80);
static_assert(sizeof(X509Info) == 72 && offsetof(X509Info, encCipher) == 24 && offsetof(X509Info, encLen) == 48 && offsetof(X509Info, references) == 64);

std::mutex& InfoMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<const X509Info*, std::unique_ptr<X509Info>>& Infos() {
    static std::unordered_map<const X509Info*, std::unique_ptr<X509Info>> infos;
    return infos;
}

void DestroyInfo(std::unique_ptr<X509Info> info) {
    FreeX509(info->x509);
    FreeCrl(info->crl);
    if (info->xPkey) {
        FreePkey(info->xPkey->decPkey);
        delete info->xPkey;
    }
}

X509Info* NewInfo() {
    auto info = std::make_unique<X509Info>();
    *info = {};
    info->references = 1;
    auto* pointer = info.get();
    std::lock_guard lock(InfoMutex());
    Infos().emplace(pointer, std::move(info));
    return pointer;
}

void FreeInfo(X509Info* info) {
    if (!info) return;
    std::unique_ptr<X509Info> owned;
    {
        std::lock_guard lock(InfoMutex());
        const auto found = Infos().find(info);
        if (found == Infos().end()) throw std::invalid_argument("X509_INFO_free: the X509_INFO was not created by PEM_X509_INFO_read_bio");
        if (std::atomic_ref<int>(info->references).fetch_sub(1) > 1) return;
        owned = std::move(found->second);
        Infos().erase(found);
    }
    DestroyInfo(std::move(owned));
}

X509Object* CertificateFromDer(const std::vector<unsigned char>& der, bool trusted) {
    const std::size_t length = DerElementLength(der.data(), der.size());
    if (!length || length > der.size() || (length != der.size() && !trusted)) return nullptr;
    return X509FromDer(der.data(), length);
}

}

}

using namespace LibreSsl;

extern "C" {

X509Object* APS5_VABI PEM_read_bio_X509_AUX_nid_postfix(Bio* bp, X509Object** x, void*, void*) {
    if (x && *x) throw std::invalid_argument("PEM_read_bio_X509_AUX: reusing an existing X509 is not supported");
    PemObject object;
    if (!ReadPemFromBio(bp, object, IsCertificateLabel, __func__)) return nullptr;
    if (!object.headers.empty()) throw std::invalid_argument("PEM_read_bio_X509_AUX: encrypted or annotated certificates are not supported");
    auto* result = CertificateFromDer(object.der, object.label == "TRUSTED CERTIFICATE");
    if (result && x) *x = result;
    return result;
}

Stack* APS5_VABI PEM_X509_INFO_read_bio_nid_postfix(Bio* bp, Stack* sk, void*, void*) {
    constexpr const char* function = "PEM_X509_INFO_read_bio";
    if (sk && !IsHeapStack(sk)) throw std::invalid_argument("PEM_X509_INFO_read_bio: the stack was not created by this library");
    const auto bytes = BioReadAll(bp, function);
    const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    std::vector<X509Info*> created;
    const auto fail = [&](int reason) -> Stack* {
        for (auto* info : created) FreeInfo(info);
        PushError(ErrLibPem, reason == PemReasonBadBase64Decode ? PemFunctionReadBio : PemFunctionX509InfoReadBio, reason);
        return nullptr;
    };
    try {
        X509Info* current = nullptr;
        std::size_t offset = 0;
        const auto wanted = [](std::string_view label) {
            return IsCertificateLabel(label) || label == "X509 CRL" || label == "RSA PRIVATE KEY" || label == "EC PRIVATE KEY" || label == "DSA PRIVATE KEY";
        };
        while (true) {
            PemObject object;
            const auto status = NextPem(text, offset, object, wanted);
            if (status == PemStatus::NoStartLine) break;
            if (status == PemStatus::BadBase64) return fail(PemReasonBadBase64Decode);
            if (object.label == "DSA PRIVATE KEY") throw std::invalid_argument("PEM_X509_INFO_read_bio: DSA keys are not supported");
            const bool isCert = IsCertificateLabel(object.label);
            const bool isCrl = object.label == "X509 CRL";
            if (!object.headers.empty()) throw std::invalid_argument("PEM_X509_INFO_read_bio: encrypted PEM objects are not supported");
            if (!current || (isCert && current->x509) || (isCrl && current->crl) || (!isCert && !isCrl && current->xPkey)) {
                current = NewInfo();
                created.push_back(current);
            }
            if (isCert) {
                current->x509 = CertificateFromDer(object.der, object.label == "TRUSTED CERTIFICATE");
                if (!current->x509) return fail(ErrReasonAsn1Lib);
            } else if (isCrl) {
                if (DerElementLength(object.der.data(), object.der.size()) != object.der.size()) return fail(ErrReasonAsn1Lib);
                current->crl = NewCrl(object.der.data(), object.der.size());
                if (!current->crl) return fail(ErrReasonAsn1Lib);
            } else {
                EvpPkey* pkey = nullptr;
                if (ParsePrivateKey(object.der.data(), object.der.size(), nullptr, pkey, function) != KeyParse::Ok) return fail(ErrReasonAsn1Lib);
                if ((object.label == "RSA PRIVATE KEY") != (pkey->type == NidRsaEncryption)) {
                    FreePkey(pkey);
                    return fail(ErrReasonAsn1Lib);
                }
                current->xPkey = new X509Pkey{};
                current->xPkey->decPkey = pkey;
                current->xPkey->references = 1;
            }
        }
    } catch (...) {
        for (auto* info : created) FreeInfo(info);
        throw;
    }
    Stack* stack = sk ? sk : NewHeapStack();
    for (auto* info : created) HeapStackPush(stack, info);
    return stack;
}

void APS5_VABI X509_INFO_free_nid_postfix(X509Info* a) {
    FreeInfo(a);
}

int APS5_VABI PEM_write_bio_X509_nid_postfix(Bio* bp, X509Object* x) {
    if (!bp) throw std::invalid_argument("PEM_write_bio_X509: null BIO");
    if (!x) throw std::invalid_argument("PEM_write_bio_X509: null certificate");
    std::size_t size = 0;
    mbedtls_base64_encode(nullptr, 0, &size, x->crt.raw.p, x->crt.raw.len);
    std::vector<unsigned char> base64(size);
    if (mbedtls_base64_encode(base64.data(), base64.size(), &size, x->crt.raw.p, x->crt.raw.len) != 0) throw std::runtime_error("PEM_write_bio_X509: Mbed TLS failed");
    std::string text = "-----BEGIN CERTIFICATE-----\n";
    for (std::size_t index = 0; index < size; index += 64) {
        text.append(reinterpret_cast<const char*>(base64.data()) + index, size - index < 64 ? size - index : 64);
        text += '\n';
    }
    text += "-----END CERTIFICATE-----\n";
    if (BIO_write_nid_postfix(bp, text.data(), static_cast<int>(text.size())) != static_cast<int>(text.size())) {
        PushError(ErrLibPem, PemFunctionWriteBio, ErrReasonBufLib);
        return 0;
    }
    return 1;
}

}
