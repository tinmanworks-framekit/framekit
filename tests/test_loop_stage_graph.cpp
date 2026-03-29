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

} // namespace

int main() {
    TestBaselineGuiExecutionOrder();
    TestHeadlessRenderStagesOmitted();
    TestInvalidPolicyRejected();
    TestUnknownDisabledStageRejected();
    return 0;
}
