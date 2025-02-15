#pragma once

#include "aot/Logger.h"
#include "aot/common/exchange_trading_pair.h"
#include "aot/strategy/arbitrage/trade_state.h"
#include "aot/strategy/arbitrage/transaction_report.h"

namespace aot {
class PnlCalculator {
    ExchangeTradingPairs& exchange_trading_pairs_;

  public:
    PnlCalculator(ExchangeTradingPairs& exchange_trading_pairs)
        : exchange_trading_pairs_(exchange_trading_pairs) {}
    /**
     * @brief
     *
     * @param transaction
     * @return std::pair<bool,double> return true if success with given value or
     * return false
     */
    std::pair<bool, double> CalculatePnl(
        aot::TransactionReport& transaction) const {
        auto* pair_info = exchange_trading_pairs_.GetPairInfo(
            transaction.exchange_id, transaction.trading_pair);
        if (!pair_info) {
            logi("exchange_trading_pairs not contain {} {}",
                 transaction.exchange_id, transaction.trading_pair);
            return {false, 0.0};
        }

        auto min_qty = std::min(transaction.entry_qty, transaction.exit_qty);
        auto min_qty_double = pair_info->GetQtyDouble(min_qty);
        auto price_exit_double =
            pair_info->GetPriceDouble(transaction.exit_price);
        auto price_enter_double =
            pair_info->GetPriceDouble(transaction.entry_price);

        double pnl =
            (transaction.operation == aot::Operation::kSell)
                ? (price_enter_double - price_exit_double) * min_qty_double
                : (price_exit_double - price_enter_double) * min_qty_double;
        transaction.pnl = pnl;
        return {true, pnl};
    };
    std::pair<bool, double> CalculatePnl(aot::TradeState& trade_state) const {
        double total_pnl = 0.0;

        for (auto& [transaction_id, transaction] :
             trade_state.transaction_states) {
            auto [status, pnl] = CalculatePnl(transaction);
            if (!status) {
                logi("calculatePnl return false for transaction_id {}",
                     transaction_id);
                return {false, 0.0};  // Возвращаем false, если хотя бы одна
                                      // транзакция не прошла расчет PnL
            }
            total_pnl += pnl;  // Добавляем PnL для текущей транзакции к общему
        }

        return {true, total_pnl};
    };
};
};  // namespace aot