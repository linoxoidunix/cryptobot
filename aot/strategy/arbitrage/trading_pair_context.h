#pragma once
#include "aot/common/types.h"
#include "nlohmann/json.hpp"

namespace aot {
struct TradingPairContext {
    // Лучшая цена покупки (bid)
    common::ExchangeId exchange_id;  // Лучшая цена продажи (ask)
    common::MarketType market_type =
        common::MarketType::kInvalid;  // Тип торговли (обычная или
                                       // ограниченная)
    common::TradingPair trading_pair;  // Торговая пара (например, BTC/USDT) //
    TradingPairContext(common::ExchangeId _exchange_id,
                       common::MarketType _market_type,
                       common::TradingPair _trading_pair)
        : exchange_id(_exchange_id),
          market_type(_market_type),
          trading_pair(_trading_pair) {}

    bool operator==(const TradingPairContext& other) const {
        return exchange_id == other.exchange_id &&
               market_type == other.market_type &&
               trading_pair == other.trading_pair;
    }
    void SerializeToJson(nlohmann::json& j) const {
        common::ExchangeIdPrinter exchange_id_printer;
        common::MarketTypePrinter market_type_printer;
        j = {
            {"exchange_id",
             fmt::format("{}", exchange_id_printer.ToString(exchange_id))},
            {"market_type",
             fmt::format("{}", market_type_printer.ToString(market_type))},
            {"trading_pair", fmt::format("{}", trading_pair)},
        };
    }
};

struct TradingPairContextHash {
    std::size_t operator()(const TradingPairContext& step) const;
};
};  // namespace aot