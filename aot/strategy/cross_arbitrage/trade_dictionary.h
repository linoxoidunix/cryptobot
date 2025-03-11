#pragma once

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>

#include "aot/strategy/arbitrage/arbitrage_trade.h"
#include "list"

namespace cross_exchange {
class TradeDictionary
    : public std::unordered_multimap<size_t,  // key is now the combined hash
                                              // value (size_t) of exchange id,
                                              // market_type, trading_apir
                                     ArbitrageTrade> {
  public:
    std::list<ArbitrageTrade> GetUniqueTrades() const {
        std::unordered_set<ArbitrageTrade, ArbitrageTradeHash> unique_trades;
        for (const auto& [key, value] : *this) {
            unique_trades.insert(value);
        }
        return {unique_trades.begin(), unique_trades.end()};
    }
};
};  // namespace cross_exchange