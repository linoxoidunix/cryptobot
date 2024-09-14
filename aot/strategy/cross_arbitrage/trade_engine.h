#pragma once
#include <unordered_map>

#include "aot/strategy/cross_arbitrage/signals.h"
#include "aot/strategy/trade_engine.h"

namespace startegy {
namespace cross_arbitrage {
class TradeEngine : public Trading::TradeEngine {
    strategy::cross_arbitrage::LFQueue *lf_queue_;
    std::unordered_map<common::ExchangeId, common::TradingPair> &working_pairs_;
    std::list<common::ExchangeId> &exchanges_;

  public:
    TradeEngine(strategy::cross_arbitrage::LFQueue *lf_queue,
                std::unordered_map<common::ExchangeId, common::TradingPair>
                    &working_pairs,
                std::list<common::ExchangeId> &exchanges,
                common::TradingPairHashMap &pairs,
                base_strategy::Strategy *predictor)
        : Trading::TradeEngine(nullptr, nullptr, nullptr, nullptr, nullptr,
                               nullptr, {}, pairs, nullptr),
          lf_queue_(lf_queue),
          working_pairs_(working_pairs),
          exchanges_(exchanges) {};

  private:
    void Run() noexcept override {
        if (exchanges_.size() != 2) {
            logw(
                "can't launch cross arbitrage because number_trading_pairs:{} "
                "!= 2",
                exchanges_.size());
            return;
        }

        const auto &exch1 = *exchanges_.begin();
        const auto &exch2 = *exchanges_.rbegin();

        strategy::cross_arbitrage::Event *new_events[50];
        struct BBO {
            common::Price best_bid = common::kPriceInvalid;
            common::Qty qty_bid    = common::kQtyInvalid;
            common::Price best_ask = common::kPriceInvalid;
            common::Qty qty_ask    = common::kQtyInvalid;
        };

        std::unordered_map<common::ExchangeId, BBO> prices;

        // Implementation of cross-arbitrage trading logic goes here
        while (run_) {
            // Dequeue signals from the queue
            auto count = lf_queue_->try_dequeue_bulk(new_events, 50);
            // Process signals
            for (int i = 0; i < count; i++) {
                auto signal = new_events[i];
                auto type   = signal->GetType();
                if (type == strategy::cross_arbitrage::EventType::kBidUpdate) {
                    {
                        auto event = static_cast<
                            strategy::cross_arbitrage::BBidUpdated *>(signal);
                        prices[event->exchange].best_bid = event->price;
                        prices[event->exchange].qty_bid  = event->qty;
                    }
                } else if (type ==
                           strategy::cross_arbitrage::EventType::kAskUpdate) {
                    auto event =
                        static_cast<strategy::cross_arbitrage::BAskUpdated *>(
                            signal);
                    prices[event->exchange].best_ask = event->price;
                    prices[event->exchange].qty_ask  = event->qty;
                } else {
                    loge("Unknown signal type");
                }
                bool condition1 =
                    prices[exch2].best_bid == common::kPriceInvalid ||
                    prices[exch1].best_ask == common::kPriceInvalid;
                if (condition1)
                    if (prices[exch2].best_bid - prices[exch1].best_ask > 0) {
                        logi("buy on exch1 and sell on exch2");
                        continue;
                    }
                bool condition2 =
                    prices[exch1].best_bid == common::kPriceInvalid ||
                    prices[exch2].best_ask == common::kPriceInvalid;

                if (condition2)
                    if (prices[exch1].best_bid - prices[exch2].best_ask > 0) {
                        logi("buy on exch2 and sell on exch1");
                        continue;
                    }
                    if(!condition1 && !condition2){
                        logw("there aren't conditions for cross arbitrage");
                    }
            }
        }
    }
};
};  // namespace cross_arbitrage
};  // namespace startegy