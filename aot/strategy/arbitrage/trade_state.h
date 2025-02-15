#pragma once

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
};  // namespace aot