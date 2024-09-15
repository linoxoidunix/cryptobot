#pragma once
#include <array>
#include <unordered_map>

#include "aot/strategy/cross_arbitrage/signals.h"
#include "aot/strategy/trade_engine.h"
#include "aot/strategy/base_strategy.h"

// class Trading{
//     class BaseStrategy;
// }
namespace startegy {
namespace cross_arbitrage {
class TradeEngine : public Trading::TradeEngine {
    strategy::cross_arbitrage::LFQueue *lf_queue_ = nullptr;
    std::unordered_map<common::ExchangeId, common::TradingPair> &working_pairs_;
    std::list<common::ExchangeId>& exchanges_;
    strategy::cross_arbitrage::CrossArbitrage* ca_strategy_ = nullptr;
  public:
    TradeEngine(strategy::cross_arbitrage::LFQueue *lf_queue,
                std::unordered_map<common::ExchangeId, common::TradingPair>
                    &working_pairs,
                std::list<common::ExchangeId> &exchanges,
                common::TradingPairHashMap &pairs)
        : Trading::TradeEngine(nullptr, nullptr, nullptr, nullptr, nullptr,
                               nullptr, {}, pairs, nullptr),
          lf_queue_(lf_queue),
          working_pairs_(working_pairs),
          exchanges_(exchanges){};
    const Trading::PositionKeeper* PositionKeeper() const{return &position_keeper_;}
    void SetStrategy(strategy::cross_arbitrage::CrossArbitrage* strategy){ca_strategy_ = strategy;};

  private:
    void Run() noexcept override {
        if (exchanges_.size() != 2) {
            logw(
                "can't launch cross arbitrage because number_trading_pairs:{} "
                "!= 2",
                exchanges_.size());
            return;
        }
        std::array<strategy::cross_arbitrage::Event*, 50> new_events{};
        while (run_) {
            // Dequeue signals from the queue
            auto count = lf_queue_->try_dequeue_bulk(new_events.begin(), 50);
            // Process signals
            for (int i = 0; i < count; i++) {
                auto signal = new_events[i];
                ca_strategy_->OnNewSignal(signal);
            }
        }
    }
};
};  // namespace cross_arbitrage
};  // namespace startegy