#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace framekit::runtime {

enum class ExecutionServicePhase : std::uint8_t {
    kOpen = 0,
    kFrozen = 1,
    kShuttingDown = 2,
};

enum class ExecutionServiceOwnership : std::uint8_t {
    kContextOwned = 0,
    kExternalOwned = 1,
};

struct ExecutionServiceShutdownRecord {
    std::string key;
    ExecutionServiceOwnership ownership = ExecutionServiceOwnership::kContextOwned;
};

using ExecutionTaskId = std::uint64_t;

enum class ExecutionTaskState : std::uint8_t {
    kQueued = 0,
    kRunning = 1,
    kCompleted = 2,
    kCancelled = 3,
    kFailed = 4,
    kRejected = 5,
};

struct ExecutionTaskResult {
    ExecutionTaskId task_id = 0;
    ExecutionTaskState state = ExecutionTaskState::kRejected;
    std::string detail;
};

enum class SchedulerPolicyKind : std::uint8_t {
    kSingleThreaded = 0,
    kFixedPool = 1,
    kStageAffine = 2,
};

struct SchedulerPolicyConfig {
    SchedulerPolicyKind kind = SchedulerPolicyKind::kSingleThreaded;
    std::size_t worker_count = 1;
    bool stage_affinity_enabled = false;
};

struct ExecutionServiceMetrics {
    std::uint64_t submitted = 0;
    std::uint64_t rejected = 0;
    std::uint64_t cancelled = 0;
    std::uint64_t completed = 0;
    std::uint64_t failed = 0;
    std::size_t pending = 0;
};

class IExecutionService {
public:
    virtual ~IExecutionService() = default;

    virtual bool Submit(std::function<void()> task) = 0;
    virtual bool BeginShutdown() = 0;
    virtual bool AwaitShutdown(std::chrono::milliseconds timeout) = 0;
};

class InlineExecutionService final : public IExecutionService {
public:
    InlineExecutionService() = default;

    void ConfigurePolicy(SchedulerPolicyConfig policy) {
        std::unique_lock lock(mutex_);
        policy_ = std::move(policy);
        if (policy_.kind == SchedulerPolicyKind::kSingleThreaded) {
            policy_.worker_count = 1;
            policy_.stage_affinity_enabled = false;
        } else if (policy_.worker_count == 0) {
            policy_.worker_count = 1;
        }
    }

    SchedulerPolicyConfig Policy() const {
        std::shared_lock lock(mutex_);
        return policy_;
    }

    bool Submit(std::function<void()> task) override {
        const auto result = SubmitTask(std::move(task));
        return result.state == ExecutionTaskState::kQueued;
    }

    ExecutionTaskResult SubmitTask(std::function<void()> task) {
        std::unique_lock lock(mutex_);

        if (shutting_down_ || !task) {
            metrics_.rejected += 1;
            return ExecutionTaskResult{
                .task_id = 0,
                .state = ExecutionTaskState::kRejected,
                .detail = shutting_down_ ? "service is shutting down" : "task is empty",
            };
        }

        const auto task_id = next_task_id_++;
        pending_.push_back(PendingTask{
            .task_id = task_id,
            .task = std::move(task),
        });

        const auto queued = ExecutionTaskResult{
            .task_id = task_id,
            .state = ExecutionTaskState::kQueued,
            .detail = "queued",
        };
        results_[task_id] = queued;
        metrics_.submitted += 1;
        metrics_.pending = pending_.size();
        return queued;
    }

    bool Cancel(ExecutionTaskId task_id) {
        std::unique_lock lock(mutex_);
        if (shutting_down_) {
            return false;
        }

        for (auto it = pending_.begin(); it != pending_.end(); ++it) {
            if (it->task_id != task_id) {
                continue;
            }

            pending_.erase(it);
            results_[task_id] = ExecutionTaskResult{
                .task_id = task_id,
                .state = ExecutionTaskState::kCancelled,
                .detail = "cancelled before execution",
            };
            metrics_.cancelled += 1;
            metrics_.pending = pending_.size();
            return true;
        }

        return false;
    }

    std::vector<ExecutionTaskResult> Drain() {
        std::vector<PendingTask> to_run;
        {
            std::unique_lock lock(mutex_);
            to_run.swap(pending_);
            metrics_.pending = pending_.size();
        }

        std::vector<ExecutionTaskResult> drained;
        drained.reserve(to_run.size());

        for (auto& pending : to_run) {
            {
                std::unique_lock lock(mutex_);
                results_[pending.task_id] = ExecutionTaskResult{
                    .task_id = pending.task_id,
                    .state = ExecutionTaskState::kRunning,
                    .detail = "running",
                };
            }

            ExecutionTaskResult outcome{
                .task_id = pending.task_id,
                .state = ExecutionTaskState::kCompleted,
                .detail = "completed",
            };

            try {
                pending.task();
            } catch (const std::exception& ex) {
                outcome.state = ExecutionTaskState::kFailed;
                outcome.detail = ex.what();
            } catch (...) {
                outcome.state = ExecutionTaskState::kFailed;
                outcome.detail = "task threw non-standard exception";
            }

            {
                std::unique_lock lock(mutex_);
                results_[pending.task_id] = outcome;
                if (outcome.state == ExecutionTaskState::kCompleted) {
                    metrics_.completed += 1;
                } else if (outcome.state == ExecutionTaskState::kFailed) {
                    metrics_.failed += 1;
                }
            }
            drained.push_back(outcome);
        }

        return drained;
    }

    bool BeginShutdown() override {
        std::unique_lock lock(mutex_);
        if (shutting_down_) {
            return true;
        }

        shutting_down_ = true;
        for (const auto& pending : pending_) {
            results_[pending.task_id] = ExecutionTaskResult{
                .task_id = pending.task_id,
                .state = ExecutionTaskState::kCancelled,
                .detail = "cancelled by shutdown",
            };
            metrics_.cancelled += 1;
        }
        pending_.clear();
        metrics_.pending = pending_.size();
        return true;
    }

    bool AwaitShutdown(std::chrono::milliseconds timeout) override {
        (void)timeout;
        std::shared_lock lock(mutex_);
        return shutting_down_;
    }

    std::size_t PendingCount() const {
        std::shared_lock lock(mutex_);
        return pending_.size();
    }

    std::optional<ExecutionTaskResult> FindResult(ExecutionTaskId task_id) const {
        std::shared_lock lock(mutex_);
        const auto found = results_.find(task_id);
        if (found == results_.end()) {
            return std::nullopt;
        }

        return found->second;
    }

    ExecutionServiceMetrics Metrics() const {
        std::shared_lock lock(mutex_);
        return metrics_;
    }

private:
    struct PendingTask {
        ExecutionTaskId task_id = 0;
        std::function<void()> task;
    };

    mutable std::shared_mutex mutex_;
    bool shutting_down_ = false;
    SchedulerPolicyConfig policy_;
    ExecutionServiceMetrics metrics_;
    ExecutionTaskId next_task_id_ = 1;
    std::vector<PendingTask> pending_;
    std::unordered_map<ExecutionTaskId, ExecutionTaskResult> results_;
};

class ExecutionServiceRegistry {
public:
    ExecutionServiceRegistry() = default;

    ExecutionServicePhase Phase() const {
        std::shared_lock lock(mutex_);
        return phase_;
    }

    bool Freeze() {
        std::unique_lock lock(mutex_);
        if (phase_ != ExecutionServicePhase::kOpen) {
            return false;
        }

        phase_ = ExecutionServicePhase::kFrozen;
        freeze_count_ += 1;
        return true;
    }

    std::uint64_t FreezeCount() const {
        std::shared_lock lock(mutex_);
        return freeze_count_;
    }

    bool Register(
        std::shared_ptr<IExecutionService> service,
        std::string key = {},
        ExecutionServiceOwnership ownership = ExecutionServiceOwnership::kContextOwned,
        bool replace_existing = false) {
        if (!service) {
            return false;
        }

        std::unique_lock lock(mutex_);
        if (phase_ != ExecutionServicePhase::kOpen) {
            return false;
        }

        const auto found = entries_.find(key);
        if (found != entries_.end() && !replace_existing) {
            return false;
        }

        std::size_t sequence = registration_sequence_;
        if (found == entries_.end()) {
            registration_sequence_ += 1;
        } else {
            sequence = found->second.sequence;
        }

        ServiceEntry entry;
        entry.ownership = ownership;
        entry.sequence = sequence;
        if (ownership == ExecutionServiceOwnership::kExternalOwned) {
            entry.external_instance = std::move(service);
        } else {
            entry.instance = std::move(service);
        }

        entries_[std::move(key)] = std::move(entry);
        return true;
    }

    std::shared_ptr<IExecutionService> Find(std::string key = {}) const {
        std::shared_lock lock(mutex_);
        if (phase_ == ExecutionServicePhase::kShuttingDown) {
            return nullptr;
        }

        const auto found = entries_.find(key);
        if (found == entries_.end()) {
            return nullptr;
        }

        if (found->second.ownership == ExecutionServiceOwnership::kExternalOwned) {
            return found->second.external_instance.lock();
        }

        return found->second.instance;
    }

    bool BeginShutdown(std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) {
        std::vector<OrderedEntry> ordered;
        std::unordered_map<std::string, ServiceEntry> external_entries;

        {
            std::unique_lock lock(mutex_);

            if (phase_ == ExecutionServicePhase::kShuttingDown) {
                return true;
            }
            if (phase_ != ExecutionServicePhase::kOpen && phase_ != ExecutionServicePhase::kFrozen) {
                return false;
            }

            phase_ = ExecutionServicePhase::kShuttingDown;

            ordered.reserve(entries_.size());
            for (const auto& [key, entry] : entries_) {
                if (entry.ownership != ExecutionServiceOwnership::kContextOwned) {
                    external_entries.emplace(key, entry);
                    continue;
                }

                ordered.push_back(OrderedEntry{
                    .key = key,
                    .instance = entry.instance,
                    .ownership = entry.ownership,
                    .sequence = entry.sequence,
                });
            }

            std::sort(
                ordered.begin(),
                ordered.end(),
                [](const OrderedEntry& left, const OrderedEntry& right) {
                    return left.sequence > right.sequence;
                });

            last_shutdown_order_.clear();
            for (const auto& item : ordered) {
                last_shutdown_order_.push_back(ExecutionServiceShutdownRecord{
                    .key = item.key,
                    .ownership = item.ownership,
                });
            }

            entries_ = external_entries;
        }

        bool ok = true;
        for (const auto& item : ordered) {
            if (!item.instance) {
                ok = false;
                continue;
            }

            ok = item.instance->BeginShutdown() && ok;
            ok = item.instance->AwaitShutdown(timeout) && ok;
        }

        return ok;
    }

    bool ResetForNextStartCycle() {
        std::unique_lock lock(mutex_);
        if (phase_ == ExecutionServicePhase::kFrozen) {
            return false;
        }

        phase_ = ExecutionServicePhase::kOpen;
        entries_.clear();
        registration_sequence_ = 0;
        return true;
    }

    std::size_t ServiceCount() const {
        std::shared_lock lock(mutex_);
        return entries_.size();
    }

    std::vector<ExecutionServiceShutdownRecord> LastShutdownOrder() const {
        std::shared_lock lock(mutex_);
        return last_shutdown_order_;
    }

private:
    struct ServiceEntry {
        std::shared_ptr<IExecutionService> instance;
        std::weak_ptr<IExecutionService> external_instance;
        ExecutionServiceOwnership ownership = ExecutionServiceOwnership::kContextOwned;
        std::size_t sequence = 0;
    };

    struct OrderedEntry {
        std::string key;
        std::shared_ptr<IExecutionService> instance;
        ExecutionServiceOwnership ownership = ExecutionServiceOwnership::kContextOwned;
        std::size_t sequence = 0;
    };

    mutable std::shared_mutex mutex_;
    ExecutionServicePhase phase_ = ExecutionServicePhase::kOpen;
    std::uint64_t freeze_count_ = 0;
    std::size_t registration_sequence_ = 0;
    std::unordered_map<std::string, ServiceEntry> entries_;
    std::vector<ExecutionServiceShutdownRecord> last_shutdown_order_;
};

} // namespace framekit::runtime