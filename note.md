# test note

completed Events
- [X] RequestSnapshot
- [X] RequestDiffOrderBook

completed BusEvents
- [X] BusEventRequestNewSnapshot
- [X] BusEventRequestDiffOrderBook

------EXAMPLE--------------------------
```c++
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
    void Accept(bus::Component* comp, const OnWssResponse* cb) override {
        comp->AsyncHandleEvent(this, cb);
    }
    void Release() override {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (memory_pool_) {
                memory_pool_->Deallocate(this);  // Return BusEvent to pool
            }
        }

        if (wrapped_event_) {
            wrapped_event_->Release();  // Decrement reference count for Event
        }
    };
    RequestDiffOrderBook* WrappedEvent() { return wrapped_event_; }
  private:
    RequestDiffOrderBook* wrapped_event_ = nullptr;
};
------------------------------------------------------------
auto* ptr = request_snapshot_mem_pool_.Allocate(info.exchange_id, info.trading_pair, info.snapshot_depth, request_snapshot_mem_pool_);
auto* bus_evt_request =  request_bus_event_snapshot_mem_pool_.Allocate(ptr, request_bus_event_snapshot_mem_pool_);

```