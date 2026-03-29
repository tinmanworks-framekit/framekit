#include "framekit/runtime/input_routing_runtime.hpp"

#include <algorithm>
#include <utility>

namespace framekit::runtime {

bool PassthroughInputNormalizer::Normalize(
    const RawInputEvent& raw_event,
    NormalizedInputEvent* normalized_event) const {
    if (!normalized_event) {
        return false;
    }

    normalized_event->device = raw_event.device;
    normalized_event->action = raw_event.action;
    normalized_event->timestamp_ns = raw_event.timestamp_ns;
    normalized_event->source_id = raw_event.source_id;
    normalized_event->window_id = raw_event.window_id;
    normalized_event->pointer_x = raw_event.pointer_x;
    normalized_event->pointer_y = raw_event.pointer_y;
    normalized_event->key_code = raw_event.key_code;
    normalized_event->text = raw_event.text;
    normalized_event->modifiers = raw_event.modifiers;
    normalized_event->backend_extension = raw_event.backend_payload;
    return true;
}

InputRouter::HandlerId InputRouter::SubscribeWindow(std::string window_id, InputHandler handler) {
    if (window_id.empty() || !handler) {
        return 0;
    }

    const HandlerId id = next_handler_id_++;
    window_handlers_[window_id].push_back(HandlerEntry{
        .id = id,
        .sequence = next_sequence_++,
        .handler = std::move(handler),
    });
    return id;
}

InputRouter::HandlerId InputRouter::SubscribeGlobal(InputHandler handler) {
    if (!handler) {
        return 0;
    }

    const HandlerId id = next_handler_id_++;
    global_handlers_.push_back(HandlerEntry{
        .id = id,
        .sequence = next_sequence_++,
        .handler = std::move(handler),
    });
    return id;
}

bool InputRouter::UnsubscribeWindow(const std::string& window_id, HandlerId handler_id) {
    auto found = window_handlers_.find(window_id);
    if (found == window_handlers_.end()) {
        return false;
    }

    auto& handlers = found->second;
    const auto erase_it = std::remove_if(
        handlers.begin(),
        handlers.end(),
        [handler_id](const HandlerEntry& entry) { return entry.id == handler_id; });

    if (erase_it == handlers.end()) {
        return false;
    }

    handlers.erase(erase_it, handlers.end());
    if (handlers.empty()) {
        window_handlers_.erase(found);
    }

    return true;
}

bool InputRouter::UnsubscribeGlobal(HandlerId handler_id) {
    const auto erase_it = std::remove_if(
        global_handlers_.begin(),
        global_handlers_.end(),
        [handler_id](const HandlerEntry& entry) { return entry.id == handler_id; });

    if (erase_it == global_handlers_.end()) {
        return false;
    }

    global_handlers_.erase(erase_it, global_handlers_.end());
    return true;
}

void InputRouter::SetFocusedWindow(std::string window_id) {
    focused_window_ = std::move(window_id);
}

void InputRouter::SetPointerCaptureWindow(std::string window_id) {
    pointer_capture_window_ = std::move(window_id);
}

void InputRouter::ClearPointerCaptureWindow() {
    pointer_capture_window_.clear();
}

void InputRouter::SetMissingWindowTargetPolicy(MissingWindowTargetPolicy policy) {
    missing_target_policy_ = policy;
}

InputDispatchResult InputRouter::Route(const NormalizedInputEvent& event) const {
    const auto target_window = ResolveTargetWindow(event);

    if (!target_window.empty()) {
        const auto found = window_handlers_.find(target_window);
        if (found != window_handlers_.end()) {
            auto result = RouteHandlers(found->second, event);
            if (result.consumed) {
                return result;
            }

            auto global_result = RouteHandlers(global_handlers_, event);
            result.delivered_handlers += global_result.delivered_handlers;
            result.dropped_events += global_result.dropped_events;
            result.consumed = global_result.consumed;
            return result;
        }

        if (missing_target_policy_ == MissingWindowTargetPolicy::kDropWithDiagnostics) {
            return InputDispatchResult{
                .delivered_handlers = 0,
                .dropped_events = 1,
                .consumed = false,
            };
        }
    }

    return RouteHandlers(global_handlers_, event);
}

std::string InputRouter::ResolveTargetWindow(const NormalizedInputEvent& event) const {
    if (event.device == InputDevice::kPointer && !pointer_capture_window_.empty()) {
        return pointer_capture_window_;
    }

    if (!event.window_id.empty()) {
        return event.window_id;
    }

    if ((event.device == InputDevice::kKeyboard || event.device == InputDevice::kText) &&
        !focused_window_.empty()) {
        return focused_window_;
    }

    return {};
}

InputDispatchResult InputRouter::RouteHandlers(
    const std::vector<HandlerEntry>& handlers,
    const NormalizedInputEvent& event) const {
    InputDispatchResult result;
    for (const auto& handler_entry : handlers) {
        result.delivered_handlers += 1;
        if (handler_entry.handler && handler_entry.handler(event)) {
            result.consumed = true;
            break;
        }
    }

    return result;
}

InputRoutingRuntime::InputRoutingRuntime()
    : normalizer_(std::make_shared<PassthroughInputNormalizer>()) {}

void InputRoutingRuntime::SetNormalizer(std::shared_ptr<IInputNormalizer> normalizer) {
    normalizer_ = std::move(normalizer);
}

InputRouter& InputRoutingRuntime::Router() {
    return router_;
}

const InputRouter& InputRoutingRuntime::Router() const {
    return router_;
}

void InputRoutingRuntime::EnqueueRawInput(RawInputEvent event) {
    raw_queue_.push_back(std::move(event));
}

bool InputRoutingRuntime::NormalizePending() {
    if (!normalizer_) {
        last_error_ = "input normalizer is not set";
        return false;
    }

    std::vector<NormalizedInputEvent> normalized;
    normalized.reserve(raw_queue_.size());

    for (const auto& raw : raw_queue_) {
        NormalizedInputEvent event;
        if (!normalizer_->Normalize(raw, &event)) {
            last_error_ = "failed to normalize raw input event";
            return false;
        }
        normalized.push_back(std::move(event));
    }

    raw_queue_.clear();
    for (auto& event : normalized) {
        normalized_queue_.push_back(std::move(event));
    }

    last_error_.clear();
    return true;
}

InputDispatchResult InputRoutingRuntime::DispatchImmediate() {
    InputDispatchResult aggregate;

    for (const auto& event : normalized_queue_) {
        auto result = router_.Route(event);
        aggregate.delivered_handlers += result.delivered_handlers;
        aggregate.dropped_events += result.dropped_events;
        if (result.consumed) {
            aggregate.consumed = true;
        }
    }

    normalized_queue_.clear();
    return aggregate;
}

std::size_t InputRoutingRuntime::RawQueueSize() const {
    return raw_queue_.size();
}

std::size_t InputRoutingRuntime::NormalizedQueueSize() const {
    return normalized_queue_.size();
}

std::size_t InputRoutingRuntime::DiscardPending() {
    const auto discarded = raw_queue_.size() + normalized_queue_.size();
    raw_queue_.clear();
    normalized_queue_.clear();
    return discarded;
}

const std::string& InputRoutingRuntime::LastError() const {
    return last_error_;
}

} // namespace framekit::runtime
