#include <deque>
#include <unordered_map>

#include "aot/strategy/arbitrage/arbitrage_cycle.h"
#include "aot/strategy/arbitrage/arbitrage_step.h"
#include "aot/strategy/arbitrage/trade_state.h"
#include "aot/strategy/market_order.h"

namespace aot {
class SpreadMonitor {
    static constexpr std::string_view name_component_ = "SpreadMonitor";
    using StrategyKey                                 = size_t;
    using SpreadHistory                               = std::deque<double>;
    std::unordered_map<StrategyKey, SpreadHistory>
        spread_history_;  // История спредов
    size_t time_window_;  // Окно для расчета среднего спреда (в секундах)
    Trading::ExchangeBBOMap& exchange_bbo_map_;
    aot::TradesState& trades_state_;

  public:
    SpreadMonitor(Trading::ExchangeBBOMap& exchange_bbo_map,
                  aot::TradesState& trade_states)
        : exchange_bbo_map_(exchange_bbo_map),
          trades_state_(trade_states),
          time_window_(10) {
        // TO DO INIT spread_history_ with empty SpreadHistory
    }

    // Получаем текущий спред между двумя биржами

    // Добавляем новый спред в историю для данной стратегии
    void AddSpreadToHistory(StrategyKey strategy_id, double spread) {
        // Проверяем, существует ли уже ключ для стратегии
        if (spread_history_.find(strategy_id) == spread_history_.end()) {
            spread_history_[strategy_id] =
                SpreadHistory();  // Инициализация истории
        }

        // Добавляем спред в историю
        spread_history_[strategy_id].push_back(spread);

        // Если история превышает окно времени, удаляем старые значения
        if (spread_history_[strategy_id].size() > time_window_) {
            spread_history_[strategy_id].pop_front();
        }
    }

    // Рассчитываем среднее значение спреда за заданное окно времени
    double CalculateAverageSpread(StrategyKey strategy_id) {
        // Проверяем, существует ли ключ для стратегии
        if (spread_history_.find(strategy_id) == spread_history_.end()) {
            return 0.0;  // Если нет истории, возвращаем 0
        }

        const auto& history = spread_history_[strategy_id];
        double sum          = 0;
        for (double spread : history) {
            sum += spread;
        }
        return history.empty() ? 0.0 : sum / history.size();
    }

    // Рассчитываем процентное отклонение спреда от среднего значения
    double CalculatePercentageDeviation(StrategyKey strategy_id,
                                        double current_spread) {
        double average_spread = CalculateAverageSpread(strategy_id);
        if (average_spread == 0) return 0.0;  // Избегаем деления на 0
        return ((current_spread - average_spread) / average_spread) * 100.0;
    }

    // Проверка на процентное отклонение
    bool IsSpreadOutOfBounds(StrategyKey strategy_id, double spread,
                             double threshold) {
        double percentage_deviation =
            CalculatePercentageDeviation(strategy_id, spread);
        return std::abs(percentage_deviation) > threshold;
    }

    std::pair<bool, double> GetCurrentSpread(aot::ArbitrageCycle& cycle) {
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
                    SpreadMonitor::name_component_, key);
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

        // If any of the prices or quantities are invalid, return false
        if (open_buy_price == common::kPriceInvalid ||
            open_sell_price == common::kPriceInvalid ||
            exit_buy_price == common::kPriceInvalid ||
            exit_sell_price == common::kPriceInvalid ||
            open_buy_qty == common::kQtyInvalid ||
            exit_buy_qty == common::kQtyInvalid ||
            open_sell_qty == common::kQtyInvalid ||
            exit_sell_qty == common::kQtyInvalid) {
            return {false, 0.0};
        }

        // Calculate the spread between the exchanges
        double spread =
            open_sell_price - open_buy_price;  // Spread between buy on first
                                               // exchange and sell on second

        return {true, spread};
    }

    // // Логика торговли при отклонении спреда
    void EvaluateOpportunityArbitrageCycle(aot::ArbitrageCycle& cycle) {
        auto [status, spread] = GetCurrentSpread(cycle);
        if (!status) return;
        ArbitrageCycleHash hasher_trade;
        AddSpreadToHistory(hasher_trade(cycle), spread);

        // std::cout << "Current spread: " << spread
        //           << ", Average spread: " << CalculateAverageSpread()
        //           << ", Percentage deviation: "
        //           << CalculatePercentageDeviation(spread) << "%" <<
        //           std::endl;

        // if (IsSpreadOutOfBounds(spread, threshold)) {
        //     if (spread > 0) {
        //         std::cout << "Spread is too high, selling on first exchange "
        //                      "and buying on second exchange"
        //                   << std::endl;
        //         // Логика продажи на первой бирже и покупки на второй
        //     } else {
        //         std::cout << "Spread is too low, buying on first exchange and
        //         "
        //                      "selling on second exchange"
        //                   << std::endl;
        //         // Логика покупки на первой бирже и продажи на второй
        //     }
        // }
    }
};
};  // namespace aot
