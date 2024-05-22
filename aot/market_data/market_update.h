#pragma once

#include <limits>
#include <list>

#include "aot/Logger.h"
#include "aot/common/types.h"
#include "moodycamel/concurrentqueue.h"
namespace Exchange {
    enum class MarketUpdateType : uint8_t {
    CLEAR = 0,//if this event occured then neen clear order book
    DEFAULT = 1
  };

  inline std::string marketUpdateTypeToString(MarketUpdateType type) {
    switch (type) {
      case MarketUpdateType::CLEAR:
        return "CLEAR";
      default:
        return "DEFAULT";
    }
    return "UNKNOWN";
  };
struct MEMarketUpdate {
    MarketUpdateType type_ = MarketUpdateType::DEFAULT;


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
        return fmt::format("BookSnapshotElem[price:{} qty:{}]", price, qty);
    };
};

struct BookSnapshot {
    std::list<BookSnapshotElem> bids;
    std::list<BookSnapshotElem> asks;
    uint64_t lastUpdateId = std::numeric_limits<uint64_t>::max();
};

struct BookDiffSnapshot {
    std::list<BookSnapshotElem> bids;
    std::list<BookSnapshotElem> asks;
    uint64_t first_id = std::numeric_limits<uint64_t>::max();
    uint64_t last_id  = std::numeric_limits<uint64_t>::max();
    auto ToString() const {
        return fmt::format("BookDiffSnapshot[first_id:{} last_id:{}]", first_id, last_id);
    };
};

//using NewBidLFQueue = moodycamel::ConcurrentQueue<MEMarketUpdate>;
//using NewAskLFQueue = moodycamel::ConcurrentQueue<MEMarketUpdate>;
using EventLFQueue = moodycamel::ConcurrentQueue<MEMarketUpdate>;
using BookDiffLFQueue = moodycamel::ConcurrentQueue<BookDiffSnapshot>;

};  // namespace Exchange
