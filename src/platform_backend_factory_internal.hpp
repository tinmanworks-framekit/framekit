#pragma once

#include "framekit/platform/host_runtime.hpp"

#include <memory>

namespace framekit::runtime::detail {

bool CocoaBackendAvailable();
std::shared_ptr<IPlatformHost> CreateCocoaPlatformHost();
std::shared_ptr<IWindowHost> CreateCocoaWindowHost();

} // namespace framekit::runtime::detail
