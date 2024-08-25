#include <iostream>

#include "gtest/gtest.h"

#define FMT_HEADER_ONLY
#include "aot/third_party/fmt/core.h"

#include "aot/Exchange.h"

TEST(TradingPairInfo, Access_1) {
    using namespace Common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";

    TradingPairHashMap pair;
    TradingPairInfo pair_info{"BTCUSDT", 2, 5};
    pair[{1, 2}] = pair_info;

    EXPECT_EQ(2, (pair[{1, 2}].price_precission));
    EXPECT_EQ(5, (pair[{1, 2}].qty_precission));
}

TEST(TradingPairInfo, Access_2) {
    using namespace Common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";

    TradingPairHashMap pair;
    TradingPairInfo pair_info{"BTCUSDT", 2, 5};
    pair[{1, 2}] = pair_info;
    auto trading_pair = TradingPair{1,2};
    EXPECT_EQ(2, (pair[trading_pair].price_precission));
    EXPECT_EQ(5, (pair[trading_pair].qty_precission));
}

TEST(TradingPairInfo, Access_2) {
    using namespace Common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";

    TradingPairHashMap pair;
    TradingPairInfo pair_info{"BTCUSDT", 2, 5};
    pair[{1, 2}] = pair_info;
    auto trading_pair = TradingPair{1,2};
    EXPECT_EQ(2, (pair[trading_pair].price_precission));
    EXPECT_EQ(5, (pair[trading_pair].qty_precission));
}
