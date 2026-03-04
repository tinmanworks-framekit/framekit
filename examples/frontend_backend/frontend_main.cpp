#include "framekit/framekit.hpp"
#include "netkit/netkit.hpp"

#include <cstdint>
#include <vector>

int main() {
    framekit::ApplicationSpec app_spec;
    app_spec.application_name = "framekit-frontend";
    app_spec.mode = framekit::AppMode::kMultiProcess;
    app_spec.transport_kind = framekit::IpcTransportKind::kShmPipe;

    netkit::transport::ShmPipeFactory factory;
    netkit::transport::ShmPipeConfig config;
    config.channel_endpoint = "framekit-fb-control";
    config.shared_memory_name = "framekit-fb-shm";
    config.shared_memory_capacity = 1024;

    auto bundle = factory.Create(config);
    if (!bundle.control || !bundle.data || !bundle.control->IsOpen() || !bundle.data->IsOpen()) {
        return 1;
    }

    netkit::control::ControlEnvelope envelope;
    envelope.sequence_id = 1;
    envelope.message_type = "frontend-ready";
    envelope.payload = app_spec.application_name;
    if (!bundle.control->Send(envelope)) {
        return 2;
    }

    std::vector<std::uint8_t> command_payload{0x10, 0x20, 0x30};
    if (!bundle.data->Write(command_payload)) {
        return 3;
    }

    return 0;
}
