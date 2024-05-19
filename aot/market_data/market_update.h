#pragma once

#include "aot/Logger.h"
#include "aot/common/types.h"
// #include "moodycamel/concurrentqueue.h"
#include <limits>
#include <list>
//using namespace fmt;
namespace Exchange {
struct MEMarketUpdate {
    Common::OrderId order_id_   = Common::OrderId_INVALID;
    Common::TickerId ticker_id_ = Common::TickerId_INVALID;
    Common::Side side_          = Common::Side::INVALID;
    Common::Price price_        = Common::Price_INVALID;
    Common::Qty qty_            = Common::Qty_INVALID;
    // Priority priority_  = Priority_INVALID;

    auto toString() const {
        return fmt::format(
            "MEMarketUpdate[ticker:{} oid:{} side:{} qty:{} price:{}]",
            Common::tickerIdToString(ticker_id_),
            Common::orderIdToString(order_id_), sideToString(side_),
            Common::qtyToString(qty_), Common::priceToString(price_));
    };
};

struct BookSnapshotElem {
    double price = std::numeric_limits<double>::max();
    double qty   = std::numeric_limits<double>::max();
    BookSnapshotElem(double _price, double _qty) : price(_price), qty(_qty){};
     auto ToString() const {
        return fmt::format(
            "BookSnapshotElem[price:{} qty:{}]",
            price, qty);
    };
};

struct BookSnapshot {
    std::list<BookSnapshotElem> bids;
    std::list<BookSnapshotElem> asks;
};

// using MEMarketUpdateLFQueue = moodycamel::ConcurrentQueue<MEMarketUpdate>;
};  // namespace Exchange

