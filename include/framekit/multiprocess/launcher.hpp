#pragma once

#include "framekit/spec/process_role.hpp"
#include "framekit/spec/process_spec.hpp"

#include <optional>
#include <vector>

namespace framekit::runtime {

struct ChildProcessHandle {
    ProcessIdentity identity;
    bool running = false;
};

class IProcessLauncher {
public:
    virtual ~IProcessLauncher() = default;

    virtual std::optional<ChildProcessHandle> LaunchChild(
        const ProcessIdentity& parent,
        const ProcessSpec& spec) = 0;
    virtual bool StopChild(const ProcessIdentity& identity) = 0;
    virtual std::vector<ProcessIdentity> RunningChildren() const = 0;
};

} // namespace framekit::runtime
