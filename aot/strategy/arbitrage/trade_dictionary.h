#pragma once

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>

#include "aot/strategy/arbitrage/arbitrage_cycle.h"
#include "list"

namespace aot {
class TradeDictionary
    : public std::unordered_multimap<size_t,  // key is now the combined hash
                                              // value (size_t) of exchange id,
                                              // market_type, trading_apir
                                     ArbitrageCycle> {
  public:
    inline void SerializeToJson(nlohmann::json& j) const {
        auto array = nlohmann::json::array();  // Инициализируем как массив
        for (auto& [key, value] : *this) {
            nlohmann::json j;
            value.SerializeToJson(j);
            array.push_back(j);
        }
        j["trade_dictionary"] = array;
    }
    std::list<ArbitrageCycle> GetUniqueCycles() const {
        std::unordered_set<ArbitrageCycle, ArbitrageCycleHash> uniqueCycles;
        for (const auto& [key, value] : *this) {
            uniqueCycles.insert(value);
        }
        return {uniqueCycles.begin(), uniqueCycles.end()};
    }
};
};  // namespace aot