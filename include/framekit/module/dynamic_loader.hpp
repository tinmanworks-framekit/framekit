#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace framekit::runtime {

enum class DynamicModuleRefusalReason : std::uint8_t {
    kNone = 0,
    kInvalidManifest = 1,
    kDuplicateModuleId = 2,
    kDependencyUnavailable = 3,
    kIllegalLifecycleState = 4,
    kUnloadNotAllowed = 5,
};

enum class DynamicModuleLoadState : std::uint8_t {
    kDiscovered = 0,
    kRegistered = 1,
    kLoaded = 2,
    kStarted = 3,
    kRefused = 4,
    kUnloaded = 5,
};

enum class DynamicLoaderHostPhase : std::uint8_t {
    kBootstrapping = 0,
    kInitializing = 1,
    kRunning = 2,
    kStopping = 3,
    kStopped = 4,
};

struct DynamicModuleManifest {
    std::string module_id;
    std::string module_version;
    std::vector<std::string> required_dependencies;
    std::vector<std::string> optional_dependencies;
    std::vector<std::string> exported_services;
    bool hot_unload_allowed = false;
};

struct DynamicModuleDecision {
    bool accepted = false;
    DynamicModuleRefusalReason refusal_reason = DynamicModuleRefusalReason::kNone;
    std::string message;
};

struct DynamicModuleRollbackPlan {
    bool required = false;
    std::string failed_module_id;
    std::vector<std::string> unload_order;
};

struct DynamicModuleRollbackResult {
    bool success = true;
    std::size_t unload_failures = 0;
    std::vector<std::string> failed_unloads;
};

class IDynamicModuleLoader {
public:
    virtual ~IDynamicModuleLoader() = default;

    virtual DynamicModuleDecision RegisterManifest(const DynamicModuleManifest& manifest) = 0;
    virtual DynamicModuleDecision LoadModule(std::string_view module_id) = 0;
    virtual DynamicModuleDecision UnloadModule(std::string_view module_id) = 0;
};

inline DynamicModuleDecision EvaluateDynamicLoadRequest(
    DynamicLoaderHostPhase host_phase,
    bool module_already_registered,
    bool dependencies_available) {
    if (host_phase != DynamicLoaderHostPhase::kRunning) {
        return DynamicModuleDecision{
            .accepted = false,
            .refusal_reason = DynamicModuleRefusalReason::kIllegalLifecycleState,
            .message = "dynamic loading is only allowed while host phase is running",
        };
    }

    if (module_already_registered) {
        return DynamicModuleDecision{
            .accepted = false,
            .refusal_reason = DynamicModuleRefusalReason::kDuplicateModuleId,
            .message = "module is already registered",
        };
    }

    if (!dependencies_available) {
        return DynamicModuleDecision{
            .accepted = false,
            .refusal_reason = DynamicModuleRefusalReason::kDependencyUnavailable,
            .message = "required dependencies are unavailable",
        };
    }

    return DynamicModuleDecision{
        .accepted = true,
        .refusal_reason = DynamicModuleRefusalReason::kNone,
        .message = "load accepted",
    };
}

inline DynamicModuleDecision EvaluateDynamicUnloadRequest(
    DynamicLoaderHostPhase host_phase,
    const DynamicModuleManifest& manifest,
    bool module_is_loaded,
    bool has_runtime_dependents) {
    if (host_phase != DynamicLoaderHostPhase::kRunning) {
        return DynamicModuleDecision{
            .accepted = false,
            .refusal_reason = DynamicModuleRefusalReason::kIllegalLifecycleState,
            .message = "dynamic unload is only allowed while host phase is running",
        };
    }

    if (!module_is_loaded) {
        return DynamicModuleDecision{
            .accepted = false,
            .refusal_reason = DynamicModuleRefusalReason::kIllegalLifecycleState,
            .message = "module is not loaded",
        };
    }

    if (!manifest.hot_unload_allowed) {
        return DynamicModuleDecision{
            .accepted = false,
            .refusal_reason = DynamicModuleRefusalReason::kUnloadNotAllowed,
            .message = "module manifest does not allow hot unload",
        };
    }

    if (has_runtime_dependents) {
        return DynamicModuleDecision{
            .accepted = false,
            .refusal_reason = DynamicModuleRefusalReason::kDependencyUnavailable,
            .message = "module has active dependents",
        };
    }

    return DynamicModuleDecision{
        .accepted = true,
        .refusal_reason = DynamicModuleRefusalReason::kNone,
        .message = "unload accepted",
    };
}

inline DynamicModuleRollbackPlan BuildDynamicRollbackPlan(
    const std::vector<std::string>& loaded_modules_in_order,
    std::string_view failed_module_id) {
    DynamicModuleRollbackPlan plan;
    plan.failed_module_id = std::string(failed_module_id);

    if (failed_module_id.empty()) {
        return plan;
    }

    auto it = std::find(
        loaded_modules_in_order.begin(),
        loaded_modules_in_order.end(),
        failed_module_id);
    if (it == loaded_modules_in_order.end()) {
        return plan;
    }

    plan.required = true;
    for (auto reverse = loaded_modules_in_order.rbegin(); reverse != loaded_modules_in_order.rend(); ++reverse) {
        plan.unload_order.push_back(*reverse);
        if (*reverse == failed_module_id) {
            break;
        }
    }

    return plan;
}

inline DynamicModuleDecision AssessDynamicRollbackResult(const DynamicModuleRollbackResult& rollback_result) {
    if (rollback_result.success && rollback_result.unload_failures == 0 && rollback_result.failed_unloads.empty()) {
        return DynamicModuleDecision{
            .accepted = true,
            .refusal_reason = DynamicModuleRefusalReason::kNone,
            .message = "rollback succeeded",
        };
    }

    return DynamicModuleDecision{
        .accepted = false,
        .refusal_reason = DynamicModuleRefusalReason::kIllegalLifecycleState,
        .message = "rollback left modules in a partially loaded state",
    };
}

inline bool ValidateDynamicModuleManifest(const DynamicModuleManifest& manifest, std::string* error) {
    auto fail = [error](std::string message) {
        if (error) {
            *error = std::move(message);
        }
        return false;
    };

    if (manifest.module_id.empty()) {
        return fail("module manifest id must be non-empty");
    }

    if (manifest.module_version.empty()) {
        return fail("module manifest version must be non-empty");
    }

    if (manifest.required_dependencies.empty() && manifest.optional_dependencies.empty()) {
        return true;
    }

    std::unordered_set<std::string> required_set;
    required_set.reserve(manifest.required_dependencies.size());
    for (const auto& dependency : manifest.required_dependencies) {
        if (dependency.empty()) {
            return fail("required dependency id must be non-empty");
        }
        if (dependency == manifest.module_id) {
            return fail("module manifest cannot require itself");
        }
        if (!required_set.insert(dependency).second) {
            return fail("required dependency list contains duplicate entries");
        }
    }

    std::unordered_set<std::string> optional_set;
    optional_set.reserve(manifest.optional_dependencies.size());
    for (const auto& dependency : manifest.optional_dependencies) {
        if (dependency.empty()) {
            return fail("optional dependency id must be non-empty");
        }
        if (dependency == manifest.module_id) {
            return fail("module manifest cannot optionally depend on itself");
        }
        if (!optional_set.insert(dependency).second) {
            return fail("optional dependency list contains duplicate entries");
        }
        if (required_set.find(dependency) != required_set.end()) {
            return fail("dependency cannot be both required and optional");
        }
    }

    return true;
}

} // namespace framekit::runtime
