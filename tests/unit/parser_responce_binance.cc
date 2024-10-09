#include <cmath>
#include <gtest/gtest.h>

#include "aot/Binance.h" // Adjust this include based on your project structure
#include "aot/common/types.h"

class ParserResponseTests : public ::testing::Test {
protected:
    common::TradingPairHashMap pairs_;
    common::TradingPairReverseHashMap pairs_reverse_;
    binance::detail::FamilyLimitOrder::ParserResponse parser{pairs_, pairs_reverse_}; // The parser object to test

    void SetUp() override {
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
        pairs_[{2, 1}] = pair_info;
        pairs_reverse_[pair_info.https_json_request] = common::TradingPair{2,1}; 
    }

    void TearDown() override {
        // Any cleanup after each test can go here
    }
};

// // Test case for a successful parsing scenario
TEST_F(ParserResponseTests, SuccessfulParsing) {
    std::string_view json_response = R"({
        "symbol": "BTCUSDT",
        "clientOrderId": "12345",
        "status": "NEW",
        "price": "10000.0",
        "cummulativeQuoteQty": "0.1",
        "side": "BUY",
        "executedQty": "0.1",
        "origQty": "0.2"
    })";

    Exchange::MEClientResponse response = parser.Parse(json_response);
    
    ASSERT_EQ(response.order_id, 12345);
    ASSERT_EQ(response.trading_pair, (common::TradingPair{2,1}));
    ASSERT_EQ(response.side, common::Side::BUY);
    ASSERT_EQ(response.exec_qty, 0.1 * std::pow(10, /*qty precision*/ 5)); // Adjust precision
}

// // Test case for missing "symbol"
TEST_F(ParserResponseTests, MissingSymbol) {
    std::string_view json_response = R"({
        "clientOrderId": "12345",
        "status": "NEW"
    })";
    Exchange::MEClientResponse response = parser.Parse(json_response);
    ASSERT_EQ(response.order_id, (common::kOrderIdInvalid));
}

// Test case for missing "status"
TEST_F(ParserResponseTests, MissingStatus) {
    std::string_view json_response = R"({
        "symbol": "BTCUSDT",
        "clientOrderId": "12345"
    })"; 
    Exchange::MEClientResponse response = parser.Parse(json_response);
    ASSERT_EQ(response.order_id, (common::kOrderIdInvalid));
}

// Test case for invalid "price"
TEST_F(ParserResponseTests, InvalidPrice) {
    std::string_view json_response = R"({
        "symbol": "BTCUSDT",
        "clientOrderId": "12345",
        "status": "NEW",
        "price": "invalid",  // Invalid price
        "side": "BUY",
        "executedQty": "0.1",
        "origQty": "0.2"
    })";
    Exchange::MEClientResponse response = parser.Parse(json_response);
    ASSERT_EQ(response.price, common::kPriceInvalid); // Should be 0 on error
}

// Test case for successfully filled order
TEST_F(ParserResponseTests, SuccessfullyFilledOrder) {
    std::string_view json_response = R"({
        "symbol": "BTCUSDT",
        "clientOrderId": "56789",
        "status": "FILLED",
        "cummulativeQuoteQty": "1.0",
        "side": "SELL",
        "executedQty": "1.0",
        "origQty": "1.0"
    })";
    Exchange::MEClientResponse response = parser.Parse(json_response);
    
    ASSERT_EQ(response.order_id, 56789);
    ASSERT_EQ(response.side, common::Side::SELL);
    ASSERT_EQ(response.exec_qty, 1.0 * std::pow(10, /*qty precision*/ 5)); // Adjust precision
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}