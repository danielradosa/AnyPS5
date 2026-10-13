#ifndef CORE_LIBS_PRX_LIBKERNEL_FILE_HOSTPIPE_HPP
#define CORE_LIBS_PRX_LIBKERNEL_FILE_HOSTPIPE_HPP

#include <cstdint>
#include <optional>

namespace File {

struct HostPipeState {
    bool readEnd = false;
    bool writeEnd = false;
    std::int64_t available = 0;
    std::int64_t space = 0;
    bool peerClosed = false;
};

std::optional<HostPipeState> QueryHostPipe(int descriptor);

}

#endif
