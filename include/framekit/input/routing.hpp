#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace framekit::runtime {

enum class InputDevice : std::uint8_t {
    kUnknown = 0,
    kPointer = 1,
    kKeyboard = 2,
    kText = 3,
};

enum class InputAction : std::uint8_t {
    kUnknown = 0,
    kPointerMove = 1,
    kPointerButtonDown = 2,
    kPointerButtonUp = 3,
    kKeyDown = 4,
    kKeyUp = 5,
    kTextInput = 6,
};

struct InputModifiers {
    bool shift = false;
    bool control = false;
    bool alt = false;
    bool meta = false;
};

struct RawInputEvent {
    std::string backend_type;
    std::string backend_payload;
    InputDevice device = InputDevice::kUnknown;
    InputAction action = InputAction::kUnknown;
    std::uint64_t timestamp_ns = 0;
    std::string source_id;
    std::string window_id;
    double pointer_x = 0.0;
    double pointer_y = 0.0;
    std::uint32_t key_code = 0;
    std::string text;
    InputModifiers modifiers;
};

struct NormalizedInputEvent {
    InputDevice device = InputDevice::kUnknown;
    InputAction action = InputAction::kUnknown;
    std::uint64_t timestamp_ns = 0;
    std::string source_id;
    std::string window_id;
    double pointer_x = 0.0;
    double pointer_y = 0.0;
    std::uint32_t key_code = 0;
    std::string text;
    InputModifiers modifiers;
    std::string backend_extension;
};

class IInputNormalizer {
public:
    virtual ~IInputNormalizer() = default;

    virtual bool Normalize(const RawInputEvent& raw_event, NormalizedInputEvent* normalized_event) const = 0;
};

class PassthroughInputNormalizer : public IInputNormalizer {
public:
    bool Normalize(const RawInputEvent& raw_event, NormalizedInputEvent* normalized_event) const override;
};

enum class MissingWindowTargetPolicy : std::uint8_t {
    kDropWithDiagnostics = 0,
    kFallbackToGlobal = 1,
};

struct InputDispatchResult {
    std::size_t delivered_handlers = 0;
    std::size_t dropped_events = 0;
    bool consumed = false;
};

class InputRouter {
public:
    using HandlerId = std::uint64_t;
    using InputHandler = std::function<bool(const NormalizedInputEvent& event)>;

    HandlerId SubscribeWindow(std::string window_id, InputHandler handler);
    HandlerId SubscribeGlobal(InputHandler handler);
    bool UnsubscribeWindow(const std::string& window_id, HandlerId handler_id);
    bool UnsubscribeGlobal(HandlerId handler_id);

    void SetFocusedWindow(std::string window_id);
    void SetPointerCaptureWindow(std::string window_id);
    void ClearPointerCaptureWindow();
    void SetMissingWindowTargetPolicy(MissingWindowTargetPolicy policy);

    InputDispatchResult Route(const NormalizedInputEvent& event) const;

private:
    struct HandlerEntry {
        HandlerId id = 0;
        std::uint64_t sequence = 0;
        InputHandler handler;
    };

    std::string ResolveTargetWindow(const NormalizedInputEvent& event) const;
    InputDispatchResult RouteHandlers(
        const std::vector<HandlerEntry>& handlers,
        const NormalizedInputEvent& event) const;

    std::unordered_map<std::string, std::vector<HandlerEntry>> window_handlers_;
    std::vector<HandlerEntry> global_handlers_;
    std::string focused_window_;
    std::string pointer_capture_window_;
    MissingWindowTargetPolicy missing_target_policy_ = MissingWindowTargetPolicy::kDropWithDiagnostics;
    HandlerId next_handler_id_ = 1;
    std::uint64_t next_sequence_ = 1;
};

class InputRoutingRuntime {
public:
    InputRoutingRuntime();

    void SetNormalizer(std::shared_ptr<IInputNormalizer> normalizer);

    InputRouter& Router();
    const InputRouter& Router() const;

    void EnqueueRawInput(RawInputEvent event);
    bool NormalizePending();
    InputDispatchResult DispatchImmediate();

    std::size_t RawQueueSize() const;
    std::size_t NormalizedQueueSize() const;
    std::size_t DiscardPending();

    const std::string& LastError() const;

private:
    std::shared_ptr<IInputNormalizer> normalizer_;
    InputRouter router_;
    std::vector<RawInputEvent> raw_queue_;
    std::vector<NormalizedInputEvent> normalized_queue_;
    std::string last_error_;
};

} // namespace framekit::runtime
