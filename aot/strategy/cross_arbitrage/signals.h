#pragma once

#include "boost/noncopyable.hpp"

namespace strategy {
namespace cross_arbitrage {
enum class EventType { kBidUpdate, kAskUpdate };
class Event {
  public:
    virtual ~Event()            = default;
    virtual EventType GetType() = 0;
};
struct BidUpdated : public Event, public boost::noncopyable {
    Common::TradingPair trading_pair;
    Common::PriceD price;
    Common::QtyD qty;
    BidUpdated() = delete;
    BidUpdated(Common::TradingPair _trading_pair, Common::PriceD _price,
               Common::QtyD _qty)
        : trading_pair(_trading_pair), price(_price), qty(_qty) {};
    ~BidUpdated() override = default;
    EventType GetType() override { return EventType::kBidUpdate; };
};
}  // namespace cross_arbitrage
}  // namespace strategy
