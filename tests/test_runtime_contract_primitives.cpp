#include "framekit/framekit.hpp"

#include <algorithm>
#include <any>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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

struct TestService {
    int value = 0;
};

struct AlternateService {
    std::string name;
};

void TestLifecycleStateMachine() {
    framekit::runtime::LifecycleStateMachine machine;

    REQUIRE(machine.State() == framekit::runtime::LifecycleState::kStopped);
    REQUIRE(!machine.CanTransitionTo(framekit::runtime::LifecycleState::kRunning));

    REQUIRE(machine.TransitionTo(framekit::runtime::LifecycleState::kBootstrapping));
    REQUIRE(!machine.TransitionTo(framekit::runtime::LifecycleState::kRunning));
    REQUIRE(machine.TransitionTo(framekit::runtime::LifecycleState::kInitializing));
    REQUIRE(machine.TransitionTo(framekit::runtime::LifecycleState::kRunning));
    REQUIRE(!machine.TransitionTo(framekit::runtime::LifecycleState::kBootstrapping));
    REQUIRE(machine.TransitionTo(framekit::runtime::LifecycleState::kStopping));
    REQUIRE(machine.TransitionTo(framekit::runtime::LifecycleState::kStopped));
}

void TestLoopPolicyValidation() {
    framekit::runtime::LoopPolicy baseline;
    const auto baseline_result = framekit::runtime::ValidateLoopPolicy(baseline);
    REQUIRE(baseline_result.valid);

    framekit::runtime::LoopPolicy fixed_catch_up;
    fixed_catch_up.timing_mode = framekit::runtime::TimingMode::kFixedDelta;
    fixed_catch_up.overrun_mode = framekit::runtime::OverrunMode::kCatchUpBounded;
    fixed_catch_up.max_catch_up_steps = 0;
    const auto fixed_result = framekit::runtime::ValidateLoopPolicy(fixed_catch_up);
    REQUIRE(!fixed_result.valid);

    framekit::runtime::LoopPolicy headless;
    headless.profile = framekit::runtime::LoopProfile::kHeadless;
    headless.rendering_enabled = false;
    const auto missing_disable = framekit::runtime::ValidateLoopPolicy(headless);
    REQUIRE(!missing_disable.valid);

    headless.disabled_optional_stages = {"PreRender", "Render", "PostRender"};
    const auto valid_headless = framekit::runtime::ValidateLoopPolicy(headless);
    REQUIRE(valid_headless.valid);

    framekit::runtime::LoopPolicy invalid_required_disable;
    invalid_required_disable.profile = framekit::runtime::LoopProfile::kHeadless;
    invalid_required_disable.overrun_mode = framekit::runtime::OverrunMode::kDropNonCritical;
    invalid_required_disable.disabled_optional_stages = {"Update"};
    const auto required_disable = framekit::runtime::ValidateLoopPolicy(invalid_required_disable);
    REQUIRE(!required_disable.valid);
}

void TestServiceContext() {
    framekit::runtime::ServiceContext services;

    auto test_service = std::make_shared<TestService>();
    test_service->value = 7;

    REQUIRE(services.Register<TestService>(test_service));
    REQUIRE(!services.Register<TestService>(std::make_shared<TestService>()));
    REQUIRE(services.Register<AlternateService>(
        std::make_shared<AlternateService>(AlternateService{"alt"}),
        "alt",
        framekit::runtime::ServiceOwnership::kExternalOwned));

    REQUIRE(services.Freeze());
    REQUIRE(services.Phase() == framekit::runtime::ServiceContextPhase::kFrozen);
    REQUIRE(services.FreezeCount() == 1);
    REQUIRE(!services.Register<TestService>(std::make_shared<TestService>(), "late"));

    auto found = services.Find<TestService>();
    REQUIRE(found != nullptr);
    REQUIRE(found->value == 7);

    auto alt = services.Find<AlternateService>("alt");
    REQUIRE(alt != nullptr);
    REQUIRE(alt->name == "alt");

    REQUIRE(services.BeginTeardown());
    REQUIRE(services.Phase() == framekit::runtime::ServiceContextPhase::kTeardown);
    REQUIRE(services.Find<TestService>() == nullptr);

    const auto& order = services.LastTeardownOrder();
    REQUIRE(order.size() == 1);
    REQUIRE(order.front().contract_type == std::type_index(typeid(TestService)));

    REQUIRE(services.ResetForNextStartCycle());
    REQUIRE(services.Phase() == framekit::runtime::ServiceContextPhase::kOpen);
}

void TestModuleGraphValidation() {
    const std::vector<framekit::runtime::ModuleSpec> modules = {
        framekit::runtime::ModuleSpec{
            .id = "frontend",
            .required_dependencies = {"backend"},
            .optional_dependencies = {},
        },
        framekit::runtime::ModuleSpec{
            .id = "backend",
            .required_dependencies = {},
            .optional_dependencies = {},
        },
        framekit::runtime::ModuleSpec{
            .id = "metrics",
            .required_dependencies = {},
            .optional_dependencies = {"optional-missing"},
        },
    };

    const auto result = framekit::runtime::ValidateModuleGraph(modules);
    REQUIRE(result.valid);
    REQUIRE(result.base_order.size() == 3);
    REQUIRE(result.base_order[0] == "frontend");
    REQUIRE(result.base_order[1] == "backend");
    REQUIRE(result.base_order[2] == "metrics");
    REQUIRE(result.startup_order.size() == 3);
    REQUIRE(result.startup_order[0] == "metrics");
    REQUIRE(result.startup_order[1] == "backend");
    REQUIRE(result.startup_order[2] == "frontend");

    const std::vector<framekit::runtime::ModuleSpec> cyclic = {
        framekit::runtime::ModuleSpec{
            .id = "a",
            .required_dependencies = {"b"},
            .optional_dependencies = {},
        },
        framekit::runtime::ModuleSpec{
            .id = "b",
            .required_dependencies = {"a"},
            .optional_dependencies = {},
        },
    };

    const auto cyclic_result = framekit::runtime::ValidateModuleGraph(cyclic);
    REQUIRE(!cyclic_result.valid);
    REQUIRE(!cyclic_result.errors.empty());
    REQUIRE(cyclic_result.errors.front().code == framekit::runtime::ModuleGraphErrorCode::kCycleDetected);

    const std::vector<framekit::runtime::ModuleSpec> missing_required = {
        framekit::runtime::ModuleSpec{
            .id = "a",
            .required_dependencies = {"missing"},
            .optional_dependencies = {},
        },
    };

    const auto missing_result = framekit::runtime::ValidateModuleGraph(missing_required);
    REQUIRE(!missing_result.valid);
    REQUIRE(!missing_result.errors.empty());
    REQUIRE(missing_result.errors.front().code == framekit::runtime::ModuleGraphErrorCode::kMissingRequiredDependency);
}

void TestEventBusSemantics() {
    framekit::runtime::EventBus bus;

    std::vector<std::string> observed;
    bus.Subscribe("evt", [&observed](framekit::runtime::EventContext& context) {
        observed.push_back("first");
        context.Consume();
    });
    bus.Subscribe("evt", [&observed](framekit::runtime::EventContext& context) {
        (void)context;
        observed.push_back("second");
    });
    bus.Subscribe("evt", [&observed](framekit::runtime::EventContext& context) {
        (void)context;
        observed.push_back("monitor");
    }, true);

    const auto immediate = bus.DispatchImmediate("evt", std::any{});
    REQUIRE(immediate.had_handlers);
    REQUIRE(immediate.consumed);
    REQUIRE(observed.size() == 2);
    REQUIRE(observed[0] == "first");
    REQUIRE(observed[1] == "monitor");

    framekit::runtime::EventBus monitor_bus;
    std::vector<std::string> monitor_observed;
    monitor_bus.Subscribe("evt", [&monitor_observed](framekit::runtime::EventContext& context) {
        monitor_observed.push_back("monitor");
        context.Consume();
    }, true);
    monitor_bus.Subscribe("evt", [&monitor_observed](framekit::runtime::EventContext& context) {
        (void)context;
        monitor_observed.push_back("normal");
    });

    const auto monitor_result = monitor_bus.DispatchImmediate("evt", std::any{});
    REQUIRE(monitor_result.had_handlers);
    REQUIRE(!monitor_result.consumed);
    REQUIRE(monitor_observed.size() == 2);
    REQUIRE(monitor_observed[0] == "monitor");
    REQUIRE(monitor_observed[1] == "normal");

    framekit::runtime::EventBus queue_bus;
    std::vector<std::string> deferred_order;
    queue_bus.Subscribe("high", [&deferred_order](framekit::runtime::EventContext& context) {
        deferred_order.push_back(context.EventType());
    });
    queue_bus.Subscribe("normal", [&deferred_order](framekit::runtime::EventContext& context) {
        deferred_order.push_back(context.EventType());
    });
    queue_bus.Subscribe("low", [&deferred_order](framekit::runtime::EventContext& context) {
        deferred_order.push_back(context.EventType());
    });

    REQUIRE(queue_bus.EnqueueDeferred("low", 1, framekit::runtime::DeferredPriority::kLow));
    REQUIRE(queue_bus.EnqueueDeferred("high", 1, framekit::runtime::DeferredPriority::kHigh));
    REQUIRE(queue_bus.EnqueueDeferred("normal", 1, framekit::runtime::DeferredPriority::kNormal));

    REQUIRE(queue_bus.DrainDeferred() == 3);
    REQUIRE(deferred_order.size() == 3);
    REQUIRE(deferred_order[0] == "high");
    REQUIRE(deferred_order[1] == "normal");
    REQUIRE(deferred_order[2] == "low");

    framekit::runtime::EventBus bounded_bus;
    bounded_bus.SetDeferredCapacity(1);
    bounded_bus.SetDeferredOverflowPolicy(framekit::runtime::DeferredOverflowPolicy::kDropNewest);

    REQUIRE(bounded_bus.EnqueueDeferred("evt-a", 1));
    REQUIRE(!bounded_bus.EnqueueDeferred("evt-b", 2));
    REQUIRE(bounded_bus.DroppedDeferredCount() == 1);
}

void TestKernelRuntime() {
    framekit::runtime::KernelRuntime runtime;

    std::vector<std::string> module_stop_order;
    std::vector<std::string> drained_events;
    bool diagnostics_flushed = false;

    runtime.SetBootstrapStep([](framekit::runtime::ServiceContext& services) {
        auto config = std::make_shared<framekit::runtime::ConfigurationService>();
        config->values["mode"] = "test";

        bool ok = true;
        ok = ok && services.Register(std::make_shared<framekit::runtime::DiagnosticsService>());
        ok = ok && services.Register(config);
        ok = ok && services.Register(std::make_shared<framekit::runtime::TimingService>());
        return ok;
    });

    runtime.SetInitializeStep([](framekit::runtime::ServiceContext& services) {
        auto config = services.Find<framekit::runtime::ConfigurationService>();
        return config != nullptr && config->values["mode"] == "test";
    });

    runtime.SetModuleStopHandler([&module_stop_order](const std::string& module_id) {
        module_stop_order.push_back(module_id);
        return true;
    });

    runtime.AddPlatformTeardownStep("platform-window", []() { return true; });
    runtime.AddPlatformTeardownStep("platform-host", []() { return true; });
    runtime.SetDiagnosticsFlushStep([&diagnostics_flushed]() { diagnostics_flushed = true; });

    auto event_bus = std::make_shared<framekit::runtime::EventBus>();
    event_bus->Subscribe("deferred", [&drained_events](framekit::runtime::EventContext& context) {
        drained_events.push_back(context.EventType());
    });
    runtime.SetEventBus(event_bus);
    runtime.SetShutdownDeferredEventPolicy(framekit::runtime::ShutdownDeferredEventPolicy::kDrainAll);
    REQUIRE(event_bus->EnqueueDeferred("deferred", 1));

    runtime.RecordStartedModules({"module-core", "module-app"});

    REQUIRE(runtime.Start());
    REQUIRE(runtime.IsRunning());
    REQUIRE(runtime.State() == framekit::runtime::LifecycleState::kRunning);
    REQUIRE(runtime.Services().Phase() == framekit::runtime::ServiceContextPhase::kFrozen);
    REQUIRE(!runtime.Services().Register(std::make_shared<framekit::runtime::DiagnosticsService>(), "late"));

    REQUIRE(runtime.Stop());
    REQUIRE(runtime.State() == framekit::runtime::LifecycleState::kStopped);
    REQUIRE(!runtime.IsRunning());
    REQUIRE(diagnostics_flushed);
    REQUIRE(module_stop_order.size() == 2);
    REQUIRE(module_stop_order[0] == "module-app");
    REQUIRE(module_stop_order[1] == "module-core");
    REQUIRE(runtime.LastPlatformTeardownOrder().size() == 2);
    REQUIRE(runtime.LastPlatformTeardownOrder()[0] == "platform-window");
    REQUIRE(runtime.LastPlatformTeardownOrder()[1] == "platform-host");
    REQUIRE(drained_events.size() == 1);
    REQUIRE(drained_events[0] == "deferred");
    REQUIRE(event_bus->DeferredCount() == 0);

    REQUIRE(runtime.Start());
    REQUIRE(runtime.Stop());

    framekit::runtime::KernelRuntime faulty;
    faulty.SetInitializeStep([](framekit::runtime::ServiceContext& services) {
        (void)services;
        return false;
    });

    REQUIRE(!faulty.Start());
    REQUIRE(faulty.State() == framekit::runtime::LifecycleState::kFaulted);
    REQUIRE(!faulty.LastFaultReason().empty());
    REQUIRE(faulty.Stop());
    REQUIRE(faulty.State() == framekit::runtime::LifecycleState::kStopped);
}

} // namespace

int main() {
    TestLifecycleStateMachine();
    TestLoopPolicyValidation();
    TestServiceContext();
    TestModuleGraphValidation();
    TestEventBusSemantics();
    TestKernelRuntime();
    return 0;
}
