#include "framekit/runtime/fault_policy_runtime.hpp"

#include <array>
#include <string>

namespace framekit::runtime {

namespace {

std::size_t FaultClassIndex(FaultClass fault_class) {
    return static_cast<std::size_t>(fault_class);
}

std::string WithBudgetContext(
    const char* reason,
    std::uint32_t observed_count,
    std::uint32_t limit) {
    std::string message = reason;
    message += " (";
    message += std::to_string(observed_count);
    message += "/";
    message += std::to_string(limit);
    message += ")";
    return message;
}

} // namespace

const char* FaultClassName(FaultClass fault_class) {
    static constexpr std::array<const char*, 6> kNames = {
        "Bootstrap",
        "Module",
        "Event",
        "Render",
        "Timing",
        "Shutdown",
    };

    const auto index = FaultClassIndex(fault_class);
    if (index >= kNames.size()) {
        return "UnknownFaultClass";
    }

    return kNames[index];
}

bool FaultPolicyRuntime::Configure(const LoopPolicy& policy, FaultPolicyLimits limits) {
    const auto validation = ValidateLoopPolicy(policy);
    if (!validation.valid) {
        configured_ = false;
        last_decision_reason_ = "cannot configure fault policy runtime: ";
        last_decision_reason_ += validation.reason;
        return false;
    }

    policy_ = policy;
    limits_ = limits;
    counters_.fill(0);
    configured_ = true;
    last_decision_reason_.clear();
    return true;
}

FaultPolicyDecision FaultPolicyRuntime::Evaluate(const FaultPolicyContext& context) {
    if (!configured_) {
        return FailFast("fault policy runtime is not configured", false, 0);
    }

    if (context.invariant_breach) {
        const auto observed = IncrementAndGet(context.fault_class);
        return FailFast("always-fail-fast invariant breached", false, observed);
    }

    if (policy_.profile == LoopProfile::kDeterministicHost) {
        const auto observed = IncrementAndGet(context.fault_class);
        return FailFast("deterministic host profile enforces fail-fast fault handling", false, observed);
    }

    switch (context.fault_class) {
        case FaultClass::kBootstrap: {
            const auto observed = IncrementAndGet(FaultClass::kBootstrap);
            return FailFast("bootstrap faults are always fail-fast", false, observed);
        }
        case FaultClass::kModule:
            if (context.required_module || context.contract_violation) {
                const auto observed = IncrementAndGet(FaultClass::kModule);
                return FailFast("required or invalid module fault requires fail-fast", false, observed);
            }
            return DegradeWithLimit(
                FaultClass::kModule,
                limits_.module_optional_limit,
                "optional module fault degraded",
                "optional module degrade limit exceeded; escalating to fail-fast");
        case FaultClass::kEvent:
            if (context.contract_violation) {
                const auto observed = IncrementAndGet(FaultClass::kEvent);
                return FailFast("event contract violation requires fail-fast", false, observed);
            }
            return DegradeWithLimit(
                FaultClass::kEvent,
                limits_.event_noncritical_limit,
                "noncritical event fault degraded",
                "event degrade limit exceeded; escalating to fail-fast");
        case FaultClass::kRender:
            if (!policy_.rendering_enabled) {
                const auto observed = IncrementAndGet(FaultClass::kRender);
                return FailFast("render degrade unavailable when rendering is disabled", false, observed);
            }
            if (context.contract_violation) {
                const auto observed = IncrementAndGet(FaultClass::kRender);
                return FailFast("render contract violation requires fail-fast", false, observed);
            }
            return DegradeWithLimit(
                FaultClass::kRender,
                limits_.render_isolation_limit,
                "render fault degraded via isolation",
                "render degrade limit exceeded; escalating to fail-fast");
        case FaultClass::kTiming:
            if (policy_.overrun_mode == OverrunMode::kFailFast || context.contract_violation) {
                const auto observed = IncrementAndGet(FaultClass::kTiming);
                return FailFast("timing fault requires fail-fast under active policy", false, observed);
            }
            return DegradeWithLimit(
                FaultClass::kTiming,
                limits_.timing_overrun_limit,
                "timing fault degraded via bounded catch-up/slip",
                "timing degrade limit exceeded; escalating to fail-fast");
        case FaultClass::kShutdown:
            if (context.fatal_shutdown_violation || context.contract_violation) {
                const auto observed = IncrementAndGet(FaultClass::kShutdown);
                return FailFast("fatal shutdown fault requires fail-fast", false, observed);
            }
            return DegradeWithLimit(
                FaultClass::kShutdown,
                limits_.shutdown_nonfatal_limit,
                "non-fatal shutdown fault degraded",
                "shutdown degrade limit exceeded; escalating to fail-fast");
    }

    const auto observed = IncrementAndGet(context.fault_class);
    return FailFast("unknown fault class", false, observed);
}

void FaultPolicyRuntime::Reset() {
    counters_.fill(0);
    last_decision_reason_.clear();
}

bool FaultPolicyRuntime::IsConfigured() const {
    return configured_;
}

const LoopPolicy& FaultPolicyRuntime::Policy() const {
    return policy_;
}

const FaultPolicyLimits& FaultPolicyRuntime::Limits() const {
    return limits_;
}

std::uint32_t FaultPolicyRuntime::FaultCount(FaultClass fault_class) const {
    const auto index = FaultClassIndex(fault_class);
    if (index >= counters_.size()) {
        return 0;
    }
    return counters_[index];
}

const std::string& FaultPolicyRuntime::LastDecisionReason() const {
    return last_decision_reason_;
}

std::uint32_t FaultPolicyRuntime::IncrementAndGet(FaultClass fault_class) {
    const auto index = FaultClassIndex(fault_class);
    if (index >= counters_.size()) {
        return 0;
    }

    counters_[index] += 1;
    return counters_[index];
}

FaultPolicyDecision FaultPolicyRuntime::FailFast(
    std::string reason,
    bool escalated,
    std::uint32_t observed_count) {
    last_decision_reason_ = std::move(reason);
    return FaultPolicyDecision{
        .disposition = FaultDisposition::kFailFast,
        .escalated = escalated,
        .consumed_budget = false,
        .observed_count = observed_count,
        .reason = last_decision_reason_,
    };
}

FaultPolicyDecision FaultPolicyRuntime::DegradeWithLimit(
    FaultClass fault_class,
    std::uint32_t limit,
    const char* degrade_reason,
    const char* escalation_reason) {
    const auto observed = IncrementAndGet(fault_class);

    if (limit == 0 || observed > limit) {
        return FailFast(escalation_reason, true, observed);
    }

    last_decision_reason_ = WithBudgetContext(degrade_reason, observed, limit);
    return FaultPolicyDecision{
        .disposition = FaultDisposition::kDegrade,
        .escalated = false,
        .consumed_budget = true,
        .observed_count = observed,
        .reason = last_decision_reason_,
    };
}

} // namespace framekit::runtime
