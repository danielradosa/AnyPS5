#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

#include "prx/libc/include/General.hpp"

namespace {

class ErrnoGuard {
public:
    ErrnoGuard() : saved(errno) {}
    ~ErrnoGuard() { errno = saved; }
    ErrnoGuard(const ErrnoGuard&) = delete;
    ErrnoGuard& operator=(const ErrnoGuard&) = delete;

private:
    int saved;
};

struct ExtendedBits {
    std::uint64_t mantissa;
    std::uint16_t signExponent;
};

ExtendedBits Bits(const long double& value) {
    ExtendedBits bits;
    unsigned char bytes[sizeof(long double)];
    std::memcpy(bytes, &value, sizeof(bytes));
    std::memcpy(&bits.mantissa, bytes, sizeof(bits.mantissa));
    std::memcpy(&bits.signExponent, bytes + 8, sizeof(bits.signExponent));
    return bits;
}

constexpr short Denormal = -2, Finite = -1, Zero = 0, Infinite = 1, NotANumber = 2;

short TestExtended(const ExtendedBits& bits) {
    const int exponent = bits.signExponent & 0x7fff;
    const std::uint64_t fraction = bits.mantissa & 0x7fffffffffffffffull;
    if (exponent == 0x7fff) return fraction != 0 ? NotANumber : Infinite;
    if (exponent != 0 || (bits.mantissa >> 63) != 0) return Finite;
    return bits.mantissa != 0 ? Denormal : Zero;
}

template<typename TFloat>
short TestFloat(TFloat value) {
    switch (std::fpclassify(value)) {
    case FP_NAN:
        return NotANumber;
    case FP_INFINITE:
        return Infinite;
    case FP_ZERO:
        return Zero;
    case FP_SUBNORMAL:
        return Denormal;
    default:
        return Finite;
    }
}

template<typename TFloat>
TFloat Remainder(TFloat x, TFloat y) {
    const bool domain = !std::isnan(x) && !std::isnan(y) && (std::isinf(x) || y == 0);
    TFloat result;
    {
        ErrnoGuard guard;
        result = std::remainder(x, y);
    }
    if (domain) errno = EDOM;
    return result;
}

template<typename TFloat>
TFloat Scale(TFloat x, TFloat exponent) {
    ErrnoGuard guard;
    if (std::isnan(x) || std::isnan(exponent)) return x * exponent;
    if (std::isinf(exponent)) return exponent > 0 ? x * exponent : x / -exponent;
    if (std::rint(exponent) != exponent) return (exponent - exponent) / (exponent - exponent);
    if (exponent > TFloat(65000)) return std::scalbn(x, 65000);
    if (exponent < TFloat(-65000)) return std::scalbn(x, -65000);
    return std::scalbn(x, static_cast<int>(exponent));
}

}

extern "C" {

int APS5_VABI ilogbl_nid_postfix(long double x) {
    if ((Bits(x).signExponent & 0x7fff) == 0x7fff) return INT_MAX;
    ErrnoGuard guard;
    return std::ilogb(x);
}

double APS5_VABI drem_nid_postfix(double x, double y) { return Remainder(x, y); }
float APS5_VABI dremf_nid_postfix(float x, float y) { return Remainder(x, y); }

double APS5_VABI scalb_nid_postfix(double x, double exponent) { return Scale(x, exponent); }
float APS5_VABI scalbf_nid_postfix(float x, float exponent) { return Scale(x, exponent); }

int APS5_VABI finite_nid_postfix(double x) { return std::isfinite(x) ? 1 : 0; }
int APS5_VABI finitef_nid_postfix(float x) { return std::isfinite(x) ? 1 : 0; }

int APS5_VABI __fpclassifyl_nid_postfix(long double x) {
    const ExtendedBits bits = Bits(x);
    if ((bits.signExponent & 0x7fff) != 0 || bits.mantissa == 0) return TestExtended(bits);
    return Denormal;
}
int APS5_VABI __isinfl_nid_postfix(long double x) { return TestExtended(Bits(x)) == Infinite ? 1 : 0; }
int APS5_VABI __isnanl_nid_postfix(long double x) { return TestExtended(Bits(x)) == NotANumber ? 1 : 0; }

int APS5_VABI __isnormall_nid_postfix(long double x) {
    const int exponent = Bits(x).signExponent & 0x7fff;
    return exponent != 0 && exponent != 0x7fff ? 1 : 0;
}

short APS5_VABI _LDtest_nid_postfix(const long double* value) { return TestExtended(Bits(*value)); }
int APS5_VABI _Dclass_nid_postfix(double x) { return TestFloat(x); }
int APS5_VABI _FDclass_nid_postfix(float x) { return TestFloat(x); }
int APS5_VABI _LDclass_nid_postfix(long double x) { return TestExtended(Bits(x)); }
short APS5_VABI _LDsign_nid_postfix(long double x) { return static_cast<short>(Bits(x).signExponent & 0x8000); }

}
