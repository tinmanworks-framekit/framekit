#pragma once

#include "framekit/spec/process_spec.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace framekit {

struct ProcessTopologySpec {
    std::vector<ProcessSpec> nodes;
    std::unordered_map<std::string, std::vector<std::string>> dependencies;
};

} // namespace framekit
