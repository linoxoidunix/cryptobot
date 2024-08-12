#pragma once
 
#include "moodycamel/concurrentqueue.h"

namespace prometheus{
    enum class EventType{
        kNoEvent,
        /**
         * @brief kDiffMarketOrderBookExchangeIncoming occured when new bids or asks events income from exchange 
         * 
         */
        kDiffMarketOrderBookExchangeIncoming,
        /**
         * @brief kSnapshotMarketOrderBookIncoming occured when new snapsot market order book income from exchange 
         * 
         */
        kSnapshotMarketOrderBookIncoming,
        /*
        kLFQueuePushNewBidAsksEvents occured when LFQueue ready to push new batch bid or ask event
        */
        kLFQueuePushNewBidAsksEvents,
        /*
        kLFQueuePullNewBidAsksEvents occured when LFQueue ready to pull new batch bid or ask event
        */
        kLFQueuePullNewBidAsksEvents,
        /*
        kStrategyOnNewKLineBefore time when strategy want call cb on new kline 
        */
        kStrategyOnNewKLineBefore,
        /*
        kStrategyOnNewKLineAfter time when strategy executed cb on new kline 
        */
        kStrategyOnNewKLineAfter

    };
    struct Event{
        EventType event_type;
        /**
         * @brief time_ - time when event occured
         * 
         */
        common::Nanos time;
        Event(EventType _event_type, common::Nanos _time): event_type(_event_type), time(_time){};
        Event() : event_type(EventType::kNoEvent){};
    };
    using EventLFQueue = moodycamel::ConcurrentQueue<Event>;
};