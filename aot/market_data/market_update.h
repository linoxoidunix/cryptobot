#pragma once

#include <limits>
#include <list>

#include "aot/Logger.h"
#include "aot/bus/bus_component.h"
#include "aot/bus/bus_event.h"
#include "aot/common/mem_pool.h"
#include "aot/common/types.h"
#include "aot/event/general_event.h"
#include "concurrentqueue.h"  //if link form source

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

struct BookSnapshot2;
using BookSnapshot2Pool = common::MemoryPool<BookSnapshot2>;

struct BookSnapshot2 : public aot::Event<BookSnapshot2Pool> {
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    std::list<BookSnapshotElem> bids;
    std::list<BookSnapshotElem> asks;
    uint64_t lastUpdateId = std::numeric_limits<uint64_t>::max();
    BookSnapshot2() : aot::Event<BookSnapshot2Pool>(nullptr) {};

    BookSnapshot2(BookSnapshot2Pool* mem_pool, common::ExchangeId _exchange_id,
                  common::TradingPair _trading_pair,
                  std::list<BookSnapshotElem>&& _bids,
                  std::list<BookSnapshotElem>&& _asks, uint64_t _lastUpdateId)
        : aot::Event<BookSnapshot2Pool>(mem_pool),
          exchange_id(_exchange_id),
          trading_pair(_trading_pair),
          bids(std::move(_bids)),
          asks(std::move(_asks)),
          lastUpdateId(_lastUpdateId) {};
    void Release() override {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (memory_pool_) {
                memory_pool_->Deallocate(this);  // Return to the pool
            }
        }
    }

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
};

struct BookSnapshot;
using BookSnapshotPool = common::MemoryPool<BookSnapshot>;

struct BookSnapshot {
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    std::list<BookSnapshotElem> bids;
    std::list<BookSnapshotElem> asks;
    uint64_t lastUpdateId = std::numeric_limits<uint64_t>::max();

    BookSnapshot()        = default;
    Exchange::BookSnapshot& operator=(Exchange::BookSnapshot2&& element);
    // {
    //     if(element == nullptr){
    //         loge("element = nullptr");
    //         return *this;
    //     }
    //     exchange_id = std::move(element->exchange_id);
    //     trading_pair = std::move(element->trading_pair);
    //     bids         = std::move(element->bids);
    //     asks         = std::move(element->asks);
    //     lastUpdateId = std::move(element->lastUpdateId);
    // }

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
};

struct BusEventResponseNewSnapshot;
using BusEventResponseNewSnapshotPool =
    common::MemoryPool<BusEventResponseNewSnapshot>;

struct BusEventResponseNewSnapshot
    : public bus::Event2<BusEventResponseNewSnapshotPool, BookSnapshot2Pool> {
    explicit BusEventResponseNewSnapshot(
        BusEventResponseNewSnapshotPool* mem_pool,
        Exchange::BookSnapshot2* _response)
        : bus::Event2<BusEventResponseNewSnapshotPool, BookSnapshot2Pool>(
              mem_pool, _response),
          wrapped_event_(_response) {};
    ~BusEventResponseNewSnapshot() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }
    void Release() override {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (memory_pool_) {
                memory_pool_->Deallocate(this);  // Return BusEvent to pool
            }
        }
        // if (wrapped_event_) {
        //     wrapped_event_->Release();  // Decrement reference count for
        //     Event
        // }
    };
    Exchange::BookSnapshot2* WrappedEvent() { return wrapped_event_; }

  private:
    Exchange::BookSnapshot2* wrapped_event_ = nullptr;
};

struct RequestSnapshot;
using RequestSnapshotPool = common::MemoryPool<RequestSnapshot>;

class RequestSnapshot : public aot::Event<RequestSnapshotPool> {
  public:
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    unsigned int depth;
    virtual ~RequestSnapshot() = default;
    auto ToString() const {
        return fmt::format("RequestSnapshot[{}, {}, depth:{}]", exchange_id,
                           trading_pair.ToString(), depth);
    }
    void Release() override {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (memory_pool_) {
                memory_pool_->Deallocate(this);  // Return to the pool
            }
        }
    }
    RequestSnapshot() : aot::Event<RequestSnapshotPool>(nullptr) {};
    RequestSnapshot(RequestSnapshotPool* mem_pool,
                    common::ExchangeId _exchange_id,
                    common::TradingPair _trading_pair,
                    unsigned int _depth = 1000)
        : aot::Event<RequestSnapshotPool>(mem_pool),
          exchange_id(_exchange_id),
          trading_pair(_trading_pair),
          depth(_depth) {}
};

struct BusEventRequestNewSnapshot;
using BusEventRequestNewSnapshotPool =
    common::MemoryPool<BusEventRequestNewSnapshot>;

struct BusEventRequestNewSnapshot
    : public bus::Event2<BusEventRequestNewSnapshotPool, RequestSnapshotPool> {
    explicit BusEventRequestNewSnapshot(
        BusEventRequestNewSnapshotPool* mem_pool, RequestSnapshot* request)
        : bus::Event2<BusEventRequestNewSnapshotPool, RequestSnapshotPool>(
              mem_pool, request),
          wrapped_event_(request) {};
    ~BusEventRequestNewSnapshot() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }

    void Release() override {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (memory_pool_) {
                memory_pool_->Deallocate(this);  // Return BusEvent to pool
            }
        }

        // if (wrapped_event_) {
        //     wrapped_event_->Release();  // Decrement reference count for
        //     Event
        // }
    };

    RequestSnapshot* WrappedEvent() { return wrapped_event_; }

  private:
    RequestSnapshot* wrapped_event_ = nullptr;
};

struct BookDiffSnapshot2;
using BookDiff2SnapshotPool = common::MemoryPool<BookDiffSnapshot2>;
struct BookDiffSnapshot;
struct BookDiffSnapshot2 : public aot::Event<BookDiff2SnapshotPool> {
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    std::list<BookSnapshotElem> bids;
    std::list<BookSnapshotElem> asks;
    uint64_t first_id = std::numeric_limits<uint64_t>::max();
    uint64_t last_id  = std::numeric_limits<uint64_t>::max();
    BookDiffSnapshot2() : aot::Event<BookDiff2SnapshotPool>(nullptr) {};

    BookDiffSnapshot2(BookDiff2SnapshotPool* mem_pool,
                      common::ExchangeId _exchange_id,
                      common::TradingPair _trading_pair,
                      std::list<BookSnapshotElem>&& _bids,
                      std::list<BookSnapshotElem>&& _asks, uint64_t _first_id,
                      uint64_t _last_id)
        : aot::Event<BookDiff2SnapshotPool>(mem_pool),
          exchange_id(_exchange_id),
          trading_pair(_trading_pair),
          bids(std::move(_bids)),
          asks(std::move(_asks)),
          first_id(_first_id),
          last_id(_last_id) {}
    auto ToString() const {
        return fmt::format("BookDiffSnapshot2[first_id:{} last_id:{}]",
                           first_id, last_id);
    };
    // void AddToQueue(EventLFQueue& queue) {
    //     for (const auto& bid : bids) {
    //         MEMarketUpdate event;
    //         event.side     = common::Side::SELL;
    //         event.price    = bid.price;
    //         event.qty      = bid.qty;
    //         auto status_op = queue.try_enqueue(event);
    //         logi("{}", event.ToString());
    //         if (!status_op) [[unlikely]]
    //             loge("can't enqueue more elements. my lfqueue is busy");
    //     }
    //     for (const auto& ask : asks) {
    //         MEMarketUpdate event;
    //         event.side     = common::Side::BUY;
    //         event.price    = ask.price;
    //         event.qty      = ask.qty;
    //         auto status_op = queue.try_enqueue(event);
    //         logi("{}", event.ToString());
    //         if (!status_op) [[unlikely]]
    //             loge("can't enqueue more elements. my lfqueue is busy");
    //     }
    // }
    void Release() override {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (memory_pool_) {
                memory_pool_->Deallocate(this);  // Return BusEvent to pool
            }
        }
    };
};

struct BookDiffSnapshot;
using BookDiffSnapshotPool = common::MemoryPool<BookDiffSnapshot>;
struct BookDiffSnapshot {
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
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
    BookDiffSnapshotPool* mem_pool = nullptr;
    void Deallocate() {
        if (!mem_pool) {
            logw("mem_pool = nullptr. can.t deallocate MEClientResponse");
            return;
        }
        mem_pool->Deallocate(this);
    };
};

struct BusEventBookDiffSnapshot;
using BusEventBookDiffSnapshotPool =
    common::MemoryPool<BusEventBookDiffSnapshot>;

struct BusEventBookDiffSnapshot
    : public bus::Event2<BusEventBookDiffSnapshotPool, BookDiff2SnapshotPool> {
    explicit BusEventBookDiffSnapshot(BusEventBookDiffSnapshotPool* mem_pool,
                                      BookDiffSnapshot2* response)
        : bus::Event2<BusEventBookDiffSnapshotPool, BookDiff2SnapshotPool>(
              mem_pool, response),
          wrapped_event_(response) {};
    ~BusEventBookDiffSnapshot() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }
    BookDiffSnapshot2* WrappedEvent() { return wrapped_event_; }

  private:
    BookDiffSnapshot2* wrapped_event_ = nullptr;
};

struct RequestDiffOrderBook;
using RequestDiffOrderBookPool = common::MemoryPool<RequestDiffOrderBook>;
struct RequestDiffOrderBook : public aot::Event<RequestDiffOrderBookPool> {
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    common::FrequencyMS frequency = common::kFrequencyMSInvalid;
    RequestDiffOrderBook() : aot::Event<RequestDiffOrderBookPool>(nullptr) {};

    RequestDiffOrderBook(RequestDiffOrderBookPool* mem_pool,
                         common::ExchangeId _exchange_id,
                         common::TradingPair _trading_pair,
                         common::FrequencyMS _frequency)
        : aot::Event<RequestDiffOrderBookPool>(mem_pool),
          exchange_id(_exchange_id),
          trading_pair(_trading_pair),
          frequency(_frequency) {}
    void Release() override {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (memory_pool_) {
                memory_pool_->Deallocate(this);  // Return to the pool
            }
        }
    }
};

struct BusEventRequestDiffOrderBook;
using BusEventRequestDiffOrderBookPool =
    common::MemoryPool<BusEventRequestDiffOrderBook>;
struct BusEventRequestDiffOrderBook
    : public bus::Event2<BusEventRequestDiffOrderBookPool,
                         RequestDiffOrderBookPool> {
    explicit BusEventRequestDiffOrderBook(
        BusEventRequestDiffOrderBookPool* mem_pool,
        RequestDiffOrderBook* request)
        : bus::Event2<BusEventRequestDiffOrderBookPool,
                      RequestDiffOrderBookPool>(mem_pool, request),
          wrapped_event_(request) {};
    ~BusEventRequestDiffOrderBook() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }
    void Release() override {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (memory_pool_) {
                memory_pool_->Deallocate(this);  // Return BusEvent to pool
            }
        }

        // if (wrapped_event_) {
        //     wrapped_event_->Release();  // Decrement reference count for
        //     Event
        // }
    };
    RequestDiffOrderBook* WrappedEvent() { return wrapped_event_; }

  private:
    RequestDiffOrderBook* wrapped_event_ = nullptr;
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