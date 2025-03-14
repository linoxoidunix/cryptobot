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
    common::TradingPair trading_pair_;
    common::TradingPairHashMap pairs_;
    backtesting::TradeEngine trade_engine_;
    backtesting::OrderManager order_manager_;

  protected:
    OrdermanagerBacktestingTest()
        : trade_engine_(&klines_, common::TradingPair{2, 1},
                        pair_, nullptr),
          order_manager_(&trade_engine_) {
        tickers_[1] = "usdt";
        tickers_[2] = "btc";
        common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
        pair_[{2, 1}] = pair_info;
        order_manager_.NewOrder(common::ExchangeId::kBinance, common::TradingPair{2, 1}, 3300,
                                common::Side::kAsk, 700000);
    };
};

TEST_F(OrdermanagerBacktestingTest, InsertFirstOrder) {
    auto order = order_manager_.TestGetOrder(common::TradingPair{2, 1},
                                             common::Side::kAsk);
    EXPECT_EQ(order->price, 3300);
    EXPECT_EQ(order->order_id, 1);
    EXPECT_EQ(order->qty, 700000);
    EXPECT_EQ(order->side, common::Side::kAsk);
    EXPECT_EQ(order->trading_pair, (TradingPair{2, 1}));
}

TEST_F(OrdermanagerBacktestingTest, CancelOrder) {
    order_manager_.CancelOrder(common::ExchangeId::kBinance, common::TradingPair{2, 1},
                                             common::Side::kAsk);
    auto order = order_manager_.TestGetOrder(common::TradingPair{2, 1},
                                             common::Side::kAsk);
    EXPECT_EQ(order->order_id, 1);
    EXPECT_EQ(order->side, common::Side::kAsk);
    EXPECT_EQ(order->trading_pair, (TradingPair{2, 1}));
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
