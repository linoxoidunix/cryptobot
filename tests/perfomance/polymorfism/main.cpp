#include <iostream>
#include <benchmark/benchmark.h>

// Inline function
inline int add_inline(int a, int b) {
    return a + b;
}

// Direct function call
int add_direct(int a, int b) {
    return a + b;
}

// Base class for dynamic polymorphism
class Base {
public:
    virtual int add(int a, int b) {
        return a + b;
    }
};

class Derived : public Base {
public:
    int add(int a, int b) override {
        return a + b;
    }
};

// Template for static polymorphism
template <typename T>
class StaticPolymorphicAdder {
public:
    int add(int a, int b) {
        return static_cast<T*>(this)->add_impl(a, b);
    }
};

class StaticDerived : public StaticPolymorphicAdder<StaticDerived> {
public:
    int add_impl(int a, int b) {
        return a + b;
    }
};

// Dynamic cast example
class BaseDynamic {
public:
    virtual ~BaseDynamic() = default; // Ensure polymorphic behavior
};

class DerivedDynamic : public BaseDynamic {
public:
    int add(int a, int b) {
        return a + b;
    }
};

// Benchmark functions
static void BenchmarkInline(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(add_inline(1, 2));
    }
}

static void BenchmarkDirect(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(add_direct(1, 2));
    }
}

static void BenchmarkDynamicPolymorphism(benchmark::State& state) {
    Base* obj = new Derived();
    for (auto _ : state) {
        benchmark::DoNotOptimize(obj->add(1, 2));
    }
    delete obj;  // Clean up
}

static void BenchmarkStaticPolymorphism(benchmark::State& state) {
    StaticDerived obj;
    for (auto _ : state) {
        benchmark::DoNotOptimize(obj.add(1, 2));
    }
}

static void BenchmarkDynamicCast(benchmark::State& state) {
    BaseDynamic* base = new DerivedDynamic();
    for (auto _ : state) {
        DerivedDynamic* derived = dynamic_cast<DerivedDynamic*>(base);
        if (derived) {
            benchmark::DoNotOptimize(derived->add(1, 2));
        }
    }
    delete base;  // Clean up
}

// Register benchmarks
BENCHMARK(BenchmarkInline);
BENCHMARK(BenchmarkDirect);
BENCHMARK(BenchmarkDynamicPolymorphism);
BENCHMARK(BenchmarkStaticPolymorphism);
BENCHMARK(BenchmarkDynamicCast);

// Main function to run benchmarks
int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}