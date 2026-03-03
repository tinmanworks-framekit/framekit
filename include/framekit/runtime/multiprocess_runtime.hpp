#pragma once

#include "framekit/application_spec.hpp"
#include "framekit/runtime/supervisor_policy.hpp"

#include <memory>

namespace framekit::runtime {

class MultiprocessRuntime {
public:
    MultiprocessRuntime() = default;

    void Configure(const framekit::ApplicationSpec& spec);
    void SetSupervisorPolicy(std::shared_ptr<ISupervisorPolicy> policy);

    bool Start();
    void Tick();
    void Stop();

    bool IsRunning() const { return running_; }

private:
    framekit::ApplicationSpec spec_;
    std::shared_ptr<ISupervisorPolicy> supervisor_policy_;
    bool running_ = false;
};

} // namespace framekit::runtime
