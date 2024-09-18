#pragma once

#include "boost/noncopyable.hpp"
// if link form source
#include "aot/common/mem_pool.h"
#include "concurrentqueue.h"

namespace strategy {
namespace cross_arbitrage {
constexpr size_t kMaxEvents = 10000;

enum class EventType { kBidUpdate, kAskUpdate, kNoEvent };
class Event {
  public:
    virtual ~Event()            = default;
    virtual EventType GetType() = 0;
};
struct BaseEvent : public Event {
    ~BaseEvent() override = default;
    EventType GetType() override { return EventType::kNoEvent; };
};

using LFQueue = moodycamel::ConcurrentQueue<Event*>;

/**best bid updated pool*/
class BBidUpdated;
using BBUPool = common::MemPool<BBidUpdated>;

/**
 * @brief best bid updated
 *
 */
struct BBidUpdated : public BaseEvent {
    common::ExchangeId exchange = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    common::Price price         = common::kPriceInvalid;
    common::Qty qty             = common::kQtyInvalid;
    BBUPool* mem_pool           = nullptr;
    BBidUpdated()               = default;
    explicit BBidUpdated(common::ExchangeId _exchange, common::TradingPair _trading_pair, common::Price _price,
                common::Qty _qty, BBUPool* pool)
        : exchange(_exchange), trading_pair(_trading_pair), price(_price), qty(_qty), mem_pool(pool) {};
    ~BBidUpdated() override = default;
    void Deallocate(){
        if(!mem_pool){
            loge("can't free BBidUpdated event");
            return;
        }
        mem_pool->deallocate(this);
    }
    EventType GetType() override { return EventType::kBidUpdate; };
};

/**best ask updated pool*/
class BAskUpdated;
using BAUPool = common::MemPool<BAskUpdated>;

/**
 * @brief best ask updated
 * 
 */
struct BAskUpdated : public BaseEvent {
    common::ExchangeId exchange = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    common::Price price         = common::kPriceInvalid;
    common::Qty qty             = common::kQtyInvalid;
    BAUPool* mem_pool           = nullptr;
    BAskUpdated()               = default;
    explicit BAskUpdated(common::ExchangeId _exchange, common::TradingPair _trading_pair, common::Price _price,
                common::Qty _qty, BAUPool* pool)
        : exchange(_exchange), trading_pair(_trading_pair), price(_price), qty(_qty), mem_pool(pool) {};
    ~BAskUpdated() override = default;
        void Deallocate(){
        if(!mem_pool){
            loge("can't free BAskUpdated event");
            return;
        }
        mem_pool->deallocate(this);
    }
    EventType GetType() override { return EventType::kAskUpdate; };
};
}  // namespace cross_arbitrage
}  // namespace strategy
