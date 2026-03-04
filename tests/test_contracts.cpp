#include "framekit/framekit.hpp"

#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>

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

class FakeProcessLauncher : public framekit::runtime::IProcessLauncher {
public:
    std::optional<framekit::runtime::ChildProcessHandle> LaunchChild(
        const framekit::ProcessIdentity& parent,
        const framekit::ProcessSpec& spec) override {
        (void)parent;
        framekit::runtime::ChildProcessHandle handle;
        handle.running = true;
        handle.identity.process_id = next_pid_++;
        handle.identity.role_name = spec.name;
        handle.identity.role = spec.role;
        running_.push_back(handle.identity);
        return handle;
    }

    bool StopChild(const framekit::ProcessIdentity& identity) override {
        for (auto it = running_.begin(); it != running_.end(); ++it) {
            if (it->process_id == identity.process_id) {
                running_.erase(it);
                return true;
            }
        }
        return false;
    }

    std::vector<framekit::ProcessIdentity> RunningChildren() const override {
        return running_;
    }

private:
    std::uint64_t next_pid_ = 42;
    std::vector<framekit::ProcessIdentity> running_;
};

class FakeSupervisorPolicy : public framekit::runtime::ISupervisorPolicy {
public:
    bool ShouldRestart(const framekit::ProcessIdentity& identity, int exit_code) override {
        (void)identity;
        (void)exit_code;
        return false;
    }

    void OnHeartbeatMissed(const framekit::ProcessIdentity& identity) override {
        missed_heartbeat_ids.push_back(identity.process_id);
    }

    void OnStopped(const framekit::ProcessIdentity& identity) override {
        stopped_ids.push_back(identity.process_id);
    }

    std::vector<std::uint64_t> missed_heartbeat_ids;
    std::vector<std::uint64_t> stopped_ids;
};

class FakeLivenessPolicy : public framekit::runtime::ILivenessPolicy {
public:
    void OnHeartbeat(const framekit::runtime::HeartbeatEvent& event) override {
        heartbeat_ids.push_back(event.child.process_id);
    }

    bool IsProcessAlive(
        const framekit::ProcessIdentity& identity,
        std::uint64_t observed_heartbeat_count) const override {
        (void)identity;
        return observed_heartbeat_count > 0;
    }

    std::vector<std::uint64_t> heartbeat_ids;
};

class FakeLifecycleHooks : public framekit::runtime::ILifecycleHooks {
public:
    void OnGracefulStopRequested(const framekit::ProcessIdentity& identity) override {
        stop_requested_ids.push_back(identity.process_id);
    }

    void OnGracefulStopCompleted(const framekit::ProcessIdentity& identity, bool stopped) override {
        (void)identity;
        if (stopped) {
            stop_completed += 1;
        }
    }

    void OnRestartRequested(const framekit::ProcessIdentity& identity) override {
        restart_requested_ids.push_back(identity.process_id);
    }

    void OnRestartCompleted(
        const framekit::ProcessIdentity& old_identity,
        const framekit::ProcessIdentity& new_identity,
        bool restarted) override {
        if (restarted) {
            restart_from_ids.push_back(old_identity.process_id);
            restart_to_ids.push_back(new_identity.process_id);
        }
    }

    int stop_completed = 0;
    std::vector<std::uint64_t> stop_requested_ids;
    std::vector<std::uint64_t> restart_requested_ids;
    std::vector<std::uint64_t> restart_from_ids;
    std::vector<std::uint64_t> restart_to_ids;
};

} // namespace

int main() {
    framekit::ApplicationSpec single_spec;
    REQUIRE(single_spec.mode == framekit::AppMode::kSingleProcess);
    REQUIRE(single_spec.transport_kind == framekit::IpcTransportKind::kNone);

    framekit::runtime::MultiprocessRuntime runtime;
    runtime.Configure(single_spec);

    REQUIRE(runtime.Start());
    REQUIRE(runtime.IsRunning());
    runtime.Stop();
    REQUIRE(!runtime.IsRunning());

    framekit::ApplicationSpec multiprocess_spec;
    multiprocess_spec.mode = framekit::AppMode::kMultiProcess;

    framekit::ProcessSpec backend_spec;
    backend_spec.name = "backend";
    backend_spec.role = framekit::ProcessRole::kBackend;
    backend_spec.executable_path = "bin/backend_app";
    multiprocess_spec.process_topology.nodes.push_back(backend_spec);

    auto launcher = std::make_shared<FakeProcessLauncher>();
    auto supervisor = std::make_shared<FakeSupervisorPolicy>();
    auto liveness = std::make_shared<FakeLivenessPolicy>();
    auto lifecycle = std::make_shared<FakeLifecycleHooks>();
    runtime.Configure(multiprocess_spec);
    runtime.SetProcessLauncher(launcher);
    runtime.SetSupervisorPolicy(supervisor);
    runtime.SetLivenessPolicy(liveness);
    runtime.SetLifecycleHooks(lifecycle);

    REQUIRE(runtime.Start());
    REQUIRE(runtime.IsRunning());
    REQUIRE(runtime.ChildProcesses().size() == 1);
    const auto child = runtime.ChildProcesses().front();
    REQUIRE(child.role == framekit::ProcessRole::kBackend);

    const auto handshake = runtime.ChildHandshake(child.process_id);
    REQUIRE(handshake.has_value());
    REQUIRE(handshake->state == framekit::runtime::ChildHandshakeState::kSpawned);

    runtime.SetChildHandshakeState(
        child.process_id,
        framekit::runtime::ChildHandshakeState::kReady,
        "ready-signal");
    const auto updated = runtime.ChildHandshake(child.process_id);
    REQUIRE(updated.has_value());
    REQUIRE(updated->state == framekit::runtime::ChildHandshakeState::kReady);
    REQUIRE(updated->detail == "ready-signal");

    REQUIRE(runtime.AllChildHandshakes().size() == 1);

    runtime.Tick();
    REQUIRE(supervisor->missed_heartbeat_ids.size() == 1);
    REQUIRE(supervisor->missed_heartbeat_ids.front() == child.process_id);

    runtime.RecordHeartbeat(child.process_id, 1);
    REQUIRE(runtime.HeartbeatCount(child.process_id) == 1);
    REQUIRE(liveness->heartbeat_ids.size() == 1);
    REQUIRE(liveness->heartbeat_ids.front() == child.process_id);

    runtime.Tick();
    REQUIRE(supervisor->missed_heartbeat_ids.size() == 1);

    REQUIRE(runtime.RestartChild(child.process_id));
    REQUIRE(lifecycle->restart_requested_ids.size() == 1);
    REQUIRE(lifecycle->restart_requested_ids.front() == child.process_id);
    REQUIRE(lifecycle->restart_from_ids.size() == 1);
    REQUIRE(lifecycle->restart_to_ids.size() == 1);
    REQUIRE(lifecycle->restart_to_ids.front() != child.process_id);
    REQUIRE(runtime.ChildProcesses().size() == 1);

    const auto restarted_child = runtime.ChildProcesses().front();
    REQUIRE(runtime.GracefulStopChild(restarted_child.process_id));
    REQUIRE(lifecycle->stop_requested_ids.size() == 1);
    REQUIRE(lifecycle->stop_requested_ids.front() == restarted_child.process_id);
    REQUIRE(lifecycle->stop_completed == 1);
    REQUIRE(runtime.ChildProcesses().empty());

    runtime.Stop();
    REQUIRE(!runtime.IsRunning());

    return 0;
}
