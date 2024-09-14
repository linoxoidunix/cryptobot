#include "aot/common/types.h"
#include "aot/Logger.h"
#include "aot/strategy/market_order_book.h"
#include "aot/Binance.h"
#include "cmath"
#include "gtest/gtest.h"

TEST(MarketOrderBookBacktesting, INSERT_EVENT) {
    using namespace backtesting;
    using namespace binance;
    common::TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";
    
    common::TradingPairHashMap pair;
    binance::Symbol symbol(tickers[2], tickers[1]);
    common::TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
    pair[{2, 1}] = pair_info;
    MarketOrderBook book(common::TradingPair{2,1}, pair);
    Exchange::MEMarketUpdate market_update;
    market_update.price = 7;
    market_update.qty   = 1;
    book.OnMarketUpdate(&market_update);
    auto bbo = book.GetBBO();
    EXPECT_EQ(bbo->GetWeightedPrice(), 7);
    EXPECT_EQ(bbo->price, 7);
    EXPECT_EQ(bbo->qty, 1);
}

TEST(MarketOrderBookBacktesting, UPDATE_BBO) {
    using namespace backtesting;
    using namespace binance;
    common::TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";
    
    common::TradingPairHashMap pair;
    binance::Symbol symbol(tickers[2], tickers[1]);
    common::TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
    pair[{2, 1}] = pair_info;
    MarketOrderBook book(common::TradingPair{2,1}, pair);
    Exchange::MEMarketUpdate market_update;
    market_update.price = 7;
    market_update.qty   = 1;
    book.OnMarketUpdate(&market_update);
    market_update.price = 6;
    market_update.qty   = 2;
    book.OnMarketUpdate(&market_update);
    auto bbo = book.GetBBO();
    EXPECT_EQ(bbo->GetWeightedPrice(), 6);
    EXPECT_EQ(bbo->price, 6);
    EXPECT_EQ(bbo->qty, 2);
}

TEST(OrderBookService, Launch) {
    using namespace Trading;
    Exchange::EventLFQueue queue;
    common::TradingPairHashMap pair;
    common::TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";
    binance::Symbol symbol(tickers[2], tickers[1]);
    common::TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
    pair[{2, 1}] = pair_info;
    Exchange::MEMarketUpdate market_update;
    market_update.price = 7;
    market_update.qty   = 1;
    market_update.side  = common::Side::BUY;

    queue.enqueue(market_update);
    market_update.price = 6;
    market_update.qty   = 2;
    market_update.side  = common::Side::SELL;

    queue.enqueue(market_update);
   
    MarketOrderBook book(common::TradingPair{2,1}, pair);
    OrderBookService service(&book, &queue);
    service.Start();
    service.StopWaitAllQueue();

    EXPECT_EQ(book.getBBO()->ask_price, 7);
    EXPECT_EQ(book.getBBO()->ask_qty, 1);
    EXPECT_EQ(book.getBBO()->bid_price, 6);
    EXPECT_EQ(book.getBBO()->bid_qty, 2);
}

int main(int argc, char** argv) {
    // fmtlog::setLogLevel(fmtlog::OFF);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}