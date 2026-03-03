#include "framekit/framekit.hpp"

#include <cassert>

int main() {
    framekit::ApplicationSpec spec;
    assert(spec.mode == framekit::AppMode::kSingleProcess);
    assert(spec.transport_kind == framekit::IpcTransportKind::kNone);

    framekit::runtime::MultiprocessRuntime runtime;
    runtime.Configure(spec);

    assert(runtime.Start());
    assert(runtime.IsRunning());
    runtime.Stop();
    assert(!runtime.IsRunning());

    return 0;
}
