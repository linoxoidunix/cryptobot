#include "cmath"

#include "gtest/gtest.h"

#include "aot/Logger.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/trade_engine.h"
#include "aot/Binance.h"

TEST(order_manager_backtesting, InsertFirstOrder) {
    TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";
    
    TradingPairHashMap pair;
    binance::Symbol symbol(tickers[2], tickers[1]);
    TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
    pair[{2, 1}] = pair_info;

    Exchange::EventLFQueue market_updates;
    OHLCVILFQueue klines;
    Common::TradingPair trading_pair;
    Common::TradingPairHashMap pairs;
    backtesting::TradeEngine trade_engine(&market_updates, &klines, Common::TradingPair{2,1}, pair, nullptr);

    backtesting::OrderManager order_manager(&trade_engine);
    order_manager.NewOrder(Common::TradingPair{2,1}, 33.1, Common::Side::BUY, 7.2);
    auto order = order_manager.TestGetOrder(Common::TradingPair{2,1}, Common::Side::BUY);
    EXPECT_EQ(order->price,33.1);
    EXPECT_EQ(order->order_id,1);
    EXPECT_EQ(order->qty,7.2);
    EXPECT_EQ(order->side,Common::Side::BUY);
    EXPECT_EQ(order->trading_pair,(TradingPair{2,1}));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
