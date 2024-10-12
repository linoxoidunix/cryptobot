#include "gtest/gtest.h"

#include "aot/Logger.h"
#include "aot/common/types.h"
#include "aot/strategy/cross_arbitrage/signals.h"
#include "aot/strategy/cross_arbitrage/trade_engine.h"
#include "aot/wallet_asset.h"

TEST(CrossArbitrageEvent, Create) {
    using namespace strategy::cross_arbitrage;
    using namespace common;
    common::ExchangeId exchange = common::ExchangeId::kBinance;
    BBidUpdated bid_updated(exchange, TradingPair{2, 1}, 100.0, 14.0, nullptr);
    EXPECT_EQ(bid_updated.GetType(), EventType::kBidUpdate);
    EXPECT_EQ(bid_updated.price, 100);
    EXPECT_EQ(bid_updated.qty, 14);
    EXPECT_EQ(bid_updated.trading_pair, (TradingPair{2, 1}));
}

TEST(MemPoolEvents, Using) {
    using namespace strategy::cross_arbitrage;
    using namespace common;
    common::ExchangeId exchange = common::ExchangeId::kBinance;
    BBUPool pool{10};
    LFQueue queue;
    for (int i = 0; i < 9; i++) {
        auto ptr = pool.allocate(BBidUpdated(exchange, TradingPair{2, 1}, 100.0, 14.0, &pool));
        queue.enqueue(ptr);
    }
    Event* event;
    queue.try_dequeue(event);
    if (event->GetType() == EventType::kBidUpdate) {
        auto bid_event = static_cast<BBidUpdated*>(event);
        EXPECT_EQ(bid_event->exchange, common::ExchangeId::kBinance);
        EXPECT_EQ(bid_event->GetType(), EventType::kBidUpdate);
        EXPECT_EQ(bid_event->price, 100);
        EXPECT_EQ(bid_event->qty, 14);
        EXPECT_EQ(bid_event->trading_pair, (TradingPair{2, 1}));
        bid_event->Deallocate();
    } else
    assert(false);
}

TEST(MemPoolEvents, UsingInThread) {
    using namespace strategy::cross_arbitrage;
    using namespace common;
    common::ExchangeId exchange = common::ExchangeId::kBinance;
    BBUPool pool{10};
    LFQueue queue;
    std::jthread t1([&pool, &queue, &exchange]() {
        for (int i = 0; i < 9; i++) {
            auto ptr =
                pool.allocate(BBidUpdated(exchange, TradingPair{2, 1}, 100.0+i, 14.0+i, &pool));
            queue.enqueue(ptr);
        }
        while (queue.size_approx()) {
        };
        for (int i = 0; i < 9; i++) {
            auto ptr =
                pool.allocate(BBidUpdated(exchange, TradingPair{2, 1}, 100.0 + 9 + i , 14.0 + 9 + i, &pool));
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
                    EXPECT_EQ(bid_event->exchange, common::ExchangeId::kBinance);
                    EXPECT_EQ(bid_event->GetType(), EventType::kBidUpdate);
                    EXPECT_EQ(bid_event->price, 100+number_dequed);
                    EXPECT_EQ(bid_event->qty, 14+number_dequed);
                    EXPECT_EQ(bid_event->trading_pair, (TradingPair{2, 1}));
                    number_dequed++;
                    bid_event->Deallocate();
                    //pool.deallocate(bid_event);
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
    Trading::OrderManager* order_manager_ = nullptr;
    TradeEngineCfgHashMap ticker_cfg;
    strategy::cross_arbitrage::CrossArbitrage* strategy_ = nullptr;
    Trading::TradeEngine* trade_engine_ = nullptr;
    BBUPool bbu_pool{10};
    BAUPool bau_pool{10};
    void SetUp() override {
        working_pairs_[common::ExchangeId::kBinance] = common::TradingPair{2,1};
        working_pairs_[common::ExchangeId::kBybit] = common::TradingPair{2,1};
        exchanges_.emplace_back(common::ExchangeId::kBinance);
        exchanges_.emplace_back(common::ExchangeId::kBybit);
        trade_engine_ = new startegy::cross_arbitrage::TradeEngine(&lf_queue_,
                                                            working_pairs_,
                                                            exchanges_,
                                                            pairs_);
        order_manager_ = new backtesting::OrderManager(trade_engine_);
        trade_engine_->SetOrderManager(order_manager_);
        strategy_ = new strategy::cross_arbitrage::CrossArbitrage(working_pairs_, exchanges_, trade_engine_, trade_engine_->OrderManager(), ticker_cfg, pairs_);        
        trade_engine_->SetStrategy(strategy_);
        common::TradeEngineCfg btcusdt_cfg;
        btcusdt_cfg.clip      = 1;
        ticker_cfg[{2,1}] = btcusdt_cfg;
    }

    void TearDown() override {
        delete strategy_;
       delete trade_engine_;
    }
};

TEST_F(TradeEngineTest, RunValidArbitrage) {
    fmtlog::setLogLevel(fmtlog::DBG);
    // Create mock events with valid data
    auto bid_event = bbu_pool.allocate(BBidUpdated(common::ExchangeId::kBinance, TradingPair{2, 1}, 100, 14, &bbu_pool));
    auto ask_event = bau_pool.allocate(BAskUpdated(common::ExchangeId::kBybit, TradingPair{2, 1}, 90, 140, &bau_pool));


    lf_queue_.enqueue(bid_event);
    lf_queue_.enqueue(ask_event);

    trade_engine_->Start(); // Run the method
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(5s);
    trade_engine_->Stop(); // Run the method
    // Verify expected behavior, state changes, or logs
    logd("st:{}",trade_engine_->GetStatistics());
    fmtlog::poll();
}

} // namespace cross_arbitrage
} // namespace strategy




int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}