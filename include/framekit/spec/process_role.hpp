#pragma once

#include <cstdint>
#include <string>

namespace framekit {

enum class ProcessRole : std::uint8_t {
    kPrimary = 0,
    kFrontend = 1,
    kBackend = 2,
    kWorker = 3,
    kService = 4,
    kUnknown = 255
};

struct ProcessIdentity {
    std::uint64_t process_id = 0;
    std::string role_name;
    ProcessRole role = ProcessRole::kUnknown;
};

} // namespace framekit
