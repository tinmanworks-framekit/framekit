#include "framekit/runtime/module_graph.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace framekit::runtime {

namespace {

ModuleGraphValidationResult Invalid(ModuleGraphError error) {
    ModuleGraphValidationResult result;
    result.valid = false;
    result.errors.push_back(std::move(error));
    return result;
}

} // namespace

ModuleGraphValidationResult ValidateModuleGraph(const std::vector<ModuleSpec>& modules) {
    std::set<std::string> known_ids;
    for (const auto& module : modules) {
        if (module.id.empty()) {
            return Invalid(ModuleGraphError{
                .code = ModuleGraphErrorCode::kDuplicateModuleId,
                .module_id = module.id,
                .dependency_id = {},
                .message = "module id must be non-empty",
            });
        }

        if (!known_ids.insert(module.id).second) {
            return Invalid(ModuleGraphError{
                .code = ModuleGraphErrorCode::kDuplicateModuleId,
                .module_id = module.id,
                .dependency_id = {},
                .message = "duplicate module id",
            });
        }
    }

    std::map<std::string, std::set<std::string>> adjacency;
    std::map<std::string, std::size_t> indegree;
    for (const auto& id : known_ids) {
        adjacency[id] = {};
        indegree[id] = 0;
    }

    for (const auto& module : modules) {
        std::unordered_set<std::string> seen_dependencies;

        const auto validate_dependency = [&](const std::string& dependency, bool required)
            -> std::optional<ModuleGraphValidationResult> {
            if (dependency == module.id) {
                return Invalid(ModuleGraphError{
                    .code = ModuleGraphErrorCode::kSelfDependency,
                    .module_id = module.id,
                    .dependency_id = dependency,
                    .message = "self dependency is invalid",
                });
            }

            if (!seen_dependencies.insert(dependency).second) {
                return Invalid(ModuleGraphError{
                    .code = ModuleGraphErrorCode::kDuplicateDependencyEntry,
                    .module_id = module.id,
                    .dependency_id = dependency,
                    .message = "duplicate dependency entry",
                });
            }

            const bool declared = known_ids.find(dependency) != known_ids.end();
            if (required && !declared) {
                return Invalid(ModuleGraphError{
                    .code = ModuleGraphErrorCode::kMissingRequiredDependency,
                    .module_id = module.id,
                    .dependency_id = dependency,
                    .message = "required dependency was not declared",
                });
            }

            if (declared) {
                adjacency[module.id].insert(dependency);
            }

            return std::nullopt;
        };

        for (const auto& dependency : module.required_dependencies) {
            if (auto error = validate_dependency(dependency, true); error.has_value()) {
                return *error;
            }
        }

        for (const auto& dependency : module.optional_dependencies) {
            if (auto error = validate_dependency(dependency, false); error.has_value()) {
                return *error;
            }
        }
    }

    for (const auto& [consumer, dependencies] : adjacency) {
        (void)consumer;
        for (const auto& dependency : dependencies) {
            indegree[dependency] += 1;
        }
    }

    std::set<std::string> ready;
    for (const auto& [id, count] : indegree) {
        if (count == 0) {
            ready.insert(id);
        }
    }

    std::vector<std::string> base_order;
    base_order.reserve(known_ids.size());

    while (!ready.empty()) {
        const auto current_it = ready.begin();
        const auto current = *current_it;
        ready.erase(current_it);
        base_order.push_back(current);

        for (const auto& dependency : adjacency[current]) {
            auto& count = indegree[dependency];
            if (count > 0) {
                count -= 1;
            }
            if (count == 0) {
                ready.insert(dependency);
            }
        }
    }

    if (base_order.size() != known_ids.size()) {
        return Invalid(ModuleGraphError{
            .code = ModuleGraphErrorCode::kCycleDetected,
            .module_id = {},
            .dependency_id = {},
            .message = "dependency cycle detected",
        });
    }

    std::vector<std::string> startup_order = base_order;
    std::reverse(startup_order.begin(), startup_order.end());

    return ModuleGraphValidationResult{
        .valid = true,
        .base_order = std::move(base_order),
        .startup_order = std::move(startup_order),
        .errors = {},
    };
}

} // namespace framekit::runtime
