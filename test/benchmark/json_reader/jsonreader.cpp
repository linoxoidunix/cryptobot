#include <benchmark/benchmark.h>
#include <iostream>
#include "simdjson.h"

#define FMT_HEADER_ONLY
#include <bybit/third_party/fmt/core.h>
using namespace simdjson;

std::string current_path = PATH_TO_TEST;

static void BM_StringCreation(benchmark::State& state) {
    ondemand::parser parser;
    padded_string json = padded_string::load(fmt::format("{0}/{1}", current_path, "input.txt"));

    for (auto _ : state)
    {
        ondemand::document tweets = parser.iterate(json);
        auto v = std::string_view(tweets["k"]["o"]);
        //std::cout << std::string_view(tweets["k"]["o"]);//<< (tweets["k"]["c"]);
    }
}
// Register the function as a benchmark
BENCHMARK(BM_StringCreation);


BENCHMARK_MAIN();
