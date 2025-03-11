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
#include "aot/strategy/arbitrage/trade_dictionary.h"
#include "aot/strategy/arbitrage/trade_state.h"
#include "aot/strategy/arbitrage/transaction_report.h"
#include "aot/strategy/market_order.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;
namespace aot {
/**
 * @brief this structure need only for transfer data where all key with uint64_t
 * as string because default react json parser not support not support size_t
 *
 */
class TransactionReportString {
    common::ExchangeId exchange_id_ = common::kExchangeIdInvalid;
    common::TradingPair trading_pair_;
    std::string entry_price_;
    std::string exit_price_;
    std::string entry_qty_;
    std::string exit_qty_;
    std::string entry_time_;
    std::string exit_time_;
    aot::ExchangeTradingPairs exchange_trading_pairs_;
    double pnl;

  public:
    TransactionReportString(const TransactionReport& ref,
                            aot::ExchangeTradingPairs& exchange_trading_pairs)
        : exchange_id_(ref.exchange_id),
          trading_pair_(ref.trading_pair),
          entry_time_(std::to_string(ref.entry_time)),
          exit_time_(std::to_string(ref.exit_time)),
          exchange_trading_pairs_(exchange_trading_pairs),
          pnl(ref.pnl) {
        auto* info =
            exchange_trading_pairs.GetPairInfo(exchange_id_, trading_pair_);
        if (!info) {
            logw("No info for {} {}", exchange_id_, trading_pair_);
            return;
        }
        entry_price_ = std::to_string(info->GetPriceDouble(ref.entry_price));
        exit_price_  = std::to_string(info->GetPriceDouble(ref.exit_price));
        entry_qty_   = std::to_string(info->GetQtyDouble(ref.entry_qty));
        exit_qty_    = std::to_string(info->GetQtyDouble(ref.exit_qty));
    }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TransactionReportString, entry_price_,
                                   exit_price_, entry_qty_, exit_qty_, pnl,
                                   entry_time_, exit_time_)
};

struct ArbitrageReport;
using TransactionHash     = size_t;
using ArbitrageReportPool = common::MemoryPool<ArbitrageReport>;
struct ArbitrageReport : public aot::Event<ArbitrageReportPool> {
    size_t uid_trade;
    common::Nanos time_open;
    common::Nanos time_close;
    double pnl;
    std::unordered_map<TransactionHash, TransactionReport> transactions;

    ArbitrageReport() : aot::Event<ArbitrageReportPool>(nullptr) {};

    ArbitrageReport(ArbitrageReportPool* mem_pool, size_t _uid_trade)
        : aot::Event<ArbitrageReportPool>(mem_pool), uid_trade(_uid_trade) {}
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
    // void SerializeToJson(nlohmann::json& j) const {
    //     j = nlohmann::json{{"trade",
    //                         {{"uid_trade", std::to_string(uid_trade)},
    //                          {"pnl", std::to_string(pnl)},
    //                          {"time_open", std::to_string(time_open)},
    //                          {"time_close", std::to_string(time_close)},
    //                          {"transactions", transactions}}}};
    // };
};

struct ArbitrageReportString {
    std::string uid_trade;
    std::string time_open;
    std::string time_close;
    double pnl;
    std::unordered_map<TransactionHash, TransactionReportString> transactions;
    aot::ExchangeTradingPairs& exchange_trading_pairs;
    explicit ArbitrageReportString(
        const ArbitrageReport& report,
        aot::ExchangeTradingPairs& _exchange_trading_pairs)
        : uid_trade(std::to_string(report.uid_trade)),
          time_open(std::to_string(report.time_open)),
          time_close(std::to_string(report.time_close)),
          pnl(report.pnl),
          exchange_trading_pairs(_exchange_trading_pairs) {
        for (const auto& [hash, transaction] : report.transactions) {
            transactions.emplace(
                hash,
                TransactionReportString(transaction, exchange_trading_pairs));
        }
    }

    void SerializeToJson(nlohmann::json& j) const {
        j = nlohmann::json{{"trade",
                            {{"uid_trade", uid_trade},
                             {"pnl", pnl},
                             {"time_open", time_open},
                             {"time_close", time_close},
                             {"transactions", transactions}}}};
    }
};

struct BusEventArbitrageReport;
using BusEventArbitrageReportPool = common::MemoryPool<BusEventArbitrageReport>;
struct BusEventArbitrageReport
    : public bus::Event2<BusEventArbitrageReportPool> {
    explicit BusEventArbitrageReport(
        BusEventArbitrageReportPool* mem_pool,
        boost::intrusive_ptr<ArbitrageReport> request)
        : bus::Event2<BusEventArbitrageReportPool>(mem_pool),
          wrapped_event_(request) {};
    ~BusEventArbitrageReport() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }
    friend void intrusive_ptr_release(BusEventArbitrageReport* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(BusEventArbitrageReport* ptr) {
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

// key is arbitrage id

template <typename ThreadPool>
class ArbitrageStrategyComponent : public bus::Component {
    ThreadPool& thread_pool_;
    aot::CoBus& bus_;
    Trading::ExchangeBBOMap exchange_bbo_map_;
    TradeDictionary exchange_trading_pair_arbitrage_map_;
    static constexpr std::string_view name_component_ =
        "ArbitrageStrategyComponent";
    ArbitrageReportPool arbitrage_report_pool_;
    BusEventArbitrageReportPool bus_event_arbitrage_report_pool_;
    // using TradesState = std::unordered_map<size_t, TradeState>;
    aot::TradesState trades_state_;
    ExchangeTradingPairs& exchange_trading_pairs_;

  public:
    explicit ArbitrageStrategyComponent(
        ThreadPool& thread_pool, aot::CoBus& bus, size_t max_event_per_time,
        ExchangeTradingPairs& exchange_trading_pairs)
        : thread_pool_(thread_pool),
          bus_(bus),
          arbitrage_report_pool_(max_event_per_time),
          bus_event_arbitrage_report_pool_(max_event_per_time),
          exchange_trading_pairs_(exchange_trading_pairs) {}
    void AddArbitrageCycle(ArbitrageCycle& cycle) {
        ArbitrageCycleHash hasher;
        StepHash hasher_step;
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
        // Create 1 Trade state for unique trade
        auto uid_trade = hasher(cycle);
        if (trades_state_.contains(uid_trade)) return;
        trades_state_[uid_trade] = {};
        auto& trade_state        = trades_state_[uid_trade];
        // need create transaction state for new trade state
        StepHash step_hasher;
        for (auto& step : cycle) {
            trade_state.transaction_states[step_hasher(step)] = {
                step.exchange_id, step.trading_pair, step.operation};
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
    const TradeDictionary& GetTradeDictionary() const {
        return exchange_trading_pair_arbitrage_map_;
    }

  private:
    void AddOrUpdateBBO(std::size_t key, Trading::BBO& bbo) {
        // Attempt to insert the key-value pair, or update the value if it
        // already exists
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
        StepHash transaction_hasher;
        auto open_buy_price  = common::kPriceInvalid;
        auto exit_buy_price  = common::kPriceInvalid;
        auto open_sell_price = common::kPriceInvalid;
        auto exit_sell_price = common::kPriceInvalid;

        auto open_buy_qty    = common::kQtyInvalid;
        auto exit_buy_qty    = common::kQtyInvalid;

        auto open_sell_qty   = common::kQtyInvalid;
        auto exit_sell_qty   = common::kQtyInvalid;

        size_t transaction_buy;
        size_t transaction_sell;

        for (auto& step : cycle) {
            auto key = common::HashCombined(step.exchange_id, step.market_type,
                                            step.trading_pair);
            if (!exchange_bbo_map_.contains(key)) {
                // The BBO for this exchange, trading pair, market_type has not
                // yet been received from the OrderBook, so we skip the
                // arbitrage opportunity evaluation at this moment.
                logw(
                    "[{}] exchange_bbo_map_ doesn't contain hashed key: "
                    "{}. "
                    "skip process step in arbitrage cycle",
                    ArbitrageStrategyComponent<ThreadPool>::name_component_,
                    key);
                return;
            }
            if (step.operation == aot::Operation::kBuy) {
                open_buy_price  = exchange_bbo_map_[key].ask_price;
                open_buy_qty    = exchange_bbo_map_[key].ask_qty;
                exit_buy_price  = exchange_bbo_map_[key].bid_price;
                exit_buy_qty    = exchange_bbo_map_[key].bid_qty;
                transaction_buy = transaction_hasher(step);
            } else if (step.operation == aot::Operation::kSell) {
                open_sell_price  = exchange_bbo_map_[key].bid_price;
                open_sell_qty    = exchange_bbo_map_[key].bid_qty;
                exit_sell_price  = exchange_bbo_map_[key].ask_price;
                exit_sell_qty    = exchange_bbo_map_[key].ask_qty;
                transaction_sell = transaction_hasher(step);
            }
        }
        if (open_buy_price == common::kPriceInvalid) return;
        if (open_sell_price == common::kPriceInvalid) return;
        if (exit_buy_price == common::kPriceInvalid) return;
        if (exit_sell_price == common::kPriceInvalid) return;
        if (open_buy_qty == common::kQtyInvalid) return;
        if (exit_buy_qty == common::kQtyInvalid) return;
        if (open_sell_qty == common::kQtyInvalid) return;
        if (exit_sell_qty == common::kQtyInvalid) return;

        ArbitrageCycleHash hasher_trade;
        auto uid_trade = hasher_trade(cycle);
        if (!trades_state_.contains(uid_trade)) return;
        auto& trade_state = trades_state_[uid_trade];
        if (!trade_state.transaction_states.contains(transaction_buy)) return;
        if (!trade_state.transaction_states.contains(transaction_sell)) return;
        if (!trade_state.IsOpened()) {
            // check that can open
            if (open_sell_price > open_buy_price) {
                trade_state.is_open = true;
                trade_state.transaction_states[transaction_buy].entry_price =
                    open_buy_price;
                trade_state.transaction_states[transaction_buy].entry_qty =
                    open_buy_qty;
                trade_state.transaction_states[transaction_buy].entry_time =
                    common::getCurrentNanoS();

                trade_state.transaction_states[transaction_sell].entry_price =
                    open_sell_price;
                trade_state.transaction_states[transaction_sell].entry_qty =
                    open_sell_qty;
                trade_state.transaction_states[transaction_sell].entry_time =
                    common::getCurrentNanoS();
                trade_state.entry_time = std::min(
                    trade_state.transaction_states[transaction_buy].entry_time,
                    trade_state.transaction_states[transaction_sell]
                        .entry_time);
            }
        }
        if (trade_state.IsOpened()) {
            // Проверяем условия закрытия позиции
            auto buy_entry_price =
                trade_state.transaction_states[transaction_buy].entry_price;
            auto sell_entry_price =
                trade_state.transaction_states[transaction_sell].entry_price;

            // Приводим цены к int64_t для правильного вычисления разницы
            int64_t exit_sell_price_signed =
                static_cast<int64_t>(exit_sell_price);
            int64_t sell_entry_price_signed =
                static_cast<int64_t>(sell_entry_price);
            int64_t exit_buy_price_signed =
                static_cast<int64_t>(exit_buy_price);
            int64_t buy_entry_price_signed =
                static_cast<int64_t>(buy_entry_price);

            // Вычисляем прибыль/убыток для Take Profit и Stop Loss
            int64_t take_profit_condition =
                -exit_sell_price_signed + sell_entry_price_signed +
                exit_buy_price_signed - buy_entry_price_signed;

            int64_t stop_loss_condition =
                -exit_sell_price_signed + sell_entry_price_signed +
                exit_buy_price_signed - buy_entry_price_signed;

            // Определение порога для прибыли и убытка в процентах
            // const double take_profit_threshold_percent = 1.0;  // 1% для
            // прибыли const double stop_loss_threshold_percent = -10.0;  // -1%
            // для убытка

            const double take_profit_threshold_percent = 0;  // 1% для прибыли
            const double stop_loss_threshold_percent   = 0;  // -1% для убытка

            // Вычисляем разницу между ценами покупки и продажи
            double profit_or_loss_buy = static_cast<double>(exit_buy_price) -
                                        static_cast<double>(buy_entry_price);
            double profit_or_loss_sell = static_cast<double>(sell_entry_price) -
                                         static_cast<double>(exit_sell_price);

            // Вычисляем процентное изменение
            double profit_or_loss_percent =
                (profit_or_loss_buy + profit_or_loss_sell) /
                static_cast<double>(buy_entry_price) * 100.0;
            if (profit_or_loss_percent > 0) {
                auto x = profit_or_loss_percent;
            }
            // Определяем условия Take Profit и Stop Loss
            bool take_profit =
                (profit_or_loss_percent >= take_profit_threshold_percent);
            bool stop_loss =
                (profit_or_loss_percent <= stop_loss_threshold_percent);

            if (take_profit) {
                logi(
                    "Fix profit with {} percent {} thr {}: exit_sell_price {} "
                    "< sell_entry_price "
                    "{} && "
                    "exit_buy_price {} > buy_entry_price {}",
                    take_profit_condition, profit_or_loss_percent,
                    take_profit_threshold_percent, exit_sell_price,
                    sell_entry_price, exit_buy_price, buy_entry_price);
            } else if (stop_loss) {
                logi(
                    "Fix loss with {} percent {} thr {}: exit_sell_price {} > "
                    "sell_entry_price "
                    "{} || "
                    "exit_buy_price {} < buy_entry_price {}",
                    stop_loss_condition, profit_or_loss_percent,
                    stop_loss_threshold_percent, exit_sell_price,
                    sell_entry_price, exit_buy_price, buy_entry_price);
            }
            trade_state.is_open = false;

            if (stop_loss) {
                return;
            }
            if (take_profit) {
                trade_state.transaction_states[transaction_buy].exit_price =
                    exit_buy_price;
                trade_state.transaction_states[transaction_buy].exit_qty =
                    exit_buy_qty;
                trade_state.transaction_states[transaction_buy].exit_time =
                    common::getCurrentNanoS();

                trade_state.transaction_states[transaction_sell].exit_price =
                    exit_sell_price;
                trade_state.transaction_states[transaction_sell].exit_qty =
                    exit_sell_qty;
                trade_state.transaction_states[transaction_sell].exit_time =
                    common::getCurrentNanoS();

                logi(
                    "Closed arbitrage transaction: buy at {}, sell at {}, "
                    "exit_sell_price: {}",
                    buy_entry_price, sell_entry_price, exit_sell_price);

                trade_state.exit_time = std::max(
                    trade_state.transaction_states[transaction_buy].exit_time,
                    trade_state.transaction_states[transaction_sell].exit_time);

                PnlCalculator pnl_calculator(exchange_trading_pairs_);
                auto [status, pnl] = pnl_calculator.CalculatePnl(trade_state);
                if (!status) {
                    return;
                }
                trade_state.pnl = pnl;

                // Отправляем отчёт об арбитраже
                SendArbitrageReportToBus(uid_trade);
            }
        }
    }

    boost::asio::awaitable<void> HandleNewBBO(
        boost::intrusive_ptr<Trading::NewBBO> wrapped_event) {
        auto key = common::HashCombined(wrapped_event->exchange_id,
                                        wrapped_event->market_type,
                                        wrapped_event->trading_pair);
        AddOrUpdateBBO(key, wrapped_event->bbo);
        // get access to list arbitrage cycle using exchange_id,
        // trading_pair необходимо выбрать только те пары, которые в которых
        // присутсвуют биржа и торговая пара

        if (!exchange_trading_pair_arbitrage_map_.contains(key)) co_return;
        auto range = exchange_trading_pair_arbitrage_map_.equal_range(key);

        // Перебираем все элементы в этом диапазоне
        for (auto it = range.first; it != range.second; ++it) {
            // Получаем арбитражный цикл из мультимапа
            auto& cycle = it->second;  // Для каждого арбитражного цикла

            // Теперь можно провести оценку арбитражной возможности для
            // каждого цикла
            EvaluateOpportunityArbitrageCycle(cycle);
        }
        co_return;
    }
    void SendArbitrageReportToBus(const size_t uid_trade) {
        ArbitrageCycleHash hasher;
        auto event =
            arbitrage_report_pool_.Allocate(&arbitrage_report_pool_, uid_trade);
        event->transactions   = trades_state_[uid_trade].transaction_states;
        event->time_open      = trades_state_[uid_trade].entry_time;
        event->time_close     = trades_state_[uid_trade].exit_time;
        event->pnl            = trades_state_[uid_trade].pnl;
        auto intr_ptr_request = boost::intrusive_ptr<ArbitrageReport>(event);
        auto bus_event        = bus_event_arbitrage_report_pool_.Allocate(
            &bus_event_arbitrage_report_pool_, intr_ptr_request);
        auto intr_ptr_bus_request =
            boost::intrusive_ptr<BusEventArbitrageReport>(bus_event);
        bus_.AsyncSend(this, intr_ptr_bus_request);
    }
};
};  // namespace aot
