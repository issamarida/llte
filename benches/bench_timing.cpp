#include <benchmark/benchmark.h>
#include <llte/timing.hpp>

#include <chrono>

static void BM_now_cycles(benchmark::State& state) {
    for (auto _ : state) {
        auto cycles = llte::timing::now_cycles();
        benchmark::DoNotOptimize(cycles);
    }
}
BENCHMARK(BM_now_cycles);

static void BM_chrono_steady_now(benchmark::State& state) {
    for (auto _ : state) {
        auto timestamp = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(timestamp);
    }
}
BENCHMARK(BM_chrono_steady_now);

static void BM_chrono_system_now(benchmark::State& state) {
    for (auto _ : state) {
        auto timestamp = std::chrono::system_clock::now();
        benchmark::DoNotOptimize(timestamp);
    }
}
BENCHMARK(BM_chrono_system_now);

static void BM_now_cycles_delta(benchmark::State& state) {
    for (auto _ : state) {
        auto start_cycles   = llte::timing::now_cycles();
        auto end_cycles     = llte::timing::now_cycles();
        auto elapsed_cycles = end_cycles - start_cycles;
        benchmark::DoNotOptimize(elapsed_cycles);
    }
}
BENCHMARK(BM_now_cycles_delta);
