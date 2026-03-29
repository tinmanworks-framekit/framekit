#pragma once

#include "framekit/runtime/service_context.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace framekit::runtime {

struct DiagnosticsService {
    std::vector<std::string> messages;

    void Log(std::string message) {
        messages.push_back(std::move(message));
    }
};

struct ConfigurationService {
    std::unordered_map<std::string, std::string> values;
};

struct TimingService {
    std::uint64_t frame_index = 0;
    std::chrono::steady_clock::time_point startup_timestamp = std::chrono::steady_clock::now();
};

bool RegisterDefaultBootstrapServices(ServiceContext& services);

} // namespace framekit::runtime
