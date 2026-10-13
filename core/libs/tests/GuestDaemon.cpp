#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>
extern "C" int APS5_VABI daemon_nid_postfix(int, int);
extern "C" int* APS5_VABI __error_nid_postfix();
static void Require(bool value) { if (!value) std::abort(); }
static bool RefusesToFork(int noChdir, int noClose) {
    *__error_nid_postfix() = 0;
    return daemon_nid_postfix(noChdir, noClose) == -1 && *__error_nid_postfix() == 78;
}
int main() {
    Require(RefusesToFork(1, 0));
    Require(RefusesToFork(0, 0));
    Require(RefusesToFork(1, 1));
    Require(RefusesToFork(0, 1));
}
