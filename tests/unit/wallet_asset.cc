#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "aot/wallet_asset.h"
#include "aot/client_response.h"
#include "aot/common/types.h"

using ::testing::Return;
using ::testing::NiceMock;
using namespace common;
using namespace Exchange;

class MockResponse : public IResponse {
  public:
    MOCK_METHOD(ExchangeId, GetExchangeId, (), (const, noexcept, override));
    MOCK_METHOD(TradingPair, GetTradingPair, (), (const, noexcept, override));
    MOCK_METHOD(Side, GetSide, (), (const, noexcept, override));
    MOCK_METHOD(Qty, GetExecQty, (), (const, noexcept, override));
    MOCK_METHOD(Price, GetPrice, (), (const, noexcept, override));
    MOCK_METHOD(std::string, ToString, (), (const, noexcept, override));
    MOCK_METHOD(void, Deallocate, (), (override));
    MOCK_METHOD(Exchange::ClientResponseType, GetType, (), (const override));
    MOCK_METHOD(common::OrderId, GetOrderId, (), (const override));
};

class WalletTest : public ::testing::Test {
protected:
    testing::Wallet wallet;
    NiceMock<MockResponse> mockResponse;
    void SetUp() override{};
    void TearDown() override{};
};

TEST_F(WalletTest, ShouldCorrectlyReserveTickerWhenSufficientQuantityIsAvailable) {
    common::TickerId ticker = 1;
    common::Qty qty = 100;
    common::OrderId order_id = 1;
    wallet.insert({ticker, 200}); // Insert initial quantity

    bool result = wallet.Reserve(order_id, ticker, qty);

    EXPECT_TRUE(result);
    EXPECT_EQ(wallet.at(ticker), 100); // Check if the quantity is correctly reserved
    EXPECT_EQ(wallet.GetReserves()[order_id].reserved_ticker, ticker);
    EXPECT_EQ(wallet.GetReserves()[order_id].reserved_qty, qty);
}

TEST_F(WalletTest, ShouldInitializeTickerWhenItDoesNotExistInTheWallet) {
    common::TickerId ticker = 2;
    common::Qty qty = 50;
    common::OrderId order_id = 2;

    EXPECT_EQ(wallet.count(ticker), 0); // Ensure ticker does not exist initially
    wallet[ticker] = 50;
    bool result = wallet.Reserve(order_id, ticker, qty);

    EXPECT_TRUE(result);
    EXPECT_EQ(wallet.at(ticker), 0); // Check if the ticker is initialized and quantity is reserved
    EXPECT_EQ(wallet.GetReserves()[order_id].reserved_ticker, ticker);
    EXPECT_EQ(wallet.GetReserves()[order_id].reserved_qty, qty);
}

TEST_F(WalletTest, ShouldEraseReservationOnCancelRejectedResponse) {
    common::TickerId ticker = 1;
    common::Qty qty = 100;
    common::OrderId order_id = 1;

    wallet.insert({ticker, 200}); // Insert initial quantity
    wallet.Reserve(order_id, ticker, qty); // Reserve some quantity

    NiceMock<MockResponse> mock_response;
    ON_CALL(mock_response, GetType()).WillByDefault(Return(Exchange::ClientResponseType::CANCEL_REJECTED));
    ON_CALL(mock_response, GetOrderId()).WillByDefault(Return(order_id));

    wallet.Update(&mock_response);

    EXPECT_EQ(wallet.at(ticker), 100); // Check if the quantity is correctly restored
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0); // Check if the reservation is erased
}

TEST_F(WalletTest, ShouldEraseReservationOnAcceptedResponse) {
    common::TickerId ticker = 1;
    common::Qty qty = 100;
    common::OrderId order_id = 1;

    wallet.insert({ticker, 200}); // Insert initial quantity
    wallet.Reserve(order_id, ticker, qty); // Reserve some quantity

    EXPECT_EQ(wallet.at(ticker), 100); // Ensure quantity is reserved

    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::ACCEPTED));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id));

    wallet.Update(&mockResponse);

    EXPECT_EQ(wallet.at(ticker), 100); // Quantity should remain the same
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0); // Reservation should be erased
}

TEST_F(WalletTest, ShouldUpdateWalletAndEraseReservationOnCanceledResponse) {
    common::TickerId ticker = 1;
    common::Qty qty = 100;
    common::OrderId order_id = 1;

    wallet.insert({ticker, 200}); // Insert initial quantity
    wallet.Reserve(order_id, ticker, qty); // Reserve some quantity

    EXPECT_EQ(wallet.at(ticker), 100); // Ensure quantity is reserved

    EXPECT_CALL(mockResponse, GetType()).WillOnce(Return(Exchange::ClientResponseType::CANCELED));
    EXPECT_CALL(mockResponse, GetOrderId()).WillOnce(Return(order_id));

    wallet.Update(&mockResponse);

    EXPECT_EQ(wallet.at(ticker), 200); // Quantity should be restored
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0); // Reservation should be erased
}

TEST_F(WalletTest, ShouldReturnReservedQuantityToWalletOnInvalidResponse) {
    common::TickerId ticker = 1;
    common::Qty qty = 100;
    common::OrderId order_id = 1;

    wallet.insert({ticker, 200}); // Insert initial quantity
    wallet.Reserve(order_id, ticker, qty); // Reserve some quantity

    EXPECT_EQ(wallet.at(ticker), 100); // Ensure quantity is reserved

    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::INVALID));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id));

    wallet.Update(&mockResponse);

    EXPECT_EQ(wallet.at(ticker), 200); // Quantity should be restored
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0); // Reservation should be erased
}

TEST_F(WalletTest, ShouldUpdateWalletAndEraseReservationOnFilledBuyResponse) {
    common::TickerId ticker = 1;
    common::Qty qty = 100;
    common::OrderId order_id = 1;

    wallet.insert({ticker, 200}); // Insert initial quantity
    wallet.Reserve(order_id, ticker, qty); // Reserve some quantity

    EXPECT_EQ(wallet.at(ticker), 100); // Ensure quantity is reserved

    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::FILLED));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id));
    EXPECT_CALL(mockResponse, GetSide())
        .WillOnce(Return(common::Side::BUY));
    EXPECT_CALL(mockResponse, GetTradingPair())
        .WillOnce(Return(common::TradingPair{ticker, 0}));
    EXPECT_CALL(mockResponse, GetExecQty())
        .WillOnce(Return(qty));

    wallet.Update(&mockResponse);

    EXPECT_EQ(wallet.at(ticker), 200); // Quantity should be updated
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0); // Reservation should be erased
}

TEST_F(WalletTest, ShouldUpdateWalletAndEraseReservationOnFilledSellResponse) {
    common::TickerId ticker = 1;
    common::Qty qty = 100;
    common::OrderId order_id = 1;

    wallet.insert({ticker, 200}); // Insert initial quantity
    wallet.Reserve(order_id, ticker, qty); // Reserve some quantity

    EXPECT_EQ(wallet.at(ticker), 100); // Ensure quantity is reserved

    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::FILLED));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id));
    EXPECT_CALL(mockResponse, GetSide())
        .WillOnce(Return(common::Side::SELL));
    EXPECT_CALL(mockResponse, GetTradingPair())
        .WillOnce(Return(common::TradingPair{0, ticker}));
    EXPECT_CALL(mockResponse, GetExecQty())
        .WillOnce(Return(qty));

    wallet.Update(&mockResponse);

    EXPECT_EQ(wallet.at(ticker), 200); // Quantity should be updated
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0); // Reservation should be erased
}

TEST_F(WalletTest, ShouldNotUpdateWalletWhenFilledResponseHasZeroExecutionQuantity) {
    common::TickerId ticker = 1;
    common::Qty qty = 100;
    common::OrderId order_id = 1;

    wallet.insert({ticker, 200}); // Insert initial quantity
    wallet.Reserve(order_id, ticker, qty); // Reserve some quantity

    EXPECT_EQ(wallet.at(ticker), 100); // Ensure quantity is reserved

    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::FILLED));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id));
    EXPECT_CALL(mockResponse, GetSide())
        .WillOnce(Return(common::Side::BUY));
    EXPECT_CALL(mockResponse, GetTradingPair())
        .WillOnce(Return(common::TradingPair{ticker, 0}));
    EXPECT_CALL(mockResponse, GetExecQty())
        .WillOnce(Return(0)); // Zero execution quantity

    wallet.Update(&mockResponse);

    EXPECT_EQ(wallet.at(ticker), 100); // Quantity should remain unchanged
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0); // Reservation should be erased
}

TEST_F(WalletTest, ShouldCorrectlyHandleMultipleConsecutiveUpdatesWithDifferentResponseTypes) {
    common::TickerId ticker = 1;
    common::Qty qty = 100;
    common::OrderId order_id1 = 1;
    common::OrderId order_id2 = 2;

    wallet.insert({ticker, 300}); // Insert initial quantity
    wallet.Reserve(order_id1, ticker, qty); // Reserve some quantity for order_id1
    wallet.Reserve(order_id2, ticker, qty); // Reserve some quantity for order_id2

    EXPECT_EQ(wallet.at(ticker), 100); // Ensure quantity is reserved

    // First update: ACCEPTED response for order_id1
    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::ACCEPTED));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id1));

    wallet.Update(&mockResponse);

    EXPECT_EQ(wallet.at(ticker), 100); // Quantity should remain the same
    EXPECT_EQ(wallet.GetReserves().count(order_id1), 0); // Reservation for order_id1 should be erased

    // Second update: FILLED response for order_id2 with BUY side
    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::FILLED));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id2));
    EXPECT_CALL(mockResponse, GetSide())
        .WillOnce(Return(common::Side::BUY));
    EXPECT_CALL(mockResponse, GetTradingPair())
        .WillOnce(Return(common::TradingPair{ticker, 0}));
    EXPECT_CALL(mockResponse, GetExecQty())
        .Times(2).WillRepeatedly(Return(qty));

    wallet.Update(&mockResponse);

    EXPECT_EQ(wallet.at(ticker), 200); // Quantity should be updated
    EXPECT_EQ(wallet.GetReserves().count(order_id2), 0); // Reservation for order_id2 should be erased

    // Third update: INVALID response for order_id2
    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::INVALID));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id2));

    wallet.Update(&mockResponse);

    EXPECT_EQ(wallet.at(ticker), 200); // Quantity should remain the same
    EXPECT_EQ(wallet.GetReserves().count(order_id2), 0); // Reservation for order_id2 should remain erased
}


TEST_F(WalletTest, ShouldHandleEmptyReservesMapGracefully) {
    common::TickerId ticker = 1;
    common::OrderId order_id = 1;

    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::ACCEPTED));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id));

    // Ensure the reserves map is empty
    EXPECT_EQ(wallet.GetReserves().size(), 0);

    // Call Update with an ACCEPTED response type
    wallet.Update(&mockResponse);

    // Check that the reserves map is still empty and no exceptions were thrown
    EXPECT_EQ(wallet.GetReserves().size(), 0);
}

TEST_F(WalletTest, ShouldHandleResponseWithNonExistentOrderIdInReserves) {
    common::TickerId ticker = 1;
    common::OrderId order_id = 999; // Non-existent order ID

    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::ACCEPTED));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id));

    // Ensure the reserves map does not contain the order ID
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0);

    // Call Update with a response containing a non-existent order ID
    wallet.Update(&mockResponse);

    // Check that the reserves map is still empty and no exceptions were thrown
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0);
}

TEST_F(WalletTest, ShouldNotModifyWalletWhenResponseTypeIsInvalidAndOrderIdDoesNotExistInReserves) {
    common::TickerId ticker = 1;
    common::OrderId order_id = 999; // Non-existent order ID

    wallet.insert({ticker, 200}); // Insert initial quantity

    EXPECT_CALL(mockResponse, GetType())
        .WillOnce(Return(Exchange::ClientResponseType::INVALID));
    EXPECT_CALL(mockResponse, GetOrderId())
        .WillOnce(Return(order_id));

    // Ensure the reserves map does not contain the order ID
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0);

    // Capture the initial state of the wallet
    auto initial_qty = wallet.at(ticker);

    // Call Update with a response containing a non-existent order ID
    wallet.Update(&mockResponse);

    // Check that the wallet quantity remains unchanged
    EXPECT_EQ(wallet.at(ticker), initial_qty);
    // Check that the reserves map is still empty and no exceptions were thrown
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0);
}

TEST_F(WalletTest, ShouldNotReserveTickerWhenInsufficientQuantityIsAvailable) {
    common::TickerId ticker = 1;
    common::Qty qty = 100;
    common::OrderId order_id = 1;

    wallet.insert({ticker, 50}); // Insert initial quantity less than required

    bool result = wallet.Reserve(order_id, ticker, qty);

    EXPECT_FALSE(result);
    EXPECT_EQ(wallet.at(ticker), 50); // Check if the quantity remains unchanged
    EXPECT_EQ(wallet.GetReserves().count(order_id), 0); // Ensure no reservation was made
}



int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


