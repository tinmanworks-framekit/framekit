#include "framekit/window/window_backend.hpp"

#include "framekit/window/cocoa_backend.hpp"

namespace framekit::window {

std::unique_ptr<IWindowBackend> CreateBackend(BackendKind kind) {
    switch (kind) {
    case BackendKind::kCocoa:
        return CreateCocoaBackend();
    case BackendKind::kNone:
    default:
        return nullptr;
    }
}

bool IsBackendAvailable(BackendKind kind) {
    switch (kind) {
    case BackendKind::kCocoa:
        return IsCocoaBackendAvailable();
    case BackendKind::kNone:
    default:
        return false;
    }
}

} // namespace framekit::window
