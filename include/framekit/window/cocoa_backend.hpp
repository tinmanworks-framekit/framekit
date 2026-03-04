#pragma once

#include "framekit/window/window_backend.hpp"

namespace framekit::window {

std::unique_ptr<IWindowBackend> CreateCocoaBackend();
bool IsCocoaBackendAvailable();

} // namespace framekit::window
