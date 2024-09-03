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
        /**
         * @brief kLFQueuePushNewBidAsksEvents occured when LFQueue ready to push new batch bid or ask event
         * 
         */ 
        kLFQueuePushNewBidAsksEvents,
        /**
         * @brief kLFQueuePullNewBidAsksEvents occured when LFQueue ready to pull new batch bid or ask event
         * 
         */ 
        kLFQueuePullNewBidAsksEvents,
        /**
         * @brief kStrategyOnNewKLineBefore time when strategy want call cb on new kline 
         * 
         */ 
        kStrategyOnNewKLineBefore,
        /**
         * @brief kStrategyOnNewKLineAfter time when strategy executed cb on new kline 
         * 
         */ 
        kStrategyOnNewKLineAfter,
        /**
         * @brief kStrategyOnNewKLineAfter time when strategy executed cb on new kline 
         * 
         */ 
        kStrategyOnNewKLineRate,
        /**
         * @brief kOrderBookUpdateBefore - time when market order book start update by self
         * 
         */ 
        kOrderBookUpdateBefore,
        /**
         * @brief kOrderBookUpdatefAfter - time when market order book end update by self
         * 
         */ 
        kOrderBookUpdateAfter,
        /**
         * @brief kOrderBookUpdatefAfter - time when market order book end update by self
         * 
         */ 
        kUpdateSpeedOrderBook
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
    
    template <bool need_measure_latency, class LFQueuePtr>
    void LogEvent(prometheus::EventType type, LFQueuePtr queue) {
        if constexpr (need_measure_latency == true) {
            if (queue) [[likely]] {
                auto status = queue->try_enqueue(
                    prometheus::Event(type, common::getCurrentNanoS()));
                    if(!status)[[unlikely]]
                        loge("my queue is full");
            }
        }
    };

    template <bool need_measure_latency, class Value, class LFQueuePtr>
    void LogEvent(prometheus::EventType type, const Value& value, LFQueuePtr queue) {
        if constexpr (need_measure_latency == true) {
            if (queue) [[likely]] {
                auto status = queue->try_enqueue(
                    prometheus::Event(type, value));
                    if(!status)[[unlikely]]
                        loge("my queue is full");
            }
        }
    };
};