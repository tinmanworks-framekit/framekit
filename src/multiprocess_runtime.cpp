#include "framekit/multiprocess/runtime.hpp"

#include <algorithm>

namespace {

const char* TransportKindName(framekit::IpcTransportKind kind) {
    switch (kind) {
        case framekit::IpcTransportKind::kNone:
            return "none";
        case framekit::IpcTransportKind::kShmPipe:
            return "shm_pipe";
        case framekit::IpcTransportKind::kLocalSocket:
            return "local_socket";
        case framekit::IpcTransportKind::kCustom:
            return "custom";
    }

    return "unknown";
}

class DefaultSupervisorPolicy final : public framekit::runtime::ISupervisorPolicy {
public:
    bool ShouldRestart(const framekit::ProcessIdentity& identity, int exit_code) override {
        (void)identity;
        (void)exit_code;
        return true;
    }

    void OnHeartbeatMissed(const framekit::ProcessIdentity& identity) override {
        (void)identity;
    }

    void OnStopped(const framekit::ProcessIdentity& identity) override {
        (void)identity;
    }
};

class DefaultLivenessPolicy final : public framekit::runtime::ILivenessPolicy {
public:
    void OnHeartbeat(const framekit::runtime::HeartbeatEvent& event) override {
        (void)event;
    }

    bool IsProcessAlive(
        const framekit::ProcessIdentity& identity,
        std::uint64_t observed_heartbeat_count) const override {
        (void)identity;
        return observed_heartbeat_count > 0;
    }
};

} // namespace

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

void MultiprocessRuntime::SetTransportFactory(std::shared_ptr<framekit::ipc::ITransportFactory> factory) {
    transport_factory_ = std::move(factory);
}

void MultiprocessRuntime::SetLivenessPolicy(std::shared_ptr<ILivenessPolicy> policy) {
    liveness_policy_ = std::move(policy);
}

void MultiprocessRuntime::SetLifecycleHooks(std::shared_ptr<ILifecycleHooks> hooks) {
    lifecycle_hooks_ = std::move(hooks);
}

bool MultiprocessRuntime::Start() {
    if (running_) {
        last_error_ = "multiprocess runtime is already running";
        return false;
    }

    last_error_.clear();

    if (!supervisor_policy_) {
        supervisor_policy_ = std::make_shared<DefaultSupervisorPolicy>();
    }

    if (!liveness_policy_) {
        liveness_policy_ = std::make_shared<DefaultLivenessPolicy>();
    }

    if (spec_.mode == framekit::AppMode::kMultiProcess) {
        if (spec_.transport_kind != framekit::IpcTransportKind::kNone && !ConfigureTransport()) {
            return false;
        }

        if (!LaunchChildren()) {
            if (transport_bundle_.control_channel) {
                transport_bundle_.control_channel->Close();
            }
            transport_bundle_ = {};
            active_transport_config_ = {};
            if (last_error_.empty()) {
                last_error_ = "failed to launch child processes";
            }
            return false;
        }
    }

    running_ = true;
    return true;
}

void MultiprocessRuntime::Tick() {
    if (!running_ || !supervisor_policy_ || !liveness_policy_) {
        return;
    }

    std::vector<std::uint64_t> process_ids;
    process_ids.reserve(child_processes_.size());
    for (const auto& child : child_processes_) {
        process_ids.push_back(child.process_id);
    }

    for (const auto process_id : process_ids) {
        const auto child_it = std::find_if(
            child_processes_.begin(),
            child_processes_.end(),
            [process_id](const framekit::ProcessIdentity& identity) {
                return identity.process_id == process_id;
            });

        if (child_it == child_processes_.end()) {
            continue;
        }

        const auto child = *child_it;
        const auto heartbeat_count = HeartbeatCount(process_id);
        if (!liveness_policy_->IsProcessAlive(child, heartbeat_count)) {
            supervisor_policy_->OnHeartbeatMissed(child);

            if (supervisor_policy_->ShouldRestart(child, -1)) {
                (void)RestartChild(process_id);
            }
        }
    }
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
    restart_attempt_counters_.clear();

    if (transport_bundle_.control_channel) {
        transport_bundle_.control_channel->Close();
    }
    transport_bundle_ = {};
    active_transport_config_ = {};
    last_error_.clear();
}

bool MultiprocessRuntime::ConfigureTransport() {
    if (!transport_factory_) {
        last_error_ = "transport factory is required when transport_kind is not none";
        return false;
    }

    framekit::ipc::TransportConfig config;
    config.name = TransportKindName(spec_.transport_kind);
    config.values["application"] = spec_.application_name;
    config.values["profile"] = spec_.transport_profile;
    config.values["kind"] = config.name;

    auto bundle = transport_factory_->Create(config);
    if (!bundle.control_channel) {
        last_error_ = "transport factory returned null control channel";
        return false;
    }

    if (!bundle.control_channel->IsOpen()) {
        bundle.control_channel->Close();
        last_error_ = "transport control channel is not open";
        return false;
    }

    if (!bundle.data_plane) {
        bundle.control_channel->Close();
        last_error_ = "transport factory returned null data plane";
        return false;
    }

    active_transport_config_ = std::move(config);
    transport_bundle_ = std::move(bundle);
    return true;
}

bool MultiprocessRuntime::LaunchChildren() {
    child_processes_.clear();
    child_handshakes_.clear();
    heartbeat_counters_.clear();
    restart_attempt_counters_.clear();

    if (!process_launcher_) {
        if (!spec_.process_topology.nodes.empty()) {
            last_error_ = "process launcher is required for non-empty process topology";
            return false;
        }

        return true;
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
            last_error_ = "failed to launch child process: " + node.name;
            return false;
        }

        child_processes_.push_back(launched->identity);
        SetChildHandshakeState(launched->identity.process_id, ChildHandshakeState::kSpawned);
        heartbeat_counters_[launched->identity.process_id] = 0;
        restart_attempt_counters_[node.name] = 0;
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

std::uint32_t MultiprocessRuntime::RestartAttemptCount(const std::string& process_name) const {
    const auto it = restart_attempt_counters_.find(process_name);
    if (it == restart_attempt_counters_.end()) {
        return 0;
    }

    return it->second;
}

bool MultiprocessRuntime::HasActiveTransport() const {
    return transport_bundle_.control_channel &&
        transport_bundle_.data_plane &&
        transport_bundle_.control_channel->IsOpen();
}

const framekit::ipc::TransportConfig& MultiprocessRuntime::ActiveTransportConfig() const {
    return active_transport_config_;
}

const std::string& MultiprocessRuntime::LastError() const {
    return last_error_;
}

const framekit::ProcessSpec* MultiprocessRuntime::FindProcessSpec(
    const framekit::ProcessIdentity& identity) const {
    for (const auto& spec : spec_.process_topology.nodes) {
        if (spec.name == identity.role_name && spec.role == identity.role) {
            return &spec;
        }
    }

    return nullptr;
}

bool MultiprocessRuntime::ConsumeRestartBudget(const framekit::ProcessIdentity& identity) {
    const auto* process_spec = FindProcessSpec(identity);
    if (!process_spec) {
        last_error_ = "process spec not found for restart target";
        return false;
    }

    if (!process_spec->auto_restart) {
        last_error_ = "auto restart is disabled for process: " + process_spec->name;
        return false;
    }

    auto& restart_count = restart_attempt_counters_[process_spec->name];
    if (restart_count >= process_spec->max_restart_attempts) {
        last_error_ = "restart budget exhausted for process: " + process_spec->name;
        return false;
    }

    restart_count += 1;
    return true;
}

bool MultiprocessRuntime::GracefulStopChild(std::uint64_t process_id) {
    if (!process_launcher_) {
        last_error_ = "process launcher is not configured";
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
            last_error_ = "failed to stop child process";
            return false;
        }

        child_processes_.erase(it);
        child_handshakes_.erase(identity.process_id);
        heartbeat_counters_.erase(identity.process_id);
        return true;
    }

    last_error_ = "child process not found";
    return false;
}

bool MultiprocessRuntime::RestartChild(std::uint64_t process_id) {
    last_error_.clear();

    for (const auto& child : child_processes_) {
        if (child.process_id != process_id) {
            continue;
        }

        if (!ConsumeRestartBudget(child)) {
            return false;
        }

        return RelaunchChild(child);
    }

    last_error_ = "child process not found for restart";
    return false;
}

bool MultiprocessRuntime::RelaunchChild(const framekit::ProcessIdentity& old_identity) {
    if (!process_launcher_) {
        last_error_ = "process launcher is not configured";
        return false;
    }

    if (lifecycle_hooks_) {
        lifecycle_hooks_->OnRestartRequested(old_identity);
    }

    if (!process_launcher_->StopChild(old_identity)) {
        last_error_ = "failed to stop child process before restart";
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
                last_error_ = "failed to relaunch child process";
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

            last_error_.clear();
            return true;
        }
    }

    last_error_ = "process spec not found for restart";
    if (lifecycle_hooks_) {
        lifecycle_hooks_->OnRestartCompleted(old_identity, old_identity, false);
    }
    return false;
}

} // namespace framekit::runtime
