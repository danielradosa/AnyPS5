#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>
#include <limits>

extern "C" {
short APS5_VABI _FDsign_nid_postfix(float value);
short APS5_VABI _Dsign_nid_postfix(double value);
}

static void Require(bool value) { if (!value) std::abort(); }

int main() {
    Require(_FDsign_nid_postfix(1.0f) == 0);
    Require(_FDsign_nid_postfix(-1.0f) == static_cast<short>(0x8000));
    Require(_FDsign_nid_postfix(-0.0f) == static_cast<short>(0x8000));
    Require(_FDsign_nid_postfix(-std::numeric_limits<float>::quiet_NaN()) == static_cast<short>(0x8000));

    Require(_Dsign_nid_postfix(1.0) == 0);
    Require(_Dsign_nid_postfix(-1.0) == static_cast<short>(0x8000));
    Require(_Dsign_nid_postfix(-0.0) == static_cast<short>(0x8000));
    Require(_Dsign_nid_postfix(-std::numeric_limits<double>::infinity()) == static_cast<short>(0x8000));
    Require(_Dsign_nid_postfix(std::numeric_limits<double>::infinity()) == 0);
    return 0;
}
