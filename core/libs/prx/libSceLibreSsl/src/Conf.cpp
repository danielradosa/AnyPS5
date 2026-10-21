#include "prx/libc/include/General.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

struct FileStream;

extern "C" {
FileStream* APS5_VABI fopen_nid_postfix(const char* filename, const char* mode);
int APS5_VABI fclose_nid_postfix(FileStream* stream);
int* APS5_VABI __error_nid_postfix();

int APS5_VABI CONF_modules_load_file_nid_postfix(const char* filename, const char*, std::uint64_t flags) {
    constexpr std::uint64_t IgnoreMissingFile = 0x10;
    constexpr int GuestEnoent = 2;
    const char* path = filename ? filename : "/etc/ssl/openssl.cnf";
    *__error_nid_postfix() = 0;
    if (auto* stream = fopen_nid_postfix(path, "r")) {
        fclose_nid_postfix(stream);
        throw std::invalid_argument(std::string("CONF_modules_load_file: configuration files are not parsed by this build (") + path + ")");
    }
    if (*__error_nid_postfix() == GuestEnoent && (flags & IgnoreMissingFile)) return 1;
    throw std::invalid_argument(std::string("CONF_modules_load_file: the failure value for ") + path +
        " (missing without CONF_MFLAGS_IGNORE_MISSING_FILE or not readable) is only documented as zero or negative");
}

}
