#pragma once

#include "framekit/spec/process_role.hpp"

namespace framekit::runtime {

class ILifecycleHooks {
public:
    virtual ~ILifecycleHooks() = default;

    virtual void OnGracefulStopRequested(const framekit::ProcessIdentity& identity) = 0;
    virtual void OnGracefulStopCompleted(
        const framekit::ProcessIdentity& identity,
        bool stopped) = 0;
    virtual void OnRestartRequested(const framekit::ProcessIdentity& identity) = 0;
    virtual void OnRestartCompleted(
        const framekit::ProcessIdentity& old_identity,
        const framekit::ProcessIdentity& new_identity,
        bool restarted) = 0;
};

} // namespace framekit::runtime
