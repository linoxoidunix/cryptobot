#include <benchmark/benchmark.h>
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <uv.h>
#include "aot/common/mem_pool.h"

// Number of tasks to submit
constexpr int NUM_TASKS = 10000;

// Function to simulate a task
void task() {
    // Simulated workload
    std::this_thread::sleep_for(std::chrono::microseconds(1));
}

// Custom memory pool for uv_work_t
class MemoryPool {
public:
    MemoryPool(size_t size) : pool(size) {
        for (size_t i = 0; i < size; ++i) {
            free_list.push_back(&pool[i]);
        }
    }

    uv_work_t* allocate() {
        if (free_list.empty()) {
            return nullptr; // Handle out of memory if needed
        }
        uv_work_t* obj = free_list.back();
        free_list.pop_back();
        return obj;
    }

    void deallocate(uv_work_t* obj) {
        free_list.push_back(obj);
    }

private:
    std::vector<uv_work_t> pool;
    std::vector<uv_work_t*> free_list;
};

// Work callback function for libuv
void work_callback(uv_work_t* req) {
    task(); // Perform the task
}

// After work callback function for libuv
void after_work_callback(uv_work_t* req, int status) {
    MemoryPool* pool = static_cast<MemoryPool*>(req->data); // Retrieve the memory pool pointer
    pool->deallocate(req); // Return the work request to the memory pool
}

// Libuv benchmark
static void BM_Libuv(benchmark::State& state) {
    MemoryPool memoryPool(NUM_TASKS); // Create a memory pool for uv_work_t
    uv_loop_t* loop = uv_loop_new(); // Create a new event loop

    for (auto _ : state) {
        for (int i = 0; i < NUM_TASKS; ++i) {
            uv_work_t* req = memoryPool.allocate(); // Allocate from memory pool
            if (req == nullptr) {
                std::cerr << "Memory pool exhausted!" << std::endl;
                return;
            }
            req->data = &memoryPool; // Store the memory pool reference in the request

            uv_queue_work(loop, req, work_callback, after_work_callback);
        }

        uv_run(loop, UV_RUN_DEFAULT); // Run the event loop
        uv_loop_delete(loop); // Clean up the loop after running
        loop = uv_loop_new(); // Create a new event loop for the next iteration
    }

    uv_loop_delete(loop); // Clean up the last loop
}

// Boost.Asio benchmark
static void BM_BoostAsio(benchmark::State& state) {
    for (auto _ : state) {
        boost::asio::io_context io_context;
        boost::asio::thread_pool pool(4); // Create a thread pool with 4 threads

        for (int i = 0; i < NUM_TASKS; ++i) {
            boost::asio::post(pool, task);
        }

        pool.join(); // Wait for all tasks to complete
    }
}
BENCHMARK(BM_BoostAsio);
BENCHMARK(BM_Libuv);

// Main function to run benchmarks
int main(int argc, char** argv) {
    setenv("UV_THREADPOOL_SIZE", "4", 1); // Setting to 4 threads
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}