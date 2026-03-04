#pragma once

#include "framekit/application_spec.hpp"
#include "framekit/runtime/child_handshake.hpp"
#include "framekit/runtime/liveness_policy.hpp"
#include "framekit/runtime/process_launcher.hpp"
#include "framekit/runtime/supervisor_policy.hpp"

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
    void SetLivenessPolicy(std::shared_ptr<ILivenessPolicy> policy);

    bool Start();
    void Tick();
    void Stop();

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

private:
    bool LaunchChildren();

    framekit::ApplicationSpec spec_;
    std::shared_ptr<ISupervisorPolicy> supervisor_policy_;
    std::shared_ptr<IProcessLauncher> process_launcher_;
    std::shared_ptr<ILivenessPolicy> liveness_policy_;
    std::vector<framekit::ProcessIdentity> child_processes_;
    std::unordered_map<std::uint64_t, ChildHandshakeStatus> child_handshakes_;
    std::unordered_map<std::uint64_t, std::uint64_t> heartbeat_counters_;
    bool running_ = false;
};

} // namespace framekit::runtime
