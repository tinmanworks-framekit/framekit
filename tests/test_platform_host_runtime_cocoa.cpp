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

class FakeWindowHost : public framekit::runtime::IWindowHost {
public:
    bool CreatePrimaryWindow(const framekit::runtime::WindowSpec& spec) override {
        create_calls += 1;
        last_title = spec.title;
        last_visible = spec.visible;
        return true;
    }

    bool PreRender() override {
        pre_render_calls += 1;
        return true;
    }

    bool Render() override {
        render_calls += 1;
        return true;
    }

    bool PostRender() override {
        post_render_calls += 1;
        return true;
    }

    bool TeardownWindows() override {
        teardown_calls += 1;
        return true;
    }

    int create_calls = 0;
    int pre_render_calls = 0;
    int render_calls = 0;
    int post_render_calls = 0;
    int teardown_calls = 0;
    std::string last_title;
    bool last_visible = true;
};

framekit::runtime::LoopPolicy GuiPolicy() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kGui;
    policy.rendering_enabled = true;
    return policy;
}

framekit::runtime::LoopPolicy HybridPolicy() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHybrid;
    policy.rendering_enabled = true;
    return policy;
}

framekit::runtime::WindowSpec HiddenWindowSpec(std::string title) {
    framekit::runtime::WindowSpec spec;
    spec.title = title;
    spec.visible = false;
    spec.width = 640;
    spec.height = 480;
    return spec;
}

void TestGuiRuntimeAutoWiresCocoaBackend() {
    framekit::runtime::PlatformHostRuntime runtime;

    REQUIRE(runtime.Configure(GuiPolicy(), HiddenWindowSpec("framekit-cocoa-gui")));
    REQUIRE(runtime.Start());
    REQUIRE(runtime.Tick());
    REQUIRE(runtime.Stop());
}

void TestHybridRuntimeAutoWiresCocoaBackend() {
    framekit::runtime::PlatformHostRuntime runtime;

    REQUIRE(runtime.Configure(HybridPolicy(), HiddenWindowSpec("framekit-cocoa-hybrid")));
    REQUIRE(runtime.Start());
    REQUIRE(runtime.Tick());
    REQUIRE(runtime.Stop());
}

void TestPartialManualInjectionStillFails() {
    framekit::runtime::PlatformHostRuntime runtime;
    runtime.SetPlatformHost(std::make_shared<FakePlatformHost>());

    REQUIRE(runtime.Configure(GuiPolicy(), HiddenWindowSpec("framekit-cocoa-partial")));
    REQUIRE(!runtime.Start());
    REQUIRE(runtime.LastError() == "window host is required when render stages are active");
}

void TestExplicitInjectedHostsStillWin() {
    auto platform = std::make_shared<FakePlatformHost>();
    auto window = std::make_shared<FakeWindowHost>();

    framekit::runtime::PlatformHostRuntime runtime;
    runtime.SetPlatformHost(platform);
    runtime.SetWindowHost(window);

    REQUIRE(runtime.Configure(GuiPolicy(), HiddenWindowSpec("framekit-manual-hosts")));
    REQUIRE(runtime.Start());
    REQUIRE(runtime.Tick());
    REQUIRE(runtime.Stop());

    REQUIRE(platform->initialize_calls == 1);
    REQUIRE(platform->pump_calls == 1);
    REQUIRE(platform->teardown_calls == 1);
    REQUIRE(window->create_calls == 1);
    REQUIRE(window->pre_render_calls == 1);
    REQUIRE(window->render_calls == 1);
    REQUIRE(window->post_render_calls == 1);
    REQUIRE(window->teardown_calls == 1);
    REQUIRE(window->last_title == "framekit-manual-hosts");
    REQUIRE(!window->last_visible);
}

} // namespace

int main() {
    TestGuiRuntimeAutoWiresCocoaBackend();
    TestHybridRuntimeAutoWiresCocoaBackend();
    TestPartialManualInjectionStillFails();
    TestExplicitInjectedHostsStillWin();
    return 0;
}
