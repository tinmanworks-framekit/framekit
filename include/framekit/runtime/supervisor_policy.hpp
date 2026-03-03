#pragma once

#include "framekit/process_role.hpp"

namespace framekit::runtime {

class ISupervisorPolicy {
public:
    virtual ~ISupervisorPolicy() = default;

    virtual bool ShouldRestart(const ProcessIdentity& identity, int exit_code) = 0;
    virtual void OnHeartbeatMissed(const ProcessIdentity& identity) = 0;
    virtual void OnStopped(const ProcessIdentity& identity) = 0;
};

} // namespace framekit::runtime
