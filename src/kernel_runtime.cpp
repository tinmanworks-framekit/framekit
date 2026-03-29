#include "framekit/kernel/runtime.hpp"

#include <algorithm>

namespace framekit::runtime {

void KernelRuntime::SetBootstrapStep(BootstrapStep step) {
    bootstrap_step_ = std::move(step);
}

void KernelRuntime::SetInitializeStep(InitializeStep step) {
    initialize_step_ = std::move(step);
}

void KernelRuntime::SetModuleStopHandler(ModuleStopHandler handler) {
    module_stop_handler_ = std::move(handler);
}

void KernelRuntime::SetDiagnosticsFlushStep(DiagnosticsFlushStep step) {
    diagnostics_flush_step_ = std::move(step);
}

void KernelRuntime::SetEventBus(std::shared_ptr<EventBus> event_bus) {
    event_bus_ = std::move(event_bus);
}

void KernelRuntime::SetShutdownDeferredEventPolicy(
    ShutdownDeferredEventPolicy policy,
    std::size_t bounded_limit) {
    deferred_event_policy_ = policy;
    deferred_event_bounded_limit_ = bounded_limit;
}

void KernelRuntime::AddPlatformTeardownStep(std::string name, PlatformTeardownStep step) {
    platform_teardown_steps_.push_back(NamedTeardownStep{
        .name = std::move(name),
        .step = std::move(step),
    });
}

void KernelRuntime::RecordStartedModules(std::vector<std::string> startup_order) {
    started_modules_ = std::move(startup_order);
}

bool KernelRuntime::Start() {
    if (!lifecycle_.TransitionTo(LifecycleState::kBootstrapping)) {
        return false;
    }

    const bool bootstrap_ok = bootstrap_step_
        ? bootstrap_step_(services_)
        : RegisterDefaultBootstrapServices(services_);

    if (!bootstrap_ok) {
        Fault("kernel bootstrap step failed");
        return false;
    }

    if (!lifecycle_.TransitionTo(LifecycleState::kInitializing)) {
        Fault("invalid transition into Initializing");
        return false;
    }

    const bool initialize_ok = initialize_step_ ? initialize_step_(services_) : true;
    if (!initialize_ok) {
        Fault("kernel initialize step failed");
        return false;
    }

    if (!services_.Freeze()) {
        Fault("service context freeze failed");
        return false;
    }

    if (!lifecycle_.TransitionTo(LifecycleState::kRunning)) {
        Fault("invalid transition into Running");
        return false;
    }

    last_fault_reason_.clear();
    return true;
}

bool KernelRuntime::Stop() {
    const auto current = lifecycle_.State();
    if (current == LifecycleState::kStopped) {
        return true;
    }

    if (!lifecycle_.TransitionTo(LifecycleState::kStopping)) {
        Fault("invalid transition into Stopping");
        return false;
    }

    if (!ExecuteShutdownSequence()) {
        Fault("shutdown sequence failed");
        return false;
    }

    if (!lifecycle_.TransitionTo(LifecycleState::kStopped)) {
        Fault("invalid transition into Stopped");
        return false;
    }

    (void)services_.ResetForNextStartCycle();
    return true;
}

LifecycleState KernelRuntime::State() const {
    return lifecycle_.State();
}

bool KernelRuntime::IsRunning() const {
    return lifecycle_.State() == LifecycleState::kRunning;
}

ServiceContext& KernelRuntime::Services() {
    return services_;
}

const ServiceContext& KernelRuntime::Services() const {
    return services_;
}

const std::vector<std::string>& KernelRuntime::LastModuleShutdownOrder() const {
    return last_module_shutdown_order_;
}

const std::vector<std::string>& KernelRuntime::LastPlatformTeardownOrder() const {
    return last_platform_teardown_order_;
}

const std::string& KernelRuntime::LastFaultReason() const {
    return last_fault_reason_;
}

void KernelRuntime::Fault(std::string reason) {
    last_fault_reason_ = std::move(reason);
    if (lifecycle_.State() != LifecycleState::kFaulted) {
        (void)lifecycle_.TransitionTo(LifecycleState::kFaulted);
    }
}

bool KernelRuntime::ExecuteShutdownSequence() {
    bool had_failure = false;

    last_module_shutdown_order_.clear();
    for (auto it = started_modules_.rbegin(); it != started_modules_.rend(); ++it) {
        last_module_shutdown_order_.push_back(*it);

        if (module_stop_handler_ && !module_stop_handler_(*it)) {
            had_failure = true;
        }
    }

    if (event_bus_) {
        switch (deferred_event_policy_) {
            case ShutdownDeferredEventPolicy::kDrainAll:
                (void)event_bus_->DrainDeferred();
                break;
            case ShutdownDeferredEventPolicy::kDrainBounded:
                (void)event_bus_->DrainDeferred(deferred_event_bounded_limit_);
                break;
            case ShutdownDeferredEventPolicy::kDiscardWithDiagnostics:
                (void)event_bus_->DiscardDeferred();
                break;
        }
    }

    last_platform_teardown_order_.clear();
    for (auto& step : platform_teardown_steps_) {
        last_platform_teardown_order_.push_back(step.name);

        if (step.step && !step.step()) {
            had_failure = true;
        }
    }

    if (!services_.BeginTeardown()) {
        had_failure = true;
    }

    if (diagnostics_flush_step_) {
        diagnostics_flush_step_();
    }

    return !had_failure;
}

} // namespace framekit::runtime
