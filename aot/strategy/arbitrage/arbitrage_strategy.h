
#pragma once
#include <iostream>

#include "aot/Logger.h"
#include "aot/bus/bus_component.h"
#include "aot/common/types.h"
#include "aot/strategy/arbitrage/arbitrage_cycle.h"
#include "aot/strategy/market_order.h"

namespace aot {
template <typename ThreadPool>
class ArbitrageStrategyComponent : public bus::Component {
    ThreadPool& thread_pool_;
    using ExchangeBBOMap = std::unordered_map<
        common::ExchangeId,
        std::unordered_map<common::TradingPair, Trading::BBO,
                           common::TradingPairHash, common::TradingPairEqual>>;
    ExchangeBBOMap exchange_bbo_map_;
    using ExchangeTradingPairArbitrageMap = std::unordered_map<
        common::ExchangeId,
        std::unordered_map<common::TradingPair,
                           std::unordered_set<ArbitrageCycle, ArbitrageCycleHash>,
                           common::TradingPairHash, common::TradingPairEqual>>;
    ExchangeTradingPairArbitrageMap exchange_trading_pair_arbitrage_map_;
    static constexpr std::string_view name_component_ =
        "ArbitrageStrategyComponent";

  public:
    explicit ArbitrageStrategyComponent(ThreadPool& thread_pool)
        : thread_pool_(thread_pool) {}
    void AddArbitrageCycle(ArbitrageCycle& cycle) {
        auto& map = cycle.GetExchangePairs();
        for (auto& [exchange, set_trading_pair] : map){
            auto& nested_map = exchange_trading_pair_arbitrage_map_[exchange];
            for(auto& trading_pair : set_trading_pair)
                nested_map[trading_pair].insert(cycle);
        }
    }
    void AsyncHandleEvent(
        boost::intrusive_ptr<Trading::BusEventNewBBO> event) override {
        if (!event) {
            logw("[{}] Received nullptr event in AsyncHandleEvent",
                 ArbitrageStrategyComponent<ThreadPool>::name_component_);
            return;
        }

        auto wrapped_event = event->WrappedEventIntrusive();
        if (!wrapped_event) {
            logw("[{}] Wrapped event is nullptr in AsyncHandleEvent",
                 ArbitrageStrategyComponent<ThreadPool>::name_component_);
            return;
        }

        boost::asio::co_spawn(
            thread_pool_,
            [this, wrapped_event]() -> boost::asio::awaitable<void> {
                co_await HandleNewBBO(wrapped_event);
            },
            boost::asio::detached);
    }

  private:
    void AddOrUpdateBBO(common::ExchangeId& exchange_id,
                        common::TradingPair& trading_pair, Trading::BBO& bbo) {
        // Find or create the inner map for the given exchange ID
        auto& inner_map =
            exchange_bbo_map_
                .try_emplace(
                    exchange_id,
                    std::unordered_map<common::TradingPair, Trading::BBO,
                                       common::TradingPairHash,
                                       common::TradingPairEqual>())
                .first->second;

        // Insert or update the BBO for the given trading pair
        auto result = inner_map.try_emplace(trading_pair, bbo);
        if (!result.second) {
            // If the trading pair already exists, update the BBO value
            result.first->second = bbo;
            logi("[{}] BBO updated for {} on {}",
                 ArbitrageStrategyComponent<ThreadPool>::name_component_,
                 trading_pair, exchange_id);
        } else {
            logi("[{}] BBO added for {} on {}",
                 ArbitrageStrategyComponent<ThreadPool>::name_component_,
                 trading_pair, exchange_id);
        }
    }

    void EvaluateOpportunityArbitrageCycle(const ArbitrageCycle& cycle) {
        auto buy_price  = common::kPriceInvalid;
        auto sell_price = common::kPriceInvalid;
        for (auto& step : cycle) {
            auto need_buy = step.operation;
            if (need_buy == aot::Operation::kBuy)
                buy_price =
                    exchange_bbo_map_[step.exchange_id][step.trading_pair]
                        .ask_price;
            else if (need_buy == aot::Operation::kSell)
                sell_price =
                    exchange_bbo_map_[step.exchange_id][step.trading_pair]
                        .bid_price;
        }
        if (buy_price == common::kPriceInvalid) return;
        if (sell_price == common::kPriceInvalid) return;
        if (sell_price > buy_price)
            logi("[{}] there is arbitrage opportunity with delta:{}",
                 ArbitrageStrategyComponent<ThreadPool>::name_component_,  sell_price-buy_price);
    }
    net::awaitable<void> HandleNewBBO(
        boost::intrusive_ptr<Trading::NewBBO> wrapped_event) {
        AddOrUpdateBBO(wrapped_event->exchange_id, wrapped_event->trading_pair,
                       wrapped_event->bbo);
        //get access to list arbitrage cycle using exchange_id, trading_pair
        //необходимо выбрать только те пары, которые в которых присутсвуют биржа и торговая пара
        if(!exchange_trading_pair_arbitrage_map_.contains(wrapped_event->exchange_id))
            co_return;
        auto& exchange_trading_pairs = exchange_trading_pair_arbitrage_map_[wrapped_event->exchange_id];
        if(!exchange_trading_pairs.contains(wrapped_event->trading_pair))
            co_return;
        std::cout << fmt::format("start evaluate opportunity for {} {}", wrapped_event->exchange_id, wrapped_event->trading_pair) << std::endl;
        for(const auto& cycle : exchange_trading_pairs[wrapped_event->trading_pair]){
            EvaluateOpportunityArbitrageCycle(cycle);
        }
        co_return;
    }
};
};  // namespace aot