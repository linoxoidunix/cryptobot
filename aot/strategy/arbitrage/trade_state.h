#pragma once

#include <unordered_map>

#include "aot/strategy/arbitrage/transaction_report.h"

namespace aot {
struct TradeState {
    std::unordered_map<std::size_t, TransactionReport> transaction_states;
    bool is_open = false;
    bool IsOpened() const { return is_open; }
    double pnl = 0;
    common::Nanos entry_time;
    common::Nanos exit_time;
};

using TradesState = std::unordered_map<size_t, TradeState>;
};  // namespace aot