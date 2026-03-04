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

void MultiprocessRuntime::SetLivenessPolicy(std::shared_ptr<ILivenessPolicy> policy) {
    liveness_policy_ = std::move(policy);
}

void MultiprocessRuntime::SetLifecycleHooks(std::shared_ptr<ILifecycleHooks> hooks) {
    lifecycle_hooks_ = std::move(hooks);
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
    if (!supervisor_policy_ || !liveness_policy_) {
        return;
    }

    for (const auto& child : child_processes_) {
        const auto heartbeat_count = HeartbeatCount(child.process_id);
        if (!liveness_policy_->IsProcessAlive(child, heartbeat_count)) {
            supervisor_policy_->OnHeartbeatMissed(child);
        }
    }

    (void)spec_;
}

void MultiprocessRuntime::Stop() {
    std::vector<std::uint64_t> process_ids;
    process_ids.reserve(child_processes_.size());
    for (const auto& child : child_processes_) {
        process_ids.push_back(child.process_id);
    }

    for (const auto process_id : process_ids) {
        (void)GracefulStopChild(process_id);
    }

    running_ = false;
    child_processes_.clear();
    child_handshakes_.clear();
    heartbeat_counters_.clear();
}

bool MultiprocessRuntime::LaunchChildren() {
    child_processes_.clear();
    child_handshakes_.clear();
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
        SetChildHandshakeState(launched->identity.process_id, ChildHandshakeState::kSpawned);
        heartbeat_counters_[launched->identity.process_id] = 0;
    }

    return true;
}

void MultiprocessRuntime::SetChildHandshakeState(
    std::uint64_t process_id,
    ChildHandshakeState state,
    std::string detail) {
    auto it = child_handshakes_.find(process_id);
    if (it == child_handshakes_.end()) {
        ChildHandshakeStatus status;
        status.child.process_id = process_id;
        status.state = state;
        status.detail = std::move(detail);
        child_handshakes_.emplace(process_id, std::move(status));
        return;
    }

    it->second.state = state;
    it->second.detail = std::move(detail);
}

std::optional<ChildHandshakeStatus> MultiprocessRuntime::ChildHandshake(std::uint64_t process_id) const {
    const auto it = child_handshakes_.find(process_id);
    if (it == child_handshakes_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<ChildHandshakeStatus> MultiprocessRuntime::AllChildHandshakes() const {
    std::vector<ChildHandshakeStatus> statuses;
    statuses.reserve(child_handshakes_.size());
    for (const auto& item : child_handshakes_) {
        statuses.push_back(item.second);
    }
    return statuses;
}

void MultiprocessRuntime::RecordHeartbeat(std::uint64_t process_id, std::uint64_t sequence_id) {
    heartbeat_counters_[process_id] += 1;

    if (!liveness_policy_) {
        return;
    }

    for (const auto& child : child_processes_) {
        if (child.process_id != process_id) {
            continue;
        }
        HeartbeatEvent event;
        event.child = child;
        event.sequence_id = sequence_id;
        liveness_policy_->OnHeartbeat(event);
        break;
    }
}

std::uint64_t MultiprocessRuntime::HeartbeatCount(std::uint64_t process_id) const {
    const auto it = heartbeat_counters_.find(process_id);
    if (it == heartbeat_counters_.end()) {
        return 0;
    }
    return it->second;
}

bool MultiprocessRuntime::GracefulStopChild(std::uint64_t process_id) {
    if (!process_launcher_) {
        return false;
    }

    for (auto it = child_processes_.begin(); it != child_processes_.end(); ++it) {
        if (it->process_id != process_id) {
            continue;
        }

        const auto identity = *it;
        if (lifecycle_hooks_) {
            lifecycle_hooks_->OnGracefulStopRequested(identity);
        }

        const bool stopped = process_launcher_->StopChild(identity);
        if (supervisor_policy_) {
            supervisor_policy_->OnStopped(identity);
        }

        if (lifecycle_hooks_) {
            lifecycle_hooks_->OnGracefulStopCompleted(identity, stopped);
        }

        if (!stopped) {
            return false;
        }

        child_processes_.erase(it);
        child_handshakes_.erase(identity.process_id);
        heartbeat_counters_.erase(identity.process_id);
        return true;
    }

    return false;
}

bool MultiprocessRuntime::RestartChild(std::uint64_t process_id) {
    for (const auto& child : child_processes_) {
        if (child.process_id != process_id) {
            continue;
        }
        return RelaunchChild(child);
    }
    return false;
}

bool MultiprocessRuntime::RelaunchChild(const framekit::ProcessIdentity& old_identity) {
    if (!process_launcher_) {
        return false;
    }

    if (lifecycle_hooks_) {
        lifecycle_hooks_->OnRestartRequested(old_identity);
    }

    if (!process_launcher_->StopChild(old_identity)) {
        if (lifecycle_hooks_) {
            lifecycle_hooks_->OnRestartCompleted(old_identity, old_identity, false);
        }
        return false;
    }

    for (auto it = child_processes_.begin(); it != child_processes_.end(); ++it) {
        if (it->process_id != old_identity.process_id) {
            continue;
        }

        framekit::ProcessIdentity parent_identity;
        parent_identity.role = framekit::ProcessRole::kPrimary;
        parent_identity.role_name = "primary";
        parent_identity.process_id = 1;

        for (const auto& spec : spec_.process_topology.nodes) {
            if (spec.name != old_identity.role_name || spec.role != old_identity.role) {
                continue;
            }

            auto relaunched = process_launcher_->LaunchChild(parent_identity, spec);
            if (!relaunched.has_value() || !relaunched->running) {
                if (lifecycle_hooks_) {
                    lifecycle_hooks_->OnRestartCompleted(old_identity, old_identity, false);
                }
                return false;
            }

            const auto new_identity = relaunched->identity;
            *it = new_identity;

            child_handshakes_.erase(old_identity.process_id);
            child_handshakes_[new_identity.process_id] = ChildHandshakeStatus{
                .child = new_identity,
                .state = ChildHandshakeState::kSpawned,
                .detail = {}};

            heartbeat_counters_.erase(old_identity.process_id);
            heartbeat_counters_[new_identity.process_id] = 0;

            if (lifecycle_hooks_) {
                lifecycle_hooks_->OnRestartCompleted(old_identity, new_identity, true);
            }
            return true;
        }
    }

    if (lifecycle_hooks_) {
        lifecycle_hooks_->OnRestartCompleted(old_identity, old_identity, false);
    }
    return false;
}

} // namespace framekit::runtime
