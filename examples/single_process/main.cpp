#include "framekit/framekit.hpp"

#include <iostream>

int main() {
    framekit::ApplicationSpec spec;
    spec.application_name = "framekit-single-process-example";
    spec.mode = framekit::AppMode::kSingleProcess;

    framekit::runtime::MultiprocessRuntime runtime;
    runtime.Configure(spec);

    if (!runtime.Start()) {
        std::cerr << "runtime failed to start" << std::endl;
        return 1;
    }

    runtime.Tick();
    runtime.Stop();

    std::cout << "framekit example completed" << std::endl;
    return 0;
}
