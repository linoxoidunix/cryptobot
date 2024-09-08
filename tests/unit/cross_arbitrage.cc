
#include "aot/Logger.h"
#include "aot/common/types.h"
#include "aot/strategy/cross_arbitrage/signals.h"
#include "gtest/gtest.h"

TEST(CrossArbitrageEvent, Create) {
    using namespace strategy::cross_arbitrage;
    using namespace Common;
    BBidUpdated bid_updated(TradingPair{2, 1}, 100.0, 14.0);
    EXPECT_EQ(bid_updated.GetType(), EventType::kBidUpdate);
    EXPECT_EQ(bid_updated.price, 100);
    EXPECT_EQ(bid_updated.qty, 14);
    EXPECT_EQ(bid_updated.trading_pair, (TradingPair{2, 1}));
}

TEST(MemPoolEvents, Using) {
    using namespace strategy::cross_arbitrage;
    using namespace Common;
    BUPool pool{10};
    LFQueue queue;
    for (int i = 0; i < 9; i++) {
        auto ptr = pool.allocate(BBidUpdated(TradingPair{2, 1}, 100.0, 14.0));
        queue.enqueue(ptr);
    }
    Event* event;
    queue.try_dequeue(event);
    if (event->GetType() == EventType::kBidUpdate) {
        auto bid_event = static_cast<BBidUpdated*>(event);
        EXPECT_EQ(bid_event->GetType(), EventType::kBidUpdate);
        EXPECT_EQ(bid_event->price, 100);
        EXPECT_EQ(bid_event->qty, 14);
        EXPECT_EQ(bid_event->trading_pair, (TradingPair{2, 1}));
    }
    assert(false);
}

TEST(MemPoolEvents, UsingInThread) {
    using namespace strategy::cross_arbitrage;
    using namespace Common;
    BUPool pool{10};
    LFQueue queue;
    std::jthread t1([&pool, &queue]() {
        for (int i = 0; i < 9; i++) {
            auto ptr =
                pool.allocate(BBidUpdated(TradingPair{2, 1}, 100.0+i, 14.0+i));
            queue.enqueue(ptr);
        }
        while (queue.size_approx()) {
        };
        for (int i = 0; i < 9; i++) {
            auto ptr =
                pool.allocate(BBidUpdated(TradingPair{2, 1}, 100.0 + 9 + i , 14.0 + 9 + i));
            queue.enqueue(ptr);
        }
    });
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    auto number_dequed = 0;
    std::jthread t2([&pool, &queue, &number_dequed]() {
        Event* event;
        while (number_dequed != 18) {
            auto status = queue.try_dequeue(event);
            if(status)
                if (event->GetType() == EventType::kBidUpdate) {
                    auto bid_event = static_cast<BBidUpdated*>(event);
                    EXPECT_EQ(bid_event->GetType(), EventType::kBidUpdate);
                    EXPECT_EQ(bid_event->price, 100+number_dequed);
                    EXPECT_EQ(bid_event->qty, 14+number_dequed);
                    EXPECT_EQ(bid_event->trading_pair, (TradingPair{2, 1}));
                    number_dequed++;
                    pool.deallocate(bid_event);
                }
        }
    });
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}