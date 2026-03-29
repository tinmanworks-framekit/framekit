#include "framekit/framekit.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
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

std::vector<std::string> StageNames(const std::vector<framekit::runtime::LoopStage>& stages) {
    std::vector<std::string> names;
    names.reserve(stages.size());
    for (const auto stage : stages) {
        names.push_back(framekit::runtime::LoopStageName(stage));
    }
    return names;
}

std::size_t CountStage(
    const std::vector<framekit::runtime::LoopStage>& stages,
    framekit::runtime::LoopStage target) {
    return static_cast<std::size_t>(std::count(stages.begin(), stages.end(), target));
}

void TestBaselineGuiExecutionOrder() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kGui;
    policy.rendering_enabled = true;

    framekit::runtime::LoopStageGraphRunner runner;
    REQUIRE(runner.Configure(policy));

    std::vector<std::string> observed;
    for (const auto stage : framekit::runtime::BaselineLoopStageOrder()) {
        runner.SetStageHandler(stage, [&observed, stage]() {
            observed.push_back(framekit::runtime::LoopStageName(stage));
        });
    }

    REQUIRE(runner.ExecuteFrame());
    const auto expected = StageNames(framekit::runtime::BaselineLoopStageOrder());
    REQUIRE(observed == expected);
    REQUIRE(StageNames(runner.LastExecutedStages()) == expected);
}

void TestHeadlessRenderStagesOmitted() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHeadless;
    policy.rendering_enabled = false;
    policy.disabled_optional_stages = {"PreRender", "Render", "PostRender"};

    framekit::runtime::LoopStageGraphRunner runner;
    REQUIRE(runner.Configure(policy));

    REQUIRE(runner.ActiveStages().size() == 9);
    const auto active_names = StageNames(runner.ActiveStages());

    REQUIRE(std::find(active_names.begin(), active_names.end(), "PreRender") == active_names.end());
    REQUIRE(std::find(active_names.begin(), active_names.end(), "Render") == active_names.end());
    REQUIRE(std::find(active_names.begin(), active_names.end(), "PostRender") == active_names.end());

    REQUIRE(runner.ExecuteFrame());
    REQUIRE(StageNames(runner.LastExecutedStages()) == active_names);
}

void TestInvalidPolicyRejected() {
    framekit::runtime::LoopPolicy invalid;
    invalid.profile = framekit::runtime::LoopProfile::kHeadless;
    invalid.rendering_enabled = false;

    framekit::runtime::LoopStageGraphRunner runner;
    REQUIRE(!runner.Configure(invalid));
    REQUIRE(!runner.LastError().empty());
    REQUIRE(!runner.IsConfigured());
}

void TestUnknownDisabledStageRejected() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHeadless;
    policy.rendering_enabled = false;
    policy.disabled_optional_stages = {
        "PreRender",
        "Render",
        "PostRender",
        "NotAStage",
    };

    framekit::runtime::LoopStageGraphRunner runner;
    REQUIRE(!runner.Configure(policy));
    REQUIRE(!runner.LastError().empty());
}

void TestFixedDeltaCatchUpBoundedRunsExtraUpdateSteps() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHybrid;
    policy.rendering_enabled = true;
    policy.timing_mode = framekit::runtime::TimingMode::kFixedDelta;
    policy.overrun_mode = framekit::runtime::OverrunMode::kCatchUpBounded;
    policy.fixed_delta_ns = 1000000;
    policy.max_catch_up_steps = 2;

    framekit::runtime::LoopStageGraphRunner runner;
    REQUIRE(runner.Configure(policy));
    REQUIRE(runner.ExecuteFrame(policy.fixed_delta_ns * 3));

    REQUIRE(runner.LastUpdateStepCount() == 3);
    REQUIRE(runner.LastDroppedUpdateStepCount() == 0);
    REQUIRE(!runner.LastFrameDroppedNonCriticalStages());

    const auto& executed = runner.LastExecutedStages();
    REQUIRE(CountStage(executed, framekit::runtime::LoopStage::kPreUpdate) == 3);
    REQUIRE(CountStage(executed, framekit::runtime::LoopStage::kUpdate) == 3);
    REQUIRE(CountStage(executed, framekit::runtime::LoopStage::kPostUpdate) == 3);
    REQUIRE(CountStage(executed, framekit::runtime::LoopStage::kRender) == 1);
}

void TestFixedDeltaCatchUpBoundedDropsBeyondBudget() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHybrid;
    policy.rendering_enabled = true;
    policy.timing_mode = framekit::runtime::TimingMode::kFixedDelta;
    policy.overrun_mode = framekit::runtime::OverrunMode::kCatchUpBounded;
    policy.fixed_delta_ns = 1000000;
    policy.max_catch_up_steps = 1;

    framekit::runtime::LoopStageGraphRunner runner;
    REQUIRE(runner.Configure(policy));
    REQUIRE(runner.ExecuteFrame(policy.fixed_delta_ns * 4));

    REQUIRE(runner.LastUpdateStepCount() == 2);
    REQUIRE(runner.LastDroppedUpdateStepCount() == 2);
}

void TestFixedDeltaFailFastRejectsOverrun() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kHybrid;
    policy.rendering_enabled = true;
    policy.timing_mode = framekit::runtime::TimingMode::kFixedDelta;
    policy.overrun_mode = framekit::runtime::OverrunMode::kFailFast;
    policy.fixed_delta_ns = 1000000;

    framekit::runtime::LoopStageGraphRunner runner;
    REQUIRE(runner.Configure(policy));
    REQUIRE(!runner.ExecuteFrame(policy.fixed_delta_ns * 2));
    REQUIRE(runner.LastError().find("timing overrun") != std::string::npos);
}

void TestDropNonCriticalDropsOptionalRenderStagesForDeterministicHost() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kDeterministicHost;
    policy.rendering_enabled = true;
    policy.timing_mode = framekit::runtime::TimingMode::kFixedDelta;
    policy.overrun_mode = framekit::runtime::OverrunMode::kDropNonCritical;
    policy.fixed_delta_ns = 1000000;

    framekit::runtime::LoopStageGraphRunner runner;
    REQUIRE(runner.Configure(policy));

    int render_calls = 0;
    runner.SetStageHandler(framekit::runtime::LoopStage::kPreRender, [&render_calls]() {
        render_calls += 1;
    });
    runner.SetStageHandler(framekit::runtime::LoopStage::kRender, [&render_calls]() {
        render_calls += 1;
    });
    runner.SetStageHandler(framekit::runtime::LoopStage::kPostRender, [&render_calls]() {
        render_calls += 1;
    });

    REQUIRE(runner.ExecuteFrame(policy.fixed_delta_ns * 2));
    REQUIRE(runner.LastFrameDroppedNonCriticalStages());
    REQUIRE(runner.LastUpdateStepCount() == 1);
    REQUIRE(runner.LastDroppedUpdateStepCount() == 1);
    REQUIRE(render_calls == 0);

    const auto executed_names = StageNames(runner.LastExecutedStages());
    REQUIRE(std::find(executed_names.begin(), executed_names.end(), "PreRender") == executed_names.end());
    REQUIRE(std::find(executed_names.begin(), executed_names.end(), "Render") == executed_names.end());
    REQUIRE(std::find(executed_names.begin(), executed_names.end(), "PostRender") == executed_names.end());
}

void TestDropNonCriticalDoesNotDropRequiredRenderStagesInGuiProfile() {
    framekit::runtime::LoopPolicy policy;
    policy.profile = framekit::runtime::LoopProfile::kGui;
    policy.rendering_enabled = true;
    policy.timing_mode = framekit::runtime::TimingMode::kFixedDelta;
    policy.overrun_mode = framekit::runtime::OverrunMode::kDropNonCritical;
    policy.fixed_delta_ns = 1000000;

    framekit::runtime::LoopStageGraphRunner runner;
    REQUIRE(runner.Configure(policy));

    int render_calls = 0;
    runner.SetStageHandler(framekit::runtime::LoopStage::kRender, [&render_calls]() {
        render_calls += 1;
    });

    REQUIRE(runner.ExecuteFrame(policy.fixed_delta_ns * 2));
    REQUIRE(!runner.LastFrameDroppedNonCriticalStages());
    REQUIRE(runner.LastDroppedUpdateStepCount() == 1);
    REQUIRE(render_calls == 1);
}

} // namespace

int main() {
    TestBaselineGuiExecutionOrder();
    TestHeadlessRenderStagesOmitted();
    TestInvalidPolicyRejected();
    TestUnknownDisabledStageRejected();
    TestFixedDeltaCatchUpBoundedRunsExtraUpdateSteps();
    TestFixedDeltaCatchUpBoundedDropsBeyondBudget();
    TestFixedDeltaFailFastRejectsOverrun();
    TestDropNonCriticalDropsOptionalRenderStagesForDeterministicHost();
    TestDropNonCriticalDoesNotDropRequiredRenderStagesInGuiProfile();
    return 0;
}
