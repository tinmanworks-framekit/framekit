#include "framekit/framekit.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace {

class LocalWorkerLauncher : public framekit::runtime::IProcessLauncher {
public:
    std::optional<framekit::runtime::ChildProcessHandle> LaunchChild(
        const framekit::ProcessIdentity& parent,
        const framekit::ProcessSpec& spec) override {
        (void)parent;
        framekit::runtime::ChildProcessHandle handle;
        handle.running = true;
        handle.identity.process_id = next_pid_++;
        handle.identity.role = spec.role;
        handle.identity.role_name = spec.name;
        workers_.push_back(handle.identity);
        return handle;
    }

    bool StopChild(const framekit::ProcessIdentity& identity) override {
        for (auto it = workers_.begin(); it != workers_.end(); ++it) {
            if (it->process_id == identity.process_id) {
                workers_.erase(it);
                return true;
            }
        }
        return false;
    }

    std::vector<framekit::ProcessIdentity> RunningChildren() const override {
        return workers_;
    }

private:
    std::uint64_t next_pid_ = 1000;
    std::vector<framekit::ProcessIdentity> workers_;
};

} // namespace

int main() {
    framekit::ApplicationSpec spec;
    spec.application_name = "framekit-multi-worker";
    spec.mode = framekit::AppMode::kMultiProcess;

    framekit::ProcessSpec worker_a;
    worker_a.name = "worker-a";
    worker_a.role = framekit::ProcessRole::kWorker;
    worker_a.executable_path = "bin/worker-a";

    framekit::ProcessSpec worker_b;
    worker_b.name = "worker-b";
    worker_b.role = framekit::ProcessRole::kWorker;
    worker_b.executable_path = "bin/worker-b";

    spec.process_topology.nodes.push_back(worker_a);
    spec.process_topology.nodes.push_back(worker_b);
    spec.process_topology.dependencies["worker-b"] = {"worker-a"};

    framekit::runtime::MultiprocessRuntime runtime;
    auto launcher = std::make_shared<LocalWorkerLauncher>();
    runtime.Configure(spec);
    runtime.SetProcessLauncher(launcher);

    if (!runtime.Start()) {
        return 1;
    }
    if (runtime.ChildProcesses().size() != 2) {
        return 2;
    }

    for (const auto& child : runtime.ChildProcesses()) {
        runtime.RecordHeartbeat(child.process_id, 1);
    }

    runtime.Stop();
    return runtime.IsRunning() ? 3 : 0;
}
