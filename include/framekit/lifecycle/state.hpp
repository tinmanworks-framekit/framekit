#pragma once

#include <cstdint>

namespace framekit::runtime {

enum class LifecycleState : std::uint8_t {
    kBootstrapping = 0,
    kInitializing = 1,
    kRunning = 2,
    kStopping = 3,
    kStopped = 4,
    kFaulted = 5,
};

} // namespace framekit::runtime
