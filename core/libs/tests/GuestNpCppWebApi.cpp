#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceNpCppWebApi/include/CppWebApiObjects.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

using NpCppWebApi::Binary;

extern "C" {
std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common6Binary4sizeEv(const Binary*);
const void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common6Binary9getBinaryEv(const Binary*);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

void TestBinaryReportsItsBytes() {
    std::uint8_t bytes[] = {1, 2, 3, 4, 5};
    int context = 0;
    const Binary binary{1, bytes, sizeof(bytes), &context};
    Require(_ZNK3sce2Np9CppWebApi6Common6Binary9getBinaryEv(&binary) == bytes);
    Require(_ZNK3sce2Np9CppWebApi6Common6Binary4sizeEv(&binary) == sizeof(bytes));

    const Binary empty{1, nullptr, 0, &context};
    Require(_ZNK3sce2Np9CppWebApi6Common6Binary9getBinaryEv(&empty) == nullptr);
    Require(_ZNK3sce2Np9CppWebApi6Common6Binary4sizeEv(&empty) == 0);
}

}

int main() {
    TestBinaryReportsItsBytes();
    return 0;
}
