#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace framekit::runtime {

enum class LoopProfile : std::uint8_t {
    kGui = 0,
    kHeadless = 1,
    kHybrid = 2,
    kDeterministicHost = 3,
};

enum class TimingMode : std::uint8_t {
    kVariableDelta = 0,
    kFixedDelta = 1,
    kExternallyClocked = 2,
};

enum class OverrunMode : std::uint8_t {
    kSlip = 0,
    kCatchUpBounded = 1,
    kDropNonCritical = 2,
    kFailFast = 3,
};

enum class ThreadingMode : std::uint8_t {
    kSingleThreaded = 0,
    kSplitUpdateRender = 1,
    kHostManaged = 2,
};

struct LoopPolicy {
    LoopProfile profile = LoopProfile::kGui;
    TimingMode timing_mode = TimingMode::kVariableDelta;
    OverrunMode overrun_mode = OverrunMode::kSlip;
    ThreadingMode threading_mode = ThreadingMode::kSingleThreaded;
    std::uint32_t max_catch_up_steps = 0;
    bool rendering_enabled = true;
    std::vector<std::string> disabled_optional_stages;
};

struct LoopPolicyValidation {
    bool valid = false;
    std::string reason;
};

LoopPolicyValidation ValidateLoopPolicy(const LoopPolicy& policy);

} // namespace framekit::runtime
