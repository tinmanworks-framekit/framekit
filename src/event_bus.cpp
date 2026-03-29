#include "framekit/event/bus.hpp"

#include <algorithm>
#include <stdexcept>

namespace framekit::runtime {

EventBus::HandlerId EventBus::Subscribe(
    std::string event_type,
    EventHandler handler,
    bool monitor) {
    if (!handler) {
        return 0;
    }

    std::lock_guard lock(mutex_);

    const HandlerId handler_id = next_handler_id_;
    next_handler_id_ += 1;

    handlers_[std::move(event_type)].push_back(HandlerEntry{
        .id = handler_id,
        .handler = std::move(handler),
        .monitor = monitor,
        .sequence = next_handler_sequence_++,
    });

    return handler_id;
}

bool EventBus::Unsubscribe(const std::string& event_type, HandlerId handler_id) {
    std::lock_guard lock(mutex_);

    auto it = handlers_.find(event_type);
    if (it == handlers_.end()) {
        return false;
    }

    auto& entries = it->second;
    const auto erase_it = std::remove_if(
        entries.begin(),
        entries.end(),
        [handler_id](const HandlerEntry& entry) { return entry.id == handler_id; });

    if (erase_it == entries.end()) {
        return false;
    }

    entries.erase(erase_it, entries.end());
    if (entries.empty()) {
        handlers_.erase(it);
    }

    return true;
}

DispatchResult EventBus::DispatchImmediate(std::string event_type, std::any payload) {
    auto handlers = SnapshotHandlers(event_type);

    EventContext context(std::move(event_type), std::move(payload));
    bool had_handlers = false;

    for (auto& entry : handlers) {
        if (context.IsConsumed() && !entry.monitor) {
            continue;
        }

        had_handlers = true;

        if (entry.monitor) {
            const bool consumed_before = context.IsConsumed();
            entry.handler(context);
            context.SetConsumed(consumed_before);
            continue;
        }

        entry.handler(context);
    }

    return DispatchResult{
        .had_handlers = had_handlers,
        .consumed = context.IsConsumed(),
    };
}

bool EventBus::EnqueueDeferred(std::string event_type, std::any payload, DeferredPriority priority) {
    DeferredEvent event{
        .event_type = std::move(event_type),
        .payload = std::move(payload),
        .priority = priority,
        .sequence = 0,
    };

    return PushDeferred(std::move(event));
}

std::size_t EventBus::DrainDeferred(std::size_t max_events) {
    std::vector<DeferredEvent> events_to_dispatch;

    {
        std::lock_guard lock(mutex_);

        auto pop_event = [&](std::vector<DeferredEvent>& queue) {
            if (queue.empty() || events_to_dispatch.size() >= max_events) {
                return;
            }

            events_to_dispatch.push_back(std::move(queue.front()));
            queue.erase(queue.begin());
        };

        while (events_to_dispatch.size() < max_events) {
            const std::size_t before = events_to_dispatch.size();
            pop_event(high_priority_queue_);
            if (events_to_dispatch.size() >= max_events) {
                break;
            }
            pop_event(normal_priority_queue_);
            if (events_to_dispatch.size() >= max_events) {
                break;
            }
            pop_event(low_priority_queue_);

            if (events_to_dispatch.size() == before) {
                break;
            }
        }
    }

    for (auto& event : events_to_dispatch) {
        (void)DispatchImmediate(std::move(event.event_type), std::move(event.payload));
    }

    return events_to_dispatch.size();
}

std::size_t EventBus::DiscardDeferred() {
    std::lock_guard lock(mutex_);

    const auto total = high_priority_queue_.size() +
        normal_priority_queue_.size() +
        low_priority_queue_.size();

    high_priority_queue_.clear();
    normal_priority_queue_.clear();
    low_priority_queue_.clear();
    return total;
}

void EventBus::SetDeferredCapacity(std::size_t capacity) {
    std::lock_guard lock(mutex_);
    deferred_capacity_ = capacity;
}

void EventBus::SetDeferredOverflowPolicy(DeferredOverflowPolicy policy) {
    std::lock_guard lock(mutex_);
    overflow_policy_ = policy;
}

std::size_t EventBus::DeferredCount() const {
    std::lock_guard lock(mutex_);
    return high_priority_queue_.size() +
        normal_priority_queue_.size() +
        low_priority_queue_.size();
}

std::size_t EventBus::DroppedDeferredCount() const {
    std::lock_guard lock(mutex_);
    return dropped_deferred_count_;
}

bool EventBus::PushDeferred(DeferredEvent event) {
    std::lock_guard lock(mutex_);

    if (IsQueueFull()) {
        if (overflow_policy_ == DeferredOverflowPolicy::kFailFast) {
            throw std::runtime_error("event bus deferred queue overflow");
        }
        if (overflow_policy_ == DeferredOverflowPolicy::kDropNewest) {
            dropped_deferred_count_ += 1;
        }
        return false;
    }

    event.sequence = next_deferred_sequence_++;

    switch (event.priority) {
        case DeferredPriority::kHigh:
            high_priority_queue_.push_back(std::move(event));
            break;
        case DeferredPriority::kNormal:
            normal_priority_queue_.push_back(std::move(event));
            break;
        case DeferredPriority::kLow:
            low_priority_queue_.push_back(std::move(event));
            break;
    }

    return true;
}

bool EventBus::IsQueueFull() const {
    const auto total = high_priority_queue_.size() +
        normal_priority_queue_.size() +
        low_priority_queue_.size();

    return total >= deferred_capacity_;
}

std::vector<EventBus::HandlerEntry> EventBus::SnapshotHandlers(const std::string& event_type) const {
    std::lock_guard lock(mutex_);

    const auto found = handlers_.find(event_type);
    if (found == handlers_.end()) {
        return {};
    }

    auto snapshot = found->second;
    std::sort(
        snapshot.begin(),
        snapshot.end(),
        [](const HandlerEntry& left, const HandlerEntry& right) {
            return left.sequence < right.sequence;
        });

    return snapshot;
}

} // namespace framekit::runtime
