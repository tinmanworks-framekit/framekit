#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace framekit::ipc {

class IDataPlane {
public:
    virtual ~IDataPlane() = default;

    virtual bool Write(const std::uint8_t* data, std::size_t size) = 0;
    virtual std::vector<std::uint8_t> Read() = 0;
    virtual std::size_t Capacity() const = 0;
};

} // namespace framekit::ipc
