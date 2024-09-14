#pragma once

#include "boost/noncopyable.hpp"
//if link form source
#include "concurrentqueue.h"

#include "aot/common/mem_pool.h"

namespace strategy {
namespace cross_arbitrage {
    constexpr size_t kMaxEvents = 10000;

enum class EventType { kBidUpdate, kAskUpdate, kNoEvent };
class Event {
  public:
    virtual ~Event()            = default;
    virtual EventType GetType() = 0;
};
struct BaseEvent : public Event{
    ~BaseEvent() override = default;
    EventType GetType() override { return EventType::kNoEvent; };
};
/**
 * @brief best bid updated
 * 
 */
struct BBidUpdated : public BaseEvent {
    common::TradingPair trading_pair;
    common::Price price = common::kPriceInvalid;
    common::Qty qty = common::kQtyInvalid;
    BBidUpdated() = default;
    BBidUpdated(common::TradingPair _trading_pair, common::Price _price,
               common::Qty _qty)
        : trading_pair(_trading_pair), price(_price), qty(_qty) {};
    ~BBidUpdated() override = default;
    EventType GetType() override { return EventType::kBidUpdate; };
};

struct BAskUpdated : public BaseEvent {
    common::TradingPair trading_pair;
    common::Price price = common::kPriceInvalid;
    common::Qty qty = common::kQtyInvalid;
    BAskUpdated() = default;
    BAskUpdated(common::TradingPair _trading_pair, common::Price _price,
               common::Qty _qty)
        : trading_pair(_trading_pair), price(_price), qty(_qty) {};
    ~BAskUpdated() override = default;
    EventType GetType() override { return EventType::kAskUpdate; };
};

using LFQueue =  moodycamel::ConcurrentQueue<Event*>;

/**best bid updated pool*/
using BBUPool = common::MemPool<BBidUpdated>;

/**best ask updated pool*/
using BAUPool = common::MemPool<BAskUpdated>;
}  // namespace cross_arbitrage
}  // namespace strategy
