#include "gtest/gtest.h"

#include "aot/Logger.h"
#include "aot/common/types.h"
#include "aot/strategy/cross_arbitrage/signals.h"
#include "aot/strategy/cross_arbitrage/trade_engine.h"

TEST(CrossArbitrageEvent, Create) {
    using namespace strategy::cross_arbitrage;
    using namespace common;
    common::ExchangeId exchange = 1;
    BBidUpdated bid_updated(exchange, TradingPair{2, 1}, 100.0, 14.0);
    EXPECT_EQ(bid_updated.GetType(), EventType::kBidUpdate);
    EXPECT_EQ(bid_updated.price, 100);
    EXPECT_EQ(bid_updated.qty, 14);
    EXPECT_EQ(bid_updated.trading_pair, (TradingPair{2, 1}));
}

TEST(MemPoolEvents, Using) {
    using namespace strategy::cross_arbitrage;
    using namespace common;
    common::ExchangeId exchange = 1;
    BBUPool pool{10};
    LFQueue queue;
    for (int i = 0; i < 9; i++) {
        auto ptr = pool.allocate(BBidUpdated(exchange, TradingPair{2, 1}, 100.0, 14.0));
        queue.enqueue(ptr);
    }
    Event* event;
    queue.try_dequeue(event);
    if (event->GetType() == EventType::kBidUpdate) {
        auto bid_event = static_cast<BBidUpdated*>(event);
        EXPECT_EQ(bid_event->exchange, 1);
        EXPECT_EQ(bid_event->GetType(), EventType::kBidUpdate);
        EXPECT_EQ(bid_event->price, 100);
        EXPECT_EQ(bid_event->qty, 14);
        EXPECT_EQ(bid_event->trading_pair, (TradingPair{2, 1}));
    } else
    assert(false);
}

TEST(MemPoolEvents, UsingInThread) {
    using namespace strategy::cross_arbitrage;
    using namespace common;
    common::ExchangeId exchange = 1;
    BBUPool pool{10};
    LFQueue queue;
    std::jthread t1([&pool, &queue, &exchange]() {
        for (int i = 0; i < 9; i++) {
            auto ptr =
                pool.allocate(BBidUpdated(exchange, TradingPair{2, 1}, 100.0+i, 14.0+i));
            queue.enqueue(ptr);
        }
        while (queue.size_approx()) {
        };
        for (int i = 0; i < 9; i++) {
            auto ptr =
                pool.allocate(BBidUpdated(exchange, TradingPair{2, 1}, 100.0 + 9 + i , 14.0 + 9 + i));
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
                    EXPECT_EQ(bid_event->exchange, 1);
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

namespace strategy {
namespace cross_arbitrage {

class TradeEngineTest : public ::testing::Test {
protected:
    strategy::cross_arbitrage::LFQueue lf_queue_;
    std::unordered_map<common::ExchangeId, common::TradingPair> working_pairs_;
    common::TradingPairHashMap pairs_;
    std::list<common::ExchangeId> exchanges_;
    base_strategy::Strategy *predictor_;
    startegy::cross_arbitrage::TradeEngine* trade_engine_;

    void SetUp() override {
        working_pairs_[1] = common::TradingPair{2,1};
        working_pairs_[2] = common::TradingPair{2,1};
        exchanges_.emplace_back(1);
        exchanges_.emplace_back(2);
        predictor_ = nullptr; // could also be mocked if necessary
        trade_engine_ = new startegy::cross_arbitrage::TradeEngine(&lf_queue_,
                                                            working_pairs_,
                                                            exchanges_,
                                                            pairs_,
                                                            predictor_);
    }

    void TearDown() override {
       delete trade_engine_;
    }
};

TEST_F(TradeEngineTest, RunValidArbitrage) {
    // Create mock events with valid data
    strategy::cross_arbitrage::BBidUpdated bid_event;
    bid_event.exchange = 1;
    bid_event.price = 100;
    bid_event.qty = 10;

    strategy::cross_arbitrage::BAskUpdated ask_event;
    ask_event.exchange = 2;
    ask_event.price = 90;
    ask_event.qty = 10;

    strategy::cross_arbitrage::Event* events[2] = { &bid_event, &ask_event };
    lf_queue_.enqueue(&bid_event);
    lf_queue_.enqueue(&ask_event);

    trade_engine_->Start(); // Run the method
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(2s);
    trade_engine_->Stop(); // Run the method
    // Verify expected behavior, state changes, or logs
    fmtlog::poll();
}

} // namespace cross_arbitrage
} // namespace strategy




int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}