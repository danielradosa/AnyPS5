#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <thread>

struct GuestEvpMd;
struct alignas(8) GuestEvpMdCtx { unsigned char bytes[48]; };

extern "C" {
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
std::uint64_t APS5_VABI ERR_peek_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
void APS5_VABI ERR_remove_thread_state_nid_postfix(const void*);
void APS5_VABI ERR_free_strings_nid_postfix();
char* APS5_VABI ERR_error_string_nid_postfix(std::uint64_t, char*);
void APS5_VABI ERR_error_string_n_nid_postfix(std::uint64_t, char*, std::size_t);
void APS5_VABI OPENSSL_add_all_algorithms_noconf_nid_postfix();
void APS5_VABI EVP_cleanup_nid_postfix();
int APS5_VABI SSL_library_init_nid_postfix();
void APS5_VABI SSL_load_error_strings_nid_postfix();
void APS5_VABI OBJ_cleanup_nid_postfix();
void APS5_VABI CONF_modules_free_nid_postfix();
void APS5_VABI OPENSSL_load_builtin_modules_nid_postfix();
void APS5_VABI CRYPTO_cleanup_all_ex_data_nid_postfix();
int APS5_VABI CRYPTO_num_locks_nid_postfix();
void APS5_VABI CRYPTO_set_locking_callback_nid_postfix(void (APS5_VABI*)(int, int, const char*, int));
void APS5_VABI CRYPTO_set_add_lock_callback_nid_postfix(int (APS5_VABI*)(int*, int, int, const char*, int));
void* APS5_VABI ENGINE_by_id_nid_postfix(const char*);
void APS5_VABI ENGINE_load_builtin_engines_nid_postfix();
void APS5_VABI ENGINE_cleanup_nid_postfix();
int APS5_VABI ENGINE_init_nid_postfix(void*);
int APS5_VABI ENGINE_finish_nid_postfix(void*);
int APS5_VABI ENGINE_free_nid_postfix(void*);
int APS5_VABI ENGINE_ctrl_cmd_nid_postfix(void*, const char*, std::int64_t, void*, void (*)(), int);
int APS5_VABI ENGINE_set_default_nid_postfix(void*, unsigned int);
void* APS5_VABI ENGINE_load_private_key_nid_postfix(void*, const char*, void*, void*);
const char* APS5_VABI ENGINE_get_id_nid_postfix(const void*);
void APS5_VABI EVP_MD_CTX_init_nid_postfix(GuestEvpMdCtx*);
int APS5_VABI EVP_DigestInit_ex_nid_postfix(GuestEvpMdCtx*, const GuestEvpMd*, void*);
}

static void Require(bool value) {
    if (!value) std::abort();
}

template <typename TCall>
static bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

static int lockCalls = 0;

static void APS5_VABI Lock(int, int, const char*, int) {
    ++lockCalls;
}

static int APS5_VABI AddLock(int* number, int amount, int, const char*, int) {
    ++lockCalls;
    return *number += amount;
}

static void PushNoDigest() {
    GuestEvpMdCtx context;
    EVP_MD_CTX_init_nid_postfix(&context);
    Require(EVP_DigestInit_ex_nid_postfix(&context, nullptr, nullptr) == 0);
}

static void TestQueue() {
    constexpr std::uint64_t noDigestSet = (6ull << 24) | (128ull << 12) | 139;
    ERR_clear_error_nid_postfix();
    Require(ERR_get_error_nid_postfix() == 0 && ERR_peek_error_nid_postfix() == 0);
    PushNoDigest();
    Require(ERR_peek_error_nid_postfix() == noDigestSet);
    Require(ERR_peek_error_nid_postfix() == noDigestSet);
    Require(ERR_get_error_nid_postfix() == noDigestSet);
    Require(ERR_get_error_nid_postfix() == 0);

    for (int index = 0; index < 20; ++index) PushNoDigest();
    int count = 0;
    while (ERR_get_error_nid_postfix()) ++count;
    Require(count == 15);

    PushNoDigest();
    std::thread other([] {
        Require(ERR_peek_error_nid_postfix() == 0);
        PushNoDigest();
        ERR_remove_thread_state_nid_postfix(nullptr);
        Require(ERR_peek_error_nid_postfix() == 0);
    });
    other.join();
    Require(ERR_peek_error_nid_postfix() == noDigestSet);
    ERR_clear_error_nid_postfix();
    Require(ERR_peek_error_nid_postfix() == 0);
    PushNoDigest();
    ERR_remove_thread_state_nid_postfix(nullptr);
    Require(ERR_get_error_nid_postfix() == 0);
    int marker = 0;
    Require(Throws([&] { ERR_remove_thread_state_nid_postfix(&marker); }));
}

static void ExpectString(std::uint64_t code, const char* expected) {
    std::array<char, 256> buffer{};
    Require(ERR_error_string_nid_postfix(code, buffer.data()) == buffer.data());
    Require(std::strcmp(buffer.data(), expected) == 0);
}

static void TestStrings() {
    ExpectString(0, "error:00000000:lib(0):func(0):reason(0)");
    ExpectString(0xe1abc789, "error:E1ABC789:lib(225):func(2748):reason(1929)");
    ExpectString(0x12345678e1abc789, "error:12345678E1ABC789:lib(225):func(2748):reason(1929)");
    auto* shared = ERR_error_string_nid_postfix(0xe1abc789, nullptr);
    Require(shared != nullptr);
    Require(ERR_error_string_nid_postfix(0, nullptr) == shared);
    Require(std::strcmp(shared, "error:00000000:lib(0):func(0):reason(0)") == 0);
    constexpr auto expected = "error:E1ABC789:lib(225):func(2748):reason(1929)";
    for (std::size_t size = 0; size <= std::strlen(expected) + 2; ++size) {
        std::array<char, 260> buffer;
        buffer.fill('#');
        ERR_error_string_n_nid_postfix(0xe1abc789, buffer.data() + 1, size);
        Require(buffer.front() == '#' && buffer[size + 1] == '#');
        if (size == 0) continue;
        const auto count = std::strlen(buffer.data() + 1);
        Require(count < size);
        if (size > 4 && size <= std::strlen(expected)) {
            std::size_t colons = 0;
            for (std::size_t index = 1; index <= count; ++index) colons += buffer[index] == ':';
            Require(colons == 4);
        }
        if (size > std::strlen(expected)) Require(std::strcmp(buffer.data() + 1, expected) == 0);
    }
    char tiny[6]{};
    ERR_error_string_n_nid_postfix(0xe1abc789, tiny, sizeof(tiny));
    Require(std::strcmp(tiny, "e::::") == 0);
    ERR_error_string_n_nid_postfix(0, nullptr, 0);
}

static void TestLibrary() {
    SSL_load_error_strings_nid_postfix();
    Require(SSL_library_init_nid_postfix() == 1);
    OPENSSL_add_all_algorithms_noconf_nid_postfix();
    OPENSSL_load_builtin_modules_nid_postfix();
    Require(CRYPTO_num_locks_nid_postfix() == 41);
    CRYPTO_set_locking_callback_nid_postfix(Lock);
    CRYPTO_set_add_lock_callback_nid_postfix(AddLock);
    PushNoDigest();
    ERR_clear_error_nid_postfix();
    CRYPTO_set_locking_callback_nid_postfix(nullptr);
    CRYPTO_set_add_lock_callback_nid_postfix(nullptr);
    Require(lockCalls == 0);

    ENGINE_load_builtin_engines_nid_postfix();
    Require(ENGINE_by_id_nid_postfix("dynamic") == nullptr);
    Require(ENGINE_by_id_nid_postfix(nullptr) == nullptr);
    int engine = 0;
    Require(ENGINE_init_nid_postfix(&engine) == 0);
    Require(ENGINE_finish_nid_postfix(&engine) == 0);
    Require(ENGINE_free_nid_postfix(&engine) == 0);
    Require(ENGINE_ctrl_cmd_nid_postfix(&engine, "SO_PATH", 0, nullptr, nullptr, 0) == 0);
    Require(ENGINE_set_default_nid_postfix(&engine, 0xffff) == 0);
    Require(ENGINE_load_private_key_nid_postfix(&engine, "key", nullptr, nullptr) == nullptr);
    const char* id = ENGINE_get_id_nid_postfix(&engine);
    Require(id && std::strlen(id) == 0);
    ENGINE_cleanup_nid_postfix();

    ERR_free_strings_nid_postfix();
    EVP_cleanup_nid_postfix();
    OBJ_cleanup_nid_postfix();
    CONF_modules_free_nid_postfix();
    CRYPTO_cleanup_all_ex_data_nid_postfix();
}

int main() {
    TestQueue();
    TestStrings();
    TestLibrary();
    return 0;
}
