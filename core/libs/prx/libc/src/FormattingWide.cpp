#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <stdexcept>
#include <string>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/WindowsFormatting.hpp"

#ifdef _WIN32
#define APS5_VA_BEGIN(last) __builtin_sysv_va_list args; __builtin_sysv_va_start(args, last)
#define APS5_VA_END() __builtin_sysv_va_end(args)
#else
#define APS5_VA_BEGIN(last) std::va_list args; va_start(args, last)
#define APS5_VA_END() va_end(args)
#endif

extern "C" std::size_t APS5_VABI wcrtomb_nid_postfix(char* destination, std::uint16_t value, void* state);

namespace {

bool In(char16_t character, const char* set) {
    return character != 0 && character < 128 && std::strchr(set, static_cast<char>(character)) != nullptr;
}

void AppendUtf16(std::u16string& out, const char* text, std::size_t limit) {
    for (std::size_t index = 0; out.size() < limit && text[index] != '\0';) {
        const unsigned char lead = static_cast<unsigned char>(text[index]);
        std::size_t extra = lead >= 0xf0 ? 3 : lead >= 0xe0 ? 2 : lead >= 0xc0 ? 1 : 0;
        std::uint32_t code = extra == 3 ? lead & 0x07 : extra == 2 ? lead & 0x0f : extra == 1 ? lead & 0x1f : lead;
        std::size_t used = 1;
        for (; used <= extra; ++used) {
            const unsigned char next = static_cast<unsigned char>(text[index + used]);
            if ((next & 0xc0) != 0x80) { extra = 0; code = lead; used = 1; break; }
            code = (code << 6) | (next & 0x3f);
        }
        if (extra != 0) used = extra + 1;
        index += used;
        if (code >= 0x10000) {
            if (limit - out.size() < 2) break;
            code -= 0x10000;
            out.push_back(static_cast<char16_t>(0xd800 + (code >> 10)));
            out.push_back(static_cast<char16_t>(0xdc00 + (code & 0x3ff)));
        } else out.push_back(static_cast<char16_t>(code));
    }
}

std::string ToUtf8(const std::u16string& text) {
    std::string out;
    for (std::size_t index = 0; index < text.size(); ++index) {
        std::uint32_t code = text[index];
        if (code >= 0xd800 && code < 0xdc00 && index + 1 < text.size() && text[index + 1] >= 0xdc00 && text[index + 1] < 0xe000)
            code = 0x10000 + ((code - 0xd800) << 10) + (text[++index] - 0xdc00);
        if (code < 0x80) out.push_back(static_cast<char>(code));
        else if (code < 0x800) {
            out.push_back(static_cast<char>(0xc0 | (code >> 6)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
        } else if (code < 0x10000) {
            out.push_back(static_cast<char>(0xe0 | (code >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
        } else {
            out.push_back(static_cast<char>(0xf0 | (code >> 18)));
            out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
        }
    }
    return out;
}

template<class TValue> void AppendNumber(std::u16string& out, const std::string& spec, TValue value) {
    const int size = std::snprintf(nullptr, 0, spec.c_str(), value);
    if (size < 0) throw std::runtime_error("Formatting conversion failed");
    std::string text(static_cast<std::size_t>(size) + 1, '\0');
    std::snprintf(&text[0], text.size(), spec.c_str(), value);
    out.append(text.begin(), text.begin() + size);
}

void AppendPadded(std::u16string& out, const std::u16string& text, bool left, int width) {
    const std::size_t pad = width > 0 && static_cast<std::size_t>(width) > text.size() ? static_cast<std::size_t>(width) - text.size() : 0;
    if (!left) out.append(pad, u' ');
    out += text;
    if (left) out.append(pad, u' ');
}

std::u16string FormatWide(const char16_t* format, VaList* source, bool secure = false) {
    if (format == nullptr || source == nullptr) throw std::invalid_argument("Null formatting argument");
    LibcDetail::FormatArguments args(source);
    const char16_t* const start = format;
    std::string ascii;
    for (const char16_t* unit = format; *unit != 0; ++unit) ascii.push_back(*unit < 0x80 ? static_cast<char>(*unit) : '?');
    args.Number(LibcDetail::NumberedArgumentClasses(ascii.c_str()));
    const auto position = [&] {
        const char* cursor = ascii.c_str() + (format - start);
        const std::size_t found = LibcDetail::ArgumentPosition(cursor);
        format = start + (cursor - ascii.c_str());
        return found;
    };
    const auto star = [&] {
        const std::size_t found = position();
        return found ? args.At<int>(found) : args.Next<int>();
    };
    std::u16string out;
    while (*format) {
        if (*format != u'%') { out.push_back(*format++); continue; }
        ++format;
        if (*format == u'%') { out.push_back(*format++); continue; }
        if (const std::size_t found = position()) args.Seek(found);
        std::string spec = "%";
        bool left = false;
        int width = 0;
        int precision = -1;
        while (In(*format, "-+ #0'")) {
            if (*format == u'-') left = true;
            if (*format != u'\'') spec += static_cast<char>(*format);
            ++format;
        }
        if (*format == u'*') {
            ++format;
            width = star();
            if (width < 0) { left = true; spec += '-'; width = -width; }
            spec += std::to_string(width);
        } else {
            while (*format >= u'0' && *format <= u'9') { width = width * 10 + (*format - u'0'); spec += static_cast<char>(*format++); }
        }
        if (*format == u'.') {
            ++format;
            precision = 0;
            if (*format == u'*') {
                ++format;
                precision = star();
                if (precision >= 0) spec += "." + std::to_string(precision);
            } else {
                spec += '.';
                while (*format >= u'0' && *format <= u'9') { precision = precision * 10 + (*format - u'0'); spec += static_cast<char>(*format++); }
            }
        }
        std::string length;
        if (In(*format, "hljztL")) {
            length += static_cast<char>(*format++);
            if ((length == "h" && *format == u'h') || (length == "l" && *format == u'l')) length += static_cast<char>(*format++);
        }
        const char16_t conversion = *format;
        if (!conversion) throw std::invalid_argument("Incomplete format conversion");
        ++format;
        const bool integerLength = length.empty() || length == "h" || length == "hh" || length == "l" || length == "ll" ||
            length == "j" || length == "z" || length == "t";
        if (conversion == u'd' || conversion == u'i') {
            if (!integerLength) throw std::invalid_argument("Invalid integer length");
            long long value;
            if (length.empty() || length == "h" || length == "hh") {
                value = args.Next<int>();
                if (length == "h") value = static_cast<short>(value);
                if (length == "hh") value = static_cast<signed char>(value);
            } else value = args.Next<long long>();
            AppendNumber(out, spec + "ll" + static_cast<char>(conversion), value);
        } else if (In(conversion, "ouxX")) {
            if (!integerLength) throw std::invalid_argument("Invalid integer length");
            unsigned long long value;
            if (length.empty() || length == "h" || length == "hh") {
                value = args.Next<unsigned int>();
                if (length == "h") value = static_cast<unsigned short>(value);
                if (length == "hh") value = static_cast<unsigned char>(value);
            } else value = args.Next<unsigned long long>();
            if (conversion == u'o' && value == 0 && precision == 0 && spec.find('#') != std::string::npos)
                spec = spec.substr(0, spec.find('.')) + ".1";
            AppendNumber(out, spec + "ll" + static_cast<char>(conversion), value);
        } else if (In(conversion, "aAeEfFgG")) {
            if (length == "L") AppendNumber(out, spec + "L" + static_cast<char>(conversion), args.Next<long double>());
            else {
                if (!length.empty() && length != "l") throw std::invalid_argument("Invalid floating length");
                AppendNumber(out, spec + static_cast<char>(conversion), args.Next<double>());
            }
        } else if (conversion == u'p' && length.empty()) {
            const std::string text = LibcDetail::FormatPointer(spec, args.Next<void*>());
            out.append(text.begin(), text.end());
        } else if (conversion == u'c' || conversion == u'C') {
            const int value = args.Next<int>();
            std::u16string text;
            if (conversion == u'C' || length == "l") text.push_back(static_cast<char16_t>(value));
            else text.push_back(static_cast<char16_t>(static_cast<unsigned char>(value)));
            AppendPadded(out, text, left, width);
        } else if (conversion == u's' || conversion == u'S') {
            std::u16string text;
            const std::size_t limit = precision < 0 ? SIZE_MAX : static_cast<std::size_t>(precision);
            if (conversion == u'S' || length == "l") {
                const char16_t* value = args.Next<const char16_t*>();
                if (value == nullptr && secure) throw std::invalid_argument("Null string argument");
                if (value == nullptr) value = u"(null)";
                for (std::size_t index = 0; index < limit && value[index] != 0; ++index) text.push_back(value[index]);
            } else {
                const char* value = args.Next<const char*>();
                if (value == nullptr && secure) throw std::invalid_argument("Null string argument");
                AppendUtf16(text, value != nullptr ? value : "(null)", limit);
            }
            AppendPadded(out, text, left, width);
        } else if (conversion == u'n' && integerLength && spec == "%" && !secure) {
            void* pointer = args.Next<void*>();
            if (!pointer) throw std::invalid_argument("Null format count pointer");
            const int count = static_cast<int>(out.size());
            if (length == "hh") *static_cast<signed char*>(pointer) = static_cast<signed char>(count);
            else if (length == "h") *static_cast<short*>(pointer) = static_cast<short>(count);
            else if (length.empty()) *static_cast<int*>(pointer) = count;
            else *static_cast<long long*>(pointer) = count;
        } else {
            throw std::invalid_argument("Unsupported format conversion");
        }
    }
    return out;
}

bool IsScanSpace(char16_t character) {
    return In(character, " \t\n\v\f\r");
}

struct ScanSet {
    const char16_t* first = nullptr;
    const char16_t* last = nullptr;
    bool negated = false;
};

ScanSet ParseScanSet(const char16_t*& format) {
    ScanSet set;
    set.negated = *format == u'^';
    if (set.negated) ++format;
    set.first = format;
    if (*format == u']') ++format;
    while (*format != 0 && *format != u']') ++format;
    if (*format != u']') throw std::invalid_argument("Unterminated scan set");
    set.last = format++;
    return set;
}

bool InScanSet(const ScanSet& set, char16_t character) {
    for (const char16_t* item = set.first; item < set.last; ++item) {
        if (item + 2 < set.last && item[1] == u'-' && item[0] <= item[2]) {
            if (character >= item[0] && character <= item[2]) return !set.negated;
            item += 2;
        } else if (character == *item) {
            return !set.negated;
        }
    }
    return set.negated;
}

bool ValidScanLength(const std::string& length, char16_t conversion) {
    if (In(conversion, "diouxXn"))
        return length.empty() || length == "hh" || length == "h" || length == "l" || length == "ll" || length == "j" ||
            length == "z" || length == "t";
    if (In(conversion, "aAeEfFgG")) return length.empty() || length == "l" || length == "L";
    if (In(conversion, "cs[")) return length.empty() || length == "l";
    return conversion == u'p' && length.empty();
}

std::string HostScanLength(const std::string& length, char16_t conversion) {
    if (In(conversion, "diouxX") && (length == "l" || length == "ll" || length == "j" || length == "z" || length == "t")) return "ll";
    return length;
}

void StoreScanCount(void* target, const std::string& length, std::ptrdiff_t count) {
    if (length == "hh") *static_cast<signed char*>(target) = static_cast<signed char>(count);
    else if (length == "h") *static_cast<short*>(target) = static_cast<short>(count);
    else if (length.empty()) *static_cast<int*>(target) = static_cast<int>(count);
    else {
        const std::int64_t value = count;
        std::memcpy(target, &value, sizeof(value));
    }
}

template<class TNextPointer, class TNextCapacity>
int ScanWide(const char16_t* buffer, const char16_t* format, TNextPointer nextPointer, TNextCapacity nextCapacity) {
    if (buffer == nullptr || format == nullptr) return EOF;
    const char16_t* input = buffer;
    int assigned = 0;
    bool converted = false;
    while (*format != 0) {
        if (IsScanSpace(*format)) {
            while (IsScanSpace(*format)) ++format;
            while (IsScanSpace(*input)) ++input;
            continue;
        }
        if (*format != u'%' || format[1] == u'%') {
            const bool percent = *format == u'%';
            const char16_t expected = *format;
            format += percent ? 2 : 1;
            if (percent) while (IsScanSpace(*input)) ++input;
            if (*input == 0) return converted ? assigned : EOF;
            if (*input != expected) return assigned;
            ++input;
            continue;
        }
        ++format;
        const bool suppress = *format == u'*';
        if (suppress) ++format;
        std::size_t width = 0;
        while (*format >= u'0' && *format <= u'9') {
            if (width > (SIZE_MAX - 9) / 10) throw std::invalid_argument("Scan field width too large");
            width = width * 10 + static_cast<std::size_t>(*format++ - u'0');
        }
        std::string length;
        while (In(*format, "hljztL")) length += static_cast<char>(*format++);
        const char16_t conversion = *format;
        if (conversion == 0) throw std::invalid_argument("Incomplete scan conversion");
        ++format;
        if (conversion == u'S' || conversion == u'C') return assigned;
        if (!ValidScanLength(length, conversion)) throw std::invalid_argument("Unsupported scan conversion");
        const ScanSet set = conversion == u'[' ? ParseScanSet(format) : ScanSet{};
        if (conversion == u'n') {
            if (!suppress) {
                void* target = nextPointer();
                if (target == nullptr) return EOF;
                StoreScanCount(target, length, input - buffer);
            }
            continue;
        }
        if (conversion != u'c' && conversion != u'[') while (IsScanSpace(*input)) ++input;
        if (*input == 0) return converted ? assigned : EOF;
        void* target = suppress ? nullptr : nextPointer();
        if (!suppress && target == nullptr) return EOF;
        if (In(conversion, "cs[")) {
            const std::size_t capacity = suppress ? 0 : static_cast<std::size_t>(nextCapacity());
            const std::size_t limit = width != 0 ? width : conversion == u'c' ? 1 : SIZE_MAX;
            std::size_t count = 0;
            while (count < limit && input[count] != 0 && (conversion == u'c' ||
                (conversion == u's' ? !IsScanSpace(input[count]) : InScanSet(set, input[count])))) ++count;
            if (count == 0 || (conversion == u'c' && count < limit)) return assigned;
            if (!suppress) {
                const std::size_t terminator = conversion == u'c' ? 0 : 1;
                if (length == "l") {
                    if (count + terminator > capacity) {
                        if (capacity != 0) static_cast<char16_t*>(target)[0] = 0;
                        return assigned;
                    }
                    std::memcpy(target, input, count * sizeof(char16_t));
                    if (terminator != 0) static_cast<char16_t*>(target)[count] = 0;
                } else {
                    std::string bytes;
                    char unit[MB_LEN_MAX];
                    for (std::size_t index = 0; index < count; ++index) {
                        const std::size_t size = wcrtomb_nid_postfix(unit, input[index], nullptr);
                        if (size == static_cast<std::size_t>(-1)) return converted ? assigned : EOF;
                        bytes.append(unit, size);
                    }
                    if (bytes.size() + terminator > capacity) {
                        if (capacity != 0) static_cast<char*>(target)[0] = '\0';
                        return assigned;
                    }
                    std::memcpy(target, bytes.data(), bytes.size());
                    if (terminator != 0) static_cast<char*>(target)[bytes.size()] = '\0';
                }
            }
            input += count;
        } else {
            std::string field;
            for (const char16_t* cursor = input; *cursor != 0 && *cursor < 0x80 && !IsScanSpace(*cursor) &&
                (width == 0 || field.size() < width); ++cursor) field.push_back(static_cast<char>(*cursor));
            std::string directive = suppress ? "%*" : "%";
            directive += HostScanLength(length, conversion);
            directive += static_cast<char>(conversion);
            directive += "%n";
            int consumed = -1;
            const int matched = suppress ? std::sscanf(field.c_str(), directive.c_str(), &consumed) :
                std::sscanf(field.c_str(), directive.c_str(), target, &consumed);
            if (consumed < 0 || (!suppress && matched != 1)) return assigned;
            input += consumed;
        }
        converted = true;
        if (!suppress) ++assigned;
    }
    return assigned;
}

}  // namespace

extern "C" {
extern FileStream* __stdoutp_nid_postfix;
std::int32_t APS5_VABI fgetwc_nid_postfix(FileStream* stream);
int APS5_VABI ferror_nid_postfix(FileStream* stream);

int APS5_VABI vswprintf_nid_postfix(char16_t* buffer, std::size_t size, const char16_t* format, VaList* args) {
    if (buffer == nullptr || size == 0) { errno = 22; return -1; }
    try {
        const std::u16string text = FormatWide(format, args);
        const std::size_t copied = text.size() < size - 1 ? text.size() : size - 1;
        std::memcpy(buffer, text.data(), copied * sizeof(char16_t));
        buffer[copied] = 0;
        return copied == text.size() ? static_cast<int>(copied) : -1;
    } catch (const std::exception&) {
        buffer[0] = 0;
        errno = 22;
        return -1;
    }
}

int APS5_VABI swprintf_nid_postfix(char16_t* buffer, std::size_t size, const char16_t* format, ...) {
    APS5_VA_BEGIN(format);
    const int result = vswprintf_nid_postfix(buffer, size, format, reinterpret_cast<VaList*>(args));
    APS5_VA_END();
    return result;
}

int APS5_VABI snwprintf_s_nid_postfix(char16_t* buffer, std::size_t size, const char16_t* format, ...) {
    constexpr std::size_t RsizeMax = SIZE_MAX >> 1;
    int result = -1;
    if (buffer != nullptr && format != nullptr && size != 0 && size <= RsizeMax) {
        APS5_VA_BEGIN(format);
        try {
            const std::u16string text = FormatWide(format, reinterpret_cast<VaList*>(args), true);
            if (text.size() <= static_cast<std::size_t>(INT_MAX)) {
                const std::size_t copied = text.size() < size - 1 ? text.size() : size - 1;
                std::memcpy(buffer, text.data(), copied * sizeof(char16_t));
                buffer[copied] = 0;
                result = static_cast<int>(text.size());
            }
        } catch (const std::invalid_argument&) {
        }
        APS5_VA_END();
    }
    if (result < 0 && buffer != nullptr && size != 0 && size < RsizeMax) buffer[0] = 0;
    return result;
}

int APS5_VABI vswprintf_s_nid_postfix(char16_t* buffer, std::size_t size, const char16_t* format, VaList* args) {
    constexpr std::size_t RsizeMax = (SIZE_MAX >> 1) / sizeof(char16_t);
    int result = -1;
    if (buffer != nullptr && format != nullptr && args != nullptr && size != 0 && size <= RsizeMax) {
        try {
            const std::u16string text = FormatWide(format, args, true);
            if (text.size() + 1 <= size && text.size() <= static_cast<std::size_t>(INT_MAX)) {
                std::memcpy(buffer, text.data(), text.size() * sizeof(char16_t));
                buffer[text.size()] = 0;
                result = static_cast<int>(text.size());
            }
        } catch (const std::invalid_argument&) {
        }
    }
    if (result < 0 && buffer != nullptr && size != 0 && size <= RsizeMax) buffer[0] = 0;
    return result;
}

int APS5_VABI swprintf_s_nid_postfix(char16_t* buffer, std::size_t size, const char16_t* format, ...) {
    constexpr std::size_t RsizeMax = (SIZE_MAX >> 1) / sizeof(char16_t);
    int result = -1;
    if (buffer != nullptr && format != nullptr && size != 0 && size <= RsizeMax) {
        APS5_VA_BEGIN(format);
        try {
            const std::u16string text = FormatWide(format, reinterpret_cast<VaList*>(args), true);
            if (text.size() + 1 <= size && text.size() <= static_cast<std::size_t>(INT_MAX)) {
                std::memcpy(buffer, text.data(), text.size() * sizeof(char16_t));
                buffer[text.size()] = 0;
                result = static_cast<int>(text.size());
            }
        } catch (const std::invalid_argument&) {
        }
        APS5_VA_END();
    }
    if (result < 0 && buffer != nullptr && size != 0 && size <= RsizeMax) buffer[0] = 0;
    return result;
}

int APS5_VABI vsnwprintf_s_nid_postfix(char16_t* buffer, std::size_t size, const char16_t* format, VaList* args) {
    constexpr std::size_t RsizeMax = (SIZE_MAX >> 1) / sizeof(char16_t);
    int result = -1;
    if (buffer != nullptr && format != nullptr && args != nullptr && size != 0 && size <= RsizeMax) {
        try {
            const std::u16string text = FormatWide(format, args, true);
            if (text.size() <= static_cast<std::size_t>(INT_MAX)) {
                const std::size_t copied = text.size() < size - 1 ? text.size() : size - 1;
                std::memcpy(buffer, text.data(), copied * sizeof(char16_t));
                buffer[copied] = 0;
                result = static_cast<int>(text.size());
            }
        } catch (const std::invalid_argument&) {
        }
    }
    if (result < 0 && buffer != nullptr && size != 0 && size <= RsizeMax) buffer[0] = 0;
    return result;
}

int APS5_VABI vswscanf_s_nid_postfix(const char16_t* buffer, const char16_t* format, VaList* args) {
    if (buffer == nullptr || format == nullptr) return EOF;
    LibcDetail::FormatArguments arguments(args);
    return ScanWide(buffer, format, [&] { return arguments.Next<void*>(); }, [&] { return arguments.Next<unsigned int>(); });
}

int APS5_VABI swscanf_s_nid_postfix(const char16_t* buffer, const char16_t* format, ...) {
    APS5_VA_BEGIN(format);
    const int result = vswscanf_s_nid_postfix(buffer, format, reinterpret_cast<VaList*>(args));
    APS5_VA_END();
    return result;
}

int APS5_VABI vswscanf_nid_postfix(const char16_t* buffer, const char16_t* format, VaList* args) {
    if (buffer == nullptr || format == nullptr) throw std::invalid_argument("swscanf: null buffer or format");
    LibcDetail::FormatArguments arguments(args);
    const auto nextPointer = [&] {
        void* const target = arguments.Next<void*>();
        if (target == nullptr) throw std::invalid_argument("swscanf: null receiving pointer");
        return target;
    };
    return ScanWide(buffer, format, nextPointer, [] { return std::size_t{SIZE_MAX}; });
}

int APS5_VABI swscanf_nid_postfix(const char16_t* buffer, const char16_t* format, ...) {
    APS5_VA_BEGIN(format);
    const int result = vswscanf_nid_postfix(buffer, format, reinterpret_cast<VaList*>(args));
    APS5_VA_END();
    return result;
}

int APS5_VABI wprintf_nid_postfix(const char16_t* format, ...) {
    APS5_VA_BEGIN(format);
    int result = -1;
    try {
        const std::u16string text = FormatWide(format, reinterpret_cast<VaList*>(args));
        const std::string bytes = ToUtf8(text);
        if (std::fwrite(bytes.data(), 1, bytes.size(), GetNativeStream(__stdoutp_nid_postfix)) == bytes.size()) result = static_cast<int>(text.size());
    } catch (const std::exception&) {
        errno = 22;
    }
    APS5_VA_END();
    return result;
}

int APS5_VABI vfwprintf_nid_postfix(FileStream* stream, const char16_t* format, VaList* args) {
    if (stream == nullptr || format == nullptr) { errno = 22; return -1; }
    if (!stream->IsOpen()) { errno = 9; return -1; }
    int result = -1;
    try {
        const std::u16string text = FormatWide(format, args);
        const std::string bytes = ToUtf8(text);
        if (std::fwrite(bytes.data(), 1, bytes.size(), GetNativeStream(stream)) == bytes.size()) result = static_cast<int>(text.size());
    } catch (const std::exception&) {
        errno = 22;
    }
    stream->SyncStatus();
    return result;
}

int APS5_VABI fwprintf_nid_postfix(FileStream* stream, const char16_t* format, ...) {
    APS5_VA_BEGIN(format);
    const int result = vfwprintf_nid_postfix(stream, format, reinterpret_cast<VaList*>(args));
    APS5_VA_END();
    return result;
}

int APS5_VABI vwprintf_nid_postfix(const char16_t* format, VaList* args) {
    return vfwprintf_nid_postfix(__stdoutp_nid_postfix, format, args);
}

int APS5_VABI vfwprintf_s_nid_postfix(FileStream* stream, const char16_t* format, VaList* args) {
    if (stream == nullptr || format == nullptr || args == nullptr) return -1;
    if (!stream->IsOpen()) { errno = 9; return -1; }
    int result = -1;
    try {
        const std::u16string text = FormatWide(format, args, true);
        const std::string bytes = ToUtf8(text);
        if (text.size() <= static_cast<std::size_t>(INT_MAX) &&
            std::fwrite(bytes.data(), 1, bytes.size(), GetNativeStream(stream)) == bytes.size()) result = static_cast<int>(text.size());
    } catch (const std::invalid_argument&) {
    }
    stream->SyncStatus();
    return result;
}

int APS5_VABI vwprintf_s_nid_postfix(const char16_t* format, VaList* args) {
    return vfwprintf_s_nid_postfix(__stdoutp_nid_postfix, format, args);
}

int APS5_VABI fwprintf_s_nid_postfix(FileStream* stream, const char16_t* format, ...) {
    APS5_VA_BEGIN(format);
    const int result = vfwprintf_s_nid_postfix(stream, format, reinterpret_cast<VaList*>(args));
    APS5_VA_END();
    return result;
}

int APS5_VABI wprintf_s_nid_postfix(const char16_t* format, ...) {
    APS5_VA_BEGIN(format);
    const int result = vfwprintf_s_nid_postfix(__stdoutp_nid_postfix, format, reinterpret_cast<VaList*>(args));
    APS5_VA_END();
    return result;
}

int APS5_VABI fputwc_nid_postfix(char16_t value, FileStream* stream) {
    if (stream == nullptr) { errno = 22; return -1; }
    try {
        const std::string bytes = ToUtf8(std::u16string(1, value));
        auto* native = GetNativeStream(stream);
        if (std::fwrite(bytes.data(), 1, bytes.size(), native) != bytes.size()) throw std::runtime_error("stream write failed");
        stream->SyncStatus();
        return value;
    } catch (const std::exception&) {
        errno = 22;
        return -1;
    }
}

int APS5_VABI fputws_nid_postfix(const char16_t* str, FileStream* stream) {
    if (str == nullptr || stream == nullptr) { errno = 22; return -1; }
    try {
        const std::u16string text(str);
        const std::string bytes = ToUtf8(text);
        auto* native = GetNativeStream(stream);
        if (std::fwrite(bytes.data(), 1, bytes.size(), native) != bytes.size()) throw std::runtime_error("stream write failed");
        stream->SyncStatus();
        return static_cast<int>(text.size());
    } catch (const std::exception&) {
        errno = 22;
        return -1;
    }
}

int APS5_VABI putwc_nid_postfix(char16_t value, FileStream* stream) {
    return fputwc_nid_postfix(value, stream);
}

int APS5_VABI putwchar_nid_postfix(char16_t value) {
    return fputwc_nid_postfix(value, __stdoutp_nid_postfix);
}

char16_t* APS5_VABI fgetws_nid_postfix(char16_t* buffer, int size, FileStream* stream) {
    if (size < 2) return nullptr;
    if (buffer == nullptr || stream == nullptr) {
        if (stream != nullptr) stream->SetError();
        errno = 22;
        return nullptr;
    }
    int count = 0;
    while (count < size - 1) {
        const std::int32_t character = fgetwc_nid_postfix(stream);
        if (character == -1) break;
        buffer[count++] = static_cast<char16_t>(character);
        if (character == static_cast<std::int32_t>(u'\n')) break;
    }
    if (count == 0) return nullptr;
    buffer[count] = 0;
    return buffer;
}

}
