#include "framekit/runtime/platform_host_runtime.hpp"

#include <algorithm>

namespace framekit::runtime {

void PlatformHostRuntime::SetPlatformHost(std::shared_ptr<IPlatformHost> host) {
    platform_host_ = std::move(host);
}

void PlatformHostRuntime::SetWindowHost(std::shared_ptr<IWindowHost> host) {
    window_host_ = std::move(host);
}

void PlatformHostRuntime::SetInputRuntime(std::shared_ptr<InputRoutingRuntime> input_runtime) {
    input_runtime_ = std::move(input_runtime);
}

bool PlatformHostRuntime::Configure(const LoopPolicy& policy, WindowSpec primary_window) {
    if (running_) {
        last_error_ = "cannot configure platform runtime while running";
        configured_ = false;
        return false;
    }

    if (!loop_runner_.Configure(policy)) {
        last_error_ = loop_runner_.LastError();
        configured_ = false;
        return false;
    }

    policy_ = policy;
    primary_window_ = std::move(primary_window);

    if (!fault_policy_.Configure(policy_)) {
        last_error_ = fault_policy_.LastDecisionReason();
        configured_ = false;
        return false;
    }

    fault_policy_.Reset();

    loop_runner_.ClearStageHandlers();
    loop_runner_.SetStageHandler(LoopStage::kProcessPlatformEvents, [this]() {
        if (!platform_host_) {
            HandleStageFault(
                LoopStage::kProcessPlatformEvents,
                FaultPolicyContext{
                    .fault_class = FaultClass::kEvent,
                    .contract_violation = true,
                },
                "platform host is not set");
            return;
        }

        if (!platform_host_->PumpEvents()) {
            HandleStageFault(
                LoopStage::kProcessPlatformEvents,
                FaultPolicyContext{
                    .fault_class = FaultClass::kEvent,
                    .contract_violation = true,
                },
                "platform host failed to pump events");
        }
    });

    loop_runner_.SetStageHandler(LoopStage::kNormalizeInput, [this]() {
        if (!input_runtime_) {
            return;
        }

        if (!input_runtime_->NormalizePending()) {
            const auto& error = input_runtime_->LastError();
            if (error.empty()) {
                HandleStageFault(
                    LoopStage::kNormalizeInput,
                    FaultPolicyContext{
                        .fault_class = FaultClass::kEvent,
                        .contract_violation = true,
                    },
                    "input normalization failed");
                return;
            }

            HandleStageFault(
                LoopStage::kNormalizeInput,
                FaultPolicyContext{
                    .fault_class = FaultClass::kEvent,
                    .contract_violation = true,
                },
                error);
        }
    });

    loop_runner_.SetStageHandler(LoopStage::kDispatchImmediateEvents, [this]() {
        if (!input_runtime_) {
            return;
        }

        (void)input_runtime_->DispatchImmediate();
    });

    loop_runner_.SetStageHandler(LoopStage::kPreRender, [this]() {
        if (!window_host_) {
            HandleStageFault(
                LoopStage::kPreRender,
                FaultPolicyContext{
                    .fault_class = FaultClass::kRender,
                    .contract_violation = true,
                },
                "window host is not set");
            return;
        }

        if (!window_host_->PreRender()) {
            HandleStageFault(
                LoopStage::kPreRender,
                FaultPolicyContext{
                    .fault_class = FaultClass::kRender,
                },
                "window host pre-render failed");
        }
    });

    loop_runner_.SetStageHandler(LoopStage::kRender, [this]() {
        if (!window_host_) {
            HandleStageFault(
                LoopStage::kRender,
                FaultPolicyContext{
                    .fault_class = FaultClass::kRender,
                    .contract_violation = true,
                },
                "window host is not set");
            return;
        }

        if (!window_host_->Render()) {
            HandleStageFault(
                LoopStage::kRender,
                FaultPolicyContext{
                    .fault_class = FaultClass::kRender,
                },
                "window host render failed");
        }
    });

    loop_runner_.SetStageHandler(LoopStage::kPostRender, [this]() {
        if (!window_host_) {
            HandleStageFault(
                LoopStage::kPostRender,
                FaultPolicyContext{
                    .fault_class = FaultClass::kRender,
                    .contract_violation = true,
                },
                "window host is not set");
            return;
        }

        if (!window_host_->PostRender()) {
            HandleStageFault(
                LoopStage::kPostRender,
                FaultPolicyContext{
                    .fault_class = FaultClass::kRender,
                },
                "window host post-render failed");
        }
    });

    frame_failed_ = false;
    last_error_.clear();
    configured_ = true;
    return true;
}

bool PlatformHostRuntime::Start() {
    if (!configured_) {
        last_error_ = "platform runtime is not configured";
        return false;
    }

    if (running_) {
        last_error_ = "platform runtime is already running";
        return false;
    }

    if (!platform_host_) {
        last_error_ = "platform host is required";
        return false;
    }

    if (!platform_host_->Initialize()) {
        last_error_ = "platform host initialization failed";
        return false;
    }

    if (HasActiveRenderStages()) {
        if (!window_host_) {
            last_error_ = "window host is required when render stages are active";
            return false;
        }

        if (!window_host_->CreatePrimaryWindow(primary_window_)) {
            last_error_ = "window host failed to create primary window";
            return false;
        }

        window_created_ = true;
    }

    running_ = true;
    last_error_.clear();
    return true;
}

bool PlatformHostRuntime::Tick() {
    if (!running_) {
        last_error_ = "platform runtime is not running";
        return false;
    }

    frame_failed_ = false;
    last_error_.clear();

    if (!loop_runner_.ExecuteFrame()) {
        last_error_ = loop_runner_.LastError();
        return false;
    }

    return !frame_failed_;
}

bool PlatformHostRuntime::Stop() {
    if (!running_) {
        return true;
    }

    bool ok = true;
    if (window_created_ && window_host_) {
        if (!window_host_->TeardownWindows()) {
            ok = false;
            if (last_error_.empty()) {
                last_error_ = "window host teardown failed";
            }
        }
    }

    if (!platform_host_->Teardown()) {
        ok = false;
        if (last_error_.empty()) {
            last_error_ = "platform host teardown failed";
        }
    }

    running_ = false;
    window_created_ = false;
    frame_failed_ = false;
    return ok;
}

bool PlatformHostRuntime::IsConfigured() const {
    return configured_;
}

bool PlatformHostRuntime::IsRunning() const {
    return running_;
}

const std::vector<LoopStage>& PlatformHostRuntime::ActiveStages() const {
    return loop_runner_.ActiveStages();
}

const std::vector<LoopStage>& PlatformHostRuntime::LastFrameStages() const {
    return loop_runner_.LastExecutedStages();
}

const std::string& PlatformHostRuntime::LastError() const {
    return last_error_;
}

bool PlatformHostRuntime::HasActiveRenderStages() const {
    const auto& active_stages = loop_runner_.ActiveStages();
    return std::find(active_stages.begin(), active_stages.end(), LoopStage::kPreRender) != active_stages.end() ||
        std::find(active_stages.begin(), active_stages.end(), LoopStage::kRender) != active_stages.end() ||
        std::find(active_stages.begin(), active_stages.end(), LoopStage::kPostRender) != active_stages.end();
}

void PlatformHostRuntime::HandleStageFault(
    LoopStage stage,
    FaultPolicyContext context,
    const std::string& message) {
    const auto decision = fault_policy_.Evaluate(context);

    if (decision.disposition == FaultDisposition::kDegrade) {
        if (!last_error_.empty()) {
            return;
        }

        last_error_ = "loop stage ";
        last_error_ += LoopStageName(stage);
        last_error_ += " degraded: ";
        last_error_ += decision.reason;
        last_error_ += "; detail: ";
        last_error_ += message;
        return;
    }

    frame_failed_ = true;
    if (!last_error_.empty()) {
        return;
    }

    last_error_ = "loop stage ";
    last_error_ += LoopStageName(stage);
    last_error_ += " failed: ";
    last_error_ += message;
    last_error_ += "; policy: ";
    last_error_ += decision.reason;
}

} // namespace framekit::runtime
