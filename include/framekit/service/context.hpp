#pragma once

#include <algorithm>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace framekit::runtime {

enum class ServiceContextPhase : std::uint8_t {
    kOpen = 0,
    kFrozen = 1,
    kTeardown = 2,
};

enum class ServiceOwnership : std::uint8_t {
    kContextOwned = 0,
    kExternalOwned = 1,
};

struct ServiceTeardownRecord {
    std::string key;
    std::type_index contract_type{typeid(void)};
    ServiceOwnership ownership = ServiceOwnership::kContextOwned;
};

class ServiceContext {
public:
    ServiceContext() = default;

    ServiceContextPhase Phase() const {
        std::shared_lock lock(mutex_);
        return phase_;
    }

    bool Freeze() {
        std::unique_lock lock(mutex_);
        if (phase_ != ServiceContextPhase::kOpen) {
            return false;
        }

        phase_ = ServiceContextPhase::kFrozen;
        freeze_count_ += 1;
        return true;
    }

    bool BeginTeardown() {
        std::unique_lock lock(mutex_);
        if (phase_ == ServiceContextPhase::kTeardown) {
            return true;
        }
        if (phase_ != ServiceContextPhase::kFrozen && phase_ != ServiceContextPhase::kOpen) {
            return false;
        }

        phase_ = ServiceContextPhase::kTeardown;

        struct OrderedEntry {
            std::type_index type;
            std::string key;
            ServiceOwnership ownership;
            std::size_t sequence = 0;
        };

        std::vector<OrderedEntry> ordered;
        ordered.reserve(entries_.size());

        for (const auto& [service_key, entry] : entries_) {
            if (entry.ownership != ServiceOwnership::kContextOwned) {
                continue;
            }

            ordered.push_back(OrderedEntry{
                .type = service_key.contract_type,
                .key = service_key.key,
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

        last_teardown_order_.clear();
        for (const auto& item : ordered) {
            last_teardown_order_.push_back(ServiceTeardownRecord{
                .key = item.key,
                .contract_type = item.type,
                .ownership = item.ownership,
            });
        }

        entries_.clear();
        return true;
    }

    bool ResetForNextStartCycle() {
        std::unique_lock lock(mutex_);
        if (phase_ == ServiceContextPhase::kFrozen) {
            return false;
        }

        phase_ = ServiceContextPhase::kOpen;
        entries_.clear();
        registration_sequence_ = 0;
        return true;
    }

    std::uint64_t FreezeCount() const {
        std::shared_lock lock(mutex_);
        return freeze_count_;
    }

    std::size_t ServiceCount() const {
        std::shared_lock lock(mutex_);
        return entries_.size();
    }

    const std::vector<ServiceTeardownRecord>& LastTeardownOrder() const {
        return last_teardown_order_;
    }

    template <typename T>
    bool Register(
        std::shared_ptr<T> service,
        std::string key = {},
        ServiceOwnership ownership = ServiceOwnership::kContextOwned,
        bool replace_existing = false) {
        if (!service) {
            return false;
        }

        std::unique_lock lock(mutex_);
        if (phase_ != ServiceContextPhase::kOpen) {
            return false;
        }

        ServiceKey service_key{.contract_type = std::type_index(typeid(T)), .key = std::move(key)};
        const auto existing = entries_.find(service_key);
        if (existing != entries_.end() && !replace_existing) {
            return false;
        }

        std::size_t sequence = registration_sequence_;
        if (existing == entries_.end()) {
            registration_sequence_ += 1;
        } else {
            sequence = existing->second.sequence;
        }

        entries_[std::move(service_key)] = ServiceEntry{
            .instance = std::static_pointer_cast<void>(std::move(service)),
            .ownership = ownership,
            .sequence = sequence,
        };

        return true;
    }

    template <typename T>
    std::shared_ptr<T> Find(std::string key = {}) const {
        std::shared_lock lock(mutex_);
        if (phase_ == ServiceContextPhase::kTeardown) {
            return nullptr;
        }

        ServiceKey service_key{.contract_type = std::type_index(typeid(T)), .key = std::move(key)};
        const auto found = entries_.find(service_key);
        if (found == entries_.end()) {
            return nullptr;
        }

        return std::static_pointer_cast<T>(found->second.instance);
    }

    template <typename T>
    std::shared_ptr<T> Require(std::string key = {}) const {
        auto service = Find<T>(std::move(key));
        if (!service) {
            throw std::runtime_error("required service not found");
        }
        return service;
    }

private:
    struct ServiceKey {
        std::type_index contract_type;
        std::string key;

        bool operator==(const ServiceKey& other) const {
            return contract_type == other.contract_type && key == other.key;
        }
    };

    struct ServiceKeyHasher {
        std::size_t operator()(const ServiceKey& key) const {
            const auto type_hash = std::hash<std::type_index>{}(key.contract_type);
            const auto key_hash = std::hash<std::string>{}(key.key);
            return type_hash ^ (key_hash + 0x9e3779b9 + (type_hash << 6) + (type_hash >> 2));
        }
    };

    struct ServiceEntry {
        std::shared_ptr<void> instance;
        ServiceOwnership ownership = ServiceOwnership::kContextOwned;
        std::size_t sequence = 0;
    };

    mutable std::shared_mutex mutex_;
    ServiceContextPhase phase_ = ServiceContextPhase::kOpen;
    std::uint64_t freeze_count_ = 0;
    std::size_t registration_sequence_ = 0;
    std::unordered_map<ServiceKey, ServiceEntry, ServiceKeyHasher> entries_;
    std::vector<ServiceTeardownRecord> last_teardown_order_;
};

} // namespace framekit::runtime
