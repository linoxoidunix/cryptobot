#include "aot/Binance.h"
#include "aot/Logger.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/trade_engine.h"
#include "cmath"
#include "gtest/gtest.h"

class OrdermanagerBacktestingTest : public testing::Test {
  public:
    TickerHashMap tickers_;
    TradingPairHashMap pair_;
    Exchange::EventLFQueue market_updates_;
    OHLCVILFQueue klines_;
    Common::TradingPair trading_pair_;
    Common::TradingPairHashMap pairs_;
    backtesting::TradeEngine trade_engine_;
    backtesting::OrderManager order_manager_;

  protected:
    OrdermanagerBacktestingTest()
        : trade_engine_(&klines_, Common::TradingPair{2, 1},
                        pair_, nullptr),
          order_manager_(&trade_engine_) {
        tickers_[1] = "usdt";
        tickers_[2] = "btc";
        binance::Symbol symbol(tickers_[2], tickers_[1]);
        TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
        pair_[{2, 1}] = pair_info;
        order_manager_.NewOrder(Common::TradingPair{2, 1}, 33.1,
                                Common::Side::BUY, 7.2);
    };
};

TEST_F(OrdermanagerBacktestingTest, InsertFirstOrder) {
    auto order = order_manager_.TestGetOrder(Common::TradingPair{2, 1},
                                             Common::Side::BUY);
    EXPECT_EQ(order->price, 33.1);
    EXPECT_EQ(order->order_id, 1);
    EXPECT_EQ(order->qty, 7.2);
    EXPECT_EQ(order->side, Common::Side::BUY);
    EXPECT_EQ(order->trading_pair, (TradingPair{2, 1}));
}

TEST_F(OrdermanagerBacktestingTest, CancelOrder) {
    order_manager_.CancelOrder(Common::TradingPair{2, 1},
                                             Common::Side::BUY);
    auto order = order_manager_.TestGetOrder(Common::TradingPair{2, 1},
                                             Common::Side::BUY);
    EXPECT_EQ(order->order_id, 1);
    EXPECT_EQ(order->side, Common::Side::BUY);
    EXPECT_EQ(order->trading_pair, (TradingPair{2, 1}));
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
