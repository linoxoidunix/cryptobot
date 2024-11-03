#include "boost/asio.hpp"
#include "boost/asio/io_context.hpp"

#include <gtest/gtest.h>

#include "aot/Binance.h"
#include "aot/Exchange.h"
#include "aot/WS.h"
#include "aot/config/config.h"

char** argv = nullptr;
int argc = 0;


// Google Test Fixture for the BookEventGetterComponent test
class BookSnapshotComponentTest : public ::testing::Test {
  protected:
    boost::asio::io_context io_context;
    ::V2::ConnectionPool<HTTPSesionType3>* session_pool = nullptr;
    size_t number_responses = 100;
    common::TradingPairHashMap pairs;
    boost::asio::thread_pool thread_pool;
    binance::HttpsConnectionPoolFactory2 factory;
    binance::testnet::HttpsExchange exchange;
    boost::asio::cancellation_signal cancel_signal;

    // The component we are going to test

    void SetUp() override {
        common::TradingPairInfo pair_info{.price_precission     = 2,
                                          .qty_precission       = 5,
                                          .https_json_request   = "BTCUSDT",
                                          .https_query_request  = "BTCUSDT",
                                          .ws_query_request     = "btcusdt",
                                          .https_query_response = "BTCUSDT"};
        pairs[{2, 1}] = pair_info;
        session_pool = factory.Create(io_context, HTTPSesionType3::Timeout{30}, 5, &exchange);
    }

    void TearDown() override {
        // Any cleanup needed after each test
    }
};

// Test case for AsyncHandleEvent
TEST_F(BookSnapshotComponentTest, TestAsyncHandleEvent) {
    fmtlog::setLogLevel(fmtlog::DBG);
    ASSERT_GE(argc, 2);

    config::ApiSecretKey config(argv[1]);
    auto [status_api_key, api_key] = config.ApiKey();
    if(!status_api_key){
        fmtlog::poll();
        ASSERT_NE(status_api_key, false) << "status_api_key must not equal false";
    }

    auto [status_secret_key, secret_key] = config.SecretKey();
    if(!status_secret_key)[[unlikely]]{
        fmtlog::poll();
        ASSERT_NE(status_secret_key, false) << "status_secret_key must not equal false";
    }
    hmac_sha256::Keys keys{api_key, secret_key};
    hmac_sha256::Signer signer(keys);

    common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs);

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard(io_context.get_executor());
    std::thread t([this] { io_context.run(); });


    binance::BookSnapshotComponent component(
        boost::asio::make_strand(thread_pool), number_responses,
        &signer, TypeExchange::TESTNET, pairs, pair_reverse, session_pool, cancel_signal);

    Exchange::RequestSnapshot request;
    request.exchange_id  = common::ExchangeId::kBinance;
    request.trading_pair = {2, 1};
    request.depth = 1000;
    
    Exchange::BusEventRequestNewSnapshot bus_event_request(&request);
    bus_event_request.AddReference();

    uint64_t counter_successfull   = 0;
    uint64_t counter_unsuccessfull = 0;

    OnHttpsResponce cb = [&component, &counter_successfull, &counter_unsuccessfull, this,
                        &pair_reverse](boost::beast::http::response<boost::beast::http::string_body>& buffer) {
        const auto& result = buffer.body();

        binance::detail::FamilyBookSnapshot::ParserResponse parser(
            pairs[{2, 1}]);
        auto snapshot = parser.Parse(result);
        snapshot.trading_pair = {2,1};
        if (snapshot.exchange_id ==
                common::ExchangeId::kBinance &&
                snapshot.bids.size() > 0 &&
                snapshot.asks.size() > 0) {
            logi("Successfull snapshot received");
            counter_successfull++;
        } else
            counter_unsuccessfull++;
        component.AsyncStop();
    };
    component.RegisterCallback(request.trading_pair, &cb);
    // auto cancelation_slot = cancel_signal.slot();
    component.AsyncHandleEvent(&bus_event_request);
    thread_pool.join();
    //std::this_thread::sleep_for(std::chrono::seconds(3));
    session_pool->CloseAllSessions();
    work_guard.reset();
    t.join();
    int x = 0;
    EXPECT_GE(counter_successfull, 1);
    // EXPECT_EQ(counter_unsuccessfull, 1);//first response from binance is status response
}

int main(int _argc, char** _argv) {
    argv = _argv;
    argc = _argc;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}