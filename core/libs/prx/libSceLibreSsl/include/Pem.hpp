#ifndef CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_PEM_HPP
#define CORE_LIBS_PRX_LIBSCELIBRESSL_INCLUDE_PEM_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "LibreSsl.hpp"

namespace LibreSsl {

constexpr int ErrLibPem = 9;
constexpr int ErrReasonAsn1Lib = 13;

struct PemObject {
    std::string label;
    std::string headers;
    std::vector<unsigned char> der;
    std::string text;
};

enum class PemStatus { Ok, NoStartLine, BadBase64 };

using LabelFilter = std::function<bool(std::string_view)>;

PemStatus NextPem(std::string_view text, std::size_t& offset, PemObject& out, const LabelFilter& wanted);
bool ReadPemFromBio(Bio* bio, PemObject& object, const LabelFilter& wanted, const char* function);

}

#endif
