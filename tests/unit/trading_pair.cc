#include <iostream>

#include "gtest/gtest.h"

#include "aot/Logger.h"

#include "aot/common/types.h"
#include "aot/Binance.h"
#include "aot/Bybit.h"

TEST(TradingPairInfo, Access_1) {
    using namespace common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";

    TradingPairHashMap pair;
    TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_query_request = "BTCUSDT"};
    pair[{1, 2}] = pair_info;

    EXPECT_EQ(2, (pair[{1, 2}].price_precission));
    EXPECT_EQ(5, (pair[{1, 2}].qty_precission));
}

TEST(TradingPairInfo, Access_2) {
    using namespace common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";

    TradingPairHashMap pair;
    TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_query_request = "BTCUSDT"};
    pair[{1, 2}] = pair_info;
    auto trading_pair = TradingPair{1,2};
    EXPECT_EQ(2, (pair[trading_pair].price_precission));
    EXPECT_EQ(5, (pair[trading_pair].qty_precission));
}

TEST(TradingPairInfo, Access_3_bybit) {
    using namespace common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";

    TradingPairHashMap pair;
    TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_query_request = "BTCUSDT"};
    pair[{1, 2}] = pair_info;
    auto trading_pair = TradingPair{1,2};
    EXPECT_EQ(2, (pair[trading_pair].price_precission));
    EXPECT_EQ(5, (pair[trading_pair].qty_precission));
    EXPECT_EQ("BTCUSDT", (pair[trading_pair].https_query_request));
}

TEST(TradingPairInfo, Access_4_binance) {
    using namespace common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";

    TradingPairHashMap pair;
    
     TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_query_request = "BTCUSDT"};
    pair[{1, 2}] = pair_info;
    auto trading_pair = TradingPair{1,2};
    EXPECT_EQ(2, (pair[trading_pair].price_precission));
    EXPECT_EQ(5, (pair[trading_pair].qty_precission));
    EXPECT_EQ("BTCUSDT", (pair[trading_pair].https_query_request));
}

TEST(TradingPairReverseHashMap, compare_with_string_view) {
    using namespace common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";
    TradingPair tr_pair{1, 2};
    TradingPairHashMap pair;
        TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_query_request = "BTCUSDT"};
    pair[{1, 2}] = pair_info;
    TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pair);
    pair_reverse["BTCUSDT"] = tr_pair;
    std::string_view s_v= "BTCUSDT";
    EXPECT_EQ(tr_pair, (pair_reverse.find(s_v)->second));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}