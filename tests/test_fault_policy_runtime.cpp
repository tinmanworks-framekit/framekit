#include "framekit/framekit.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

[[noreturn]] void FailRequirement(const char* expr, int line) {
    std::cerr << "Requirement failed at line " << line << ": " << expr << '\n';
    std::abort();
}

#define REQUIRE(expr)            \
    do {                         \
        if (!(expr)) {           \
            FailRequirement(#expr, __LINE__); \
        }                        \
    } while (false)

framekit::runtime::LoopPolicy MakeGuiPolicy() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kGui;
    policy.rendering_enabled = true;
    return policy;
}

framekit::runtime::LoopPolicy MakeHeadlessPolicy() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHeadless;
    policy.rendering_enabled = false;
    policy.disabled_optional_stages = {
        "PreRender",
        "Render",
        "PostRender",
    };
    return policy;
}

framekit::runtime::LoopPolicy MakeDeterministicPolicy() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kDeterministicHost;
    policy.timing_mode = framekit::runtime::TimingMode::kFixedDelta;
    policy.overrun_mode = framekit::runtime::OverrunMode::kFailFast;
    policy.rendering_enabled = false;
    policy.disabled_optional_stages = {
        "PreRender",
        "Render",
        "PostRender",
    };
    return policy;
}

void TestConfigurationRejectsInvalidPolicy() {
    framekit::runtime::FaultPolicyRuntime runtime;

    framekit::runtime::LoopPolicy invalid;
    invalid.profile = framekit::runtime::LoopProfile::kHeadless;
    invalid.rendering_enabled = false;

    REQUIRE(!runtime.Configure(invalid));
    REQUIRE(!runtime.IsConfigured());
}

void TestDeterministicProfileAlwaysFailFast() {
    framekit::runtime::FaultPolicyRuntime runtime;
    REQUIRE(runtime.Configure(MakeDeterministicPolicy()));

    const auto decision = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kEvent,
    });

    REQUIRE(decision.disposition == framekit::runtime::FaultDisposition::kFailFast);
    REQUIRE(!decision.escalated);
    REQUIRE(decision.reason.find("deterministic host profile") != std::string::npos);
    REQUIRE(runtime.FaultCount(framekit::runtime::FaultClass::kEvent) == 1);
}

void TestModuleDegradeEscalatesAfterLimit() {
    framekit::runtime::FaultPolicyRuntime runtime;
    framekit::runtime::FaultPolicyLimits limits;
    limits.module_optional_limit = 2;
    REQUIRE(runtime.Configure(MakeGuiPolicy(), limits));

    const auto first = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kModule,
    });
    REQUIRE(first.disposition == framekit::runtime::FaultDisposition::kDegrade);
    REQUIRE(!first.escalated);
    REQUIRE(first.consumed_budget);
    REQUIRE(first.observed_count == 1);

    const auto second = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kModule,
    });
    REQUIRE(second.disposition == framekit::runtime::FaultDisposition::kDegrade);
    REQUIRE(second.observed_count == 2);

    const auto third = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kModule,
    });
    REQUIRE(third.disposition == framekit::runtime::FaultDisposition::kFailFast);
    REQUIRE(third.escalated);
    REQUIRE(third.observed_count == 3);
    REQUIRE(runtime.FaultCount(framekit::runtime::FaultClass::kModule) == 3);
}

void TestRequiredModuleFaultAlwaysFailFast() {
    framekit::runtime::FaultPolicyRuntime runtime;
    REQUIRE(runtime.Configure(MakeGuiPolicy()));

    const auto decision = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kModule,
        .required_module = true,
    });

    REQUIRE(decision.disposition == framekit::runtime::FaultDisposition::kFailFast);
    REQUIRE(decision.reason.find("required or invalid module") != std::string::npos);
}

void TestRenderDegradeUnavailableWhenRenderingDisabled() {
    framekit::runtime::FaultPolicyRuntime runtime;
    REQUIRE(runtime.Configure(MakeHeadlessPolicy()));

    const auto decision = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kRender,
    });

    REQUIRE(decision.disposition == framekit::runtime::FaultDisposition::kFailFast);
    REQUIRE(decision.reason.find("render degrade unavailable") != std::string::npos);
}

void TestEventContractViolationAlwaysFailFast() {
    framekit::runtime::FaultPolicyRuntime runtime;
    REQUIRE(runtime.Configure(MakeGuiPolicy()));

    const auto decision = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kEvent,
        .contract_violation = true,
    });

    REQUIRE(decision.disposition == framekit::runtime::FaultDisposition::kFailFast);
    REQUIRE(decision.reason.find("event contract violation") != std::string::npos);
}

void TestTimingFailFastWhenOverrunModeIsFailFast() {
    framekit::runtime::FaultPolicyRuntime runtime;
    auto policy = MakeGuiPolicy();
    policy.overrun_mode = framekit::runtime::OverrunMode::kFailFast;
    REQUIRE(runtime.Configure(policy));

    const auto decision = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kTiming,
    });

    REQUIRE(decision.disposition == framekit::runtime::FaultDisposition::kFailFast);
    REQUIRE(decision.reason.find("timing fault requires fail-fast") != std::string::npos);
}

void TestInvariantBreachAlwaysFailFast() {
    framekit::runtime::FaultPolicyRuntime runtime;
    REQUIRE(runtime.Configure(MakeGuiPolicy()));

    const auto decision = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kShutdown,
        .invariant_breach = true,
    });

    REQUIRE(decision.disposition == framekit::runtime::FaultDisposition::kFailFast);
    REQUIRE(decision.reason.find("always-fail-fast invariant") != std::string::npos);
}

void TestShutdownDegradeEscalatesAfterLimit() {
    framekit::runtime::FaultPolicyRuntime runtime;
    framekit::runtime::FaultPolicyLimits limits;
    limits.shutdown_nonfatal_limit = 1;
    REQUIRE(runtime.Configure(MakeGuiPolicy(), limits));

    const auto first = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kShutdown,
    });
    REQUIRE(first.disposition == framekit::runtime::FaultDisposition::kDegrade);

    const auto second = runtime.Evaluate(framekit::runtime::FaultPolicyContext{
        .fault_class = framekit::runtime::FaultClass::kShutdown,
    });
    REQUIRE(second.disposition == framekit::runtime::FaultDisposition::kFailFast);
    REQUIRE(second.escalated);
    REQUIRE(second.reason.find("shutdown degrade limit exceeded") != std::string::npos);
}

} // namespace

int main() {
    TestConfigurationRejectsInvalidPolicy();
    TestDeterministicProfileAlwaysFailFast();
    TestModuleDegradeEscalatesAfterLimit();
    TestRequiredModuleFaultAlwaysFailFast();
    TestRenderDegradeUnavailableWhenRenderingDisabled();
    TestEventContractViolationAlwaysFailFast();
    TestTimingFailFastWhenOverrunModeIsFailFast();
    TestInvariantBreachAlwaysFailFast();
    TestShutdownDegradeEscalatesAfterLimit();
    return 0;
}
