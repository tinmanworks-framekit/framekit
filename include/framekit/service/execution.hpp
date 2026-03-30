#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
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

class IExecutionService {
public:
    virtual ~IExecutionService() = default;

    virtual bool Submit(std::function<void()> task) = 0;
    virtual bool BeginShutdown() = 0;
    virtual bool AwaitShutdown(std::chrono::milliseconds timeout) = 0;
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