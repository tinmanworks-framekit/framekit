#include "framekit/loop/stage_graph.hpp"

#include <algorithm>
#include <cstddef>
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

bool LoopStageGraphRunner::ExecuteFrame() {
    if (!configured_) {
        last_error_ = "loop stage graph runner is not configured";
        return false;
    }

    last_executed_stages_.clear();
    last_executed_stages_.reserve(active_stages_.size());

    for (const auto stage : active_stages_) {
        last_executed_stages_.push_back(stage);

        const auto handler_it = handlers_.find(stage);
        if (handler_it != handlers_.end() && handler_it->second) {
            handler_it->second();
        }
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

const std::string& LoopStageGraphRunner::LastError() const {
    return last_error_;
}

} // namespace framekit::runtime
