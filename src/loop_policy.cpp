#include "framekit/runtime/loop_policy.hpp"

#include <algorithm>
#include <array>

namespace framekit::runtime {

namespace {

bool ContainsStage(const std::vector<std::string>& stages, const std::string& target) {
    return std::find(stages.begin(), stages.end(), target) != stages.end();
}

bool IsKnownStage(const std::string& stage) {
    static const std::array<const char*, 12> kBaselineStages = {
        "BeginFrame",
        "ProcessPlatformEvents",
        "NormalizeInput",
        "DispatchImmediateEvents",
        "PreUpdate",
        "Update",
        "PostUpdate",
        "DispatchDeferredEvents",
        "PreRender",
        "Render",
        "PostRender",
        "EndFrame",
    };

    return std::any_of(
        kBaselineStages.begin(),
        kBaselineStages.end(),
        [&stage](const char* candidate) { return stage == candidate; });
}

bool IsRenderStage(const std::string& stage) {
    return stage == "PreRender" || stage == "Render" || stage == "PostRender";
}

bool IsAlwaysRequiredStage(const std::string& stage) {
    if (!IsKnownStage(stage)) {
        return false;
    }

    return !IsRenderStage(stage);
}

bool IsOptionalForProfile(LoopProfile profile, const std::string& stage) {
    if (!IsRenderStage(stage)) {
        return false;
    }

    return profile == LoopProfile::kHeadless || profile == LoopProfile::kDeterministicHost;
}

LoopPolicyValidation Invalid(std::string reason) {
    return LoopPolicyValidation{.valid = false, .reason = std::move(reason)};
}

} // namespace

LoopPolicyValidation ValidateLoopPolicy(const LoopPolicy& policy) {
    if (policy.timing_mode == TimingMode::kFixedDelta &&
        policy.overrun_mode == OverrunMode::kCatchUpBounded &&
        policy.max_catch_up_steps == 0) {
        return Invalid("fixed_delta + catch_up_bounded requires max_catch_up_steps > 0");
    }

    if ((policy.profile == LoopProfile::kGui || policy.profile == LoopProfile::kHybrid) &&
        !policy.rendering_enabled) {
        return Invalid("GUI and Hybrid profiles require rendering stages");
    }

    for (const auto& stage : policy.disabled_optional_stages) {
        if (!IsKnownStage(stage)) {
            return Invalid("disabled_optional_stages contains an unknown stage: " + stage);
        }

        if (IsAlwaysRequiredStage(stage)) {
            return Invalid("required stage cannot be disabled: " + stage);
        }

        if (!IsOptionalForProfile(policy.profile, stage)) {
            return Invalid("stage is not optional for active profile: " + stage);
        }
    }

    if (!policy.rendering_enabled) {
        if (!ContainsStage(policy.disabled_optional_stages, "PreRender") ||
            !ContainsStage(policy.disabled_optional_stages, "Render") ||
            !ContainsStage(policy.disabled_optional_stages, "PostRender")) {
            return Invalid("rendering disablement must declare PreRender, Render, and PostRender");
        }
    }

    return LoopPolicyValidation{.valid = true, .reason = {}};
}

} // namespace framekit::runtime
