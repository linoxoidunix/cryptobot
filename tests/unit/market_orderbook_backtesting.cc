#include "gtest/gtest.h"

#include "aot/Logger.h"
#include "aot/strategy/market_order_book.h"


TEST(MarketOrderBookBacktesting, INSERT_EVENT) {
    using namespace backtesting;
    MarketOrderBook book;
    Exchange::MEMarketUpdate market_update;
    market_update.price = 7;
    market_update.qty = 1;
    book.onMarketUpdate(&market_update);
    auto bbo = book.GetBBO();
    EXPECT_EQ(bbo->GetWeightedPrice(),7);
    EXPECT_EQ(bbo->price,7);
    EXPECT_EQ(bbo->qty,1);
}

TEST(MarketOrderBookBacktesting, UPDATE_BBO) {
    using namespace backtesting;
    MarketOrderBook book;
    Exchange::MEMarketUpdate market_update;
    market_update.price = 7;
    market_update.qty = 1;
    book.onMarketUpdate(&market_update);
    market_update.price = 6;
    market_update.qty = 2;
    book.onMarketUpdate(&market_update);
    auto bbo = book.GetBBO();
    EXPECT_EQ(bbo->GetWeightedPrice(),6);
    EXPECT_EQ(bbo->price,6);
    EXPECT_EQ(bbo->qty,2);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}