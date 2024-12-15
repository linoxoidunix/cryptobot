#pragma once

#include <limits>
#include <list>
#include <variant>

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

struct MEMarketUpdate2;
using MEMarketUpdate2Pool = common::MemoryPool<MEMarketUpdate2>;

/**
 * @brief this class for work bus
 *
 */
struct MEMarketUpdate2 : public aot::Event<MEMarketUpdate2Pool> {
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    MarketUpdateType type    = MarketUpdateType::DEFAULT;
    common::OrderId order_id = common::kOrderIdInvalid;
    common::Side side        = common::Side::INVALID;
    common::Price price      = common::kPriceInvalid;
    common::Qty qty          = common::kQtyInvalid;

    auto ToString() const {
        return fmt::format(
            "MEMarketUpdate[exch_id:{} {} oid:{} side:{} qty:{} price:{}]",
            exchange_id, trading_pair.ToString(),
            common::orderIdToString(order_id), sideToString(side),
            common::qtyToString(qty), common::priceToString(price));
    };
    explicit MEMarketUpdate2() : aot::Event<MEMarketUpdate2Pool>(nullptr) {};
    explicit MEMarketUpdate2(MEMarketUpdate2Pool* pool,
                             common::ExchangeId _exchange_id,
                             common::TradingPair _trading_pair,
                             MarketUpdateType _type, common::OrderId _order_id,
                             common::Side _side, common::Price _price,
                             common::Qty _qty)
        : aot::Event<MEMarketUpdate2Pool>(pool),
          exchange_id(_exchange_id),
          trading_pair(_trading_pair),
          type(_type),
          order_id(_order_id),
          side(_side),
          price(_price),
          qty(_qty) {};

    friend void intrusive_ptr_release(Exchange::MEMarketUpdate2* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
                logd("free MEMarketUpdate2 ptr");
            }
        }
    }
    friend void intrusive_ptr_add_ref(Exchange::MEMarketUpdate2* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }
};

struct BusEventMEMarketUpdate2;
using BusEventMEMarketUpdate2Pool = common::MemoryPool<BusEventMEMarketUpdate2>;
/**
 * @brief it is wrapper above MEMarketUpdate2
 *
 */
struct BusEventMEMarketUpdate2
    : public bus::Event2<BusEventMEMarketUpdate2Pool> {
    explicit BusEventMEMarketUpdate2(
        BusEventMEMarketUpdate2Pool* mem_pool,
        boost::intrusive_ptr<Exchange::MEMarketUpdate2> ptr)
        : bus::Event2<BusEventMEMarketUpdate2Pool>(mem_pool),
          wrapped_event_(ptr) {};
    ~BusEventMEMarketUpdate2() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }
    friend void intrusive_ptr_release(Exchange::BusEventMEMarketUpdate2* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
                logd("free BusEventMEMarketUpdate2 ptr");
            }
        }
    }
    friend void intrusive_ptr_add_ref(Exchange::BusEventMEMarketUpdate2* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }
    Exchange::MEMarketUpdate2* WrappedEvent() {
        if (!wrapped_event_) return nullptr;
        return wrapped_event_.get();
    }

  private:
    boost::intrusive_ptr<Exchange::MEMarketUpdate2> wrapped_event_;
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
    friend void intrusive_ptr_release(Exchange::BookSnapshot2* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(Exchange::BookSnapshot2* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
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
    // i think lastUpdateId must negotiate number
    uint64_t lastUpdateId = 0;

    BookSnapshot()        = default;
    Exchange::BookSnapshot& operator=(Exchange::BookSnapshot2&& element);

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
    : public bus::Event2<BusEventResponseNewSnapshotPool> {
    explicit BusEventResponseNewSnapshot(
        BusEventResponseNewSnapshotPool* mem_pool,
        boost::intrusive_ptr<Exchange::BookSnapshot2> _response)
        : bus::Event2<BusEventResponseNewSnapshotPool>(mem_pool),
          wrapped_event_(_response) {};
    ~BusEventResponseNewSnapshot() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }
    friend void intrusive_ptr_release(
        Exchange::BusEventResponseNewSnapshot* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(
        Exchange::BusEventResponseNewSnapshot* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }
    Exchange::BookSnapshot2* WrappedEvent() {
        if (!wrapped_event_) return nullptr;
        return wrapped_event_.get();
    }

  private:
    boost::intrusive_ptr<Exchange::BookSnapshot2> wrapped_event_;
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
    friend void intrusive_ptr_release(Exchange::RequestSnapshot* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(Exchange::RequestSnapshot* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
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
    : public bus::Event2<BusEventRequestNewSnapshotPool> {
    explicit BusEventRequestNewSnapshot(
        BusEventRequestNewSnapshotPool* mem_pool,
        boost::intrusive_ptr<RequestSnapshot> request)
        : bus::Event2<BusEventRequestNewSnapshotPool>(mem_pool),
          wrapped_event_(request) {};
    ~BusEventRequestNewSnapshot() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }

    friend void intrusive_ptr_release(
        Exchange::BusEventRequestNewSnapshot* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(
        Exchange::BusEventRequestNewSnapshot* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }

    RequestSnapshot* WrappedEvent() {
        if (!wrapped_event_) return nullptr;
        return wrapped_event_.get();
    }

  private:
    boost::intrusive_ptr<RequestSnapshot> wrapped_event_;
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
    friend void intrusive_ptr_release(Exchange::BookDiffSnapshot2* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(Exchange::BookDiffSnapshot2* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }
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
};

struct BusEventBookDiffSnapshot;
using BusEventBookDiffSnapshotPool =
    common::MemoryPool<BusEventBookDiffSnapshot>;

struct BusEventBookDiffSnapshot
    : public bus::Event2<BusEventBookDiffSnapshotPool> {
    explicit BusEventBookDiffSnapshot(
        BusEventBookDiffSnapshotPool* mem_pool,
        boost::intrusive_ptr<BookDiffSnapshot2> response)
        : bus::Event2<BusEventBookDiffSnapshotPool>(mem_pool),
          wrapped_event_(response) {};
    ~BusEventBookDiffSnapshot() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }

    BookDiffSnapshot2* WrappedEvent() {
        if (!wrapped_event_) return nullptr;
        return wrapped_event_.get();
    }
    boost::intrusive_ptr<Exchange::BookDiffSnapshot2> WrappedEventIntrusive() {
        return wrapped_event_;
    }

    friend void intrusive_ptr_release(Exchange::BusEventBookDiffSnapshot* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(Exchange::BusEventBookDiffSnapshot* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }

  private:
    boost::intrusive_ptr<Exchange::BookDiffSnapshot2> wrapped_event_;
};

struct RequestDiffOrderBook;
using RequestDiffOrderBookPool = common::MemoryPool<RequestDiffOrderBook>;
struct RequestDiffOrderBook : public aot::Event<RequestDiffOrderBookPool> {
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    common::FrequencyMS frequency = common::kFrequencyMSInvalid;
    bool subscribe = true;
    std::variant<std::string, int, unsigned int> id;  // id as a variant
    RequestDiffOrderBook() : aot::Event<RequestDiffOrderBookPool>(nullptr) {};

    RequestDiffOrderBook(RequestDiffOrderBookPool* mem_pool,
                         common::ExchangeId _exchange_id,
                         common::TradingPair _trading_pair,
                         common::FrequencyMS _frequency,
                         bool _subscribe,
                         const std::variant<std::string, int, unsigned int>& _id = std::string{})
        : aot::Event<RequestDiffOrderBookPool>(mem_pool),
          exchange_id(_exchange_id),
          trading_pair(_trading_pair),
          frequency(_frequency),
          subscribe(_subscribe),
          id(_id) {}
    friend void intrusive_ptr_release(Exchange::RequestDiffOrderBook* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(Exchange::RequestDiffOrderBook* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }
};

struct BusEventRequestDiffOrderBook;
using BusEventRequestDiffOrderBookPool =
    common::MemoryPool<BusEventRequestDiffOrderBook>;
struct BusEventRequestDiffOrderBook
    : public bus::Event2<BusEventRequestDiffOrderBookPool> {
    explicit BusEventRequestDiffOrderBook(
        BusEventRequestDiffOrderBookPool* mem_pool,
        boost::intrusive_ptr<RequestDiffOrderBook> request)
        : bus::Event2<BusEventRequestDiffOrderBookPool>(mem_pool),
          wrapped_event_(request) {};
    ~BusEventRequestDiffOrderBook() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }
    friend void intrusive_ptr_release(
        Exchange::BusEventRequestDiffOrderBook* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(
        Exchange::BusEventRequestDiffOrderBook* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }
    RequestDiffOrderBook* WrappedEvent() {
        if (!wrapped_event_) return nullptr;
        return wrapped_event_.get();
    }

  private:
    boost::intrusive_ptr<RequestDiffOrderBook> wrapped_event_;
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