#pragma once

#include "framekit/ipc_transport_kind.hpp"
#include "framekit/process_topology.hpp"

#include <string>

namespace framekit {

enum class AppMode {
    kSingleProcess = 0,
    kMultiProcess = 1,
};

struct ApplicationSpec {
    std::string application_name = "framekit-app";
    AppMode mode = AppMode::kSingleProcess;
    ProcessTopologySpec process_topology;
    IpcTransportKind transport_kind = IpcTransportKind::kNone;
    std::string transport_profile = "default";
};

} // namespace framekit
