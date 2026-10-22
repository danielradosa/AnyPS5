#include "prx/libSceLibreSsl/include/Asn1.hpp"
#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" int APS5_VABI BIO_write_nid_postfix(LibreSsl::Bio* b, const void* data, int len);

namespace LibreSsl {

namespace {

constexpr std::uint64_t OpensslVersionNumber = 0x20000000;

constexpr int TypeUtf8String = 12;
constexpr int TypeNumericString = 18;
constexpr int TypePrintableString = 19;
constexpr int TypeT61String = 20;
constexpr int TypeVideotexString = 21;
constexpr int TypeIa5String = 22;
constexpr int TypeUtcTime = 23;
constexpr int TypeGeneralizedTime = 24;
constexpr int TypeGraphicString = 25;
constexpr int TypeVisibleString = 26;
constexpr int TypeGeneralString = 27;
constexpr int TypeUniversalString = 28;
constexpr int TypeBmpString = 30;

constexpr unsigned long StrEsc2253 = 0x1;
constexpr unsigned long StrEscCtrl = 0x2;
constexpr unsigned long StrEscMsb = 0x4;
constexpr unsigned long StrEscQuote = 0x8;
constexpr unsigned long StrUtf8Convert = 0x10;
constexpr unsigned long StrIgnoreType = 0x20;
constexpr unsigned long StrShowType = 0x40;
constexpr unsigned long StrDumpAll = 0x80;
constexpr unsigned long StrDumpUnknown = 0x100;
constexpr unsigned long StrDumpDer = 0x200;
constexpr unsigned long StrKnown = 0x3ff;

constexpr unsigned long NameSepMask = 0xful << 16;
constexpr unsigned long NameSepCommaPlus = 1ul << 16;
constexpr unsigned long NameSepCplusSpc = 2ul << 16;
constexpr unsigned long NameSepSplusSpc = 3ul << 16;
constexpr unsigned long NameSepMultiline = 4ul << 16;
constexpr unsigned long NameDnRev = 1ul << 20;
constexpr unsigned long NameFnMask = 0x3ul << 21;
constexpr unsigned long NameFnSn = 0;
constexpr unsigned long NameFnLn = 1ul << 21;
constexpr unsigned long NameFnOid = 2ul << 21;
constexpr unsigned long NameFnNone = 3ul << 21;
constexpr unsigned long NameSpcEq = 1ul << 23;
constexpr unsigned long NameDumpUnknownFields = 1ul << 24;
constexpr unsigned long NameFnAlign = 1ul << 25;
constexpr unsigned long NameKnown = StrKnown | NameSepMask | NameDnRev | NameFnMask | NameSpcEq | NameDumpUnknownFields | NameFnAlign;

constexpr const char* Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

bool WriteAll(Bio* bio, const std::string& text) {
    if (text.empty()) return true;
    if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("LibreSSL text output above INT_MAX bytes");
    return BIO_write_nid_postfix(bio, text.data(), static_cast<int>(text.size())) == static_cast<int>(text.size());
}

int Digits(const unsigned char* data, int count) {
    int value = 0;
    for (int index = 0; index < count; ++index) {
        if (data[index] < '0' || data[index] > '9') return -1;
        value = value * 10 + (data[index] - '0');
    }
    return value;
}

bool CharacterType(int type) {
    switch (type) {
    case TypeUtf8String:
    case TypeNumericString:
    case TypePrintableString:
    case TypeT61String:
    case TypeVideotexString:
    case TypeIa5String:
    case TypeGraphicString:
    case TypeVisibleString:
    case TypeGeneralString:
    case TypeUniversalString:
    case TypeBmpString:
        return true;
    default:
        return false;
    }
}

std::vector<std::uint32_t> Characters(const Asn1String& value, unsigned long flags) {
    const auto* data = value.data;
    const auto length = static_cast<std::size_t>(value.length);
    std::vector<std::uint32_t> characters;
    const bool singleBytes = (flags & StrIgnoreType) || (value.type != TypeBmpString && value.type != TypeUniversalString && value.type != TypeUtf8String);
    if (!(flags & StrIgnoreType) && !CharacterType(value.type) && (flags & StrDumpUnknown))
        throw std::invalid_argument("X509_NAME_print_ex: dumping a value that is no character string is not supported, the hex digit case is not documented");
    if (singleBytes || value.type == TypeUtf8String) {
        characters.assign(data, data + length);
        if (singleBytes && (flags & StrUtf8Convert)) {
            for (const auto character : characters) {
                if (character >= 0x80) throw std::invalid_argument("X509_NAME_print_ex: converting a single-byte string with bytes above 0x7f to UTF-8 is not documented");
            }
        }
        return characters;
    }
    if (value.type == TypeBmpString) {
        if (length % 2) throw std::invalid_argument("X509_NAME_print_ex: a BMPString with an odd length is handled in a way the manuals do not document");
        for (std::size_t index = 0; index < length; index += 2) characters.push_back(static_cast<std::uint32_t>(data[index]) << 8 | data[index + 1]);
    } else {
        if (length % 4) throw std::invalid_argument("X509_NAME_print_ex: a UniversalString with a partial character is handled in a way the manuals do not document");
        for (std::size_t index = 0; index < length; index += 4)
            characters.push_back(static_cast<std::uint32_t>(data[index]) << 24 | static_cast<std::uint32_t>(data[index + 1]) << 16 |
                static_cast<std::uint32_t>(data[index + 2]) << 8 | data[index + 3]);
    }
    if (!(flags & StrUtf8Convert)) {
        for (const auto character : characters) {
            if (character >= 0x80)
                throw std::invalid_argument("X509_NAME_print_ex: a wide character without ASN1_STRFLGS_UTF8_CONVERT is printed as \\U or \\W with a hex digit case that is not documented");
        }
        return characters;
    }
    std::vector<std::uint32_t> bytes;
    for (const auto character : characters) {
        if (character > 0x10ffff) throw std::invalid_argument("X509_NAME_print_ex: a character above U+10FFFF is handled in a way the manuals do not document");
        if (character < 0x80) {
            bytes.push_back(character);
        } else if (character < 0x800) {
            bytes.push_back(0xc0 | (character >> 6));
            bytes.push_back(0x80 | (character & 0x3f));
        } else if (character < 0x10000) {
            bytes.push_back(0xe0 | (character >> 12));
            bytes.push_back(0x80 | ((character >> 6) & 0x3f));
            bytes.push_back(0x80 | (character & 0x3f));
        } else {
            bytes.push_back(0xf0 | (character >> 18));
            bytes.push_back(0x80 | ((character >> 12) & 0x3f));
            bytes.push_back(0x80 | ((character >> 6) & 0x3f));
            bytes.push_back(0x80 | (character & 0x3f));
        }
    }
    return bytes;
}

bool Special2253(std::uint32_t character) {
    return character == ',' || character == '+' || character == '"' || character == '\\' || character == '<' || character == '>' || character == ';';
}

std::string Value(const Asn1String& value, unsigned long flags) {
    if (flags & StrShowType) throw std::invalid_argument("X509_NAME_print_ex: ASN1_STRFLGS_SHOW_TYPE needs ASN1_tag2str names that are not documented");
    if (flags & StrDumpAll) throw std::invalid_argument("X509_NAME_print_ex: dumped values use a hex digit case that is not documented");
    const auto characters = Characters(value, flags);
    bool quote = false;
    bool quoteOnly = false;
    for (std::size_t index = 0; index < characters.size(); ++index) {
        const auto character = characters[index];
        if (character < 0x20 || character == 0x7f) {
            if (flags & StrEscCtrl) throw std::invalid_argument("X509_NAME_print_ex: escaped control characters use a hex digit case that is not documented");
        } else if (character >= 0x80) {
            if (flags & StrEscMsb) throw std::invalid_argument("X509_NAME_print_ex: escaped bytes with the MSB set use a hex digit case that is not documented");
        }
        if (!(flags & StrEsc2253)) continue;
        const bool edge = (index == 0 && (character == '#' || character == ' ')) || (index + 1 == characters.size() && character == ' ');
        if (character == '"' || character == '\\') quoteOnly = true;
        else if (Special2253(character) || edge) quote = true;
    }
    if ((flags & StrEsc2253) && (flags & StrEscQuote) && quoteOnly)
        throw std::invalid_argument("X509_NAME_print_ex: how ASN1_STRFLGS_ESC_QUOTE handles '\"' and '\\' is not documented");
    std::string out;
    const bool quoted = (flags & StrEsc2253) && (flags & StrEscQuote) && quote;
    if (quoted) out += '"';
    for (std::size_t index = 0; index < characters.size(); ++index) {
        const auto character = characters[index];
        if ((flags & StrEsc2253) && !quoted) {
            const bool edge = (index == 0 && (character == '#' || character == ' ')) || (index + 1 == characters.size() && character == ' ');
            if (Special2253(character) || edge) out += '\\';
        }
        out += static_cast<char>(character);
    }
    if (quoted) out += '"';
    return out;
}

std::string FieldName(const Asn1Object& object, unsigned long flags) {
    std::string dotted;
    if (!DecodeObjectIdentifier(object.data, static_cast<std::size_t>(object.length > 0 ? object.length : 0), dotted))
        throw std::invalid_argument("X509_NAME_print_ex: a name entry with a malformed OID is printed in a way the manuals do not document");
    const auto option = flags & NameFnMask;
    if (option == NameFnOid) return dotted;
    const int nid = ObjectNid(&object, false);
    if (nid == NidUndef) return dotted;
    if (option == NameFnSn) return TableShortName(nid, "X509_NAME_print_ex");
    const auto* table = TableObject(nid, false);
    if (!table || !table->ln) throw std::invalid_argument("X509_NAME_print_ex: the attribute has no long name in the object table, which LibreSSL resolves in a way its manuals do not document");
    return table->ln;
}

}

}

using namespace LibreSsl;

extern "C" {

std::uint64_t APS5_VABI SSLeay_nid_postfix() {
    return OpensslVersionNumber;
}

void APS5_VABI CRYPTO_THREADID_set_numeric_nid_postfix(void* id, std::uint64_t val) {
    if (!id) throw std::invalid_argument("CRYPTO_THREADID_set_numeric: null id");
    auto* fields = static_cast<std::uint64_t*>(id);
    fields[0] = 0;
    fields[1] = val;
}

int APS5_VABI ASN1_STRING_print_nid_postfix(Bio* bp, const Asn1String* v) {
    if (!bp) throw std::invalid_argument("ASN1_STRING_print: null BIO");
    if (!v) throw std::invalid_argument("ASN1_STRING_print: null string");
    if (v->length < 0 || (v->length && !v->data)) throw std::invalid_argument("ASN1_STRING_print: malformed string");
    std::string text(reinterpret_cast<const char*>(v->data), static_cast<std::size_t>(v->length));
    for (auto& character : text) {
        const auto byte = static_cast<unsigned char>(character);
        if ((byte < 0x20 && byte != '\n' && byte != '\r') || byte > 0x7e) character = '.';
    }
    return WriteAll(bp, text) ? 1 : 0;
}

int APS5_VABI ASN1_TIME_print_nid_postfix(Bio* fp, const Asn1String* a) {
    if (!fp) throw std::invalid_argument("ASN1_TIME_print: null BIO");
    if (!a) throw std::invalid_argument("ASN1_TIME_print: null time");
    const auto* data = a->data;
    const int length = a->length;
    if (length < 0 || (length && !data)) throw std::invalid_argument("ASN1_TIME_print: malformed time");
    const auto bad = [&] {
        WriteAll(fp, "Bad time value");
        return 0;
    };
    if (a->type != TypeUtcTime && a->type != TypeGeneralizedTime) return bad();
    const int yearDigits = a->type == TypeUtcTime ? 2 : 4;
    const int fieldsLength = yearDigits + 10;
    for (int index = 0; index < length; ++index) {
        const auto byte = data[index];
        if (byte == '+' || byte == '-' || byte == '.' || byte == ',')
            throw std::invalid_argument("ASN1_TIME_print: fractional seconds and time zone offsets are printed in a way the manuals do not document");
    }
    const bool gmt = length > 0 && data[length - 1] == 'Z';
    const int body = gmt ? length - 1 : length;
    for (int index = 0; index < body; ++index) {
        if (data[index] < '0' || data[index] > '9') return bad();
    }
    if (body == fieldsLength - 2) throw std::invalid_argument("ASN1_TIME_print: a time without seconds is printed in a way the manuals do not document");
    if (body != fieldsLength) return bad();
    int year = Digits(data, yearDigits);
    const int month = Digits(data + yearDigits, 2);
    const int day = Digits(data + yearDigits + 2, 2);
    const int hour = Digits(data + yearDigits + 4, 2);
    const int minute = Digits(data + yearDigits + 6, 2);
    const int second = Digits(data + yearDigits + 8, 2);
    if (year < 0 || month < 0 || day < 0 || hour < 0 || minute < 0 || second < 0) return bad();
    if (month < 1 || month > 12) return bad();
    if (day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59)
        throw std::invalid_argument("ASN1_TIME_print: which out-of-range fields 2.6 rejects as a bad time value is not documented");
    if (day < 10)
        throw std::invalid_argument("ASN1_TIME_print: the padding of a one-digit day is not documented (the manual's format says DD, its example shows one space)");
    if (yearDigits == 2) year += year < 50 ? 2000 : 1900;
    char text[48];
    std::snprintf(text, sizeof(text), "%s %d %02d:%02d:%02d %d%s", Months[month - 1], day, hour, minute, second, year,
        gmt ? " GMT" : "");
    return WriteAll(fp, text) ? 1 : 0;
}

int APS5_VABI X509_NAME_print_ex_nid_postfix(Bio* out, const Name* nm, int indent, std::uint64_t flags) {
    if (!out) throw std::invalid_argument("X509_NAME_print_ex: null BIO");
    if (!nm || !nm->entries) throw std::invalid_argument("X509_NAME_print_ex: null X509_NAME");
    if (flags & ~static_cast<std::uint64_t>(NameKnown)) throw std::invalid_argument("X509_NAME_print_ex: unknown flags");
    const auto separator = flags & NameSepMask;
    if (separator == 0) throw std::invalid_argument("X509_NAME_print_ex: XN_FLAG_COMPAT uses the X509_NAME_print format, which is not documented");
    if (flags & NameFnAlign)
        throw std::invalid_argument("X509_NAME_print_ex: how XN_FLAG_FN_ALIGN pads field names is not documented beyond their 20-character width");
    const char* dnSeparator = nullptr;
    const char* valueSeparator = nullptr;
    switch (separator) {
    case NameSepCommaPlus: dnSeparator = ","; valueSeparator = "+"; break;
    case NameSepCplusSpc: dnSeparator = ", "; valueSeparator = " + "; break;
    case NameSepSplusSpc: dnSeparator = "; "; valueSeparator = " + "; break;
    case NameSepMultiline: dnSeparator = "\n"; valueSeparator = " + "; break;
    default: throw std::invalid_argument("X509_NAME_print_ex: unknown separator option");
    }
    if (indent < 0) throw std::invalid_argument("X509_NAME_print_ex: negative indentation");
    if (indent > 0 && separator != NameSepMultiline) throw std::invalid_argument("X509_NAME_print_ex: indentation of a one-line format is not documented");
    const std::string padding(static_cast<std::size_t>(indent), ' ');
    std::vector<const NameEntry*> entries;
    for (int index = 0; index < nm->entries->num; ++index) entries.push_back(reinterpret_cast<const NameEntry*>(nm->entries->data[index]));
    if (flags & NameDnRev) std::reverse(entries.begin(), entries.end());
    std::string text;
    if (!entries.empty()) text += padding;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto& entry = *entries[index];
        if (!entry.object || !entry.value) throw std::invalid_argument("X509_NAME_print_ex: malformed name entry");
        if (index) {
            if (entry.set == entries[index - 1]->set) text += valueSeparator;
            else text += std::string(dnSeparator) + (separator == NameSepMultiline ? padding : "");
        }
        if ((flags & NameDumpUnknownFields) && ObjectNid(entry.object, false) == NidUndef)
            throw std::invalid_argument("X509_NAME_print_ex: unknown fields are dumped with a hex digit case that is not documented");
        if ((flags & NameFnMask) != NameFnNone) {
            text += FieldName(*entry.object, flags);
            text += (flags & NameSpcEq) ? " = " : "=";
        }
        text += Value(*entry.value, flags);
    }
    if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("X509_NAME_print_ex: output above INT_MAX bytes");
    return WriteAll(out, text) ? static_cast<int>(text.size()) : -1;
}

}
