#pragma once

#include "framekit/loop/policy.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace framekit::runtime {

enum class FaultClass : std::uint8_t {
    kBootstrap = 0,
    kModule = 1,
    kEvent = 2,
    kRender = 3,
    kTiming = 4,
    kShutdown = 5,
};

const char* FaultClassName(FaultClass fault_class);

enum class FaultDisposition : std::uint8_t {
    kFailFast = 0,
    kDegrade = 1,
};

struct FaultPolicyLimits {
    std::uint32_t module_optional_limit = 1;
    std::uint32_t event_noncritical_limit = 3;
    std::uint32_t render_isolation_limit = 2;
    std::uint32_t timing_overrun_limit = 3;
    std::uint32_t shutdown_nonfatal_limit = 1;
};

struct FaultPolicyContext {
    FaultClass fault_class = FaultClass::kBootstrap;
    bool invariant_breach = false;
    bool contract_violation = false;
    bool required_module = false;
    bool fatal_shutdown_violation = false;
};

struct FaultPolicyDecision {
    FaultDisposition disposition = FaultDisposition::kFailFast;
    bool escalated = false;
    bool consumed_budget = false;
    std::uint32_t observed_count = 0;
    std::string reason;
};

class FaultPolicyRuntime {
public:
    bool Configure(const LoopPolicy& policy, FaultPolicyLimits limits = {});
    FaultPolicyDecision Evaluate(const FaultPolicyContext& context);

    void Reset();

    bool IsConfigured() const;
    const LoopPolicy& Policy() const;
    const FaultPolicyLimits& Limits() const;
    std::uint32_t FaultCount(FaultClass fault_class) const;
    const std::string& LastDecisionReason() const;

private:
    static constexpr std::size_t kFaultClassCount = 6;

    std::uint32_t IncrementAndGet(FaultClass fault_class);
    FaultPolicyDecision FailFast(std::string reason, bool escalated, std::uint32_t observed_count);
    FaultPolicyDecision DegradeWithLimit(
        FaultClass fault_class,
        std::uint32_t limit,
        const char* degrade_reason,
        const char* escalation_reason);

    LoopPolicy policy_;
    FaultPolicyLimits limits_;
    std::array<std::uint32_t, kFaultClassCount> counters_{};
    bool configured_ = false;
    std::string last_decision_reason_;
};

} // namespace framekit::runtime
