#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace framekit::ipc {

struct ControlMessage {
    std::string message_type;
    std::string payload;
    std::uint64_t sequence_id = 0;
};

class IControlChannel {
public:
    virtual ~IControlChannel() = default;

    virtual bool Send(const ControlMessage& message) = 0;
    virtual std::optional<ControlMessage> Receive() = 0;
    virtual bool IsOpen() const = 0;
    virtual void Close() = 0;
};

} // namespace framekit::ipc
