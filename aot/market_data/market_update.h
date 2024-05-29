#pragma once

#include <limits>
#include <list>

#include "aot/Logger.h"
#include "aot/common/types.h"
#include "moodycamel/concurrentqueue.h"
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
    MarketUpdateType type      = MarketUpdateType::DEFAULT;

    Common::OrderId order_id   = Common::OrderId_INVALID;
    Common::TickerId ticker_id = Common::TickerId_INVALID;
    Common::Side side          = Common::Side::INVALID;
    Common::Price price        = Common::Price_INVALID;
    Common::Qty qty            = Common::Qty_INVALID;

    auto ToString() const {
        return fmt::format(
            "MEMarketUpdate[ticker:{} oid:{} side:{} qty:{} price:{}]",
            Common::tickerIdToString(ticker_id),
            Common::orderIdToString(order_id), sideToString(side),
            Common::qtyToString(qty), Common::priceToString(price));
    };
    explicit MEMarketUpdate(const MEMarketUpdateDouble*, uint precission_price,
                            uint precission_qty);
    explicit MEMarketUpdate() = default;
};

struct MEMarketUpdateDouble {
    MarketUpdateType type      = MarketUpdateType::DEFAULT;

    Common::TickerId ticker_id = Common::TickerId_INVALID;
    Common::Side side          = Common::Side::INVALID;
    double price               = std::numeric_limits<double>::max();
    double qty                 = std::numeric_limits<double>::max();

    auto ToString() const {
        return fmt::format(
            "MEMarketUpdateDouble[ticker:{} type:{} side:{} qty:{} price:{}]",
            Common::tickerIdToString(ticker_id), marketUpdateTypeToString(type),
            sideToString(side), qty, price);
    };
    explicit MEMarketUpdateDouble(const MEMarketUpdate*, uint precission_price,
                                  uint precission_qty);
    explicit MEMarketUpdateDouble() = default;
};

using EventLFQueue = moodycamel::ConcurrentQueue<MEMarketUpdateDouble>;

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
    void AddToQueue(EventLFQueue& queue) {
        for (auto& bid : bids) {
            MEMarketUpdateDouble event;
            event.side  = Common::Side::SELL;
            event.price = bid.price;
            event.qty   = bid.qty;
            queue.enqueue(event);
        }
        for (auto& ask : asks) {
            MEMarketUpdateDouble event;
            event.side  = Common::Side::BUY;
            event.price = ask.price;
            event.qty   = ask.qty;
            queue.enqueue(event);
        }
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
            event.side  = Common::Side::SELL;
            event.price = bid.price;
            event.qty   = bid.qty;
            queue.enqueue(event);
        }
        for (auto& ask : asks) {
            MEMarketUpdateDouble event;
            event.side  = Common::Side::BUY;
            event.price = ask.price;
            event.qty   = ask.qty;
            queue.enqueue(event);
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