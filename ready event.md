# test note

completed Events that used boost::intrusive_ptr
- [X] BookSnapshot2
- [X] RequestSnapshot
- [X] BookDiffSnapshot2
- [X] RequestDiffOrderBook

completed BusEvents that used boost::intrusive_ptr
- [X] BusEventResponseNewSnapshot
- [X] BusEventRequestNewSnapshot
- [X] BusEventBookDiffSnapshot
- [X] BusEventRequestDiffOrderBook

------EXAMPLE--------------------------
```c++
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
    friend void intrusive_ptr_release(Exchange::BookSnapshot2* ptr){
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

struct BusEventResponseNewSnapshot;
using BusEventResponseNewSnapshotPool =
    common::MemoryPool<BusEventResponseNewSnapshot>;

struct BusEventResponseNewSnapshot
    : public bus::Event2<BusEventResponseNewSnapshotPool> {
    explicit BusEventResponseNewSnapshot(
        BusEventResponseNewSnapshotPool* mem_pool,
        boost::intrusive_ptr<Exchange::BookSnapshot2> _response)
        : bus::Event2<BusEventResponseNewSnapshotPool>(
              mem_pool),
          wrapped_event_(_response) {};
    ~BusEventResponseNewSnapshot() override = default;
    void Accept(bus::Component* comp) override { comp->AsyncHandleEvent(this); }
    friend void intrusive_ptr_release(Exchange::BusEventResponseNewSnapshot* ptr){
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
            }
        }
    }
    friend void intrusive_ptr_add_ref(Exchange::BusEventResponseNewSnapshot* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }
    Exchange::BookSnapshot2* WrappedEvent() {
        if(!wrapped_event_)
            return nullptr;
        return wrapped_event_.get();
    }

  private:
    boost::intrusive_ptr<Exchange::BookSnapshot2> wrapped_event_;
};
------------------------------------------------------------
auto ptr = component.snapshot_mem_pool_.Allocate(&component.snapshot_mem_pool_,
            snapshot.exchange_id,
            snapshot.trading_pair,
            std::move(snapshot.bids),
            std::move(snapshot.asks),
            snapshot.lastUpdateId);
auto intr_ptr_snapsot = boost::intrusive_ptr<Exchange::BookSnapshot2>(ptr);

auto bus_event = component.bus_event_response_snapshot_mem_pool_.Allocate(&component.bus_event_response_snapshot_mem_pool_,
    intr_ptr_snapsot);
auto intr_ptr_bus_snapsot = boost::intrusive_ptr<Exchange::BusEventResponseNewSnapshot>(bus_event);

bus.AsyncSend(&component, intr_ptr_bus_snapsot);


```