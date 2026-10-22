#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <atomic>

namespace {

constexpr int CryptoNumLocks = 41;

std::atomic<void*> lockingCallback{nullptr};
std::atomic<void*> addLockCallback{nullptr};

}

extern "C" {

void APS5_VABI OPENSSL_add_all_algorithms_noconf_nid_postfix() {
}

void APS5_VABI EVP_cleanup_nid_postfix() {
}

int APS5_VABI SSL_library_init_nid_postfix() {
    return 1;
}

void APS5_VABI SSL_load_error_strings_nid_postfix() {
}

void APS5_VABI OBJ_cleanup_nid_postfix() {
    LibreSsl::ObjectsCleanup();
}

void APS5_VABI CONF_modules_free_nid_postfix() {
}

void APS5_VABI OPENSSL_load_builtin_modules_nid_postfix() {
}

void APS5_VABI CRYPTO_cleanup_all_ex_data_nid_postfix() {
}

int APS5_VABI CRYPTO_num_locks_nid_postfix() {
    return CryptoNumLocks;
}

void APS5_VABI CRYPTO_set_locking_callback_nid_postfix(void* func) {
    lockingCallback.store(func);
}

void APS5_VABI CRYPTO_set_add_lock_callback_nid_postfix(void* func) {
    addLockCallback.store(func);
}

void* APS5_VABI ENGINE_by_id_nid_postfix(const char*) {
    return nullptr;
}

void APS5_VABI ENGINE_load_builtin_engines_nid_postfix() {
}

void APS5_VABI ENGINE_cleanup_nid_postfix() {
}

int APS5_VABI ENGINE_init_nid_postfix(void*) {
    return 0;
}

int APS5_VABI ENGINE_finish_nid_postfix(void*) {
    return 0;
}

int APS5_VABI ENGINE_free_nid_postfix(void*) {
    return 0;
}

int APS5_VABI ENGINE_ctrl_cmd_nid_postfix(void*, const char*, std::int64_t, void*, void*, int) {
    return 0;
}

int APS5_VABI ENGINE_set_default_nid_postfix(void*, unsigned int) {
    return 0;
}

void* APS5_VABI ENGINE_load_private_key_nid_postfix(void*, const char*, void*, void*) {
    return nullptr;
}

const char* APS5_VABI ENGINE_get_id_nid_postfix(const void*) {
    return "";
}

}
