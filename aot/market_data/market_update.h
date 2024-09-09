#pragma once

#include <limits>
#include <list>

#include "aot/Logger.h"
#include "aot/common/types.h"

//#include "moodycamel/concurrentqueue.h"//if link as 3rd party
#include "concurrentqueue.h"//if link form source

namespace Exchange {
enum class MarketUpdateType : uint8_t {
    CLEAR   = 0,  // if this event occured then neen clear order book
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

struct MEMarketUpdateDouble;
struct MEMarketUpdate {
    MarketUpdateType type    = MarketUpdateType::DEFAULT;

    common::OrderId order_id = common::OrderId_INVALID;
    common::Side side   = common::Side::INVALID;
    common::Price price = common::kPriceInvalid;
    common::Qty qty     = common::kQtyInvalid;

    auto ToString() const {
        return fmt::format(
            "MEMarketUpdate[oid:{} side:{} qty:{} price:{}]", /*ticker,*/
            common::orderIdToString(order_id), sideToString(side),
            common::qtyToString(qty), common::priceToString(price));
    };
    explicit MEMarketUpdate(const MEMarketUpdateDouble*, uint precission_price,
                            uint precission_qty);
    explicit MEMarketUpdate() = default;
};

struct MEMarketUpdateDouble {
    MarketUpdateType type = MarketUpdateType::DEFAULT;

    common::Side side = common::Side::INVALID;
    double price      = std::numeric_limits<double>::max();
    double qty        = std::numeric_limits<double>::max();

    auto ToString() const {
        return fmt::format(
            "MEMarketUpdateDouble[type:{} side:{} qty:{} price:{}]",
            marketUpdateTypeToString(type), sideToString(side), qty,
            price);
    };
    explicit MEMarketUpdateDouble(const MEMarketUpdate*, uint precission_price,
                                  uint precission_qty);
    explicit MEMarketUpdateDouble() = default;
};

using EventLFQueue = moodycamel::ConcurrentQueue<MEMarketUpdate>;

struct BookSnapshotElem {
    common::Price price = common::kPriceInvalid;
    common::Qty qty = common::kQtyInvalid;
    BookSnapshotElem(common::Price _price, common::Qty _qty) : price(_price), qty(_qty) {};
    auto ToString() const {
        return fmt::format("BookSnapshotElem[price:{} qty:{}]", price, qty);
    };
};

struct BookSnapshot {
    std::string ticker;
    std::list<BookSnapshotElem> bids;
    std::list<BookSnapshotElem> asks;
    uint64_t lastUpdateId = std::numeric_limits<uint64_t>::max();
    void AddToQueue(EventLFQueue& queue) {
        std::vector<MEMarketUpdate> bulk;
        bulk.resize(bids.size());
        int i = 0;
        for (auto& bid : bids) {
            MEMarketUpdate event;
            //event.ticker = ticker;
            event.side   = common::Side::SELL;
            event.price  = bid.price;
            event.qty    = bid.qty;
            bulk[i]      = event;
            i++;
        }
        bool status = false;
        while (!status) status = queue.try_enqueue_bulk(&bulk[0], bids.size());

        bulk.resize(asks.size());
        i = 0;
        for (auto& ask : asks) {
            MEMarketUpdate event;
            //event.ticker = ticker;
            event.side   = common::Side::BUY;
            event.price  = ask.price;
            event.qty    = ask.qty;
            bulk[i]      = event;
            i++;
        }
        status = false;
        while (!status) status = queue.try_enqueue_bulk(&bulk[0], asks.size());
    }
    auto ToString() const {
        return fmt::format("BookSnapshot[lastUpdateId:{}]", lastUpdateId);
    };
};

struct BookDiffSnapshot {
    std::list<BookSnapshotElem> bids;
    std::list<BookSnapshotElem> asks;
    uint64_t first_id = std::numeric_limits<uint64_t>::max();
    uint64_t last_id  = std::numeric_limits<uint64_t>::max();
    auto ToString() const {
        return fmt::format("BookDiffSnapshot[first_id:{} last_id:{}]", first_id,
                           last_id);
    };
    void AddToQueue(EventLFQueue& queue) {
        for (auto& bid : bids) {
            MEMarketUpdate event;
            event.side     = common::Side::SELL;
            event.price    = bid.price;
            event.qty      = bid.qty;
            auto status_op = queue.try_enqueue(event);
            logi("{}", event.ToString());
            // if(!status_op)[[unlikely]]
            //     loge("can't enqueue more elements. my lfqueue is busy");
        }
        for (auto& ask : asks) {
            MEMarketUpdate event;
            event.side     = common::Side::BUY;
            event.price    = ask.price;
            event.qty      = ask.qty;
            auto status_op = queue.try_enqueue(event);
            logi("{}", event.ToString());
            // if(!status_op)[[unlikely]]
            //     loge("can't enqueue more elements. my lfqueue is busy");
        }
    }
};

using BookDiffLFQueue = moodycamel::ConcurrentQueue<BookDiffSnapshot>;

};  // namespace Exchange

// template <> struct fmt::formatter<Exchange::BookSnapshotElem>:
// formatter<std::string> {
//   // parse is inherited from formatter<string_view>.

//   auto format(const Exchange::BookSnapshotElem& c, format_context& ctx)
//   const;
// };

template <>
class fmt::formatter<Exchange::BookSnapshotElem> {
  public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(const Exchange::BookSnapshotElem& foo,
                          Context& ctx) const {
        return fmt::format_to(ctx.out(), "BookSnapshotElem[price:{} qty:{}]",
                              foo.price, foo.qty);
    }
};

template <>
class fmt::formatter<Exchange::BookSnapshot> {
  public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(const Exchange::BookSnapshot& foo,
                          Context& ctx) const {
        return fmt::format_to(ctx.out(),
                              "BookSnapshot[lastUpdateId:{} bids_length:{} "
                              "asks_length:{} bids:{} asks:{}]",
                              foo.lastUpdateId, foo.bids.size(),
                              foo.asks.size(), fmt::join(foo.bids, ";"),
                              fmt::join(foo.asks, ";"));
    }
};