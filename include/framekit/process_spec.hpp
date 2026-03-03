#pragma once

#include "framekit/process_role.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace framekit {

struct ProcessSpec {
    std::string name;
    ProcessRole role = ProcessRole::kUnknown;
    std::string executable_path;
    std::vector<std::string> arguments;
    std::unordered_map<std::string, std::string> environment;
    bool auto_restart = false;
    std::uint32_t max_restart_attempts = 0;
};

} // namespace framekit
