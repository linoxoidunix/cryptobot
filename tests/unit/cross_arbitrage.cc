#include "cmath"

#include "gtest/gtest.h"

#include "aot/common/types.h"
#include "aot/Logger.h"
#include "aot/strategy/cross_arbitrage/signals.h"


TEST(CrossArbitrageEvent, Create) {
    using namespace strategy::cross_arbitrage;
    using namespace Common;
    BidUpdated bid_updated(TradingPair{2,1}, 100.0, 14.0);
    EXPECT_EQ(bid_updated.GetType(),EventType::kBidUpdate);
    EXPECT_EQ(bid_updated.price,100);
    EXPECT_EQ(bid_updated.qty,14);
    EXPECT_EQ(bid_updated.trading_pair,(TradingPair{2,1}));

}



int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}