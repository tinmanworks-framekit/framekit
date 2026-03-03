#pragma once

#include <cstdint>

namespace framekit {

enum class IpcTransportKind : std::uint8_t {
    kNone = 0,
    kShmPipe = 1,
    kLocalSocket = 2,
    kCustom = 255
};

} // namespace framekit
