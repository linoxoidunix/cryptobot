#include <benchmark/benchmark.h>

#include <iostream>
#include <memory>
#include <thread>

#include "aot/Logger.h"
#include "aot/common/thread_utils.h"
#include "aot/common/time_utils.h"
#include "moodycamel/concurrentqueue.h"

using namespace std::literals::chrono_literals;
auto timeout                  = 1ms;
const unsigned int repetition = 5;

template <class T>
class FixtureLFQueue : public benchmark::Fixture {
  public:
    void SetUp(::benchmark::State& state) override { diffs.resize(10000); current = diffs.begin();}

    void TearDown(::benchmark::State& state) override {
        diffs.clear();
    }
    bool AddElem() {
        if (current == diffs.end())
            return false;
        else {
            //std::this_thread::sleep_for(10ns);
            while (!lf_queue.try_enqueue(*current)) {
            };
            // t.x2++;
            current++;
        }
        return true;
    }
    std::vector<T> diffs;
    std::vector<T>::iterator current;
    using LFQueue = moodycamel::ConcurrentQueue<T>;
    LFQueue lf_queue;
};

BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueue, TestLFQueueInt, int)
(benchmark::State& st) {
    auto work = [this]() {
        while (AddElem()) {
        };
    };
    int h;
    auto Job1 = [this, &h] {
        // logd("start thread 2");
        // fmtlog::poll();
        for (int j = 0; j < diffs.size(); j++) {
            while (lf_queue.try_dequeue(h)) {
                // logd("{}",lf_queue.size_approx());
                // fmtlog::poll();
                benchmark::DoNotOptimize(h);
            }
            // std::cout << "deque " << i << " item";
        }
    };
    for (auto _ : st) {
        auto begin = common::getCurrentNanoS();
        std::jthread t0(work);
        std::this_thread::sleep_for(timeout);
        std::jthread t1(Job1);
        t0.join();
        t1.join();
        auto end             = common::getCurrentNanoS();
        auto elapsed_seconds = (end - begin) / diffs.size() * 1e-9;
        st.SetIterationTime(elapsed_seconds);
    }
}

BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueue, TestLFQueueIntPtr, int*)
(benchmark::State& st) {
    auto work = [this]() {
        while (AddElem()) {
        };
    };
    int item;
    int* ptr  = nullptr;
    auto& h   = *ptr;
    auto Job1 = [this, &ptr] {
        // logd("start thread 2");
        // fmtlog::poll();
        for (int j = 0; j < diffs.size(); j++) {
            while (lf_queue.try_dequeue(ptr)) {
                // logd("{}",lf_queue.size_approx());
                // fmtlog::poll();
                // benchmark::DoNotOptimize(ptr);
            }
            // std::cout << "deque " << i << " item";
        }
    };
    for (auto _ : st) {
        auto begin = common::getCurrentNanoS();
        std::jthread t0(work);
        std::this_thread::sleep_for(timeout);
        std::jthread t1(Job1);
        t0.join();
        t1.join();
        auto end             = common::getCurrentNanoS();
        auto elapsed_seconds = (end - begin) / diffs.size() * 1e-9;
        st.SetIterationTime(elapsed_seconds);
    }
}

BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueue, TestLFQueueDouble, double)
(benchmark::State& st) {
    auto work = [this]() {
        while (AddElem()) {
        };
    };
    double h;
    auto Job1 = [this, &h] {
        // logd("start thread 2");
        // fmtlog::poll();
        for (int j = 0; j < diffs.size(); j++) {
            while (lf_queue.try_dequeue(h)) {
                // logd("{}",lf_queue.size_approx());
                // fmtlog::poll();
                benchmark::DoNotOptimize(h);
            }
            // std::cout << "deque " << i << " item";
        }
    };
    for (auto _ : st) {
        auto begin = common::getCurrentNanoS();
        std::jthread t0(work);
        std::this_thread::sleep_for(timeout);
        std::jthread t1(Job1);
        t0.join();
        t1.join();
        auto end             = common::getCurrentNanoS();
        auto elapsed_seconds = (end - begin) / diffs.size() * 1e-9;
        st.SetIterationTime(elapsed_seconds);
    }
}

BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueue, TestLFQueueDoublePtr, double*)
(benchmark::State& st) {
    auto work = [this]() {
        while (AddElem()) {
        };
    };
    double item;
    double* ptr = &item;
    auto& h     = *ptr;
    auto Job1   = [this, &ptr] {
        // logd("start thread 2");
        // fmtlog::poll();
        for (int j = 0; j < diffs.size(); j++) {
            while (lf_queue.try_dequeue(ptr)) {
                // logd("{}",lf_queue.size_approx());
                // fmtlog::poll();
                // benchmark::DoNotOptimize(ptr);
            }
            // std::cout << "deque " << i << " item";
        }
    };
    for (auto _ : st) {
        auto begin = common::getCurrentNanoS();
        std::jthread t0(work);
        std::this_thread::sleep_for(timeout);
        std::jthread t1(Job1);
        t0.join();
        t1.join();
        auto end             = common::getCurrentNanoS();
        auto elapsed_seconds = (end - begin) / diffs.size() * 1e-9;
        st.SetIterationTime(elapsed_seconds);
    }
}
struct CustomStruct {
    double x        = 0;
    int x1          = 0;
    double y        = 0;
    int y1          = 0;
    double z        = 0;
    unsigned int x2 = 0;
    double z3       = 0;
};

BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueue, TestLFQueueCustomStruct,
                            CustomStruct)
(benchmark::State& st) {
    auto work = [this]() {
        while (AddElem()) {
        };
    };
    CustomStruct item;
    CustomStruct* ptr = &item;
    CustomStruct& h   = *ptr;
    auto Job1         = [this, &h] {
        // logd("start thread 2");
        // fmtlog::poll();
        for (int j = 0; j < diffs.size(); j++) {
            while (lf_queue.try_dequeue(h)) {
                // logd("{}",lf_queue.size_approx());
                // fmtlog::poll();
                benchmark::DoNotOptimize(h);
            }
            // std::cout << "deque " << i << " item";
        }
    };
    for (auto _ : st) {
        auto begin = common::getCurrentNanoS();
        std::jthread t0(work);
        std::this_thread::sleep_for(timeout);
        std::jthread t1(Job1);
        t0.join();
        t1.join();
        auto end             = common::getCurrentNanoS();
        auto elapsed_seconds = (end - begin) / diffs.size() * 1e-9;
        st.SetIterationTime(elapsed_seconds);
    }
}

BENCHMARK_REGISTER_F(FixtureLFQueue, TestLFQueueInt)
    ->Iterations(repetition)
    ->UseManualTime()
    ->Unit(benchmark::kNanosecond);

// BENCHMARK_REGISTER_F(FixtureLFQueue, TestLFQueueDouble)
//     ->Iterations(5)
//     ->UseManualTime()
//     ->Unit(benchmark::kNanosecond);

// BENCHMARK_REGISTER_F(FixtureLFQueue, TestLFQueueIntPtr)
//     ->Iterations(5)
//     ->UseManualTime()
//     ->Unit(benchmark::kNanosecond);

// BENCHMARK_REGISTER_F(FixtureLFQueue, TestLFQueueDoublePtr)
//     ->Iterations(5)
//     ->UseManualTime()
//     ->Unit(benchmark::kNanosecond);

BENCHMARK_REGISTER_F(FixtureLFQueue, TestLFQueueCustomStruct)
    ->Iterations(repetition)
    ->UseManualTime()
    ->Unit(benchmark::kNanosecond);

//-----------------------------------------------------------------
template <class T>
class FixtureLFQueueRandom : public benchmark::Fixture {
  public:
    void SetUp(::benchmark::State& state) override {
        fmtlog::setLogLevel(fmtlog::DBG);
        fmtlog::setLogFile("555.txt");
    }

    void TearDown(::benchmark::State& state) override { current = 0; }
    bool AddElem() {
        if (current == i)
            return false;
        else {
            //std::this_thread::sleep_for(1ns);
            T t{0};
            t.x2++;
            t.z++;
            t.x1++;
            while (!lf_queue.try_enqueue(t)) {
            };
            current++;
        }
        return true;
    }
    unsigned int i       = 10000;
    unsigned int current = 0;
    using LFQueue        = moodycamel::ConcurrentQueue<T>;
    LFQueue lf_queue;
};

// BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueueRandom, TestLFQueueIntRandom, int)
// (benchmark::State& st) {
//     auto work = [this]() {
//         while (AddElem()) {
//         };
//     };
//     int item;

//     for (auto _ : st) {
//         auto begin = common::getCurrentNanoS();
//         std::jthread t0(work);
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(100ns);
//         // for(int j = 0; j < i; j++) {
//         while (lf_queue.try_dequeue(item)) {
//             benchmark::DoNotOptimize(item);
//             // std::cout << item << "\n";
//         }
//         // std::cout << "deque " << i << " item";
//         //}
//         auto end             = common::getCurrentNanoS();
//         auto elapsed_seconds = (end - begin) / i * 1e-9;
//         st.SetIterationTime(elapsed_seconds);
//     }
// }

BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueueRandom, TestLFQueueCustomStructRandom,
                            CustomStruct)
(benchmark::State& st) {
    auto work = [this]() {
        // logd("start thread 1");
        // fmtlog::poll();
        while (AddElem()) {
        };
    };
    CustomStruct item;
    CustomStruct* ptr = &item;
    CustomStruct& h   = *ptr;

    auto Job1         = [this, &h] {
        // logd("start thread 2");
        // fmtlog::poll();
        for (int j = 0; j < i; j++) {
            while (lf_queue.try_dequeue(h)) {
                // logd("{}",lf_queue.size_approx());
                // fmtlog::poll();
                benchmark::DoNotOptimize(h);
            }
            // std::cout << "deque " << i << " item";
        }
    };

    for (auto _ : st) {
        auto begin = common::getCurrentNanoS();
        std::jthread t0(work);
        std::this_thread::sleep_for(timeout);
        std::jthread t1(Job1);
        t0.join();
        t1.join();
        auto end             = common::getCurrentNanoS();
        auto elapsed_seconds = (end - begin) / i * 1e-9;
        st.SetIterationTime(elapsed_seconds);
        // thread->join();
    }
}

BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueueRandom,
                            TestLFQueueCustomStructRandomDequeBulk,
                            CustomStruct)
(benchmark::State& st) {
    auto work = [this]() {
        // logd("start thread 1");
        // fmtlog::poll();
        while (AddElem()) {
        };
    };
    CustomStruct item;
    CustomStruct* ptr = &item;
    CustomStruct& h   = *ptr;

    auto Job1         = [this, &h] {
        // logd("start thread 2");
        // fmtlog::poll();
        CustomStruct data[50];
        for (int j = 0; j < i; j++) {
            while (lf_queue.try_dequeue_bulk(data, 50)) {
                // logd("{}",lf_queue.size_approx());
                // fmtlog::poll();
                benchmark::DoNotOptimize(h);
            }
            // std::cout << "deque " << i << " item";
        }
    };

    for (auto _ : st) {
        auto begin = common::getCurrentNanoS();
        std::jthread t0(work);
        std::this_thread::sleep_for(timeout);
        std::jthread t1(Job1);
        t0.join();
        t1.join();
        auto end             = common::getCurrentNanoS();
        auto elapsed_seconds = (end - begin) / i * 1e-9;
        st.SetIterationTime(elapsed_seconds);
    }
}

// BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueue, TestLFQueueIntPtr, int*)
// (benchmark::State& st) {
//     auto work = [this]() {
//         auto begin = diffs.begin();
//         while (begin != diffs.end()) {
//             while (!lf_queue.try_enqueue(*begin)) {
//             }
//             // std::cout << std::distance(diffs.begin(), begin) << "\n";
//             ++begin;
//         }
//     };
//     int item;
//     int* ptr = &item;
//     auto& h  = *ptr;

//     for (auto _ : st) {
//         auto begin = common::getCurrentNanoS();
//         std::jthread t0(work);
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(10ms);
//         for (int i = 0; i < diffs.size(); i++) {
//             while (lf_queue.try_dequeue(ptr)) {
//                 benchmark::DoNotOptimize(h);
//             }
//             // std::cout << "deque " << i << " item";
//         }
//         auto end = common::getCurrentNanoS();
//         auto elapsed_seconds =
//             (end - begin) / FixtureLFQueue::diffs.size() * 1e-9;
//         st.SetIterationTime(elapsed_seconds);
//     }
// }

// BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueue, TestLFQueueDouble, double)
// (benchmark::State& st) {
//     auto work = [this]() {
//         auto begin = diffs.begin();
//         while (begin != diffs.end()) {
//             while (!lf_queue.try_enqueue(*begin)) {
//             }
//             // std::cout << std::distance(diffs.begin(), begin) << "\n";
//             ++begin;
//         }
//     };
//     double item;

//     for (auto _ : st) {
//         auto begin = common::getCurrentNanoS();
//         std::jthread t0(work);
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(10ms);
//         for (int i = 0; i < diffs.size(); i++) {
//             while (lf_queue.try_dequeue(item)) {
//                 benchmark::DoNotOptimize(item);
//             }
//             // std::cout << "deque " << i << " item";
//         }
//         auto end = common::getCurrentNanoS();
//         auto elapsed_seconds =
//             (end - begin) / FixtureLFQueue::diffs.size() * 1e-9;
//         st.SetIterationTime(elapsed_seconds);
//     }
// }

// BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueue, TestLFQueueDoublePtr, double*)
// (benchmark::State& st) {
//     auto work = [this]() {
//         auto begin = diffs.begin();
//         while (begin != diffs.end()) {
//             while (!lf_queue.try_enqueue(*begin)) {
//             }
//             // std::cout << std::distance(diffs.begin(), begin) << "\n";
//             ++begin;
//         }
//     };
//     double item;
//     double* ptr = &item;
//     auto& h     = *ptr;
//     for (auto _ : st) {
//         auto begin = common::getCurrentNanoS();
//         std::jthread t0(work);
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(10ms);
//         for (int i = 0; i < diffs.size(); i++) {
//             while (lf_queue.try_dequeue(ptr)) {
//                 benchmark::DoNotOptimize(h);
//             }
//             // std::cout << "deque " << i << " item";
//         }
//         auto end = common::getCurrentNanoS();
//         auto elapsed_seconds =
//             (end - begin) / FixtureLFQueue::diffs.size() * 1e-9;
//         st.SetIterationTime(elapsed_seconds);
//     }
// }
// struct CustomStruct {
//     double x;
//     int x1;
//     double y;
//     int y1;
//     double z;
//     unsigned int x2;
//     double z3;
// };

// BENCHMARK_TEMPLATE_DEFINE_F(FixtureLFQueue, TestLFQueueCustomStruct,
//                             CustomStruct)
// (benchmark::State& st) {
//     auto work = [this]() {
//         auto begin = diffs.begin();
//         while (begin != diffs.end()) {
//             while (!lf_queue.try_enqueue(*begin)) {
//             }
//             // std::cout << std::distance(diffs.begin(), begin) << "\n";
//             ++begin;
//         }
//     };
//     CustomStruct item{};
//     CustomStruct* ptr = &item;
//     auto& h           = *ptr;
//     for (auto _ : st) {
//         auto begin = common::getCurrentNanoS();
//         std::jthread t0(work);
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(10ms);
//         for (int i = 0; i < diffs.size(); i++) {
//             while (lf_queue.try_dequeue(h)) {
//                 benchmark::DoNotOptimize(h);
//             }
//             // std::cout << "deque " << i << " item";
//         }
//         auto end = common::getCurrentNanoS();
//         auto elapsed_seconds =
//             (end - begin) / FixtureLFQueue::diffs.size() * 1e-9;
//         st.SetIterationTime(elapsed_seconds);
//     }
// }

BENCHMARK_REGISTER_F(FixtureLFQueueRandom, TestLFQueueCustomStructRandom)
    ->Iterations(repetition)
    ->UseManualTime()
    ->Unit(benchmark::kNanosecond);

BENCHMARK_REGISTER_F(FixtureLFQueueRandom,
                     TestLFQueueCustomStructRandomDequeBulk)
    ->Iterations(repetition)
    ->UseManualTime()
    ->Unit(benchmark::kNanosecond);
// BENCHMARK_REGISTER_F(FixtureLFQueue, TestLFQueueDouble)
//     ->Iterations(5)
//     ->UseManualTime()
//     ->Unit(benchmark::kNanosecond);

// BENCHMARK_REGISTER_F(FixtureLFQueue, TestLFQueueIntPtr)
//     ->Iterations(5)
//     ->UseManualTime()
//     ->Unit(benchmark::kNanosecond);

// BENCHMARK_REGISTER_F(FixtureLFQueue, TestLFQueueDoublePtr)
//     ->Iterations(5)
//     ->UseManualTime()
//     ->Unit(benchmark::kNanosecond);

// BENCHMARK_REGISTER_F(FixtureLFQueue, TestLFQueueCustomStruct)
//     ->Iterations(5)
//     ->UseManualTime()
//     ->Unit(benchmark::kNanosecond);

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}
