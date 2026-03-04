#include "framekit/runtime/multiprocess_runtime.hpp"

namespace framekit::runtime {

void MultiprocessRuntime::Configure(const framekit::ApplicationSpec& spec) {
    spec_ = spec;
}

void MultiprocessRuntime::SetSupervisorPolicy(std::shared_ptr<ISupervisorPolicy> policy) {
    supervisor_policy_ = std::move(policy);
}

void MultiprocessRuntime::SetProcessLauncher(std::shared_ptr<IProcessLauncher> launcher) {
    process_launcher_ = std::move(launcher);
}

bool MultiprocessRuntime::Start() {
    if (running_) {
        return false;
    }

    if (spec_.mode == framekit::AppMode::kMultiProcess && !LaunchChildren()) {
        return false;
    }

    running_ = true;
    return true;
}

void MultiprocessRuntime::Tick() {
    // Skeleton runtime: process launching and heartbeat handling will be wired in follow-up issues.
    (void)spec_;
    (void)supervisor_policy_;
}

void MultiprocessRuntime::Stop() {
    running_ = false;
    child_processes_.clear();
}

bool MultiprocessRuntime::LaunchChildren() {
    child_processes_.clear();
    if (!process_launcher_) {
        return spec_.process_topology.nodes.empty();
    }

    framekit::ProcessIdentity parent_identity;
    parent_identity.role = framekit::ProcessRole::kPrimary;
    parent_identity.role_name = "primary";
    parent_identity.process_id = 1;

    for (const auto& node : spec_.process_topology.nodes) {
        if (node.role == framekit::ProcessRole::kPrimary) {
            continue;
        }

        auto launched = process_launcher_->LaunchChild(parent_identity, node);
        if (!launched.has_value() || !launched->running) {
            return false;
        }
        child_processes_.push_back(launched->identity);
    }

    return true;
}

} // namespace framekit::runtime
