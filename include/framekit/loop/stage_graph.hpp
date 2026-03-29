#pragma once

#include "framekit/loop/policy.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace framekit::runtime {

enum class LoopStage : std::uint8_t {
    kBeginFrame = 0,
    kProcessPlatformEvents = 1,
    kNormalizeInput = 2,
    kDispatchImmediateEvents = 3,
    kPreUpdate = 4,
    kUpdate = 5,
    kPostUpdate = 6,
    kDispatchDeferredEvents = 7,
    kPreRender = 8,
    kRender = 9,
    kPostRender = 10,
    kEndFrame = 11,
};

const std::vector<LoopStage>& BaselineLoopStageOrder();
const char* LoopStageName(LoopStage stage);
bool ParseLoopStageName(const std::string& name, LoopStage* out_stage);

class LoopStageGraphRunner {
public:
    using StageHandler = std::function<void()>;

    bool Configure(const LoopPolicy& policy);
    void SetStageHandler(LoopStage stage, StageHandler handler);
    void ClearStageHandlers();

    bool ExecuteFrame(std::uint64_t elapsed_ns = 0);

    bool IsConfigured() const;
    const LoopPolicy& Policy() const;
    const std::vector<LoopStage>& ActiveStages() const;
    const std::vector<LoopStage>& LastExecutedStages() const;
    std::uint32_t LastUpdateStepCount() const;
    std::uint32_t LastDroppedUpdateStepCount() const;
    bool LastFrameDroppedNonCriticalStages() const;
    const std::string& LastError() const;

private:
    struct LoopStageHash {
        std::size_t operator()(LoopStage stage) const {
            return static_cast<std::size_t>(stage);
        }
    };

    LoopPolicy policy_;
    std::unordered_map<LoopStage, StageHandler, LoopStageHash> handlers_;
    std::vector<LoopStage> active_stages_;
    std::vector<LoopStage> last_executed_stages_;
    std::uint32_t last_update_step_count_ = 1;
    std::uint32_t last_dropped_update_step_count_ = 0;
    bool last_frame_dropped_non_critical_stages_ = false;
    std::uint64_t fixed_delta_accumulator_ns_ = 0;
    std::string last_error_;
    bool configured_ = false;
};

} // namespace framekit::runtime
