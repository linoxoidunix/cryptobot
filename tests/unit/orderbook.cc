#include "aot/Logger.h"
#include "aot/strategy/market_order_book.h"
#include "cmath"
#include "gtest/gtest.h"

TEST(MarketOrderBookBacktesting, INSERT_EVENT) {
    using namespace backtesting;
    MarketOrderBook book;
    Exchange::MEMarketUpdate market_update;
    market_update.price = 7;
    market_update.qty   = 1;
    book.onMarketUpdate(&market_update);
    auto bbo = book.GetBBO();
    EXPECT_EQ(bbo->GetWeightedPrice(), 7);
    EXPECT_EQ(bbo->price, 7);
    EXPECT_EQ(bbo->qty, 1);
}

TEST(MarketOrderBookBacktesting, UPDATE_BBO) {
    using namespace backtesting;
    MarketOrderBook book;
    Exchange::MEMarketUpdate market_update;
    market_update.price = 7;
    market_update.qty   = 1;
    book.onMarketUpdate(&market_update);
    market_update.price = 6;
    market_update.qty   = 2;
    book.onMarketUpdate(&market_update);
    auto bbo = book.GetBBO();
    EXPECT_EQ(bbo->GetWeightedPrice(), 6);
    EXPECT_EQ(bbo->price, 6);
    EXPECT_EQ(bbo->qty, 2);
}

TEST(BBODoubleBacktesting, Create) {
    using namespace backtesting;
    BBO bbo;
    bbo.price = 7 * std::pow(10, 7);
    bbo.qty   = 4 * std::pow(10, 8);
    BBODouble bbodouble(&bbo, 7, 8);
    EXPECT_EQ(bbodouble.GetWeightedPrice(), 7);
    EXPECT_EQ(bbodouble.price, 7);
    EXPECT_EQ(bbodouble.qty, 4);
}

TEST(OrderBookService, Launch) {
    using namespace Trading;
    Exchange::EventLFQueue queue;
    common::TradingPair trading_pair{2, 1};
    common::TradingPairHashMap pair;
    MarketOrderBookDouble book(trading_pair, pair);
    Exchange::MEMarketUpdateDouble market_update;
    market_update.price = 7.0;
    market_update.qty   = 1.0;
    market_update.side  = common::Side::BUY;

    queue.enqueue(market_update);
    market_update.price = 6.0;
    market_update.qty   = 2.0;
    market_update.side  = common::Side::SELL;

    queue.enqueue(market_update);

    OrderBookService service(&book, &queue);
    service.Start();
    service.StopWaitAllQueue();

    EXPECT_EQ(book.getBBO()->ask_price, 7.0);
    EXPECT_EQ(book.getBBO()->ask_qty, 1.0);
    EXPECT_EQ(book.getBBO()->bid_price, 6.0);
    EXPECT_EQ(book.getBBO()->bid_qty, 2.0);
}

int main(int argc, char** argv) {
    // fmtlog::setLogLevel(fmtlog::OFF);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}