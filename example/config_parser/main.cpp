#include <string_view>
#include <vector>
#include <functional>
#include <string>
#include <iostream>
#include <memory>
#include <atomic>
#include <unordered_set>

#include "aot/config/config.h"


int main(int argc, char** argv) {
    std::string_view file_name = argv[1];
    auto toml_parser = std::make_shared<config::TomlFileParser>();
    auto ticker_manager = std::make_shared<config::TickerManager>();
    config::TickerParser parser(toml_parser, ticker_manager);
    parser.Parse(file_name);
    auto& pairs_binance = parser.PairsBinance();
    auto& pairs_bybit =  parser.PairsBybit();
    for (const auto& [info, _, pair] : pairs_binance) {
        fmt::print("{} {} {}\n", common::ExchangeId::kBinance, pair, info);
    }
    for (const auto& [info, _, pair] : pairs_bybit) {
        fmt::print("{} {} {}\n",common::ExchangeId::kBybit, pair, info);
    }
    return 0;
}

