#include "aot/Binance.h"

#include <gtest/gtest.h>


#include "aot/Exchange.h"
#include "aot/Logger.h"
#include "aot/WS.h"
#include "aot/common/types.h"


// Handler function to be called when the timer expires
void StopIoContext(boost::asio::io_context& io,
                   const boost::system::error_code& ec) {
    if (!ec) {
        io.stop();
    } else {
        std::cerr << "Error: " << ec.message() << std::endl;
    }
};

// Google Test Fixture for the BookEventGetterComponent test
class BookEventGetterComponentTest : public ::testing::Test {
  protected:
    boost::asio::io_context io_context;
    ::V2::ConnectionPool<WSSesionType3, const std::string_view&> session_pool{
        io_context, WSSesionType3::Timeout{30},
        1,          "testnet.binance.vision",
        "443",      "/ws"};
    size_t number_responses = 100;
    common::TradingPairHashMap pairs;
    boost::asio::thread_pool thread_pool;

    // The component we are going to test

    void SetUp() override {
        common::TradingPairInfo pair_info{.price_precission     = 2,
                                          .qty_precission       = 5,
                                          .https_json_request   = "BTCUSDT",
                                          .https_query_request  = "BTCUSDT",
                                          .ws_query_request     = "btcusdt",
                                          .https_query_response = "BTCUSDT"};
        pairs[{2, 1}] = pair_info;
    }

    void TearDown() override {
        // Any cleanup needed after each test
    }
};

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

TEST_F(BookEventGetterComponentTest, TestReSubscribeChannel) {
    fmtlog::setLogLevel(fmtlog::DBG);
    LogPolling log_polling(thread_pool, std::chrono::microseconds(1));

    common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs);

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard(io_context.get_executor());
    std::thread t([this] { 
        io_context.run();
        int x = 0; });

    boost::asio::cancellation_signal cancel_signal;
    boost::asio::cancellation_signal resubscribe_signal;

    binance::BookEventGetterComponent component(
        thread_pool, number_responses, TypeExchange::TESTNET, pairs,
        &session_pool, cancel_signal, resubscribe_signal);

    Exchange::RequestDiffOrderBook request;
    request.exchange_id  = common::ExchangeId::kBinance;
    request.trading_pair = {2, 1};
    request.subscribe    = true;
    Exchange::BusEventRequestDiffOrderBook bus_event_request(nullptr, &request);

    Exchange::RequestDiffOrderBook request_unsuscribe;
    request_unsuscribe.exchange_id  = common::ExchangeId::kBinance;
    request_unsuscribe.trading_pair = {2, 1};
    request_unsuscribe.subscribe    = false;

    // Exchange::BusEventRequestDiffOrderBookPool mem_pool(5);
    Exchange::BusEventRequestDiffOrderBook bus_event_request_unsuscribe(
        nullptr, &request_unsuscribe);


    binance::ParserManager  parser_manager;

    binance::ApiResponseParser api_response_parser;
    parser_manager.RegisterHandler(ResponseType::kNonQueryResponse, [&api_response_parser](simdjson::ondemand::document& doc){
        return api_response_parser.Parse(doc);    
    });

    binance::detail::FamilyBookEventGetter::ParserResponse parser_ob_diff(
                pairs, pair_reverse);
    parser_manager.RegisterHandler(ResponseType::kDepthUpdate, [&parser_ob_diff](simdjson::ondemand::document& doc){
        return parser_ob_diff.Parse(doc);    
    });



















    uint64_t counter_successfull   = 0;
    uint64_t request_accepted_by_exchange = 0;
    bool is_unsubscribed_happened  = false;
    int called_10_times            = 0;
    bool resubscribe_called        = false;
    OnWssResponse cb =
        [&component, &counter_successfull, &request_accepted_by_exchange, this,
         &resubscribe_called, &called_10_times, &bus_event_request_unsuscribe,
         &bus_event_request, &pair_reverse,
         &is_unsubscribed_happened,
         &parser_manager](boost::beast::flat_buffer& fb) {
            auto data     = fb.data();  // returns a const_buffer
            auto response = std::string_view(
                static_cast<const char*>(data.data()), data.size());
            std::cout << response << std::endl;
            binance::detail::FamilyBookEventGetter::ParserResponse parser(
                pairs, pair_reverse);
            auto answer = parser_manager.Parse(response);

            if (std::holds_alternative<Exchange::BookDiffSnapshot>(answer))
            {
                const auto& result = std::get<Exchange::BookDiffSnapshot>(answer);
                logi("{}", result.ToString());
                if ((result.exchange_id == common::ExchangeId::kBinance) &&
                    (result.trading_pair == common::TradingPair(2, 1))) {
                    counter_successfull++;
                }
                // if (counter_successfull == 5) {
                //     component.AsyncStop();
                // }
                //};
                // // Simulate the re-subscription condition after a failure (e.g.,
                // first failure) if (request_accepted_by_exchange == 1 &&
                // !resubscribe_called &&
                // counter_successfull == 3) {
                //     // Trigger re-subscription signal
                //     resubscribe_called = true;
                //     component.AsyncStop();
                //     component.AsyncHandleEvent(&bus_event_request);
                //     //resubscribe_signal.emit(boost::asio::cancellation_type::all);
                // }

                // Stop after 5 successful responses
                if (!is_unsubscribed_happened)
                    if (counter_successfull == 5) {
                        logi("unsubscribe");
                        component.AsyncHandleEvent(&bus_event_request_unsuscribe);
                        is_unsubscribed_happened = true;
                        // component.AsyncHandleEvent(&bus_event_request);
                    }
                // if (counter_successfull > 5) {
                //     if (called_10_times)
                //         component.AsyncStop();
                //     else
                //         called_10_times++;
                // }
                std::cout << "succesful:" << counter_successfull << std::endl;
                return;
            }
            if (std::holds_alternative<ApiResponseData>(answer))
            {
                const auto& result = std::get<ApiResponseData>(answer);
                logi("{}", result);
                request_accepted_by_exchange++;
                return;
            }
            std::cout << "can't parse response" << std::endl;
        };

    // Register the callback for the trading pair
    component.RegisterCallback(request.trading_pair, &cb);

    // Start handling the event asynchronously
    component.AsyncHandleEvent(&bus_event_request);

    // thread_pool.join();
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(10s);
    session_pool.CloseAllSessions();
    log_polling.Stop();
    work_guard.reset();
    thread_pool.join();
    t.join();


    // Check that 5 successful responses were received and 1 unsuccessful (due
    // to status response)
    EXPECT_GE(counter_successfull, 5);
    EXPECT_EQ(request_accepted_by_exchange,
              2);  // first response from binance is status response, last response from binance is status response
    // EXPECT_EQ(resubscribe_called, true);  // first response from binance is
    // status response
    fmtlog::poll();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}