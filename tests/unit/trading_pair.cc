#include <iostream>

#include "gtest/gtest.h"

#include "aot/Logger.h"

#include "aot/common/types.h"
#include "aot/Binance.h"
#include "aot/Bybit.h"

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

TEST(TradingPairInfo, Access_3_bybit) {
    using namespace Common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";

    TradingPairHashMap pair;
    bybit::Symbol symbol("btc", "usdt");
    TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
    pair[{1, 2}] = pair_info;
    auto trading_pair = TradingPair{1,2};
    EXPECT_EQ(2, (pair[trading_pair].price_precission));
    EXPECT_EQ(5, (pair[trading_pair].qty_precission));
    EXPECT_EQ("BTCUSDT", (pair[trading_pair].trading_pairs));
}

TEST(TradingPairInfo, Access_4_binance) {
    using namespace Common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";

    TradingPairHashMap pair;
    binance::Symbol symbol("btc", "usdt");
    TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
    pair[{1, 2}] = pair_info;
    auto trading_pair = TradingPair{1,2};
    EXPECT_EQ(2, (pair[trading_pair].price_precission));
    EXPECT_EQ(5, (pair[trading_pair].qty_precission));
    EXPECT_EQ("btcusdt", (pair[trading_pair].trading_pairs));
}

TEST(TradingPairReverseHashMap, compare_with_string_view) {
    using namespace Common;
    TickerHashMap tickers;
    tickers[1] = "btc";
    tickers[2] = "usdt";
    TradingPair tr_pair{1, 2};
    TradingPairHashMap pair;
    binance::Symbol symbol("btc", "usdt");
    TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
    pair[{1, 2}] = pair_info;
    TradingPairReverseHashMap pair_reverse;
    pair_reverse[std::string(symbol.ToString())] = tr_pair;
    auto trading_pair = TradingPair{1,2};
    std::string_view s_v= "btcusdt";
    EXPECT_EQ(tr_pair, (pair_reverse.find(s_v)->second));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}