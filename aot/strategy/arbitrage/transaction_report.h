#pragma once
#include "aot/common/types.h"
#include "aot/strategy/arbitrage/arbitrage_step.h"

namespace aot {
struct TransactionReport {
    common::ExchangeId exchange_id;
    common::TradingPair trading_pair;
    aot::Operation operation;
    common::Price entry_price = common::kPriceInvalid;
    common::Price exit_price  = common::kPriceInvalid;
    common::Qty entry_qty     = common::kQtyInvalid;
    common::Qty exit_qty      = common::kQtyInvalid;
    common::Nanos entry_time;
    common::Nanos exit_time;
    double pnl = 0;
    TransactionReport(common::ExchangeId _exchange_id,
                      common::TradingPair _trading_pair,
                      aot::Operation _operation)
        : exchange_id(_exchange_id),
          trading_pair(_trading_pair),
          operation(_operation) {}
    TransactionReport() {};
};
};  // namespace aot