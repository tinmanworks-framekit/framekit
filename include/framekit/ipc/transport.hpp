#pragma once

#include "framekit/ipc/control.hpp"
#include "framekit/ipc/data.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace framekit::ipc {

struct TransportConfig {
    std::string name;
    std::unordered_map<std::string, std::string> values;
};

struct TransportBundle {
    std::unique_ptr<IControlChannel> control_channel;
    std::unique_ptr<IDataPlane> data_plane;
};

class ITransportFactory {
public:
    virtual ~ITransportFactory() = default;

    virtual TransportBundle Create(const TransportConfig& config) = 0;
};

} // namespace framekit::ipc
