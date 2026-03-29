#include "framekit/framekit.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
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

class FakePlatformHost : public framekit::runtime::IPlatformHost {
public:
    bool Initialize() override {
        initialize_calls += 1;
        return true;
    }

    bool PumpEvents() override {
        pump_calls += 1;
        return true;
    }

    bool Teardown() override {
        teardown_calls += 1;
        return true;
    }

    int initialize_calls = 0;
    int pump_calls = 0;
    int teardown_calls = 0;
};

class FailingNormalizer : public framekit::runtime::IInputNormalizer {
public:
    bool Normalize(
        const framekit::runtime::RawInputEvent& raw_event,
        framekit::runtime::NormalizedInputEvent* normalized_event) const override {
        if (raw_event.backend_type == "fail") {
            return false;
        }
        if (!normalized_event) {
            return false;
        }

        normalized_event->device = raw_event.device;
        normalized_event->action = raw_event.action;
        normalized_event->timestamp_ns = raw_event.timestamp_ns;
        normalized_event->source_id = raw_event.source_id;
        normalized_event->window_id = raw_event.window_id;
        normalized_event->pointer_x = raw_event.pointer_x;
        normalized_event->pointer_y = raw_event.pointer_y;
        normalized_event->key_code = raw_event.key_code;
        normalized_event->text = raw_event.text;
        normalized_event->modifiers = raw_event.modifiers;
        normalized_event->backend_extension = raw_event.backend_payload;
        return true;
    }
};

void TestFocusedKeyboardRouting() {
    framekit::runtime::InputRoutingRuntime runtime;

    int focused_calls = 0;
    runtime.Router().SetFocusedWindow("main-window");
    runtime.Router().SubscribeWindow(
        "main-window",
        [&focused_calls](const framekit::runtime::NormalizedInputEvent& event) {
            (void)event;
            focused_calls += 1;
            return true;
        });

    framekit::runtime::RawInputEvent raw;
    raw.device = framekit::runtime::InputDevice::kKeyboard;
    raw.action = framekit::runtime::InputAction::kKeyDown;
    raw.key_code = 65;
    raw.timestamp_ns = 100;
    raw.source_id = "keyboard-1";
    runtime.EnqueueRawInput(raw);

    REQUIRE(runtime.NormalizePending());
    const auto dispatched = runtime.DispatchImmediate();
    REQUIRE(dispatched.delivered_handlers == 1);
    REQUIRE(dispatched.dropped_events == 0);
    REQUIRE(dispatched.consumed);
    REQUIRE(focused_calls == 1);
}

void TestPointerCaptureOverridesEventTarget() {
    framekit::runtime::InputRoutingRuntime runtime;

    int capture_calls = 0;
    runtime.Router().SetPointerCaptureWindow("capture-window");
    runtime.Router().SubscribeWindow(
        "capture-window",
        [&capture_calls](const framekit::runtime::NormalizedInputEvent& event) {
            (void)event;
            capture_calls += 1;
            return true;
        });

    framekit::runtime::RawInputEvent raw;
    raw.device = framekit::runtime::InputDevice::kPointer;
    raw.action = framekit::runtime::InputAction::kPointerMove;
    raw.window_id = "other-window";
    runtime.EnqueueRawInput(raw);

    REQUIRE(runtime.NormalizePending());
    const auto dispatched = runtime.DispatchImmediate();
    REQUIRE(dispatched.delivered_handlers == 1);
    REQUIRE(dispatched.dropped_events == 0);
    REQUIRE(capture_calls == 1);
}

void TestMissingTargetDropPolicy() {
    framekit::runtime::InputRoutingRuntime runtime;

    int global_calls = 0;
    runtime.Router().SubscribeGlobal(
        [&global_calls](const framekit::runtime::NormalizedInputEvent& event) {
            (void)event;
            global_calls += 1;
            return false;
        });

    framekit::runtime::RawInputEvent raw;
    raw.device = framekit::runtime::InputDevice::kPointer;
    raw.action = framekit::runtime::InputAction::kPointerMove;
    raw.window_id = "missing-window";
    runtime.EnqueueRawInput(raw);

    REQUIRE(runtime.NormalizePending());
    const auto dispatched = runtime.DispatchImmediate();
    REQUIRE(dispatched.delivered_handlers == 0);
    REQUIRE(dispatched.dropped_events == 1);
    REQUIRE(!dispatched.consumed);
    REQUIRE(global_calls == 0);
}

void TestMissingTargetFallbackToGlobalPolicy() {
    framekit::runtime::InputRoutingRuntime runtime;

    runtime.Router().SetMissingWindowTargetPolicy(framekit::runtime::MissingWindowTargetPolicy::kFallbackToGlobal);

    int global_calls = 0;
    runtime.Router().SubscribeGlobal(
        [&global_calls](const framekit::runtime::NormalizedInputEvent& event) {
            (void)event;
            global_calls += 1;
            return false;
        });

    framekit::runtime::RawInputEvent raw;
    raw.device = framekit::runtime::InputDevice::kPointer;
    raw.action = framekit::runtime::InputAction::kPointerMove;
    raw.window_id = "missing-window";
    runtime.EnqueueRawInput(raw);

    REQUIRE(runtime.NormalizePending());
    const auto dispatched = runtime.DispatchImmediate();
    REQUIRE(dispatched.delivered_handlers == 1);
    REQUIRE(dispatched.dropped_events == 0);
    REQUIRE(global_calls == 1);
}

void TestNormalizeFailurePropagatesToPlatformRuntime() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHeadless;
    policy.rendering_enabled = false;
    policy.disabled_optional_stages = {"PreRender", "Render", "PostRender"};

    auto platform = std::make_shared<FakePlatformHost>();
    auto input_runtime = std::make_shared<framekit::runtime::InputRoutingRuntime>();
    input_runtime->SetNormalizer(std::make_shared<FailingNormalizer>());

    framekit::runtime::RawInputEvent raw;
    raw.backend_type = "fail";
    raw.device = framekit::runtime::InputDevice::kKeyboard;
    raw.action = framekit::runtime::InputAction::kKeyDown;
    input_runtime->EnqueueRawInput(raw);

    framekit::runtime::PlatformHostRuntime runtime;
    runtime.SetPlatformHost(platform);
    runtime.SetInputRuntime(input_runtime);

    REQUIRE(runtime.Configure(policy));
    REQUIRE(runtime.Start());
    REQUIRE(!runtime.Tick());
    REQUIRE(runtime.LastError().find("NormalizeInput") != std::string::npos);
    REQUIRE(runtime.Stop());
}

void TestPlatformRuntimeDispatchesImmediateInput() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHeadless;
    policy.rendering_enabled = false;
    policy.disabled_optional_stages = {"PreRender", "Render", "PostRender"};

    auto platform = std::make_shared<FakePlatformHost>();
    auto input_runtime = std::make_shared<framekit::runtime::InputRoutingRuntime>();

    int focused_calls = 0;
    input_runtime->Router().SetFocusedWindow("editor");
    input_runtime->Router().SubscribeWindow(
        "editor",
        [&focused_calls](const framekit::runtime::NormalizedInputEvent& event) {
            (void)event;
            focused_calls += 1;
            return true;
        });

    framekit::runtime::RawInputEvent raw;
    raw.device = framekit::runtime::InputDevice::kKeyboard;
    raw.action = framekit::runtime::InputAction::kKeyDown;
    raw.key_code = 70;
    input_runtime->EnqueueRawInput(raw);

    framekit::runtime::PlatformHostRuntime runtime;
    runtime.SetPlatformHost(platform);
    runtime.SetInputRuntime(input_runtime);

    REQUIRE(runtime.Configure(policy));
    REQUIRE(runtime.Start());
    REQUIRE(runtime.Tick());
    REQUIRE(focused_calls == 1);
    REQUIRE(platform->pump_calls == 1);
    REQUIRE(runtime.Stop());
}

} // namespace

int main() {
    TestFocusedKeyboardRouting();
    TestPointerCaptureOverridesEventTarget();
    TestMissingTargetDropPolicy();
    TestMissingTargetFallbackToGlobalPolicy();
    TestNormalizeFailurePropagatesToPlatformRuntime();
    TestPlatformRuntimeDispatchesImmediateInput();
    return 0;
}
