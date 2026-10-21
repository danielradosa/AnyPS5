#include "prx/libc/include/General.hpp"

#include <stdexcept>
#include <string>

extern "C" {

const char* APS5_VABI OpenSSL_version_nid_postfix(int type) {
    switch (type) {
    case 1:
        return "compiler: information not available";
    case 2:
        return "built on: date not available";
    case 3:
        return "platform: information not available";
    case 0:
    case 4:
    case 5:
        throw std::invalid_argument("OpenSSL_version: the module's LibreSSL version, OPENSSLDIR and the meaning of type 5 are not known (type " + std::to_string(type) + ")");
    default:
        return "not available";
    }
}

}
