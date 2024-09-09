#pragma once

#include "boost/noncopyable.hpp"
#include "concurrentqueue.h"//if link form source

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
    Common::TradingPair trading_pair;
    Common::PriceD price = Common::kPRICE_DOUBLE_INVALID;
    Common::QtyD qty = Common::kQTY_DOUBLE_INVALID;
    BBidUpdated() = default;
    BBidUpdated(Common::TradingPair _trading_pair, Common::PriceD _price,
               Common::QtyD _qty)
        : trading_pair(_trading_pair), price(_price), qty(_qty) {};
    ~BBidUpdated() override = default;
    EventType GetType() override { return EventType::kBidUpdate; };
};

struct BAskUpdated : public BaseEvent {
    Common::TradingPair trading_pair;
    Common::PriceD price = Common::kPRICE_DOUBLE_INVALID;
    Common::QtyD qty = Common::kQTY_DOUBLE_INVALID;
    BAskUpdated() = default;
    BAskUpdated(Common::TradingPair _trading_pair, Common::PriceD _price,
               Common::QtyD _qty)
        : trading_pair(_trading_pair), price(_price), qty(_qty) {};
    ~BAskUpdated() override = default;
    EventType GetType() override { return EventType::kAskUpdate; };
};

using LFQueue =  moodycamel::ConcurrentQueue<Event*>;

/**best bid updated pool*/
using BBUPool = common::MemPool<BBidUpdated>;

/**best ask updated pool*/
using BAUPool = common::MemPool<BBidUpdated>;
}  // namespace cross_arbitrage
}  // namespace strategy
