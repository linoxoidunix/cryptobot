#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <boost/intrusive/unordered_set.hpp>
#include <boost/functional/hash.hpp>

#include "aot/common/types.h"
#include "aot/strategy/arbitrage/arbitrage_step.h"

namespace aot{
class ArbitrageCycle : public std::vector<Step>, public boost::intrusive::unordered_set_base_hook<>
{
    std::unordered_map<common::ExchangeId, std::unordered_set<common::TradingPair, common::TradingPairHash, common::TradingPairEqual>> exchange_pairs_;
public:
    using std::vector<Step>::vector; // Inherit all constructors
    void push_back(const Step step) {
        // Добавляем шаг в вектор
        std::vector<Step>::push_back(step);

        // Добавляем информацию о бирже и торговой паре в unordered_multimap
        exchange_pairs_[step.exchange_id].insert(step.trading_pair);    
    }
    const std::unordered_map<common::ExchangeId, std::unordered_set<common::TradingPair, common::TradingPairHash, common::TradingPairEqual>>& GetExchangePairs() const {
        return exchange_pairs_;
    }
};

struct ArbitrageCycleHash {
    std::size_t operator()(const ArbitrageCycle& cycle) const {
        std::size_t seed = 0;

        // Хэшируем содержимое вектора Step
        for (const auto& step : cycle) {
            boost::hash_combine(seed, step);  // Предполагается, что Step уже имеет свой хэш
        }

        // Хэшируем содержимое exchange_pairs_
        for (const auto& [exchange_id, pairs_set] : cycle.GetExchangePairs()) {
            boost::hash_combine(seed, std::hash<common::ExchangeId>()(exchange_id));

            for (const auto& pair : pairs_set) {
                boost::hash_combine(seed, common::TradingPairHash()(pair));
            }
        }

        return seed;
    }
};

bool operator==(const ArbitrageCycle& lhs, const ArbitrageCycle& rhs) {
    if (lhs.size() != rhs.size()) return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
};