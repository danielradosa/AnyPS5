#include "prx/libc/include/general/VabiMacros.hpp"

#include <algorithm>
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <limits>

static_assert(sizeof(long double) == 16 && std::numeric_limits<long double>::digits == 64);
static_assert(sizeof(long long) == sizeof(std::int64_t));

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

long double DomainError_nid_no_patch() {
    errno = EDOM;
    std::feraiseexcept(FE_INVALID);
    return std::numeric_limits<long double>::quiet_NaN();
}

long double CheckRange_nid_no_patch(long double input, long double result) {
    if (std::isfinite(input)) {
        if (std::isinf(result)) {
            errno = ERANGE;
            std::feraiseexcept(FE_OVERFLOW | FE_INEXACT);
        } else if (result == 0.L && input != 0.L) {
            errno = ERANGE;
            std::feraiseexcept(FE_UNDERFLOW | FE_INEXACT);
        }
    }
    return result;
}

long double CheckNeighbor_nid_no_patch(long double from, long double to, long double result) {
    if (from == to || std::isnan(result)) return result;
    if (std::isfinite(from) && std::isinf(result)) {
        errno = ERANGE;
        std::feraiseexcept(FE_OVERFLOW | FE_INEXACT);
    } else if (result == 0.L || std::fpclassify(result) == FP_SUBNORMAL) {
        errno = ERANGE;
        std::feraiseexcept(FE_UNDERFLOW | FE_INEXACT);
    }
    return result;
}

}

extern "C" {

long double APS5_VABI acosl_nid_postfix(long double value) { return std::acos(value); }
long double APS5_VABI atanl_nid_postfix(long double value) { return std::atan(value); }
long double APS5_VABI atan2l_nid_postfix(long double y, long double x) { return std::atan2(y, x); }
long double APS5_VABI tanl_nid_postfix(long double value) { return std::tan(value); }

long double APS5_VABI acoshl_nid_postfix(long double value) {
    return value < 1.L ? DomainError_nid_no_patch() : std::acosh(value);
}
long double APS5_VABI asinhl_nid_postfix(long double value) { return std::asinh(value); }
long double APS5_VABI atanhl_nid_postfix(long double value) {
    if (std::fabs(value) > 1.L) {
        return DomainError_nid_no_patch();
    }
    if (std::fabs(value) == 1.L) {
        errno = ERANGE;
        std::feraiseexcept(FE_DIVBYZERO);
        return std::copysign(std::numeric_limits<long double>::infinity(), value);
    }
    return std::atanh(value);
}
long double APS5_VABI coshl_nid_postfix(long double value) { return CheckRange_nid_no_patch(value, std::cosh(value)); }
long double APS5_VABI sinhl_nid_postfix(long double value) { return CheckRange_nid_no_patch(value, std::sinh(value)); }
long double APS5_VABI tanhl_nid_postfix(long double value) { return std::tanh(value); }

long double APS5_VABI expl_nid_postfix(long double value) { return CheckRange_nid_no_patch(value, std::exp(value)); }
long double APS5_VABI exp2l_nid_postfix(long double value) { return std::exp2(value); }
long double APS5_VABI expm1l_nid_postfix(long double value) { return CheckRange_nid_no_patch(value, std::expm1(value)); }
long double APS5_VABI log10l_nid_postfix(long double value) { return std::log10(value); }
long double APS5_VABI log1pl_nid_postfix(long double value) { return std::log1p(value); }
long double APS5_VABI log2l_nid_postfix(long double value) { return std::log2(value); }

long double APS5_VABI cbrtl_nid_postfix(long double value) { return std::cbrt(value); }
long double APS5_VABI fabsl_nid_postfix(long double value) { return std::fabs(value); }
long double APS5_VABI fmaxl_nid_postfix(long double x, long double y) {
    if (x == 0.L && y == 0.L) return std::signbit(x) ? y : x;
    return std::fmax(x, y);
}
long double APS5_VABI fminl_nid_postfix(long double x, long double y) {
    if (x == 0.L && y == 0.L) return std::signbit(x) ? x : y;
    return std::fmin(x, y);
}

long double APS5_VABI ceill_nid_postfix(long double value) { return std::ceil(value); }
long double APS5_VABI floorl_nid_postfix(long double value) { return std::floor(value); }
long double APS5_VABI nearbyintl_nid_postfix(long double value) { return std::nearbyint(value); }

long double APS5_VABI nextafterl_nid_postfix(long double from, long double to) { return CheckNeighbor_nid_no_patch(from, to, std::nextafter(from, to)); }
long double APS5_VABI nexttowardl_nid_postfix(long double from, long double to) { return CheckNeighbor_nid_no_patch(from, to, std::nexttoward(from, to)); }

long double APS5_VABI frexpl_nid_postfix(long double value, int* exponent) { return std::isfinite(value) ? std::frexp(value, exponent) : value; }
long double APS5_VABI ldexpl_nid_postfix(long double value, int exponent) { return std::ldexp(value, exponent); }
long double APS5_VABI scalbnl_nid_postfix(long double value, int exponent) { return std::scalbn(value, exponent); }

}
