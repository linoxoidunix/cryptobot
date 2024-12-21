#include "aot/Binance.h"

#include <gtest/gtest.h>


#include "aot/Exchange.h"
#include "aot/Logger.h"
#include "aot/WS.h"
#include "aot/common/types.h"


// Handler function to be called when the timer expires
// void StopIoContext(boost::asio::io_context& io,
//                    const boost::system::error_code& ec) {
//     if (!ec) {
//         io.stop();
//     } else {
//         std::cerr << "Error: " << ec.message() << std::endl;
//     }
// };

// Google Test Fixture for the BookEventGetterComponent test
                                                                        // class BookEventGetterComponentTest : public ::testing::Test {
                                                                        // protected:
                                                                        //     boost::asio::io_context io_context;
                                                                        //     ::V2::ConnectionPool<WSSesionType3, const std::string_view&> session_pool{
                                                                        //         io_context, WSSesionType3::Timeout{30},
                                                                        //         1,          "testnet.binance.vision",
                                                                        //         "443",      "/ws"};
                                                                        //     size_t number_responses = 100;
                                                                        //     common::TradingPairHashMap pairs;
                                                                        //     boost::asio::thread_pool thread_pool;

                                                                        //     // The component we are going to test

                                                                        //     void SetUp() override {
                                                                        //         common::TradingPairInfo pair_info{.price_precission     = 2,
                                                                        //                                         .qty_precission       = 5,
                                                                        //                                         .https_json_request   = "BTCUSDT",
                                                                        //                                         .https_query_request  = "BTCUSDT",
                                                                        //                                         .ws_query_request     = "btcusdt",
                                                                        //                                         .https_query_response = "BTCUSDT"};
                                                                        //         pairs[{2, 1}] = pair_info;
                                                                        //     }

                                                                        //     void TearDown() override {
                                                                        //         // Any cleanup needed after each test
                                                                        //     }
                                                                        // };

// Test case for AsyncHandleEvent
// TEST_F(BookEventGetterComponentTest, TestAsyncHandleEvent) {
//     //boost::asio::steady_timer timer(io_context, std::chrono::seconds(20));

//     fmtlog::setLogLevel(fmtlog::DBG);
//     common::TradingPairReverseHashMap pair_reverse =
//     common::InitTPsJR(pairs);

//     boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
//         work_guard(io_context.get_executor());
//     std::thread t([this] { io_context.run(); });
//     //timer.async_wait(std::bind(&StopIoContext, std::ref(io_context),
//     std::placeholders::_1)); boost::asio::cancellation_signal cancel_signal;
//     boost::asio::cancellation_signal resubscribe_signal;

//     binance::BookEventGetterComponent component(
//         boost::asio::make_strand(thread_pool), number_responses,
//         TypeExchange::TESTNET, pairs, &session_pool, cancel_signal,
//         resubscribe_signal);

//     Exchange::RequestDiffOrderBook request;
//     request.exchange_id  = common::ExchangeId::kBinance;
//     request.trading_pair = {2, 1};

//     //Exchange::BusEventRequestDiffOrderBookPool mem_pool(5);
//     Exchange::BusEventRequestDiffOrderBook bus_event_request(nullptr,
//     &request);
//     //bus_event_request.AddReference();

//     uint64_t counter_successfull   = 0;
//     uint64_t counter_unsuccessfull = 0;

//     OnWssResponse cb = [&component, &counter_successfull,
//     &counter_unsuccessfull, this,
//                         &pair_reverse](boost::beast::flat_buffer& fb) {
//         auto data     = fb.data();  // returns a const_buffer
//         auto response = std::string_view(static_cast<const
//         char*>(data.data()),
//                                          data.size());
//         binance::detail::FamilyBookEventGetter::ParserResponse parser(
//             pairs, pair_reverse);
//         auto result = parser.Parse(response);
//         if ((result.exchange_id ==
//                 common::ExchangeId::kBinance) &&
//             (result.trading_pair ==
//                 common::TradingPair(2, 1)))
//             counter_successfull++;
//         else
//             counter_unsuccessfull++;
//         if (counter_successfull == 5) {
//             component.AsyncStop();
//         }
//     };
//     auto cancelation_slot = cancel_signal.slot();
//     component.RegisterCallback(request.trading_pair, &cb);
//     component.AsyncHandleEvent(&bus_event_request);
//     thread_pool.join();
//     session_pool.CloseAllSessions();
//     work_guard.reset();
//     t.join();
//     EXPECT_GE(counter_successfull, 5);
//     EXPECT_EQ(counter_unsuccessfull, 1);//first response from binance is
//     status response
// }
//-----------------------------------------------------------------------------------------------
// Google Test Fixture for the BookEventGetterComponent test
class BookEventGetterComponentTest : public ::testing::Test {
  protected:
    boost::asio::io_context io_context;
    boost::asio::thread_pool thread_pool;
    ::V2::ConnectionPool<WSSesionType3, const std::string_view&> session_pool{
        io_context, WSSesionType3::Timeout{30},
        1, "testnet.binance.vision", "443", "/ws"};

    size_t number_responses = 100;
    common::TradingPairHashMap pairs;

    void SetUp() override {
        pairs[{2, 1}] = common::TradingPairInfo {
                .price_precission     = 2,
                .qty_precission       = 5,
                .https_json_request   = "BTCUSDT",
                .https_query_request  = "BTCUSDT",
                .ws_query_request     = "btcusdt",
                .https_query_response = "BTCUSDT"};
    };
};

// Helper function to initialize the parser manager
// binance::ParserManager InitParserManager(
//     common::TradingPairHashMap& pairs,
//     common::TradingPairReverseHashMap& pair_reverse,
//     binance::ApiResponseParser& api_response_parser,
//     binance::detail::FamilyBookEventGetter::ParserResponse& parser_ob_diff) {
//     binance::ParserManager parser_manager;

//     parser_manager.RegisterHandler(ResponseType::kNonQueryResponse,
//         [&api_response_parser](simdjson::ondemand::document& doc) {
//             return api_response_parser.Parse(doc);
//         });

//     parser_manager.RegisterHandler(ResponseType::kDepthUpdate,
//         [&parser_ob_diff](simdjson::ondemand::document& doc) {
//             return parser_ob_diff.Parse(doc);
//         });

//     return parser_manager;
// }

// // Test case for unsubscribe functionality
TEST_F(BookEventGetterComponentTest, TestUnSubscribeChannelBinance) {
    fmtlog::setLogLevel(fmtlog::DBG);
    LogPolling log_polling(thread_pool, std::chrono::microseconds(1));

    common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs);
    binance::ApiResponseParser api_response_parser;
    binance::detail::FamilyBookEventGetter::ParserResponse parser_ob_diff(
            pairs, pair_reverse);
    binance::ParserManager parser_manager = InitParserManager(pairs, pair_reverse, api_response_parser, parser_ob_diff);

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(io_context.get_executor());
    std::thread io_thread([this] { io_context.run(); });

    binance::BookEventGetterComponent component(
        thread_pool, number_responses, TypeExchange::TESTNET, pairs, &session_pool);

    // Setup subscribe and unsubscribe requests
    Exchange::RequestDiffOrderBook request_subscribe(
        nullptr,
        common::ExchangeId::kBinance,
        {2, 1},
        common::kFrequencyMSInvalid,
        true,
        777);

    Exchange::RequestDiffOrderBook request_unsubscribe(
        nullptr,
        common::ExchangeId::kBinance,
        {2, 1},
        common::kFrequencyMSInvalid,
        false,
        888);

    Exchange::BusEventRequestDiffOrderBook bus_event_subscribe(nullptr, &request_subscribe);
    Exchange::BusEventRequestDiffOrderBook bus_event_unsubscribe(nullptr, &request_unsubscribe);

    // State tracking variables
    uint64_t counter_successful = 0;
    uint64_t request_accepted_by_exchange = 0;
    bool is_unsubscribed_happened = false;
    bool accept_subscribe_successfully = false;
    bool accept_unsubscribe_successfully = false;

    // Callback for WebSocket responses
    OnWssResponse callback = [&component, &counter_successful,
                              &request_accepted_by_exchange,
                              &is_unsubscribed_happened,
                              &accept_subscribe_successfully,
                              &accept_unsubscribe_successfully,
                              &bus_event_unsubscribe,
                              &parser_manager](boost::beast::flat_buffer& fb) {
        auto response = std::string_view(static_cast<const char*>(fb.data().data()), fb.size());
        auto answer = parser_manager.Parse(response);
        if (std::holds_alternative<Exchange::BookDiffSnapshot>(answer)) {
            const auto& result = std::get<Exchange::BookDiffSnapshot>(answer);
            logi("{}", result.ToString());

            if (result.exchange_id == common::ExchangeId::kBinance && result.trading_pair == common::TradingPair(2, 1)) {
                counter_successful++;
            }

            // Trigger unsubscribe after 5 successful responses
            if (!is_unsubscribed_happened && counter_successful == 5) {
                logi("Unsubscribing...");
                component.AsyncHandleEvent(&bus_event_unsubscribe);
                is_unsubscribed_happened = true;
            }
            return;
        }

        if (std::holds_alternative<ApiResponseData>(answer)) {
            const auto& result = std::get<ApiResponseData>(answer);
            logi("{}", result);

            request_accepted_by_exchange++;
            if (result.id == 777) accept_subscribe_successfully = true;
            if (result.id == 888) accept_unsubscribe_successfully = true;

            if (request_accepted_by_exchange == 2) component.AsyncStop();
        }
    };

    // Register callbacks
    component.RegisterCallback(request_subscribe.trading_pair, &callback);

    OnCloseSession cb_on_close_session = [this,
      &work_guard](){
        session_pool.CloseAllSessions();
        work_guard.reset();
    };

    component.RegisterCallbackOnCloseSession(
        request_subscribe.trading_pair, &cb_on_close_session);

    // Start the test
    component.AsyncHandleEvent(&bus_event_subscribe);

    // Wait for completion
    io_thread.join();
    log_polling.Stop();
    thread_pool.join();

    // Assertions
    EXPECT_GE(counter_successful, 5);
    EXPECT_EQ(request_accepted_by_exchange, 2);
    EXPECT_TRUE(accept_subscribe_successfully);
    EXPECT_TRUE(accept_unsubscribe_successfully);
    fmtlog::poll();
}
//------------------------------------------------------------------------------
                                                                                            // TEST_F(BookEventGetterComponentTest, TestUnSubscribeChannel) {
                                                                                            //     fmtlog::setLogLevel(fmtlog::DBG);
                                                                                            //     LogPolling log_polling(thread_pool, std::chrono::microseconds(1));

                                                                                            //     common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs);

                                                                                            //     boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
                                                                                            //         work_guard(io_context.get_executor());
                                                                                            //     std::thread t([this] { 
                                                                                            //         io_context.run();
                                                                                            //         int x = 0; });

                                                                                            //     binance::BookEventGetterComponent component(
                                                                                            //         thread_pool, number_responses, TypeExchange::TESTNET, pairs,
                                                                                            //         &session_pool);

                                                                                            //     Exchange::RequestDiffOrderBook request;
                                                                                            //     request.exchange_id  = common::ExchangeId::kBinance;
                                                                                            //     request.trading_pair = {2, 1};
                                                                                            //     request.subscribe    = true;
                                                                                            //     request.id    = 777;
                                                                                            //     Exchange::BusEventRequestDiffOrderBook bus_event_request(nullptr, &request);

                                                                                            //     Exchange::RequestDiffOrderBook request_unsuscribe;
                                                                                            //     request_unsuscribe.exchange_id  = common::ExchangeId::kBinance;
                                                                                            //     request_unsuscribe.trading_pair = {2, 1};
                                                                                            //     request_unsuscribe.subscribe    = false;
                                                                                            //     request_unsuscribe.id    = 888;

                                                                                            //     Exchange::BusEventRequestDiffOrderBook bus_event_request_unsuscribe(
                                                                                            //         nullptr, &request_unsuscribe);

                                                                                            //     //init parser manager
                                                                                            //     binance::ParserManager  parser_manager;

                                                                                            //     binance::ApiResponseParser api_response_parser;
                                                                                            //     parser_manager.RegisterHandler(ResponseType::kNonQueryResponse, [&api_response_parser](simdjson::ondemand::document& doc){
                                                                                            //         return api_response_parser.Parse(doc);    
                                                                                            //     });

                                                                                            //     binance::detail::FamilyBookEventGetter::ParserResponse parser_ob_diff(
                                                                                            //                 pairs, pair_reverse);
                                                                                            //     parser_manager.RegisterHandler(ResponseType::kDepthUpdate, [&parser_ob_diff](simdjson::ondemand::document& doc){
                                                                                            //         return parser_ob_diff.Parse(doc);    
                                                                                            //     });
                                                                                            //     //init parser manager



















                                                                                            //     uint64_t counter_successfull   = 0;
                                                                                            //     uint64_t request_accepted_by_exchange = 0;
                                                                                            //     bool is_unsubscribed_happened  = false;
                                                                                            //     int called_10_times            = 0;
                                                                                            //     bool resubscribe_called        = false;
                                                                                            //     bool accept_subscribe_sucessfully  = false;
                                                                                            //     bool accept_unsubscribe_sucessfully  = false;

                                                                                            //     OnWssResponse cb =
                                                                                            //         [&component, &counter_successfull, &request_accepted_by_exchange, this,
                                                                                            //         &resubscribe_called, &called_10_times, &bus_event_request_unsuscribe,
                                                                                            //         &bus_event_request, &pair_reverse,
                                                                                            //         &is_unsubscribed_happened,
                                                                                            //         &accept_subscribe_sucessfully,
                                                                                            //         &accept_unsubscribe_sucessfully,
                                                                                            //         &parser_manager](boost::beast::flat_buffer& fb) {
                                                                                            //             auto data     = fb.data();  // returns a const_buffer
                                                                                            //             auto response = std::string_view(
                                                                                            //                 static_cast<const char*>(data.data()), data.size());
                                                                                            //             //std::cout << response << std::endl;
                                                                                            //             binance::detail::FamilyBookEventGetter::ParserResponse parser(
                                                                                            //                 pairs, pair_reverse);
                                                                                            //             auto answer = parser_manager.Parse(response);

                                                                                            //             if (std::holds_alternative<Exchange::BookDiffSnapshot>(answer))
                                                                                            //             {
                                                                                            //                 const auto& result = std::get<Exchange::BookDiffSnapshot>(answer);
                                                                                            //                 logi("{}", result.ToString());
                                                                                            //                 if ((result.exchange_id == common::ExchangeId::kBinance) &&
                                                                                            //                     (result.trading_pair == common::TradingPair(2, 1))) {
                                                                                            //                     counter_successfull++;
                                                                                            //                 }
                                                                                                        
                                                                                            //                 // Stop after 5 successful responses
                                                                                            //                 if (!is_unsubscribed_happened)
                                                                                            //                     if (counter_successfull == 5) {
                                                                                            //                         logi("unsubscribe");
                                                                                            //                         component.AsyncHandleEvent(&bus_event_request_unsuscribe);
                                                                                            //                         is_unsubscribed_happened = true;
                                                                                            //                         // component.AsyncHandleEvent(&bus_event_request);
                                                                                            //                     }
                                                                                            //                 return;
                                                                                            //             }
                                                                                            //             if (std::holds_alternative<ApiResponseData>(answer))
                                                                                            //             {
                                                                                            //                 const auto& result = std::get<ApiResponseData>(answer);
                                                                                            //                 logi("{}", result);
                                                                                            //                 request_accepted_by_exchange++;
                                                                                            //                 if(result.id == 777)
                                                                                            //                     accept_subscribe_sucessfully = true;
                                                                                            //                 if(result.id == 888)
                                                                                            //                     accept_unsubscribe_sucessfully = true;
                                                                                            //                 if(request_accepted_by_exchange == 2)
                                                                                            //                     component.AsyncStop();
                                                                                            //                 return;
                                                                                            //             }
                                                                                            //             //std::cout << "can't parse response" << std::endl;
                                                                                            //         };

                                                                                            //     // Register the callback for the trading pair
                                                                                            //     component.RegisterCallback(request.trading_pair, &cb);

                                                                                            //     OnCloseSession cb_on_close_session = [this,
                                                                                            //     //&log_polling,
                                                                                            //     &work_guard](){
                                                                                            //         session_pool.CloseAllSessions();
                                                                                            //         work_guard.reset();
                                                                                            //     };
                                                                                            //     component.RegisterCallbackOnCloseSession(request.trading_pair, &cb_on_close_session);

                                                                                            //     // Start handling the event asynchronously
                                                                                            //     component.AsyncHandleEvent(&bus_event_request);

                                                                                            //     // thread_pool.join();
                                                                                            //     //using namespace std::literals::chrono_literals;
                                                                                            //     //std::this_thread::sleep_for(10s);
                                                                                            //     //session_pool.CloseAllSessions();
                                                                                            //     //work_guard.reset();
                                                                                            //     //
                                                                                            //     t.join();
                                                                                            //     log_polling.Stop();
                                                                                            //     thread_pool.join();

                                                                                            //     // Check that 5 successful responses were received and 1 unsuccessful (due
                                                                                            //     // to status response)
                                                                                            //     EXPECT_GE(counter_successfull, 5);
                                                                                            //     EXPECT_EQ(request_accepted_by_exchange,
                                                                                            //             2);  // first response from binance is status response, last response from binance is status response
                                                                                            //     EXPECT_GE(accept_subscribe_sucessfully, true);
                                                                                            //     EXPECT_GE(accept_unsubscribe_sucessfully, true);
                                                                                            //     EXPECT_EQ(request_accepted_by_exchange,
                                                                                            //             2);
                                                                                            //     // EXPECT_EQ(resubscribe_called, true);  // first response from binance is
                                                                                            //     // status response
                                                                                            //     fmtlog::poll();
                                                                                            // }

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}