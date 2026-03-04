#include "framekit/window/cocoa_backend.hpp"

#import <AppKit/AppKit.h>

#include <unordered_set>

namespace framekit::window {

namespace {

class CocoaWindowBackend final : public IWindowBackend {
public:
    std::string_view Name() const override {
        return "cocoa";
    }

    bool Initialize() override {
        if (initialized_) {
            return true;
        }

        @autoreleasepool {
            NSApplication* app = [NSApplication sharedApplication];
            if (app == nil) {
                return false;
            }
            [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        }

        initialized_ = true;
        return true;
    }

    WindowHandle CreateWindow(const WindowSpec& spec) override {
        (void)spec;
        if (!initialized_) {
            return {};
        }

        WindowHandle handle;
        handle.value = next_window_id_++;
        windows_.insert(handle.value);
        return handle;
    }

    bool DestroyWindow(WindowHandle handle) override {
        return windows_.erase(handle.value) > 0;
    }

    void PollEvents() override {
        if (!initialized_) {
            return;
        }

        @autoreleasepool {
            NSEvent* event = nil;
            do {
                event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:[NSDate distantPast]
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES];
                if (event != nil) {
                    [NSApp sendEvent:event];
                }
            } while (event != nil);
        }
    }

    void Shutdown() override {
        windows_.clear();
        initialized_ = false;
    }

private:
    bool initialized_ = false;
    std::uint64_t next_window_id_ = 1;
    std::unordered_set<std::uint64_t> windows_;
};

} // namespace

std::unique_ptr<IWindowBackend> CreateCocoaBackend() {
    return std::make_unique<CocoaWindowBackend>();
}

bool IsCocoaBackendAvailable() {
    return true;
}

} // namespace framekit::window
