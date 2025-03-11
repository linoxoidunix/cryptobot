#pragma once

#include <vector>

#include "aot/Logger.h"
#include "aot/common/time_utils.h"
#include "aot/common/types.h"
#include "aot/strategy/arbitrage/arbitrage_step.h"
#include "aot/strategy/arbitrage/trading_pair_context.h"
#include "nlohmann/json.hpp"

namespace cross_exchange {
struct ArbitrageTradeHash;
class ArbitrageTrade {
    aot::TradingPairContext trading_pair_context_;
    aot::Step step_;

  public:
    // Constructor to assign a unique ID
    ArbitrageTrade(aot::TradingPairContext trading_pair_context,
                   aot::Step step);
    virtual ~ArbitrageTrade() = default;
    common::Nanos time_open_;
    common::Nanos time_close_;

    void SerializeToJson(nlohmann::json& j) const;
    friend struct ArbitrageTradeHash;
    friend bool operator==(const ArbitrageTrade& lhs,
                           const ArbitrageTrade& rhs);
    inline aot::TradingPairContext TradingPairContext() const {
        return trading_pair_context_;
    }
};
struct ArbitrageTradeHash {
    std::size_t operator()(const ArbitrageTrade& cycle) const;
};

};  // namespace cross_exchange
