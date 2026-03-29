#include "framekit/loop/stage_graph.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <set>

namespace framekit::runtime {

namespace {

const std::array<std::pair<LoopStage, const char*>, 12> kStageNameTable = {{
    {LoopStage::kBeginFrame, "BeginFrame"},
    {LoopStage::kProcessPlatformEvents, "ProcessPlatformEvents"},
    {LoopStage::kNormalizeInput, "NormalizeInput"},
    {LoopStage::kDispatchImmediateEvents, "DispatchImmediateEvents"},
    {LoopStage::kPreUpdate, "PreUpdate"},
    {LoopStage::kUpdate, "Update"},
    {LoopStage::kPostUpdate, "PostUpdate"},
    {LoopStage::kDispatchDeferredEvents, "DispatchDeferredEvents"},
    {LoopStage::kPreRender, "PreRender"},
    {LoopStage::kRender, "Render"},
    {LoopStage::kPostRender, "PostRender"},
    {LoopStage::kEndFrame, "EndFrame"},
}};

bool IsRenderStage(LoopStage stage) {
    return stage == LoopStage::kPreRender ||
        stage == LoopStage::kRender ||
        stage == LoopStage::kPostRender;
}

bool IsOptionalForProfile(LoopProfile profile, LoopStage stage) {
    if (!IsRenderStage(stage)) {
        return false;
    }

    return profile == LoopProfile::kHeadless ||
        profile == LoopProfile::kDeterministicHost;
}

bool ContainsStage(const std::vector<LoopStage>& stages, LoopStage stage) {
    return std::find(stages.begin(), stages.end(), stage) != stages.end();
}

} // namespace

const std::vector<LoopStage>& BaselineLoopStageOrder() {
    static const std::vector<LoopStage> kBaselineOrder = {
        LoopStage::kBeginFrame,
        LoopStage::kProcessPlatformEvents,
        LoopStage::kNormalizeInput,
        LoopStage::kDispatchImmediateEvents,
        LoopStage::kPreUpdate,
        LoopStage::kUpdate,
        LoopStage::kPostUpdate,
        LoopStage::kDispatchDeferredEvents,
        LoopStage::kPreRender,
        LoopStage::kRender,
        LoopStage::kPostRender,
        LoopStage::kEndFrame,
    };

    return kBaselineOrder;
}

const char* LoopStageName(LoopStage stage) {
    const auto found = std::find_if(
        kStageNameTable.begin(),
        kStageNameTable.end(),
        [stage](const auto& item) { return item.first == stage; });

    if (found == kStageNameTable.end()) {
        return "UnknownStage";
    }

    return found->second;
}

bool ParseLoopStageName(const std::string& name, LoopStage* out_stage) {
    if (!out_stage) {
        return false;
    }

    const auto found = std::find_if(
        kStageNameTable.begin(),
        kStageNameTable.end(),
        [&name](const auto& item) { return name == item.second; });

    if (found == kStageNameTable.end()) {
        return false;
    }

    *out_stage = found->first;
    return true;
}

bool LoopStageGraphRunner::Configure(const LoopPolicy& policy) {
    const auto validation = ValidateLoopPolicy(policy);
    if (!validation.valid) {
        last_error_ = validation.reason;
        configured_ = false;
        return false;
    }

    std::set<LoopStage> disabled;
    for (const auto& stage_name : policy.disabled_optional_stages) {
        LoopStage stage = LoopStage::kBeginFrame;
        if (!ParseLoopStageName(stage_name, &stage)) {
            last_error_ = "unknown loop stage in policy disable list: " + stage_name;
            configured_ = false;
            return false;
        }

        disabled.insert(stage);
    }

    active_stages_.clear();
    active_stages_.reserve(BaselineLoopStageOrder().size());
    for (const auto stage : BaselineLoopStageOrder()) {
        if (disabled.find(stage) != disabled.end()) {
            continue;
        }
        active_stages_.push_back(stage);
    }

    policy_ = policy;
    configured_ = true;
    last_error_.clear();
    last_executed_stages_.clear();
    last_update_step_count_ = 1;
    last_dropped_update_step_count_ = 0;
    last_frame_dropped_non_critical_stages_ = false;
    fixed_delta_accumulator_ns_ = 0;
    return true;
}

void LoopStageGraphRunner::SetStageHandler(LoopStage stage, StageHandler handler) {
    if (!handler) {
        handlers_.erase(stage);
        return;
    }

    handlers_[stage] = std::move(handler);
}

void LoopStageGraphRunner::ClearStageHandlers() {
    handlers_.clear();
}

bool LoopStageGraphRunner::ExecuteFrame(std::uint64_t elapsed_ns) {
    if (!configured_) {
        last_error_ = "loop stage graph runner is not configured";
        return false;
    }

    last_error_.clear();
    last_update_step_count_ = 1;
    last_dropped_update_step_count_ = 0;
    last_frame_dropped_non_critical_stages_ = false;
    bool should_drop_non_critical_stages = false;

    if (policy_.timing_mode == TimingMode::kFixedDelta) {
        if (policy_.fixed_delta_ns == 0) {
            last_error_ = "fixed-delta timing requires fixed_delta_ns > 0";
            return false;
        }

        const std::uint64_t observed_elapsed_ns = elapsed_ns == 0 ? policy_.fixed_delta_ns : elapsed_ns;
        fixed_delta_accumulator_ns_ += observed_elapsed_ns;

        std::uint64_t requested_steps_u64 = fixed_delta_accumulator_ns_ / policy_.fixed_delta_ns;
        if (requested_steps_u64 == 0) {
            requested_steps_u64 = 1;
        }

        requested_steps_u64 = std::min(
            requested_steps_u64,
            static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()));

        const auto requested_steps = static_cast<std::uint32_t>(requested_steps_u64);
        if (requested_steps > 1) {
            switch (policy_.overrun_mode) {
            case OverrunMode::kSlip:
                last_update_step_count_ = 1;
                last_dropped_update_step_count_ = requested_steps - 1;
                break;
            case OverrunMode::kCatchUpBounded: {
                const auto max_allowed_steps = static_cast<std::uint32_t>(1 + policy_.max_catch_up_steps);
                last_update_step_count_ = std::min(requested_steps, max_allowed_steps);
                last_dropped_update_step_count_ = requested_steps - last_update_step_count_;
                break;
            }
            case OverrunMode::kDropNonCritical:
                last_update_step_count_ = 1;
                last_dropped_update_step_count_ = requested_steps - 1;
                should_drop_non_critical_stages = true;
                break;
            case OverrunMode::kFailFast:
                last_error_ = "timing overrun: fixed-delta frame exceeded budget in fail-fast mode";
                return false;
            }
        }

        if (fixed_delta_accumulator_ns_ >= policy_.fixed_delta_ns) {
            fixed_delta_accumulator_ns_ %= policy_.fixed_delta_ns;
        }
    } else {
        fixed_delta_accumulator_ns_ = 0;
    }

    last_executed_stages_.clear();
    last_executed_stages_.reserve(
        active_stages_.size() + static_cast<std::size_t>(last_update_step_count_) * 2);

    const bool has_update = ContainsStage(active_stages_, LoopStage::kUpdate);
    const bool has_post_update = ContainsStage(active_stages_, LoopStage::kPostUpdate);

    auto execute_stage = [this](LoopStage stage) {
        last_executed_stages_.push_back(stage);

        const auto handler_it = handlers_.find(stage);
        if (handler_it != handlers_.end() && handler_it->second) {
            handler_it->second();
        }
    };

    for (const auto stage : active_stages_) {
        if (stage == LoopStage::kUpdate || stage == LoopStage::kPostUpdate) {
            continue;
        }

        if (stage == LoopStage::kPreUpdate) {
            for (std::uint32_t step = 0; step < last_update_step_count_; ++step) {
                execute_stage(LoopStage::kPreUpdate);
                if (has_update) {
                    execute_stage(LoopStage::kUpdate);
                }
                if (has_post_update) {
                    execute_stage(LoopStage::kPostUpdate);
                }
            }
            continue;
        }

        if (should_drop_non_critical_stages && IsOptionalForProfile(policy_.profile, stage)) {
            last_frame_dropped_non_critical_stages_ = true;
            continue;
        }

        execute_stage(stage);
    }

    return true;
}

bool LoopStageGraphRunner::IsConfigured() const {
    return configured_;
}

const LoopPolicy& LoopStageGraphRunner::Policy() const {
    return policy_;
}

const std::vector<LoopStage>& LoopStageGraphRunner::ActiveStages() const {
    return active_stages_;
}

const std::vector<LoopStage>& LoopStageGraphRunner::LastExecutedStages() const {
    return last_executed_stages_;
}

std::uint32_t LoopStageGraphRunner::LastUpdateStepCount() const {
    return last_update_step_count_;
}

std::uint32_t LoopStageGraphRunner::LastDroppedUpdateStepCount() const {
    return last_dropped_update_step_count_;
}

bool LoopStageGraphRunner::LastFrameDroppedNonCriticalStages() const {
    return last_frame_dropped_non_critical_stages_;
}

const std::string& LoopStageGraphRunner::LastError() const {
    return last_error_;
}

} // namespace framekit::runtime
