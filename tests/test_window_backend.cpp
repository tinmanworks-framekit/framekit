#include "framekit/window/window_backend.hpp"

#include <cstdlib>
#include <iostream>

namespace {

[[noreturn]] void FailRequirement(const char* expr, int line) {
    std::cerr << "Requirement failed at line " << line << ": " << expr << '\n';
    std::abort();
}

#define REQUIRE(expr)            \
    do {                         \
        if (!(expr)) {           \
            FailRequirement(#expr, __LINE__); \
        }                        \
    } while (false)

} // namespace

int main() {
    using framekit::window::BackendKind;
    using framekit::window::CreateBackend;
    using framekit::window::IsBackendAvailable;
    using framekit::window::WindowSpec;

#if defined(__APPLE__) && FRAMEKIT_HAS_COCOA_BACKEND
    REQUIRE(IsBackendAvailable(BackendKind::kCocoa));
    auto backend = CreateBackend(BackendKind::kCocoa);
    REQUIRE(static_cast<bool>(backend));
    REQUIRE(backend->Name() == "cocoa");
    REQUIRE(backend->Initialize());

    WindowSpec spec;
    spec.title = "framekit-cocoa-runtime-validation";
    spec.visible = false;
    const auto handle = backend->CreateWindow(spec);
    REQUIRE(static_cast<bool>(handle));

    backend->PollEvents();
    REQUIRE(backend->DestroyWindow(handle));
    REQUIRE(!backend->DestroyWindow(handle));
    backend->Shutdown();
#else
    REQUIRE(!IsBackendAvailable(BackendKind::kCocoa));
    auto backend = CreateBackend(BackendKind::kCocoa);
    REQUIRE(!backend);
#endif

    return 0;
}
