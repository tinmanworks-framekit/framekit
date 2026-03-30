#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace framekit::runtime {

struct ModuleSpec {
    std::string id;
    std::vector<std::string> required_dependencies;
    std::vector<std::string> optional_dependencies;
};

enum class ModuleGraphErrorCode : std::uint8_t {
    kNone = 0,
    kDuplicateModuleId = 1,
    kMissingRequiredDependency = 2,
    kCycleDetected = 3,
    kSelfDependency = 4,
    kDuplicateDependencyEntry = 5,
    kInvalidModuleId = 6,
};

struct ModuleGraphError {
    ModuleGraphErrorCode code = ModuleGraphErrorCode::kNone;
    std::string module_id;
    std::string dependency_id;
    std::string message;
};

struct ModuleGraphValidationResult {
    bool valid = false;
    std::vector<std::string> base_order;
    std::vector<std::string> startup_order;
    std::vector<ModuleGraphError> errors;
};

ModuleGraphValidationResult ValidateModuleGraph(const std::vector<ModuleSpec>& modules);

} // namespace framekit::runtime
