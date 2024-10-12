#include <memory>

#include <benchmark/benchmark.h>

#include "aot/Logger.h"
#include "aot/common/mem_pool.h"  // Include the memory pool header from your provided code
#include "aot/common/time_utils.h"  // Include the memory pool header from your provided code
// Create a dummy class for allocation

class Dummy;
class Dummy {
  public:
    char* data;
    Dummy(size_t value) : data(new char[value]) {}
    Dummy() : data(nullptr) {}
    ~Dummy() {
        delete[] data;
    }
};

static void Benchmark_MemPool_Allocate(benchmark::State& state) {
    common::MemPool<Dummy> pool(1000);  // Create pool with 1000 elements
    std::vector<Dummy*> objects;
    objects.reserve(1000);
    for (auto _ : state) {
        for (int i = 0; i < 999; ++i) {  // Fixed to 999 iterations for allocations
            Dummy* obj = pool.allocate();  // Allocate objects
            benchmark::DoNotOptimize(obj);   // Prevent compiler optimization
            objects.push_back(obj);
        }
        for (auto obj : objects) {
            pool.deallocate(obj);
        }
        objects.clear();
    }
}

// Benchmark for MemoryPool class (boost::pool-based memory pool)
static void BM_MemoryPoolAllocate(benchmark::State& state) {
    size_t poolSize = 1000;
    common::MemoryPool<Dummy> pool(1000);

    std::vector<Dummy*> objects;
    objects.reserve(poolSize);

    for (auto _ : state) {

        // Allocate objects in the pool
        for (size_t i = 0; i < poolSize; ++i) {
            auto* obj = pool.Allocate();
            benchmark::DoNotOptimize(obj);  // Prevent optimization of unused result
            objects.push_back(obj);
        }

        //Deallocate the objects
        for (auto obj : objects) {
            pool.Deallocate(obj);
        }
        objects.clear();  // Clear the vector for the next iteration
    }
}

static void BM_MemPoolAllocateRate(benchmark::State& state) {
    // Get the size of the object from the benchmark state (used for varying sizes)
    size_t objectSize = state.range(0);

    // Create a memory pool for the dynamically sized Dummy object
    common::MemPool<Dummy> pool(2);  // Pool size of 1 since we're only allocating one object

    for (auto _ : state) {
        // Start measuring time
        auto start = common::getCurrentNanoS();

        // Allocate an object of the specified size
        Dummy* obj = pool.allocate(objectSize); 
        benchmark::DoNotOptimize(obj);  // Prevent optimization of unused result

        // Stop measuring time
        auto end = common::getCurrentNanoS();
        auto duration = (end - start)* 1e-9;;  // duration in microseconds

        // Output the timing result
        //std::cout << "Allocation time for size " << objectSize << ": " << duration.count() << " microseconds" << std::endl;

        // Immediately deallocate to keep the benchmark focused on Allocate
        pool.deallocate(obj);
        state.SetIterationTime(duration);
    }

    // Set the complexity for this benchmark
    state.SetComplexityN(objectSize);
}

static void BM_MemoryPoolAllocateRate(benchmark::State& state) {
    // Get the size of the object from the benchmark state (used for varying sizes)
    size_t objectSize = state.range(0);

    // Create a memory pool for the dynamically sized Dummy object
    common::MemoryPool<Dummy> pool(1);  // Pool size of 1 since we're only allocating one object

    for (auto _ : state) {
        // Start measuring time
        auto start = common::getCurrentNanoS();

        // Allocate an object of the specified size
        Dummy* obj = pool.Allocate(objectSize); 
        benchmark::DoNotOptimize(obj);  // Prevent optimization of unused result

        // Stop measuring time
        auto end = common::getCurrentNanoS();
        auto duration = (end - start)* 1e-9;;  // duration in microseconds

        // Output the timing result
        //std::cout << "Allocation time for size " << objectSize << ": " << duration.count() << " microseconds" << std::endl;

        // Immediately deallocate to keep the benchmark focused on Allocate
        pool.Deallocate(obj);
        state.SetIterationTime(duration);
    }

    // Set the complexity for this benchmark
    state.SetComplexityN(objectSize);
}

// Register the benchmarks
BENCHMARK(Benchmark_MemPool_Allocate);
BENCHMARK(BM_MemoryPoolAllocate);
BENCHMARK(BM_MemPoolAllocateRate)->RangeMultiplier(2)->Range(1, 1 << 10)->Complexity();
BENCHMARK(BM_MemoryPoolAllocateRate)->RangeMultiplier(2)->Range(1, 1 << 10)->Complexity();


// Main function to run the benchmarks
// Main function to run benchmarks
int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}