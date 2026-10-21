#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <source_location>

extern "C" {
const char* APS5_VABI OpenSSL_version_nid_postfix(int);
}

namespace {

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "LibreSSL 3 text check failed at line %u\n", location.line());
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

}

int main() {
    Require(std::strcmp(OpenSSL_version_nid_postfix(1), "compiler: information not available") == 0);
    Require(std::strcmp(OpenSSL_version_nid_postfix(2), "built on: date not available") == 0);
    Require(std::strcmp(OpenSSL_version_nid_postfix(3), "platform: information not available") == 0);
    Require(std::strcmp(OpenSSL_version_nid_postfix(6), "not available") == 0);
    Require(std::strcmp(OpenSSL_version_nid_postfix(-1), "not available") == 0);
    Require(OpenSSL_version_nid_postfix(1) == OpenSSL_version_nid_postfix(1));
    Require(Throws([] { OpenSSL_version_nid_postfix(0); }));
    Require(Throws([] { OpenSSL_version_nid_postfix(4); }));
    Require(Throws([] { OpenSSL_version_nid_postfix(5); }));
    return 0;
}
