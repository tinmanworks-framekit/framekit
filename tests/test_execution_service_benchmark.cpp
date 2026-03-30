#include "framekit/framekit.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

namespace {

std::uint64_t ParsePositiveArg(const char* value, std::uint64_t fallback) {
    if (!value) {
        return fallback;
    }

    try {
        const auto parsed = std::stoull(value);
        return parsed == 0 ? fallback : parsed;
    } catch (...) {
        return fallback;
    }
}

} // namespace

int main(int argc, char** argv) {
    const std::uint64_t task_count = argc > 1 ? ParsePositiveArg(argv[1], 100000) : 100000;

    framekit::runtime::InlineExecutionService service;

    std::uint64_t checksum = 0;
    const auto submit_start = std::chrono::steady_clock::now();
    for (std::uint64_t index = 1; index <= task_count; ++index) {
        const auto result = service.SubmitTask([&checksum, index]() {
            checksum += index;
        });
        if (result.state != framekit::runtime::ExecutionTaskState::kQueued) {
            std::cerr << "task submission rejected at index " << index << '\n';
            return 1;
        }
    }
    const auto submit_end = std::chrono::steady_clock::now();

    const auto drain_start = std::chrono::steady_clock::now();
    const auto drained = service.Drain();
    const auto drain_end = std::chrono::steady_clock::now();

    if (drained.size() != task_count) {
        std::cerr << "unexpected drained task count: " << drained.size() << '\n';
        return 2;
    }

    for (const auto& result : drained) {
        if (result.state != framekit::runtime::ExecutionTaskState::kCompleted) {
            std::cerr << "task did not complete successfully: id=" << result.task_id << '\n';
            return 3;
        }
    }

    const auto expected_checksum = (task_count * (task_count + 1)) / 2;
    if (checksum != expected_checksum) {
        std::cerr << "checksum mismatch: got " << checksum << ", expected " << expected_checksum << '\n';
        return 4;
    }

    const auto submit_ms = std::chrono::duration_cast<std::chrono::milliseconds>(submit_end - submit_start).count();
    const auto drain_ms = std::chrono::duration_cast<std::chrono::milliseconds>(drain_end - drain_start).count();

    const auto metrics = service.Metrics();
    std::cout << "execution-service benchmark" << '\n';
    std::cout << "task_count=" << task_count << '\n';
    std::cout << "submit_ms=" << submit_ms << '\n';
    std::cout << "drain_ms=" << drain_ms << '\n';
    std::cout << "metrics.submitted=" << metrics.submitted << '\n';
    std::cout << "metrics.completed=" << metrics.completed << '\n';
    std::cout << "metrics.failed=" << metrics.failed << '\n';
    std::cout << "metrics.cancelled=" << metrics.cancelled << '\n';
    std::cout << "metrics.rejected=" << metrics.rejected << '\n';

    return 0;
}
