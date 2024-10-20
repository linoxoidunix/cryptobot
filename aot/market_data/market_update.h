#pragma once

#include <limits>
#include <list>

#include "concurrentqueue.h"  //if link form source

#include "aot/Logger.h"
#include "aot/bus/bus_component.h"
#include "aot/bus/bus_event.h"
#include "aot/common/types.h"
#include "aot/common/mem_pool.h"

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

struct MEMarketUpdate {
    MarketUpdateType type    = MarketUpdateType::DEFAULT;

    common::OrderId order_id = common::kOrderIdInvalid;
    common::Side side        = common::Side::INVALID;
    common::Price price      = common::kPriceInvalid;
    common::Qty qty          = common::kQtyInvalid;

    auto ToString() const {
        return fmt::format(
            "MEMarketUpdate[oid:{} side:{} qty:{} price:{}]", /*ticker,*/
            common::orderIdToString(order_id), sideToString(side),
            common::qtyToString(qty), common::priceToString(price));
    };
    explicit MEMarketUpdate() = default;
};

using EventLFQueue = moodycamel::ConcurrentQueue<MEMarketUpdate>;

struct BookSnapshotElem {
    common::Price price = common::kPriceInvalid;
    common::Qty qty     = common::kQtyInvalid;
    BookSnapshotElem(common::Price _price, common::Qty _qty)
        : price(_price), qty(_qty) {};
    auto ToString() const {
        return fmt::format("BookSnapshotElem[price:{} qty:{}]", price, qty);
    };
};

struct BookSnapshot;
using BookSnapshotPool = common::MemoryPool<BookSnapshot>;

struct BookSnapshot {
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    std::list<BookSnapshotElem> bids;
    std::list<BookSnapshotElem> asks;
    uint64_t lastUpdateId = std::numeric_limits<uint64_t>::max();
    BookSnapshotPool* mem_pool = nullptr;
    void AddToQueue(EventLFQueue& queue) {
        std::vector<MEMarketUpdate> bulk;
        bulk.resize(bids.size());
        int i = 0;
        for (const auto& bid : bids) {
            MEMarketUpdate event;
            event.side  = common::Side::SELL;
            event.price = bid.price;
            event.qty   = bid.qty;
            bulk[i]     = event;
            i++;
        }
        bool status = false;
        while (!status) status = queue.try_enqueue_bulk(&bulk[0], bids.size());

        bulk.resize(asks.size());
        i = 0;
        for (const auto& ask : asks) {
            MEMarketUpdate event;
            event.side  = common::Side::BUY;
            event.price = ask.price;
            event.qty   = ask.qty;
            bulk[i]     = event;
            i++;
        }
        status = false;
        while (!status) status = queue.try_enqueue_bulk(&bulk[0], asks.size());
    }
    auto ToString() const {
        return fmt::format("BookSnapshot[lastUpdateId:{}]", lastUpdateId);
    };
    void Deallocate() {
        if(!mem_pool)
        {
            logw("mem_pool = nullptr. can.t deallocate BookSnapshot");
                return;
        }
        mem_pool->Deallocate(this);
    };
};

struct BusEventResponseNewSnapshot : public bus::Event {
    explicit BusEventResponseNewSnapshot(Exchange::BookSnapshot* _response) : response(_response){};
    BookSnapshot* response;
    ~BusEventResponseNewSnapshot() override = default;
    void Accept(bus::Component* comp) override {
        comp->AsyncHandleEvent(this);
    }
    void Deallocate() override{
        logd("Deallocating resources");
        response->Deallocate();
        response = nullptr;
    }
};


struct RequestSnapshot;
using RequestSnapshotPool = common::MemoryPool<RequestSnapshot>;

class RequestSnapshot {
  public:
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    unsigned int depth;
    RequestSnapshotPool* mem_pool = nullptr;
    virtual ~RequestSnapshot() = default;
    auto ToString() const {
        return fmt::format("RequestSnapshot[{}, {}, depth:{}]", exchange_id, trading_pair.ToString(), depth);
    }
    RequestSnapshot() = default;
    RequestSnapshot(common::ExchangeId _exchange_id,
                          common::TradingPair _trading_pair, unsigned int _depth = 1000) : exchange_id(_exchange_id), 
                          trading_pair(_trading_pair),
                          depth(_depth){}

    void Deallocate() {
        if(!mem_pool)
        {
            logw("mem_pool = nullptr. can.t deallocate MEClientResponse");
                return;
        }
        mem_pool->Deallocate(this);
    };
};

struct BusEventRequestNewSnapshot : public bus::Event {
    explicit BusEventRequestNewSnapshot(Exchange::RequestSnapshot* _request) : request(_request){};
    RequestSnapshot* request;
    ~BusEventRequestNewSnapshot() override = default;
    void Accept(bus::Component* comp, const OnHttpsResponce* cb) override {
        comp->AsyncHandleEvent(this, cb);
    }
    void Deallocate() override{
        logd("Deallocating resources");
        request->Deallocate();
        request = nullptr;
    }
};

struct BookDiffSnapshot {
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    std::list<BookSnapshotElem> bids;
    std::list<BookSnapshotElem> asks;
    uint64_t first_id = std::numeric_limits<uint64_t>::max();
    uint64_t last_id  = std::numeric_limits<uint64_t>::max();
    auto ToString() const {
        return fmt::format("BookDiffSnapshot[first_id:{} last_id:{}]", first_id,
                           last_id);
    };
    void AddToQueue(EventLFQueue& queue) {
        for (const auto& bid : bids) {
            MEMarketUpdate event;
            event.side     = common::Side::SELL;
            event.price    = bid.price;
            event.qty      = bid.qty;
            auto status_op = queue.try_enqueue(event);
            logi("{}", event.ToString());
            if (!status_op) [[unlikely]]
                loge("can't enqueue more elements. my lfqueue is busy");
        }
        for (const auto& ask : asks) {
            MEMarketUpdate event;
            event.side     = common::Side::BUY;
            event.price    = ask.price;
            event.qty      = ask.qty;
            auto status_op = queue.try_enqueue(event);
            logi("{}", event.ToString());
            if (!status_op) [[unlikely]]
                loge("can't enqueue more elements. my lfqueue is busy");
        }
    }
};

struct RequestDiffOrderBook;
using RequestDiffOrderBookPool = common::MemoryPool<RequestDiffOrderBook>;
struct RequestDiffOrderBook{
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    RequestDiffOrderBookPool* mem_pool = nullptr;
    common::FrequencyMS frequency = common::kFrequencyMSInvalid;
    void Deallocate() {
        if(!mem_pool)
        {
            logw("mem_pool = nullptr. can.t deallocate MEClientResponse");
                return;
        }
        mem_pool->Deallocate(this);
    };
};

struct BusEventRequestDiffOrderBook;
using BusEventRequestDiffOrderBookPool = common::MemoryPool<BusEventRequestDiffOrderBook>;
struct BusEventRequestDiffOrderBook: public bus::Event{
    RequestDiffOrderBook* request;
    ~BusEventRequestDiffOrderBook() override = default;
    void Accept(bus::Component* comp, const OnWssResponse* cb) override {
        comp->AsyncHandleEvent(this, cb);
    }
    void Deallocate() override{
        logd("Deallocating resources");
        request->Deallocate();
        request = nullptr;
    }
};


using BookDiffLFQueue = moodycamel::ConcurrentQueue<BookDiffSnapshot>;

};  // namespace Exchange

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