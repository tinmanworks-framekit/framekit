# Execution Service Benchmark Baseline (2026-03-30)

Status: Stable
Last Reviewed: 2026-03-30

## Purpose

Provide a repeatable, non-gating benchmark harness for the baseline execution
service contract introduced in scheduler roadmap track #114.

This benchmark is advisory. It does not gate CI and should be interpreted as a
relative signal across local reruns on the same machine.

## Harness

Executable target:

- `framekit_execution_service_benchmark`

Source:

- `tests/test_execution_service_benchmark.cpp`

Workload:

- queue `N` tasks into `InlineExecutionService`
- drain all queued tasks
- verify completion states and checksum integrity
- print submit/drain timings and execution metrics

## Run Command

```bash
cmake -S . -B build-benchmark -DFRAMEKIT_BUILD_TESTS=ON -DFRAMEKIT_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-benchmark --target framekit_execution_service_benchmark
./build-benchmark/tests/framekit_execution_service_benchmark 100000
```

## Sample Output (Local)

```text
execution-service benchmark
task_count=100000
submit_ms=7
drain_ms=4
metrics.submitted=100000
metrics.completed=100000
metrics.failed=0
metrics.cancelled=0
metrics.rejected=0
```

## Interpretation Guidance

- `submit_ms` tracks queue insertion overhead.
- `drain_ms` tracks inline execution throughput.
- Any non-zero `failed`, `cancelled`, or `rejected` values in this workload are
  unexpected and should be investigated.
- Compare against prior runs on the same machine/configuration for trend
  analysis rather than absolute cross-machine comparisons.
