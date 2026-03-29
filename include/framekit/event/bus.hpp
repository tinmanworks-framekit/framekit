#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace framekit::runtime {

enum class DeferredPriority : std::uint8_t {
    kHigh = 0,
    kNormal = 1,
    kLow = 2,
};

enum class DeferredOverflowPolicy : std::uint8_t {
    kDropNewest = 0,
    kFailFast = 1,
    kBackpressureSignaling = 2,
};

class EventContext {
public:
    EventContext(std::string event_type, std::any payload)
        : event_type_(std::move(event_type)), payload_(std::move(payload)) {}

    const std::string& EventType() const { return event_type_; }

    void Consume() { consumed_ = true; }
    bool IsConsumed() const { return consumed_; }

    template <typename T>
    T* PayloadAs() {
        return std::any_cast<T>(&payload_);
    }

    template <typename T>
    const T* PayloadAs() const {
        return std::any_cast<T>(&payload_);
    }

private:
    friend class EventBus;

    void SetConsumed(bool consumed) { consumed_ = consumed; }

    std::string event_type_;
    std::any payload_;
    bool consumed_ = false;
};

struct DispatchResult {
    bool had_handlers = false;
    bool consumed = false;
};

class EventBus {
public:
    using HandlerId = std::uint64_t;
    using EventHandler = std::function<void(EventContext&)>;

    HandlerId Subscribe(std::string event_type, EventHandler handler, bool monitor = false);
    bool Unsubscribe(const std::string& event_type, HandlerId handler_id);

    DispatchResult DispatchImmediate(std::string event_type, std::any payload);

    bool EnqueueDeferred(
        std::string event_type,
        std::any payload,
        DeferredPriority priority = DeferredPriority::kNormal);

    std::size_t DrainDeferred(std::size_t max_events = std::numeric_limits<std::size_t>::max());
    std::size_t DiscardDeferred();

    void SetDeferredCapacity(std::size_t capacity);
    void SetDeferredOverflowPolicy(DeferredOverflowPolicy policy);

    std::size_t DeferredCount() const;
    std::size_t DroppedDeferredCount() const;

private:
    struct HandlerEntry {
        HandlerId id = 0;
        EventHandler handler;
        bool monitor = false;
        std::uint64_t sequence = 0;
    };

    struct DeferredEvent {
        std::string event_type;
        std::any payload;
        DeferredPriority priority = DeferredPriority::kNormal;
        std::uint64_t sequence = 0;
    };

    bool PushDeferred(DeferredEvent event);
    bool IsQueueFull() const;
    std::vector<HandlerEntry> SnapshotHandlers(const std::string& event_type) const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<HandlerEntry>> handlers_;
    std::vector<DeferredEvent> high_priority_queue_;
    std::vector<DeferredEvent> normal_priority_queue_;
    std::vector<DeferredEvent> low_priority_queue_;

    HandlerId next_handler_id_ = 1;
    std::uint64_t next_handler_sequence_ = 1;
    std::uint64_t next_deferred_sequence_ = 1;
    std::size_t deferred_capacity_ = 1024;
    std::size_t dropped_deferred_count_ = 0;
    DeferredOverflowPolicy overflow_policy_ = DeferredOverflowPolicy::kDropNewest;
};

} // namespace framekit::runtime
