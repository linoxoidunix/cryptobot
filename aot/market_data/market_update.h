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

    Common::OrderId order_id = Common::OrderId_INVALID;
    // Common::TickerId ticker_id = Common::TickerId_INVALID;
    //std::string ticker;
    Common::Side side   = Common::Side::INVALID;
    Common::Price price = Common::Price_INVALID;
    Common::Qty qty     = Common::Qty_INVALID;

    auto ToString() const {
        return fmt::format(
            "MEMarketUpdate[oid:{} side:{} qty:{} price:{}]", /*ticker,*/
            Common::orderIdToString(order_id), sideToString(side),
            Common::qtyToString(qty), Common::priceToString(price));
    };
    explicit MEMarketUpdate(const MEMarketUpdateDouble*, uint precission_price,
                            uint precission_qty);
    explicit MEMarketUpdate() = default;
};

struct MEMarketUpdateDouble {
    MarketUpdateType type = MarketUpdateType::DEFAULT;

    // Common::TickerId ticker_id = Common::TickerId_INVALID;
    //std::string ticker;
    Common::Side side = Common::Side::INVALID;
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

using EventLFQueue = moodycamel::ConcurrentQueue<MEMarketUpdateDouble>;

struct BookSnapshotElem {
    double price = std::numeric_limits<double>::max();
    double qty   = std::numeric_limits<double>::max();
    BookSnapshotElem(double _price, double _qty) : price(_price), qty(_qty) {};
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
        std::vector<MEMarketUpdateDouble> bulk;
        bulk.resize(bids.size());
        int i = 0;
        for (auto& bid : bids) {
            MEMarketUpdateDouble event;
            //event.ticker = ticker;
            event.side   = Common::Side::SELL;
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
            MEMarketUpdateDouble event;
            //event.ticker = ticker;
            event.side   = Common::Side::BUY;
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
            MEMarketUpdateDouble event;
            event.side     = Common::Side::SELL;
            event.price    = bid.price;
            event.qty      = bid.qty;
            auto status_op = queue.try_enqueue(event);
            logi("{}", event.ToString());
            // if(!status_op)[[unlikely]]
            //     loge("can't enqueue more elements. my lfqueue is busy");
        }
        for (auto& ask : asks) {
            MEMarketUpdateDouble event;
            event.side     = Common::Side::BUY;
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