#include "framekit/runtime/multiprocess_runtime.hpp"

namespace framekit::runtime {

void MultiprocessRuntime::Configure(const framekit::ApplicationSpec& spec) {
    spec_ = spec;
}

void MultiprocessRuntime::SetSupervisorPolicy(std::shared_ptr<ISupervisorPolicy> policy) {
    supervisor_policy_ = std::move(policy);
}

bool MultiprocessRuntime::Start() {
    if (running_) {
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
}

} // namespace framekit::runtime
