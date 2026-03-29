#include "platform_backend_factory_internal.hpp"

namespace framekit::runtime::detail {

bool CocoaBackendAvailable() {
    return false;
}

std::shared_ptr<IPlatformHost> CreateCocoaPlatformHost() {
    return {};
}

std::shared_ptr<IWindowHost> CreateCocoaWindowHost() {
    return {};
}

} // namespace framekit::runtime::detail
