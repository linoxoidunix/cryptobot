#pragma once

#include <boost/functional/hash.hpp>
#include <boost/container_hash/hash.hpp>

#include "aot/common/types.h"

namespace aot{
    enum class Operation { kBuy, kSell };
struct Step {
    common::TradingPair trading_pair; // Торговая пара (например, BTC/USDT)         // Лучшая цена покупки (bid)
    common::ExchangeId exchange_id;         // Лучшая цена продажи (ask)
    Operation operation = Operation::kBuy;         // Истина, если покупаем, иначе продаем

    Step(common::TradingPair _trading_pair, common::ExchangeId _exchange_id, Operation _operation)
        : trading_pair(_trading_pair), exchange_id(_exchange_id), operation(_operation) {}
    
    bool operator==(const Step& other) const {
        return trading_pair == other.trading_pair &&
               exchange_id == other.exchange_id &&
               operation == other.operation;
    }
};
struct StepHash {
    std::size_t operator()(const Step& step) const {
        std::size_t seed = 0;

        // Комбинируем хэши полей
        boost::hash_combine(seed, common::TradingPairHash()(step.trading_pair));
        boost::hash_combine(seed, std::hash<common::ExchangeId>()(step.exchange_id));
        boost::hash_combine(seed, static_cast<int>(step.operation));

        return seed;
    }
};
};

namespace boost {
template<>
struct hash<aot::Step> {
    std::size_t operator()(const aot::Step& step) const {
        return aot::StepHash()(step);
    }
};
}



