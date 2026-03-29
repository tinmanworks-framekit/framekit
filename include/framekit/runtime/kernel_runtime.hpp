#pragma once

#include "framekit/runtime/bootstrap_services.hpp"
#include "framekit/runtime/event_bus.hpp"
#include "framekit/runtime/lifecycle_state_machine.hpp"
#include "framekit/runtime/service_context.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace framekit::runtime {

enum class ShutdownDeferredEventPolicy : std::uint8_t {
    kDrainAll = 0,
    kDrainBounded = 1,
    kDiscardWithDiagnostics = 2,
};

class KernelRuntime {
public:
    using BootstrapStep = std::function<bool(ServiceContext&)>;
    using InitializeStep = std::function<bool(ServiceContext&)>;
    using ModuleStopHandler = std::function<bool(const std::string& module_id)>;
    using PlatformTeardownStep = std::function<bool()>;
    using DiagnosticsFlushStep = std::function<void()>;

    void SetBootstrapStep(BootstrapStep step);
    void SetInitializeStep(InitializeStep step);
    void SetModuleStopHandler(ModuleStopHandler handler);
    void SetDiagnosticsFlushStep(DiagnosticsFlushStep step);
    void SetEventBus(std::shared_ptr<EventBus> event_bus);

    void SetShutdownDeferredEventPolicy(
        ShutdownDeferredEventPolicy policy,
        std::size_t bounded_limit = 0);

    void AddPlatformTeardownStep(std::string name, PlatformTeardownStep step);
    void RecordStartedModules(std::vector<std::string> startup_order);

    bool Start();
    bool Stop();

    LifecycleState State() const;
    bool IsRunning() const;

    ServiceContext& Services();
    const ServiceContext& Services() const;

    const std::vector<std::string>& LastModuleShutdownOrder() const;
    const std::vector<std::string>& LastPlatformTeardownOrder() const;
    const std::string& LastFaultReason() const;

private:
    void Fault(std::string reason);
    bool ExecuteShutdownSequence();

    LifecycleStateMachine lifecycle_;
    ServiceContext services_;
    BootstrapStep bootstrap_step_;
    InitializeStep initialize_step_;
    ModuleStopHandler module_stop_handler_;
    DiagnosticsFlushStep diagnostics_flush_step_;
    std::shared_ptr<EventBus> event_bus_;

    ShutdownDeferredEventPolicy deferred_event_policy_ =
        ShutdownDeferredEventPolicy::kDrainAll;
    std::size_t deferred_event_bounded_limit_ = 0;

    std::vector<std::string> started_modules_;
    std::vector<std::string> last_module_shutdown_order_;

    struct NamedTeardownStep {
        std::string name;
        PlatformTeardownStep step;
    };

    std::vector<NamedTeardownStep> platform_teardown_steps_;
    std::vector<std::string> last_platform_teardown_order_;
    std::string last_fault_reason_;
};

} // namespace framekit::runtime
