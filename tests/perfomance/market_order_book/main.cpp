#define FMT_HEADER_ONLY
#include <benchmark/benchmark.h>

#include <fstream>
#include <iostream>
#include <regex>

#include "aot/Binance.h"
#include "aot/market_data/market_update.h"
#include "aot/strategy/market_order_book.h"
#include "aot/third_party/fmt/core.h"
#include "magic_enum.hpp"

constexpr std::string_view kPathData = PATH_TO_DATA;

class FixtureMarketOrderBook : public benchmark::Fixture {
  public:
    void SetUp(::benchmark::State& state) override{
        fmtlog::setLogLevel(fmtlog::OFF);
        std::ifstream infile(fmt::format("{}/{}", kPathData, "999.txt"));
        std::string line;
        std::regex word_regex(
            ".+ MEMarketUpdateDouble\\[ticker:(\\w*) type:(\\w*) side:(\\w+) "
            "qty:(\\d+\\.\\d+) price:(\\d+\\.\\d+)\\]");
        std::smatch pieces_match;
        diffs.reserve(200000);
        while (std::getline(infile, line)) {
            if (std::regex_match(line, pieces_match, word_regex)) {
                Exchange::MEMarketUpdate update;

                auto _type = magic_enum::enum_cast<Exchange::MarketUpdateType>(
                    std::string(pieces_match[2]));
                if (!_type.has_value()) continue;
                update.type        = _type.value();
                auto _side         = magic_enum::enum_cast<common::Side>(
                    std::string(pieces_match[3]));
                if (!_side.has_value()) continue;
                update.side         = _side.value();

                double qty          = stod(pieces_match[4]);
                double price        = stod(pieces_match[5]);

                update.qty          = qty * 10000;
                update.price        = price * 100;
                diffs.push_back(update);
            }
        }
    }

    void TearDown(::benchmark::State& state) override{
        diffs.clear();
    }

    std::vector<Exchange::MEMarketUpdate> diffs;
};

BENCHMARK_DEFINE_F(FixtureMarketOrderBook, TestMarketOrderBook)(benchmark::State& st) {
    common::TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";
    
    common::TradingPairHashMap pair;
    common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
    pair[{2, 1}] = pair_info;
    backtesting::MarketOrderBook book(common::TradingPair{2,1}, pair);
    for (auto _ : st) {
        auto begin = common::getCurrentNanoS();
        for (int i = 0; i < diffs.size(); i++) {
            book.OnMarketUpdate(&diffs[i]);
        }
        auto end             = common::getCurrentNanoS();
        auto elapsed_seconds = (end - begin) / diffs.size() * 1e-9;
        st.SetIterationTime(elapsed_seconds);
        book.ClearOrderBook();
    }
}

BENCHMARK_REGISTER_F(FixtureMarketOrderBook, TestMarketOrderBook)
    ->Iterations(100)
    ->UseManualTime()
    ->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
