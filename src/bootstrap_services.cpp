#include "framekit/runtime/bootstrap_services.hpp"

#include <memory>

namespace framekit::runtime {

bool RegisterDefaultBootstrapServices(ServiceContext& services) {
    bool ok = true;

    ok = ok && services.Register(std::make_shared<DiagnosticsService>());
    ok = ok && services.Register(std::make_shared<ConfigurationService>());
    ok = ok && services.Register(std::make_shared<TimingService>());

    return ok;
}

} // namespace framekit::runtime
