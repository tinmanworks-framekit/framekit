#pragma once

#include "framekit/process_role.hpp"

#include <cstdint>
#include <string>

namespace framekit::runtime {

enum class ChildHandshakeState : std::uint8_t {
    kNotStarted = 0,
    kSpawned = 1,
    kHelloReceived = 2,
    kReady = 3,
    kFailed = 4,
    kTimedOut = 5,
};

struct ChildHandshakeStatus {
    framekit::ProcessIdentity child;
    ChildHandshakeState state = ChildHandshakeState::kNotStarted;
    std::string detail;
};

} // namespace framekit::runtime
