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
        launch_calls += 1;

        if (!fail_launch_role_name.empty() && fail_launch_role_name == spec.name) {
            return std::nullopt;
        }

        framekit::runtime::ChildProcessHandle handle;
        handle.running = true;
        handle.identity.process_id = next_pid_++;
        handle.identity.role_name = spec.name;
        handle.identity.role = spec.role;
        running_.push_back(handle.identity);
        return handle;
    }

    bool StopChild(const framekit::ProcessIdentity& identity) override {
        stop_calls += 1;
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

    int launch_calls = 0;
    int stop_calls = 0;
    std::string fail_launch_role_name;

private:
    std::uint64_t next_pid_ = 42;
    std::vector<framekit::ProcessIdentity> running_;
};

class FakeSupervisorPolicy : public framekit::runtime::ISupervisorPolicy {
public:
    bool ShouldRestart(const framekit::ProcessIdentity& identity, int exit_code) override {
        (void)identity;
        (void)exit_code;
        should_restart_calls += 1;
        return restart_on_missed;
    }

    void OnHeartbeatMissed(const framekit::ProcessIdentity& identity) override {
        missed_heartbeat_ids.push_back(identity.process_id);
    }

    void OnStopped(const framekit::ProcessIdentity& identity) override {
        stopped_ids.push_back(identity.process_id);
    }

    int should_restart_calls = 0;
    bool restart_on_missed = false;
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

class FakeControlChannel : public framekit::ipc::IControlChannel {
public:
    bool Send(const framekit::ipc::ControlMessage& message) override {
        if (!open) {
            return false;
        }

        sent_messages.push_back(message);
        return true;
    }

    std::optional<framekit::ipc::ControlMessage> Receive() override {
        return next_message;
    }

    bool IsOpen() const override {
        return open;
    }

    void Close() override {
        open = false;
        close_calls += 1;
    }

    bool open = true;
    int close_calls = 0;
    std::vector<framekit::ipc::ControlMessage> sent_messages;
    std::optional<framekit::ipc::ControlMessage> next_message;
};

class FakeDataPlane : public framekit::ipc::IDataPlane {
public:
    bool Write(const std::uint8_t* data, std::size_t size) override {
        if (size > capacity) {
            return false;
        }

        written.assign(data, data + size);
        return true;
    }

    std::vector<std::uint8_t> Read() override {
        return written;
    }

    std::size_t Capacity() const override {
        return capacity;
    }

    std::size_t capacity = 1024;
    std::vector<std::uint8_t> written;
};

class FakeTransportFactory : public framekit::ipc::ITransportFactory {
public:
    framekit::ipc::TransportBundle Create(const framekit::ipc::TransportConfig& config) override {
        create_calls += 1;
        last_config = config;

        framekit::ipc::TransportBundle bundle;
        if (!return_null_control_channel) {
            auto control = std::make_unique<FakeControlChannel>();
            control->open = !return_closed_control_channel;
            last_control_channel = control.get();
            bundle.control_channel = std::move(control);
        } else {
            last_control_channel = nullptr;
        }

        if (!return_null_data_plane) {
            auto data = std::make_unique<FakeDataPlane>();
            last_data_plane = data.get();
            bundle.data_plane = std::move(data);
        } else {
            last_data_plane = nullptr;
        }

        return bundle;
    }

    int create_calls = 0;
    bool return_null_control_channel = false;
    bool return_null_data_plane = false;
    bool return_closed_control_channel = false;
    framekit::ipc::TransportConfig last_config;
    FakeControlChannel* last_control_channel = nullptr;
    FakeDataPlane* last_data_plane = nullptr;
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
    backend_spec.auto_restart = true;
    backend_spec.max_restart_attempts = 2;
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
    REQUIRE(supervisor->should_restart_calls == 1);
    REQUIRE(runtime.ChildProcesses().size() == 1);
    REQUIRE(runtime.ChildProcesses().front().process_id == child.process_id);

    runtime.RecordHeartbeat(child.process_id, 1);
    REQUIRE(runtime.HeartbeatCount(child.process_id) == 1);
    REQUIRE(liveness->heartbeat_ids.size() == 1);
    REQUIRE(liveness->heartbeat_ids.front() == child.process_id);

    runtime.Tick();
    REQUIRE(supervisor->missed_heartbeat_ids.size() == 1);

    REQUIRE(runtime.RestartChild(child.process_id));
    REQUIRE(runtime.RestartAttemptCount("backend") == 1);
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

    framekit::ApplicationSpec defaults_spec;
    defaults_spec.mode = framekit::AppMode::kMultiProcess;

    framekit::ProcessSpec defaults_backend_spec;
    defaults_backend_spec.name = "backend-default";
    defaults_backend_spec.role = framekit::ProcessRole::kBackend;
    defaults_backend_spec.executable_path = "bin/backend_default_app";
    defaults_backend_spec.auto_restart = true;
    defaults_backend_spec.max_restart_attempts = 1;
    defaults_spec.process_topology.nodes.push_back(defaults_backend_spec);

    framekit::runtime::MultiprocessRuntime defaults_runtime;
    auto defaults_launcher = std::make_shared<FakeProcessLauncher>();
    defaults_runtime.Configure(defaults_spec);
    defaults_runtime.SetProcessLauncher(defaults_launcher);

    REQUIRE(defaults_runtime.Start());
    REQUIRE(defaults_runtime.ChildProcesses().size() == 1);
    const auto defaults_initial_pid = defaults_runtime.ChildProcesses().front().process_id;

    defaults_runtime.Tick();
    REQUIRE(defaults_runtime.ChildProcesses().size() == 1);
    const auto defaults_restarted_pid = defaults_runtime.ChildProcesses().front().process_id;
    REQUIRE(defaults_restarted_pid != defaults_initial_pid);
    REQUIRE(defaults_runtime.RestartAttemptCount("backend-default") == 1);

    defaults_runtime.Tick();
    REQUIRE(defaults_runtime.ChildProcesses().size() == 1);
    REQUIRE(defaults_runtime.ChildProcesses().front().process_id == defaults_restarted_pid);
    REQUIRE(defaults_runtime.RestartAttemptCount("backend-default") == 1);

    defaults_runtime.RecordHeartbeat(defaults_restarted_pid, 1);
    defaults_runtime.Tick();
    REQUIRE(defaults_runtime.ChildProcesses().front().process_id == defaults_restarted_pid);

    defaults_runtime.Stop();
    REQUIRE(!defaults_runtime.IsRunning());

    framekit::ApplicationSpec disabled_restart_spec;
    disabled_restart_spec.mode = framekit::AppMode::kMultiProcess;

    framekit::ProcessSpec disabled_restart_backend_spec;
    disabled_restart_backend_spec.name = "backend-no-restart";
    disabled_restart_backend_spec.role = framekit::ProcessRole::kBackend;
    disabled_restart_backend_spec.executable_path = "bin/backend_no_restart_app";
    disabled_restart_backend_spec.auto_restart = false;
    disabled_restart_backend_spec.max_restart_attempts = 3;
    disabled_restart_spec.process_topology.nodes.push_back(disabled_restart_backend_spec);

    framekit::runtime::MultiprocessRuntime disabled_restart_runtime;
    auto disabled_restart_launcher = std::make_shared<FakeProcessLauncher>();
    disabled_restart_runtime.Configure(disabled_restart_spec);
    disabled_restart_runtime.SetProcessLauncher(disabled_restart_launcher);

    REQUIRE(disabled_restart_runtime.Start());
    REQUIRE(disabled_restart_runtime.ChildProcesses().size() == 1);
    const auto no_restart_pid = disabled_restart_runtime.ChildProcesses().front().process_id;
    REQUIRE(!disabled_restart_runtime.RestartChild(no_restart_pid));
    REQUIRE(disabled_restart_runtime.RestartAttemptCount("backend-no-restart") == 0);
    disabled_restart_runtime.Stop();
    REQUIRE(!disabled_restart_runtime.IsRunning());

    framekit::ApplicationSpec partial_launch_failure_spec;
    partial_launch_failure_spec.mode = framekit::AppMode::kMultiProcess;

    framekit::ProcessSpec launch_backend_spec;
    launch_backend_spec.name = "launch-backend";
    launch_backend_spec.role = framekit::ProcessRole::kBackend;
    launch_backend_spec.executable_path = "bin/launch_backend";
    partial_launch_failure_spec.process_topology.nodes.push_back(launch_backend_spec);

    framekit::ProcessSpec launch_worker_spec;
    launch_worker_spec.name = "launch-worker";
    launch_worker_spec.role = framekit::ProcessRole::kWorker;
    launch_worker_spec.executable_path = "bin/launch_worker";
    partial_launch_failure_spec.process_topology.nodes.push_back(launch_worker_spec);

    framekit::runtime::MultiprocessRuntime partial_launch_failure_runtime;
    auto partial_launch_failure_launcher = std::make_shared<FakeProcessLauncher>();
    partial_launch_failure_launcher->fail_launch_role_name = "launch-worker";
    partial_launch_failure_runtime.Configure(partial_launch_failure_spec);
    partial_launch_failure_runtime.SetProcessLauncher(partial_launch_failure_launcher);

    REQUIRE(!partial_launch_failure_runtime.Start());
    REQUIRE(partial_launch_failure_runtime.LastError().find("failed to launch child process: launch-worker") != std::string::npos);
    REQUIRE(partial_launch_failure_runtime.ChildProcesses().empty());
    REQUIRE(partial_launch_failure_runtime.AllChildHandshakes().empty());
    REQUIRE(partial_launch_failure_launcher->launch_calls == 2);
    REQUIRE(partial_launch_failure_launcher->stop_calls == 1);
    REQUIRE(partial_launch_failure_launcher->RunningChildren().empty());

    framekit::ApplicationSpec transport_spec;
    transport_spec.application_name = "framekit-transport-test";
    transport_spec.mode = framekit::AppMode::kMultiProcess;
    transport_spec.transport_kind = framekit::IpcTransportKind::kShmPipe;
    transport_spec.transport_profile = "integration";

    framekit::ProcessSpec transport_backend_spec;
    transport_backend_spec.name = "backend-transport";
    transport_backend_spec.role = framekit::ProcessRole::kBackend;
    transport_backend_spec.executable_path = "bin/backend_transport";
    transport_spec.process_topology.nodes.push_back(transport_backend_spec);

    framekit::runtime::MultiprocessRuntime no_factory_transport_runtime;
    auto no_factory_launcher = std::make_shared<FakeProcessLauncher>();
    no_factory_transport_runtime.Configure(transport_spec);
    no_factory_transport_runtime.SetProcessLauncher(no_factory_launcher);
    REQUIRE(!no_factory_transport_runtime.Start());
    REQUIRE(no_factory_transport_runtime.LastError().find("transport factory is required") != std::string::npos);

    framekit::runtime::MultiprocessRuntime invalid_transport_runtime;
    auto invalid_launcher = std::make_shared<FakeProcessLauncher>();
    auto invalid_factory = std::make_shared<FakeTransportFactory>();
    invalid_factory->return_null_data_plane = true;
    invalid_transport_runtime.Configure(transport_spec);
    invalid_transport_runtime.SetProcessLauncher(invalid_launcher);
    invalid_transport_runtime.SetTransportFactory(invalid_factory);
    REQUIRE(!invalid_transport_runtime.Start());
    REQUIRE(invalid_transport_runtime.LastError().find("null data plane") != std::string::npos);

    framekit::runtime::MultiprocessRuntime closed_control_runtime;
    auto closed_control_launcher = std::make_shared<FakeProcessLauncher>();
    auto closed_control_factory = std::make_shared<FakeTransportFactory>();
    closed_control_factory->return_closed_control_channel = true;
    closed_control_runtime.Configure(transport_spec);
    closed_control_runtime.SetProcessLauncher(closed_control_launcher);
    closed_control_runtime.SetTransportFactory(closed_control_factory);
    REQUIRE(!closed_control_runtime.Start());
    REQUIRE(closed_control_runtime.LastError().find("not open") != std::string::npos);

    framekit::ApplicationSpec valid_transport_spec = transport_spec;
    valid_transport_spec.transport_kind = framekit::IpcTransportKind::kLocalSocket;
    valid_transport_spec.transport_profile = "local-latency";

    framekit::runtime::MultiprocessRuntime valid_transport_runtime;
    auto valid_transport_launcher = std::make_shared<FakeProcessLauncher>();
    auto valid_transport_factory = std::make_shared<FakeTransportFactory>();
    valid_transport_runtime.Configure(valid_transport_spec);
    valid_transport_runtime.SetProcessLauncher(valid_transport_launcher);
    valid_transport_runtime.SetTransportFactory(valid_transport_factory);
    REQUIRE(valid_transport_runtime.Start());
    REQUIRE(valid_transport_runtime.IsRunning());
    REQUIRE(valid_transport_runtime.HasActiveTransport());
    REQUIRE(valid_transport_factory->create_calls == 1);

    const auto& active_transport_config = valid_transport_runtime.ActiveTransportConfig();
    REQUIRE(active_transport_config.name == "local_socket");

    const auto profile_it = active_transport_config.values.find("profile");
    REQUIRE(profile_it != active_transport_config.values.end());
    REQUIRE(profile_it->second == "local-latency");

    const auto app_it = active_transport_config.values.find("application");
    REQUIRE(app_it != active_transport_config.values.end());
    REQUIRE(app_it->second == "framekit-transport-test");

    valid_transport_runtime.Stop();
    REQUIRE(!valid_transport_runtime.IsRunning());
    REQUIRE(!valid_transport_runtime.HasActiveTransport());
    REQUIRE(valid_transport_factory->create_calls == 1);
    REQUIRE(valid_transport_runtime.LastError().empty());

    return 0;
}
