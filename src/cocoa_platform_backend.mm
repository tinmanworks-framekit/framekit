#include "platform_backend_factory_internal.hpp"

#import <AppKit/AppKit.h>

namespace framekit::runtime::detail {

namespace {

class CocoaPlatformHost final : public IPlatformHost {
public:
    bool Initialize() override {
        @autoreleasepool {
            if (![NSThread isMainThread]) {
                return false;
            }

            app_ = [NSApplication sharedApplication];
            if (!app_) {
                return false;
            }

            [app_ setActivationPolicy:NSApplicationActivationPolicyRegular];
            return true;
        }
    }

    bool PumpEvents() override {
        @autoreleasepool {
            if (!app_) {
                return false;
            }

            for (;;) {
                NSEvent* event = [app_ nextEventMatchingMask:NSEventMaskAny
                                                   untilDate:[NSDate distantPast]
                                                      inMode:NSDefaultRunLoopMode
                                                     dequeue:YES];
                if (!event) {
                    break;
                }

                [app_ sendEvent:event];
            }

            [app_ updateWindows];
            return true;
        }
    }

    bool Teardown() override {
        app_ = nil;
        return true;
    }

private:
    NSApplication* __strong app_ = nil;
};

class CocoaWindowHost final : public IWindowHost {
public:
    bool CreatePrimaryWindow(const WindowSpec& spec) override {
        @autoreleasepool {
            if (![NSThread isMainThread]) {
                return false;
            }

            NSRect frame = NSMakeRect(0.0, 0.0, spec.width, spec.height);
            NSWindowStyleMask style_mask = NSWindowStyleMaskTitled |
                NSWindowStyleMaskClosable |
                NSWindowStyleMaskMiniaturizable |
                NSWindowStyleMaskResizable;

            window_ = [[NSWindow alloc] initWithContentRect:frame
                                                  styleMask:style_mask
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
            if (!window_) {
                return false;
            }

            NSString* title = [NSString stringWithUTF8String:spec.title.c_str()];
            if (title) {
                [window_ setTitle:title];
            }

            if (spec.visible) {
                [window_ makeKeyAndOrderFront:nil];
            } else {
                [window_ orderOut:nil];
            }

            return true;
        }
    }

    bool PreRender() override {
        return true;
    }

    bool Render() override {
        return true;
    }

    bool PostRender() override {
        return true;
    }

    bool TeardownWindows() override {
        if (window_) {
            [window_ orderOut:nil];
            [window_ close];
            window_ = nil;
        }

        return true;
    }

private:
    NSWindow* __strong window_ = nil;
};

} // namespace

bool CocoaBackendAvailable() {
    return true;
}

std::shared_ptr<IPlatformHost> CreateCocoaPlatformHost() {
    return std::make_shared<CocoaPlatformHost>();
}

std::shared_ptr<IWindowHost> CreateCocoaWindowHost() {
    return std::make_shared<CocoaWindowHost>();
}

} // namespace framekit::runtime::detail
