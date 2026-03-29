#include "framekit/framekit.hpp"

#include <algorithm>
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

class FakePlatformHost : public framekit::runtime::IPlatformHost {
public:
    bool Initialize() override {
        initialize_calls += 1;
        return initialize_result;
    }

    bool PumpEvents() override {
        pump_calls += 1;
        return pump_result;
    }

    bool Teardown() override {
        teardown_calls += 1;
        return teardown_result;
    }

    int initialize_calls = 0;
    int pump_calls = 0;
    int teardown_calls = 0;
    bool initialize_result = true;
    bool pump_result = true;
    bool teardown_result = true;
};

class FakeWindowHost : public framekit::runtime::IWindowHost {
public:
    bool CreatePrimaryWindow(const framekit::runtime::WindowSpec& spec) override {
        create_calls += 1;
        last_window_title = spec.title;
        return create_result;
    }

    bool PreRender() override {
        pre_render_calls += 1;
        return pre_render_result;
    }

    bool Render() override {
        render_calls += 1;
        return render_result;
    }

    bool PostRender() override {
        post_render_calls += 1;
        return post_render_result;
    }

    bool TeardownWindows() override {
        teardown_calls += 1;
        return teardown_result;
    }

    int create_calls = 0;
    int pre_render_calls = 0;
    int render_calls = 0;
    int post_render_calls = 0;
    int teardown_calls = 0;
    bool create_result = true;
    bool pre_render_result = true;
    bool render_result = true;
    bool post_render_result = true;
    bool teardown_result = true;
    std::string last_window_title;
};

void TestGuiRuntimeCallsPlatformAndWindowStages() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kGui;
    policy.rendering_enabled = true;

    auto platform = std::make_shared<FakePlatformHost>();
    auto window = std::make_shared<FakeWindowHost>();

    framekit::runtime::PlatformHostRuntime runtime;
    runtime.SetPlatformHost(platform);
    runtime.SetWindowHost(window);

    framekit::runtime::WindowSpec spec;
    spec.title = "framekit-test-window";

    REQUIRE(runtime.Configure(policy, spec));
    REQUIRE(runtime.Start());
    REQUIRE(runtime.Tick());

    REQUIRE(platform->initialize_calls == 1);
    REQUIRE(platform->pump_calls == 1);
    REQUIRE(window->create_calls == 1);
    REQUIRE(window->last_window_title == "framekit-test-window");
    REQUIRE(window->pre_render_calls == 1);
    REQUIRE(window->render_calls == 1);
    REQUIRE(window->post_render_calls == 1);

    REQUIRE(runtime.Stop());
    REQUIRE(!runtime.IsRunning());
    REQUIRE(platform->teardown_calls == 1);
    REQUIRE(window->teardown_calls == 1);
}

void TestHeadlessRuntimeSkipsWindowStages() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHeadless;
    policy.rendering_enabled = false;
    policy.disabled_optional_stages = {"PreRender", "Render", "PostRender"};

    auto platform = std::make_shared<FakePlatformHost>();

    framekit::runtime::PlatformHostRuntime runtime;
    runtime.SetPlatformHost(platform);

    REQUIRE(runtime.Configure(policy));
    REQUIRE(runtime.Start());
    REQUIRE(runtime.Tick());
    REQUIRE(runtime.Stop());

    REQUIRE(platform->initialize_calls == 1);
    REQUIRE(platform->pump_calls == 1);
    REQUIRE(platform->teardown_calls == 1);

    const auto& active = runtime.ActiveStages();
    REQUIRE(std::find(active.begin(), active.end(), framekit::runtime::LoopStage::kPreRender) == active.end());
    REQUIRE(std::find(active.begin(), active.end(), framekit::runtime::LoopStage::kRender) == active.end());
    REQUIRE(std::find(active.begin(), active.end(), framekit::runtime::LoopStage::kPostRender) == active.end());
}

void TestStartFailsWithoutPlatformHost() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHeadless;
    policy.rendering_enabled = false;
    policy.disabled_optional_stages = {"PreRender", "Render", "PostRender"};

    framekit::runtime::PlatformHostRuntime runtime;
    REQUIRE(runtime.Configure(policy));
    REQUIRE(!runtime.Start());
    REQUIRE(!runtime.LastError().empty());
}

void TestStartFailsWhenRenderStagesActiveWithoutWindowHost() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kGui;
    policy.rendering_enabled = true;

    auto platform = std::make_shared<FakePlatformHost>();

    framekit::runtime::PlatformHostRuntime runtime;
    runtime.SetPlatformHost(platform);
    REQUIRE(runtime.Configure(policy));
    REQUIRE(!runtime.Start());
    REQUIRE(!runtime.LastError().empty());
}

void TestTickFailsOnPlatformPumpFailure() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHeadless;
    policy.rendering_enabled = false;
    policy.disabled_optional_stages = {"PreRender", "Render", "PostRender"};

    auto platform = std::make_shared<FakePlatformHost>();
    platform->pump_result = false;

    framekit::runtime::PlatformHostRuntime runtime;
    runtime.SetPlatformHost(platform);

    REQUIRE(runtime.Configure(policy));
    REQUIRE(runtime.Start());
    REQUIRE(!runtime.Tick());
    REQUIRE(runtime.LastError().find("ProcessPlatformEvents") != std::string::npos);
    REQUIRE(runtime.Stop());
}

void TestTickFailsOnWindowRenderFailure() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kGui;
    policy.rendering_enabled = true;

    auto platform = std::make_shared<FakePlatformHost>();
    auto window = std::make_shared<FakeWindowHost>();
    window->render_result = false;

    framekit::runtime::PlatformHostRuntime runtime;
    runtime.SetPlatformHost(platform);
    runtime.SetWindowHost(window);

    REQUIRE(runtime.Configure(policy));
    REQUIRE(runtime.Start());
    REQUIRE(!runtime.Tick());
    REQUIRE(runtime.LastError().find("Render") != std::string::npos);
    REQUIRE(runtime.Stop());
}

} // namespace

int main() {
    TestGuiRuntimeCallsPlatformAndWindowStages();
    TestHeadlessRuntimeSkipsWindowStages();
    TestStartFailsWithoutPlatformHost();
    TestStartFailsWhenRenderStagesActiveWithoutWindowHost();
    TestTickFailsOnPlatformPumpFailure();
    TestTickFailsOnWindowRenderFailure();
    return 0;
}
