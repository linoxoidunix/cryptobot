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
    Common::PriceD price;
    Common::QtyD qty;
    BBidUpdated() = default;
    BBidUpdated(Common::TradingPair _trading_pair, Common::PriceD _price,
               Common::QtyD _qty)
        : trading_pair(_trading_pair), price(_price), qty(_qty) {};
    ~BBidUpdated() override = default;
    EventType GetType() override { return EventType::kBidUpdate; };
};
using LFQueue =  moodycamel::ConcurrentQueue<Event*>;
using BUPool = common::MemPool<BBidUpdated>;
}  // namespace cross_arbitrage
}  // namespace strategy
