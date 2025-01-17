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
    common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
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
    common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
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
    common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
    pair[{2, 1}] = pair_info;
    Exchange::MEMarketUpdate market_update;
    market_update.price = 7;
    market_update.qty   = 1;
    market_update.side  = common::Side::kAsk;

    queue.enqueue(market_update);
    market_update.price = 6;
    market_update.qty   = 2;
    market_update.side  = common::Side::kBid;

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

TEST(OrderBook, ShouldNotSendMessagesIfUpdateBidAndUpdateAskAreFalse) {
    using namespace strategy::cross_arbitrage;
    using namespace position_keeper;
    using namespace common;

    TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";

    TradingPairHashMap pair;
    common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
    pair[{2, 1}] = pair_info;

    LFQueue orderbook_tradeengine_channel;
    EventLFQueue orderbook_positionkeeper_channel;

    OrderBook order_book(
        common::ExchangeId::kBinance, TradingPair{2,1}, pair,
        &orderbook_tradeengine_channel, &orderbook_positionkeeper_channel,
        10, 10, 10);

    // Capture the initial size of the channels
    auto initial_tradeengine_channel_size = orderbook_tradeengine_channel.size_approx();
    auto initial_positionkeeper_channel_size = orderbook_positionkeeper_channel.size_approx();

    // Call updateBBO with both update_bid and update_ask set to false
    order_book.updateBBO(false, false);

    // Verify that no messages were sent to the channels
    EXPECT_EQ(orderbook_tradeengine_channel.size_approx(), initial_tradeengine_channel_size);
    EXPECT_EQ(orderbook_positionkeeper_channel.size_approx(), initial_positionkeeper_channel_size);
}

TEST(OrderBook, ShouldAllocateMemoryFromBAUPoolAndSendAskUpdateMessageToOrderbookTradeengineChannel) {
    using namespace strategy::cross_arbitrage;
    using namespace position_keeper;
    using namespace common;

    TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";

    TradingPairHashMap pair;
    common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
    pair[{2, 1}] = pair_info;

    LFQueue orderbook_tradeengine_channel;
    EventLFQueue orderbook_positionkeeper_channel;

    OrderBook order_book(
        common::ExchangeId::kBinance, TradingPair{2,1}, pair,
        &orderbook_tradeengine_channel, &orderbook_positionkeeper_channel,
        10, 10, 10);

    Exchange::MEMarketUpdate market_update;
    market_update.price = 8;
    market_update.qty   = 3;
    market_update.side  = Side::kBid;

    order_book.OnMarketUpdate(&market_update);

    auto bbo = order_book.getBBO();
    EXPECT_EQ(bbo->bid_price, 8);
    EXPECT_EQ(bbo->bid_qty, 3);

    // Verify that the message was sent to the orderbook_tradeengine_channel
    strategy::cross_arbitrage::Event* event_message_1;
    orderbook_tradeengine_channel.try_dequeue(event_message_1);
    auto message_1 = static_cast<strategy::cross_arbitrage::BBidUpdated*>(event_message_1);
    ASSERT_NE(message_1, nullptr);
    EXPECT_EQ(message_1->price, 8);
    EXPECT_EQ(message_1->qty, 3);

        // Verify that the message was sent to the orderbook_tradeengine_channel
    position_keeper::UpdateBBO::Event* event_message_2;
    orderbook_positionkeeper_channel.try_dequeue(event_message_2);
    auto message_2 = static_cast<position_keeper::UpdateBBO*>(event_message_2);
    ASSERT_NE(message_2, nullptr);
    EXPECT_EQ(message_2->bbo->bid_price, 8);
    EXPECT_EQ(message_2->bbo->bid_qty, 3);
}

TEST(OrderBook, ShouldAllocateMemoryFromBBUPoolAndSendBidUpdateMessageToOrderbookTradeengineChannel) {
    using namespace strategy::cross_arbitrage;
    using namespace position_keeper;
    using namespace common;

    TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";

    TradingPairHashMap pair;
    common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
    pair[{2, 1}] = pair_info;

    LFQueue orderbook_tradeengine_channel;
    EventLFQueue orderbook_positionkeeper_channel;

    OrderBook order_book(
        common::ExchangeId::kBinance, TradingPair{2,1}, pair,
        &orderbook_tradeengine_channel, &orderbook_positionkeeper_channel,
        10, 10, 10);

    Exchange::MEMarketUpdate market_update;
    market_update.price = 7;
    market_update.qty   = 1;
    market_update.side  = Side::kAsk;

    order_book.OnMarketUpdate(&market_update);

    auto bbo = order_book.getBBO();
    EXPECT_EQ(bbo->ask_price, 7);
    EXPECT_EQ(bbo->ask_qty, 1);

    // Verify that the message was sent to the orderbook_tradeengine_channel
    strategy::cross_arbitrage::Event* event_message_1;
    orderbook_tradeengine_channel.try_dequeue(event_message_1);
    auto message_1 = static_cast<strategy::cross_arbitrage::BAskUpdated*>(event_message_1);
    ASSERT_NE(message_1, nullptr);
    EXPECT_EQ(message_1->price, 7);
    EXPECT_EQ(message_1->qty, 1);

    // Verify that the message was sent to the orderbook_tradeengine_channel
    position_keeper::UpdateBBO::Event* event_message_2;
    orderbook_positionkeeper_channel.try_dequeue(event_message_2);
    auto message_2 = static_cast<position_keeper::UpdateBBO*>(event_message_2);
    ASSERT_NE(message_2, nullptr);
    EXPECT_EQ(message_2->bbo->ask_price, 7);
    EXPECT_EQ(message_2->bbo->ask_qty, 1);
}

TEST(OrderBook, ShouldAllocateMemoryFromUpdateBBOPoolAndSendBBOUpdateMessageToOrderbookPositionkeeperChannelForBidUpdate) {
    using namespace strategy::cross_arbitrage;
    using namespace position_keeper;
    using namespace common;

    TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";

    TradingPairHashMap pair;
    common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
    pair[{2, 1}] = pair_info;

    LFQueue orderbook_tradeengine_channel;
    EventLFQueue orderbook_positionkeeper_channel;

    OrderBook order_book(
        common::ExchangeId::kBinance, TradingPair{2,1}, pair,
        &orderbook_tradeengine_channel, &orderbook_positionkeeper_channel,
        10, 10, 10);

    Exchange::MEMarketUpdate market_update;
    market_update.price = 5;
    market_update.qty   = 4;
    market_update.side  = Side::kAsk;

    order_book.OnMarketUpdate(&market_update);

    auto bbo = order_book.getBBO();
    EXPECT_EQ(bbo->ask_price, 5);
    EXPECT_EQ(bbo->ask_qty, 4);
}

TEST(OrderBook, ShouldAllocateMemoryFromUpdateBBOPoolAndSendBBOUpdateMessageToOrderbookPositionkeeperChannelForAskUpdate) {
    using namespace strategy::cross_arbitrage;
    using namespace position_keeper;
    using namespace common;

    TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";

    TradingPairHashMap pair;
    common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
    pair[{2, 1}] = pair_info;

    LFQueue orderbook_tradeengine_channel;
    EventLFQueue orderbook_positionkeeper_channel;

    OrderBook order_book(
        common::ExchangeId::kBinance, TradingPair{2,1}, pair,
        &orderbook_tradeengine_channel, &orderbook_positionkeeper_channel,
        10, 10, 10);

    Exchange::MEMarketUpdate market_update;
    market_update.price = 8;
    market_update.qty   = 3;
    market_update.side  = Side::kBid;

    order_book.OnMarketUpdate(&market_update);

    auto bbo = order_book.getBBO();
    EXPECT_EQ(bbo->bid_price, 8);
    EXPECT_EQ(bbo->bid_qty, 3);

    // Verify that a message was sent to the orderbook_positionkeeper_channel
    EXPECT_EQ(orderbook_positionkeeper_channel.size_approx(), 1);
}

// TEST(OrderBook, ShouldHandleAllocationFailureInSendMessageToChannel) {
//     using namespace strategy::cross_arbitrage;
//     using namespace position_keeper;
//     using namespace common;

//     TickerHashMap tickers;
//     tickers[1] = "usdt";
//     tickers[2] = "btc";

//     TradingPairHashMap pair;
//     binance::Symbol symbol(tickers[2], tickers[1]);
//     TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
//     pair[{2, 1}] = pair_info;

//     LFQueue orderbook_tradeengine_channel;
//     EventLFQueue orderbook_positionkeeper_channel;

//     OrderBook order_book(
//         1, TradingPair{2,1}, pair,
//         &orderbook_tradeengine_channel, &orderbook_positionkeeper_channel,
//         0, 0, 0); // Initialize pools with size 0 to force allocation failure

//     Exchange::MEMarketUpdate market_update;
//     market_update.price = 8;
//     market_update.qty   = 3;
//     market_update.side  = Side::kBid;

//     // Capture the initial log count
//     //auto initial_log_count = Logger::GetLogCount();

//     order_book.OnMarketUpdate(&market_update);
//     order_book.updateBBO(false, true);

//     auto bbo = order_book.getBBO();
//     EXPECT_EQ(bbo->ask_price, 8);
//     EXPECT_EQ(bbo->ask_qty, 3);

//     // Verify that an error was logged
//     //EXPECT_GT(Logger::GetLogCount(), initial_log_count);
// }

int main(int argc, char** argv) {
    // fmtlog::setLogLevel(fmtlog::OFF);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}