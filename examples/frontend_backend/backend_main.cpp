#include "framekit/framekit.hpp"
#include "netkit/netkit.hpp"

#include <cstdint>

int main() {
    framekit::ApplicationSpec app_spec;
    app_spec.application_name = "framekit-backend";
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
    envelope.sequence_id = 2;
    envelope.message_type = "backend-ready";
    envelope.payload = app_spec.application_name;
    if (!bundle.control->Send(envelope)) {
        return 2;
    }

    const auto received = bundle.control->Receive();
    (void)received;

    const auto frame = bundle.data->Read();
    (void)frame;
    return 0;
}
