#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_GUESTBACKTRACE_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_GUESTBACKTRACE_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace GuestBacktrace {

inline constexpr std::size_t GuestBacktraceDepth = 64;

extern "C" {

std::vector<std::string> GuestBacktrace_nid_no_patch(const void* frame);

}

}

#endif
