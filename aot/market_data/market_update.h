#pragma once

#include "aot/common/types.h"
#include "aot/Logger.h"
#include "moodycamel/concurrentqueue.h"


struct MEMarketUpdate {
    OrderId order_id_ = OrderId_INVALID;
    TickerId ticker_id_ = TickerId_INVALID;
    Side side_ = Side::INVALID;
    Price price_ = Price_INVALID;
    Qty qty_ = Qty_INVALID;
    Priority priority_ = Priority_INVALID;

    auto toString() const {
        return fmt::format("MEMarketUpdate[ticker:{} oid:{} side:{} qty:{} price:{}]",  tickerIdToString(ticker_id_), orderIdToString(order_id_),  sideToString(side_), qtyToString(qty_), priceToString(price_))
    }
  };

using MEMarketUpdateLFQueue = moodycamel::ConcurrentQueue<MEMarketUpdate>;
