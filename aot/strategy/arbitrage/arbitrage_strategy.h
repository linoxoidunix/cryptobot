#pragma once
#include <iostream>

#include "aot/Logger.h"
#include "aot/bus/bus.h"
#include "aot/bus/bus_component.h"
#include "aot/bus/bus_event.h"
#include "aot/common/mem_pool.h"
#include "aot/common/types.h"
#include "aot/strategy/arbitrage/arbitrage_cycle.h"
#include "aot/strategy/market_order.h"

namespace aot {
struct ArbitrageReport;
using ArbitrageReportPool = common::MemoryPool<ArbitrageReport>;
struct ArbitrageReport : public aot::Event<ArbitrageReportPool> {
    size_t uid_strategy;
    common::Nanos time_open;
    common::Nanos time_close;
    common::Qty position_open = common::kQtyInvalid;
    common::Price delta       = common::kPriceInvalid;
    common::Price buy_input   = common::kPriceInvalid;
    common::Price sell_input  = common::kPriceInvalid;
    common::Price buy_exit    = common::kPriceInvalid;
    common::Price sell_exit   = common::kPriceInvalid;

    ArbitrageReport() : aot::Event<ArbitrageReportPool>(nullptr) {};

    ArbitrageReport(ArbitrageReportPool* mem_pool, size_t _uid_strategy,
                    common::Nanos _time_open, common::Nanos _time_close,
                    common::Qty _position_open, common::Price _delta,
                    common::Price _buy_input, common::Price _sell_input,
                    common::Price _buy_exit, common::Price _sell_exit)
        : aot::Event<ArbitrageReportPool>(mem_pool),
          uid_strategy(_uid_strategy),
          time_open(_time_open),
          time_close(_time_close),
          position_open(_position_open),
          delta(_delta),
          buy_input(_buy_input),
          sell_input(_sell_input),
          buy_exit(_buy_exit),
          sell_exit(_sell_exit) {}
    friend void intrusive_ptr_release(ArbitrageReport* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(ArbitrageReport* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }
    void SerializeToJson(nlohmann::json& json) const {
        json = {
            {"arbitrage_report", {
                {"uid_strategy", uid_strategy},
                {"time_open", time_open},
                {"time_close", time_close},
                {"position_open", position_open},
                {"delta", delta},
                {"buy_input", buy_input},
                {"sell_input", sell_input},
                {"buy_exit", buy_exit},
                {"sell_exit", sell_exit}
            }}
        };
    };
};

struct BusEventArbitrageReport;
using BusEventArbitrageReportPool =
    common::MemoryPool<BusEventArbitrageReport>;
struct BusEventArbitrageReport
    : public bus::Event2<BusEventArbitrageReportPool> {
    explicit BusEventArbitrageReport(
        BusEventArbitrageReportPool* mem_pool,
        boost::intrusive_ptr<ArbitrageReport> request)
        : bus::Event2<BusEventArbitrageReportPool>(mem_pool),
          wrapped_event_(request) {};
    ~BusEventArbitrageReport() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }
    friend void intrusive_ptr_release(
        BusEventArbitrageReport* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(
        BusEventArbitrageReport* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }
    ArbitrageReport* WrappedEvent() {
        if (!wrapped_event_) return nullptr;
        return wrapped_event_.get();
    }
    boost::intrusive_ptr<ArbitrageReport> WrappedEventIntrusive() {
        return wrapped_event_;
    }
  private:
    boost::intrusive_ptr<ArbitrageReport> wrapped_event_;
};

template <typename ThreadPool>
class ArbitrageStrategyComponent : public bus::Component {
    ThreadPool& thread_pool_;
    aot::CoBus& bus_;
    using ExchangeBBOMap = std::unordered_map<
        size_t,
        Trading::BBO>;
    ExchangeBBOMap exchange_bbo_map_;
    using ExchangeTradingPairArbitrageMap =
        std::unordered_multimap<size_t,  // key is now the combined hash value
                                         // (size_t) of exchange id,
                                         // market_type, trading_apir
                                ArbitrageCycle>;
    ExchangeTradingPairArbitrageMap exchange_trading_pair_arbitrage_map_;
    static constexpr std::string_view name_component_ =
        "ArbitrageStrategyComponent";
    ArbitrageReportPool arbitrage_report_pool_;
    BusEventArbitrageReportPool bus_event_arbitrage_report_pool_;
  public:
    explicit ArbitrageStrategyComponent(ThreadPool& thread_pool, aot::CoBus& bus, size_t max_event_per_time)
        : thread_pool_(thread_pool),
        bus_(bus),
        arbitrage_report_pool_(max_event_per_time),
        bus_event_arbitrage_report_pool_(max_event_per_time) {}
    void AddArbitrageCycle(ArbitrageCycle& cycle) {
        ArbitrageCycleHash hasher;
        for (auto& step : cycle) {
            auto key = common::HashCombined(step.exchange_id, step.market_type,
                                            step.trading_pair);
            // Проверяем, существует ли уже такой ключ в мультимапе
            auto range = exchange_trading_pair_arbitrage_map_.equal_range(key);
            bool exists =
                std::any_of(range.first, range.second, [&](const auto& pair) {
                    return pair.second ==
                           cycle;  // Если нужно проверить одинаковость по
                                   // содержимому цикла
                });

            if (!exists) {
                auto inserted =
                    exchange_trading_pair_arbitrage_map_.insert({key, cycle});
                logi("add arbitrage cycle key:{} {} {} {} hash:{}", key,
                     step.exchange_id, step.market_type, step.trading_pair,
                     hasher(inserted->second));
            } else {
                logi(
                    "Arbitrage cycle with the same key already exists, "
                    "skipping: {}",
                    key);
            }
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
    void AddOrUpdateBBO(std::size_t key, Trading::BBO& bbo) {
        // Attempt to insert the key-value pair, or update the value if it already exists
        auto result = exchange_bbo_map_.try_emplace(key, bbo);
        if (!result.second) {
            // Key already exists; update the existing BBO
            result.first->second = bbo;
            logi("[{}] BBO updated for key: {}", 
                ArbitrageStrategyComponent<ThreadPool>::name_component_, key);
        } else {
            logi("[{}] BBO added for key: {}", 
                ArbitrageStrategyComponent<ThreadPool>::name_component_, key);
        }
    }

    void EvaluateOpportunityArbitrageCycle(ArbitrageCycle& cycle) {
        
        auto buy_price       = common::kPriceInvalid;
        auto exit_buy_price  = common::kPriceInvalid;
        auto sell_price      = common::kPriceInvalid;
        auto exit_sell_price = common::kPriceInvalid;

        auto bid_qty         = common::kQtyInvalid;
        auto ask_qty         = common::kQtyInvalid;
        for (auto& step : cycle) {
            auto key = common::HashCombined(step.exchange_id, step.market_type,
                                            step.trading_pair);
            if(!exchange_bbo_map_.contains(key)) {
                logw("[{}] exchange_bbo_map_ doesn't contain hashed key: {}. skip process step in arbitrage cycle", ArbitrageStrategyComponent<ThreadPool>::name_component_, key);
                continue; 
            }
            if (step.operation == aot::Operation::kBuy) {
                
                buy_price =
                    exchange_bbo_map_[key]
                        .ask_price;
                ask_qty = exchange_bbo_map_[key]
                              .ask_qty;
                exit_buy_price =
                    exchange_bbo_map_[key]
                        .bid_price;
            } else if (step.operation == aot::Operation::kSell) {
                sell_price =
                    exchange_bbo_map_[key]
                        .bid_price;
                bid_qty = exchange_bbo_map_[key]
                              .bid_qty;
                exit_sell_price =
                    exchange_bbo_map_[key]
                        .ask_price;
            }
        }
        if (buy_price == common::kPriceInvalid) return;
        if (sell_price == common::kPriceInvalid) return;
        if (bid_qty == common::kQtyInvalid) return;
        if (ask_qty == common::kQtyInvalid) return;

        if (sell_price > buy_price) {
            if (!cycle.IsTransactionOpen()) {
                cycle.StartTransaction();
                logi(
                    "Arbitrage opportunity detected with price_delta:{} "
                    "position:{}",
                    sell_price - buy_price, std::min(bid_qty, ask_qty));
                cycle.SetDeltaPrice(sell_price - buy_price);
                cycle.SetBuyInput(buy_price);
                cycle.SetSellInput(sell_price);

                cycle.SetPositionOpen(std::min(bid_qty, ask_qty));
            }
        } else {
            if (cycle.IsTransactionOpen()) {
                cycle.CloseTransaction();
                cycle.SetBuyExit(exit_buy_price);
                cycle.SetSellExit(exit_sell_price);
                logi("Arbitrage opportunity closed with delta_ns:{}",
                     cycle.GetDelta());
                SendArbitrageReportToBus(cycle);   
            }
        }
    }
    net::awaitable<void> HandleNewBBO(
        boost::intrusive_ptr<Trading::NewBBO> wrapped_event) {
        auto key = common::HashCombined(wrapped_event->exchange_id,
                            wrapped_event->market_type,
                            wrapped_event->trading_pair);
        AddOrUpdateBBO(key,
                       wrapped_event->bbo);
        // get access to list arbitrage cycle using exchange_id, trading_pair
        // необходимо выбрать только те пары, которые в которых присутсвуют
        // биржа и торговая пара


        if (!exchange_trading_pair_arbitrage_map_.contains(key)) co_return;
        auto range = exchange_trading_pair_arbitrage_map_.equal_range(key);

        // Перебираем все элементы в этом диапазоне
        for (auto it = range.first; it != range.second; ++it) {
            // Получаем арбитражный цикл из мультимапа
            auto& cycle = it->second;  // Для каждого арбитражного цикла

            // Теперь можно провести оценку арбитражной возможности для каждого
            // цикла
            EvaluateOpportunityArbitrageCycle(cycle);
        }
        co_return;
    }
    void SendArbitrageReportToBus(const ArbitrageCycle& cycle) {
        ArbitrageCycleHash hasher;
        auto event = arbitrage_report_pool_.Allocate(&arbitrage_report_pool_,
            hasher(cycle),
            cycle.time_open_,
            cycle.time_close_,
            cycle.position_open_,
            cycle.delta_,
            cycle.buy_input_,
            cycle.sell_input_,
            cycle.buy_exit_,
            cycle.sell_exit_
        );
        auto intr_ptr_request = boost::intrusive_ptr<ArbitrageReport>(event);
        auto bus_event        = bus_event_arbitrage_report_pool_.Allocate(
            &bus_event_arbitrage_report_pool_, intr_ptr_request);
         auto intr_ptr_bus_request =
           boost::intrusive_ptr<BusEventArbitrageReport>(bus_event);
        bus_.AsyncSend(this, intr_ptr_bus_request);
    }
};
};  // namespace aot