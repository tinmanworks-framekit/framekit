#pragma once

#include "framekit/spec/process_role.hpp"

#include <cstdint>

namespace framekit::runtime {

struct HeartbeatEvent {
    framekit::ProcessIdentity child;
    std::uint64_t sequence_id = 0;
};

class ILivenessPolicy {
public:
    virtual ~ILivenessPolicy() = default;

    virtual void OnHeartbeat(const HeartbeatEvent& event) = 0;
    virtual bool IsProcessAlive(
        const framekit::ProcessIdentity& identity,
        std::uint64_t observed_heartbeat_count) const = 0;
};

} // namespace framekit::runtime
