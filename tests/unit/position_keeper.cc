#include "aot/strategy/position_keeper.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unordered_map>

#include "aot/client_response.h"

using namespace Trading;
using namespace exchange;
using namespace common;
using namespace Exchange;

class MockClientResponse : public IResponse {
  public:
    MOCK_METHOD(ExchangeId, GetExchangeId, (), (const, noexcept, override));
    MOCK_METHOD(TradingPair, GetTradingPair, (), (const, noexcept, override));
    MOCK_METHOD(Side, GetSide, (), (const, noexcept, override));
    MOCK_METHOD(Qty, GetExecQty, (), (const, noexcept, override));
    MOCK_METHOD(Price, GetPrice, (), (const, noexcept, override));
    MOCK_METHOD(std::string, ToString, (), (const, noexcept, override));
};

class PositionKeeperTest : public ::testing::Test {
  protected:
    exchange::PositionKeeper positionKeeper;
};

TEST(PositionKeeperTest,
     ShouldCorrectlyAddFillWhenClientResponseHasValidExchangeId) {
    MockClientResponse mockResponse;
    Trading::PositionKeeper keeper;
    exchange::PositionKeeper::ExchangePositionKeeper map{{1, &keeper}};
    exchange::PositionKeeper positionKeeper(map);

    EXPECT_CALL(mockResponse, GetExchangeId()).WillOnce(testing::Return(1));
    EXPECT_CALL(mockResponse, GetTradingPair())
        .WillOnce(testing::Return(common::TradingPair{2, 1}));
    EXPECT_CALL(mockResponse, GetSide())
        .WillOnce(testing::Return(common::Side::BUY));
    EXPECT_CALL(mockResponse, GetExecQty()).WillOnce(testing::Return(1.0));
    EXPECT_CALL(mockResponse, GetPrice()).WillOnce(testing::Return(50000.0));
    EXPECT_CALL(mockResponse, ToString()).WillRepeatedly(testing::Return(""));

    positionKeeper.AddFill(&mockResponse);

    auto positionInfo =
        positionKeeper.GetPositionInfo(1, common::TradingPair{2, 1});
    ASSERT_NE(positionInfo, nullptr);
    EXPECT_EQ(positionInfo->position, 1);
    EXPECT_EQ(positionInfo->volume, 1.0);
    EXPECT_EQ(positionInfo->open_vwap[common::sideToIndex(common::Side::BUY)],
              50000.0);
}

TEST(PositionKeeperTest,
     ShouldHandleNullClientResponseWithoutThrowingException) {
    Trading::PositionKeeper keeper;
    exchange::PositionKeeper::ExchangePositionKeeper map{{1, &keeper}};
    exchange::PositionKeeper positionKeeper(map);
    EXPECT_NO_THROW(positionKeeper.AddFill(nullptr));
}

TEST(PositionKeeperTest, ShouldNotModifyPositionIfClientResponseIsNull) {
    Trading::PositionKeeper keeper;
    exchange::PositionKeeper::ExchangePositionKeeper map{{1, &keeper}};
    exchange::PositionKeeper positionKeeper(map);
    EXPECT_NO_THROW(positionKeeper.AddFill(nullptr));

    // Verify that position_ remains unchanged
    auto positionInfo =
        positionKeeper.GetPositionInfo(1, common::TradingPair{2, 1});
    EXPECT_EQ(positionInfo->position, 0);
    EXPECT_EQ(positionInfo->volume, 0.0);
    EXPECT_EQ(positionInfo->real_pnl, 0.0);
    EXPECT_EQ(positionInfo->unreal_pnl, 0.0);
    EXPECT_EQ(positionInfo->total_pnl, 0.0);
}

TEST(PositionKeeperTest,
     ShouldHandleClientResponseWithInvalidExchangeIdGracefully) {
    MockClientResponse mockResponse;
    Trading::PositionKeeper keeper;
    exchange::PositionKeeper::ExchangePositionKeeper map{{1, &keeper}};
    exchange::PositionKeeper positionKeeper(map);

    EXPECT_CALL(mockResponse, GetExchangeId())
        .WillOnce(testing::Return(static_cast<common::ExchangeId>(-1)));
    EXPECT_CALL(mockResponse, GetTradingPair()).Times(0);
    EXPECT_CALL(mockResponse, GetSide()).Times(0);
    EXPECT_CALL(mockResponse, GetExecQty()).Times(0);
    EXPECT_CALL(mockResponse, GetPrice()).Times(0);
    EXPECT_CALL(mockResponse, ToString()).Times(testing::AnyNumber());

    EXPECT_NO_THROW(positionKeeper.AddFill(&mockResponse));
}

TEST(PositionKeeperTest,
     ShouldCorrectlyAddFillWhenClientResponseHasBoundaryValueExchangeId) {
    MockClientResponse mockResponse;
    Trading::PositionKeeper keeper;
    exchange::PositionKeeper::ExchangePositionKeeper map{{1, &keeper}};
    exchange::PositionKeeper positionKeeper(map);

    EXPECT_CALL(mockResponse, GetExchangeId()).WillOnce(testing::Return(1));
    EXPECT_CALL(mockResponse, GetTradingPair())
        .WillOnce(testing::Return(common::TradingPair{3, 1}));
    EXPECT_CALL(mockResponse, GetSide())
        .WillOnce(testing::Return(common::Side::SELL));
    EXPECT_CALL(mockResponse, GetExecQty()).WillOnce(testing::Return(2.0));
    EXPECT_CALL(mockResponse, GetPrice()).WillOnce(testing::Return(3000.0));
    EXPECT_CALL(mockResponse, ToString()).Times(testing::AnyNumber());

    positionKeeper.AddFill(&mockResponse);

    auto positionInfo =
        positionKeeper.GetPositionInfo(1, common::TradingPair{3, 1});
    ASSERT_NE(positionInfo, nullptr);
    EXPECT_EQ(positionInfo->position, -2);
    EXPECT_EQ(positionInfo->volume, 2.0);
    EXPECT_EQ(positionInfo->open_vwap[common::sideToIndex(common::Side::SELL)],
              6000.0);
}

TEST(PositionKeeperTest,
     ShouldCorrectlyAddFillWhenClientResponseHasMinimumPossibleExchangeId) {
    MockClientResponse mockResponse;
    Trading::PositionKeeper keeper;
    exchange::PositionKeeper::ExchangePositionKeeper map{{0, &keeper}};
    exchange::PositionKeeper positionKeeper(map);

    EXPECT_CALL(mockResponse, GetExchangeId()).WillOnce(testing::Return(0));
    EXPECT_CALL(mockResponse, GetTradingPair())
        .WillOnce(testing::Return(common::TradingPair{4, 1}));
    EXPECT_CALL(mockResponse, GetSide())
        .WillOnce(testing::Return(common::Side::BUY));
    EXPECT_CALL(mockResponse, GetExecQty()).WillOnce(testing::Return(5));
    EXPECT_CALL(mockResponse, GetPrice()).WillOnce(testing::Return(200));
    EXPECT_CALL(mockResponse, ToString()).Times(testing::AnyNumber());

    positionKeeper.AddFill(&mockResponse);

    auto positionInfo =
        positionKeeper.GetPositionInfo(0, common::TradingPair{4, 1});
    ASSERT_NE(positionInfo, nullptr);
    EXPECT_EQ(positionInfo->position, 5);
    EXPECT_EQ(positionInfo->volume, 5.0);
    EXPECT_EQ(positionInfo->open_vwap[common::sideToIndex(common::Side::BUY)],
              1000);
}

TEST(PositionKeeperTest,
     ShouldCorrectlyUpdatePositionWhenClientResponseHasLargeExchangeId) {
    MockClientResponse mockResponse;
    Trading::PositionKeeper keeper;
    exchange::PositionKeeper::ExchangePositionKeeper map{{999999, &keeper}};
    exchange::PositionKeeper positionKeeper(map);

    EXPECT_CALL(mockResponse, GetExchangeId())
        .WillOnce(testing::Return(static_cast<common::ExchangeId>(999999)));
    EXPECT_CALL(mockResponse, GetTradingPair())
        .WillOnce(testing::Return(common::TradingPair{2, 1}));
    EXPECT_CALL(mockResponse, GetSide())
        .WillOnce(testing::Return(common::Side::BUY));
    EXPECT_CALL(mockResponse, GetExecQty()).WillOnce(testing::Return(1.0));
    EXPECT_CALL(mockResponse, GetPrice()).WillOnce(testing::Return(50000.0));
    EXPECT_CALL(mockResponse, ToString()).Times(testing::AnyNumber());

    positionKeeper.AddFill(&mockResponse);

    auto positionInfo = positionKeeper.GetPositionInfo(
        static_cast<common::ExchangeId>(999999), common::TradingPair{2, 1});
    ASSERT_NE(positionInfo, nullptr);
    EXPECT_EQ(positionInfo->position, 1);
    EXPECT_EQ(positionInfo->volume, 1.0);
    EXPECT_EQ(positionInfo->open_vwap[common::sideToIndex(common::Side::BUY)],
              50000.0);
}

TEST(PositionKeeperTest,
     ShouldHandleClientResponseWithMaximumPossibleExchangeIdWithoutOverflow) {
    MockClientResponse mockResponse;
    Trading::PositionKeeper keeper;
    exchange::PositionKeeper::ExchangePositionKeeper map{
        {static_cast<common::ExchangeId>(std::numeric_limits<int>::max() - 1),
         &keeper}};
    exchange::PositionKeeper positionKeeper(map);

    EXPECT_CALL(mockResponse, GetExchangeId())
        .WillOnce(testing::Return(
            static_cast<common::ExchangeId>(std::numeric_limits<int>::max() - 1)));
    EXPECT_CALL(mockResponse, GetTradingPair())
        .WillOnce(testing::Return(common::TradingPair{2, 1}));
    EXPECT_CALL(mockResponse, GetSide())
        .WillOnce(testing::Return(common::Side::BUY));
    EXPECT_CALL(mockResponse, GetExecQty()).WillOnce(testing::Return(1.0));
    EXPECT_CALL(mockResponse, GetPrice()).WillOnce(testing::Return(50000.0));
    EXPECT_CALL(mockResponse, ToString()).Times(testing::AnyNumber());

    positionKeeper.AddFill(&mockResponse);

    auto positionInfo = positionKeeper.GetPositionInfo(
        static_cast<common::ExchangeId>(std::numeric_limits<common::ExchangeId>::max() - 1),
        common::TradingPair{2, 1});
    ASSERT_NE(positionInfo, nullptr);
    EXPECT_EQ(positionInfo->position, 1);
    EXPECT_EQ(positionInfo->volume, 1.0);
    EXPECT_EQ(positionInfo->open_vwap[common::sideToIndex(common::Side::BUY)],
              50000.0);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
