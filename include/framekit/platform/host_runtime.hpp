#pragma once

#include "framekit/fault/policy_runtime.hpp"
#include "framekit/input/routing.hpp"
#include "framekit/loop/policy.hpp"
#include "framekit/loop/stage_graph.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace framekit::runtime {

struct WindowSpec {
    std::string title = "FrameKit";
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool visible = true;
};

class IPlatformHost {
public:
    virtual ~IPlatformHost() = default;

    virtual bool Initialize() = 0;
    virtual bool PumpEvents() = 0;
    virtual bool Teardown() = 0;
};

class IWindowHost {
public:
    virtual ~IWindowHost() = default;

    virtual bool CreatePrimaryWindow(const WindowSpec& spec) = 0;
    virtual bool PreRender() = 0;
    virtual bool Render() = 0;
    virtual bool PostRender() = 0;
    virtual bool TeardownWindows() = 0;
};

class PlatformHostRuntime {
public:
    void SetPlatformHost(std::shared_ptr<IPlatformHost> host);
    void SetWindowHost(std::shared_ptr<IWindowHost> host);
    void SetInputRuntime(std::shared_ptr<InputRoutingRuntime> input_runtime);

    bool Configure(const LoopPolicy& policy, WindowSpec primary_window = {});
    bool Start();
    bool Tick();
    bool Stop();

    bool IsConfigured() const;
    bool IsRunning() const;

    const std::vector<LoopStage>& ActiveStages() const;
    const std::vector<LoopStage>& LastFrameStages() const;
    const std::string& LastError() const;

private:
    bool HasActiveRenderStages() const;
    void HandleStageFault(
        LoopStage stage,
        FaultPolicyContext context,
        const std::string& message);

    LoopStageGraphRunner loop_runner_;
    LoopPolicy policy_;
    WindowSpec primary_window_;
    std::shared_ptr<IPlatformHost> platform_host_;
    std::shared_ptr<IWindowHost> window_host_;
    std::shared_ptr<InputRoutingRuntime> input_runtime_;
    FaultPolicyRuntime fault_policy_;
    bool configured_ = false;
    bool running_ = false;
    bool window_created_ = false;
    bool frame_failed_ = false;
    std::string last_error_;
};

} // namespace framekit::runtime
