#include "framekit/window/cocoa_backend.hpp"

namespace framekit::window {

std::unique_ptr<IWindowBackend> CreateCocoaBackend() {
    return nullptr;
}

bool IsCocoaBackendAvailable() {
    return false;
}

} // namespace framekit::window
