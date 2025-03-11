#pragma once
#include <iostream>

#include "aot/Logger.h"
#include "aot/bus/bus.h"
#include "aot/bus/bus_component.h"
#include "aot/bus/bus_event.h"
#include "aot/common/exchange_trading_pair.h"
#include "aot/common/mem_pool.h"
#include "aot/common/types.h"
#include "aot/pnl/pnl_calculator.h"
#include "aot/strategy/arbitrage/trade_state.h"
#include "aot/strategy/arbitrage/transaction_report.h"
// TODO NEED EXCLUDE THIS HEADER #include
// "aot/strategy/arbitrage/arbitrage_strategy.h"
#include "aot/strategy/arbitrage/arbitrage_strategy.h"
#include "aot/strategy/arbitrage/arbitrage_trade.h"
// TODO NEED EXCLUDE THIS HEADER #include
#include "aot/strategy/cross_arbitrage/trade_dictionary.h"
#include "aot/strategy/market_order.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;
namespace aot {

template <typename ThreadPool>
class CrossExchangeStrategyComponent : public bus::Component {
    ThreadPool& thread_pool_;
    aot::CoBus& bus_;
    Trading::ExchangeBBOMap exchange_bbo_map_;
    cross_exchange::TradeDictionary exchange_trading_pair_arbitrage_map_;
    static constexpr std::string_view name_component_ =
        "CrossExchangeStrategyComponent";
    ArbitrageReportPool arbitrage_report_pool_;
    BusEventArbitrageReportPool bus_event_arbitrage_report_pool_;
    // using TradesState = std::unordered_map<size_t, TradeState>;
    aot::TradesState trades_state_;
    ExchangeTradingPairs& exchange_trading_pairs_;

  public:
    explicit CrossExchangeStrategyComponent(
        ThreadPool& thread_pool, aot::CoBus& bus, size_t max_event_per_time,
        ExchangeTradingPairs& exchange_trading_pairs)
        : thread_pool_(thread_pool),
          bus_(bus),
          arbitrage_report_pool_(max_event_per_time),
          bus_event_arbitrage_report_pool_(max_event_per_time),
          exchange_trading_pairs_(exchange_trading_pairs) {}
    void AddArbitrageTrade(cross_exchange::ArbitrageTrade& trade) {
        cross_exchange::ArbitrageTradeHash hasher;
        // это торговая пара за которой я детектирую изменения
        auto trading_pair_context = trade.TradingPairContext();
        aot::TradingPairContextHash trading_pair_context_hasher;
    }

  private:
};
};  // namespace aot
