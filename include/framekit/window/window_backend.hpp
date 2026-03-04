#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace framekit::window {

enum class BackendKind : std::uint8_t {
    kNone = 0,
    kCocoa = 1
};

struct WindowSpec {
    std::string title = "FrameKit";
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool visible = true;
};

struct WindowHandle {
    std::uint64_t value = 0;

    explicit operator bool() const { return value != 0; }
};

class IWindowBackend {
public:
    virtual ~IWindowBackend() = default;

    virtual std::string_view Name() const = 0;
    virtual bool Initialize() = 0;
    virtual WindowHandle CreateWindow(const WindowSpec& spec) = 0;
    virtual bool DestroyWindow(WindowHandle handle) = 0;
    virtual void PollEvents() = 0;
    virtual void Shutdown() = 0;
};

std::unique_ptr<IWindowBackend> CreateBackend(BackendKind kind);
bool IsBackendAvailable(BackendKind kind);

} // namespace framekit::window
