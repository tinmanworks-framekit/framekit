#include "framekit/framekit.hpp"

#include <cassert>
#include <cstdint>
#include <memory>

namespace {

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

} // namespace

int main() {
    framekit::ApplicationSpec single_spec;
    assert(single_spec.mode == framekit::AppMode::kSingleProcess);
    assert(single_spec.transport_kind == framekit::IpcTransportKind::kNone);

    framekit::runtime::MultiprocessRuntime runtime;
    runtime.Configure(single_spec);

    assert(runtime.Start());
    assert(runtime.IsRunning());
    runtime.Stop();
    assert(!runtime.IsRunning());

    framekit::ApplicationSpec multiprocess_spec;
    multiprocess_spec.mode = framekit::AppMode::kMultiProcess;

    framekit::ProcessSpec backend_spec;
    backend_spec.name = "backend";
    backend_spec.role = framekit::ProcessRole::kBackend;
    backend_spec.executable_path = "bin/backend_app";
    multiprocess_spec.process_topology.nodes.push_back(backend_spec);

    auto launcher = std::make_shared<FakeProcessLauncher>();
    runtime.Configure(multiprocess_spec);
    runtime.SetProcessLauncher(launcher);

    assert(runtime.Start());
    assert(runtime.IsRunning());
    assert(runtime.ChildProcesses().size() == 1);
    assert(runtime.ChildProcesses().front().role == framekit::ProcessRole::kBackend);
    runtime.Stop();
    assert(!runtime.IsRunning());

    return 0;
}
