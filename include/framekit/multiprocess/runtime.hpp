#pragma once

#include "framekit/ipc/transport.hpp"
#include "framekit/spec/app_spec.hpp"
#include "framekit/multiprocess/handshake.hpp"
#include "framekit/lifecycle/hooks.hpp"
#include "framekit/multiprocess/liveness.hpp"
#include "framekit/multiprocess/launcher.hpp"
#include "framekit/multiprocess/supervisor.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace framekit::runtime {

class MultiprocessRuntime {
public:
    MultiprocessRuntime() = default;

    void Configure(const framekit::ApplicationSpec& spec);
    void SetSupervisorPolicy(std::shared_ptr<ISupervisorPolicy> policy);
    void SetProcessLauncher(std::shared_ptr<IProcessLauncher> launcher);
    void SetTransportFactory(std::shared_ptr<framekit::ipc::ITransportFactory> factory);
    void SetLivenessPolicy(std::shared_ptr<ILivenessPolicy> policy);
    void SetLifecycleHooks(std::shared_ptr<ILifecycleHooks> hooks);

    bool Start();
    void Tick();
    void Stop();
    bool GracefulStopChild(std::uint64_t process_id);
    bool RestartChild(std::uint64_t process_id);

    bool IsRunning() const { return running_; }
    const std::vector<framekit::ProcessIdentity>& ChildProcesses() const { return child_processes_; }

    void SetChildHandshakeState(
        std::uint64_t process_id,
        ChildHandshakeState state,
        std::string detail = {});
    std::optional<ChildHandshakeStatus> ChildHandshake(std::uint64_t process_id) const;
    std::vector<ChildHandshakeStatus> AllChildHandshakes() const;

    void RecordHeartbeat(std::uint64_t process_id, std::uint64_t sequence_id);
    std::uint64_t HeartbeatCount(std::uint64_t process_id) const;
    std::uint32_t RestartAttemptCount(const std::string& process_name) const;
    bool HasActiveTransport() const;
    const framekit::ipc::TransportConfig& ActiveTransportConfig() const;
    const std::string& LastError() const;

private:
    bool ConfigureTransport();
    bool LaunchChildren();
    const framekit::ProcessSpec* FindProcessSpec(const framekit::ProcessIdentity& identity) const;
    bool ConsumeRestartBudget(const framekit::ProcessIdentity& identity);
    bool RelaunchChild(const framekit::ProcessIdentity& old_identity);

    framekit::ApplicationSpec spec_;
    std::shared_ptr<ISupervisorPolicy> supervisor_policy_;
    std::shared_ptr<IProcessLauncher> process_launcher_;
    std::shared_ptr<framekit::ipc::ITransportFactory> transport_factory_;
    std::shared_ptr<ILivenessPolicy> liveness_policy_;
    std::shared_ptr<ILifecycleHooks> lifecycle_hooks_;
    framekit::ipc::TransportBundle transport_bundle_;
    framekit::ipc::TransportConfig active_transport_config_;
    std::vector<framekit::ProcessIdentity> child_processes_;
    std::unordered_map<std::uint64_t, ChildHandshakeStatus> child_handshakes_;
    std::unordered_map<std::uint64_t, std::uint64_t> heartbeat_counters_;
    std::unordered_map<std::string, std::uint32_t> restart_attempt_counters_;
    std::string last_error_;
    bool running_ = false;
};

} // namespace framekit::runtime
