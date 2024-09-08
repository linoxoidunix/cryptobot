#include <benchmark/benchmark.h>

#include <iostream>

#define FMT_HEADER_ONLY
#include "aot/third_party/fmt/core.h"

static void BM_INIT_VARIABLE_CONDITION(benchmark::State& state) {
    int x = 0;
    for (auto _ : state) {
        for (int i = 0; i < 100000; i++) {
            benchmark::DoNotOptimize(x++);
        }
    }
}

static void BM_INIT_VARIABLE_LIKELY(benchmark::State& state) {
    int x = 0;
    for (auto _ : state) {
        for (int i = 0; i < 100000; i++) [[likely]]
            benchmark::DoNotOptimize(x++);
    }
}

static void BM_INIT_VARIABLE_UNLIKELY(benchmark::State& state) {
    int x = 0;
    for (auto _ : state) {
        for (int i = 0; i < 100000; i++) [[unlikely]]
            benchmark::DoNotOptimize(x++);
    }
}
static void BM_INIT_ITERATE_UINT(benchmark::State& state) {
    unsigned int x = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; i++)
            benchmark::DoNotOptimize(x++);
    }
}

static void BM_INIT_ITERATE_INT(benchmark::State& state) {
    int x = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; i++)
            benchmark::DoNotOptimize(x++);
    }
}

static void BM_INIT_ITERATE_DOUBLE(benchmark::State& state) {
    double x = 0.1;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; i++)
            benchmark::DoNotOptimize(x++);
    }
}

static void BM_COPY_DOUBLE(benchmark::State& state) {
    double x = 0.1;
    double y = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; i++)
        {
            benchmark::DoNotOptimize(y = x);
            benchmark::DoNotOptimize(y = 0);
        }
    }
}

static void BM_COPY_INT(benchmark::State& state) {
    int x = 1;
    int y = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; i++){
            benchmark::DoNotOptimize(y = x);
            benchmark::DoNotOptimize(y = 0);
        }
    }
}

// Register the function as a benchmark
BENCHMARK(BM_INIT_VARIABLE_CONDITION);
BENCHMARK(BM_INIT_VARIABLE_LIKELY);
BENCHMARK(BM_INIT_VARIABLE_UNLIKELY);
BENCHMARK(BM_INIT_ITERATE_UINT);
BENCHMARK(BM_INIT_ITERATE_INT);
BENCHMARK(BM_INIT_ITERATE_DOUBLE);
BENCHMARK(BM_COPY_DOUBLE);
BENCHMARK(BM_COPY_INT);

int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
