#include "boost/asio.hpp"
#include "boost/asio/io_context.hpp"

#include <gtest/gtest.h>

#include "aot/Binance.h"
#include "aot/Exchange.h"
#include "aot/WS.h"
#include "aot/config/config.h"
#include "aot/strategy/market_order_book.h"
#include "aot/redpanda_client/redpanda_client.h"


char** argv = nullptr;
int argc    = 0;

// Google Test Fixture for the BookEventGetterComponent test
// class BookSnapshotComponentTest : public ::testing::Test {
//   protected:
//     boost::asio::io_context io_context;
//     ::V2::ConnectionPool<HTTPSesionType3>* session_pool = nullptr;
//     // ::V2::ConnectionPool<WSSesionType3, const std::string_view&>
//     // session_pool_wss{ io_context, WSSesionType3::Timeout{30}, 1,
//     // "stream.binance.com", "443",
//     // "/ws"};
//     ::V2::ConnectionPool<WSSesionType3, const std::string_view&>
//         session_pool_wss{io_context, WSSesionType3::Timeout{30},
//                          1,          "testnet.binance.vision",
//                          "443",      "/ws"};
//     size_t number_responses = 100;
//     common::TradingPairHashMap pairs;
//     boost::asio::thread_pool thread_pool{1};
//     binance::HttpsConnectionPoolFactory2 factory;
//     binance::testnet::HttpsExchange exchange;
//     boost::asio::cancellation_signal cancel_signal;

//     // The component we are going to test

//     void SetUp() override {
//         common::TradingPairInfo pair_info{.price_precission     = 2,
//                                           .qty_precission       = 5,
//                                           .https_json_request   = "BTCUSDT",
//                                           .https_query_request  = "BTCUSDT",
//                                           .ws_query_request     = "btcusdt",
//                                           .https_query_response = "BTCUSDT"};
//         pairs[{2, 1}] = pair_info;
//         session_pool  = factory.Create(io_context, HTTPSesionType3::Timeout{30},
//                                        5, &exchange);
//     }

//     void TearDown() override {
//         // Any cleanup needed after each test
//     }
// };

// Test case for AsyncHandleEvent
// TEST_F(BookSnapshotComponentTest, TestAsyncHandleEvent) {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     ASSERT_GE(argc, 2);

//     config::ApiSecretKey config(argv[1]);
//     auto [status_api_key, api_key] = config.ApiKey();
//     if(!status_api_key){
//         fmtlog::poll();
//         ASSERT_NE(status_api_key, false) << "status_api_key must not equal
//         false";
//     }

//     auto [status_secret_key, secret_key] = config.SecretKey();
//     if(!status_secret_key)[[unlikely]]{
//         fmtlog::poll();
//         ASSERT_NE(status_secret_key, false) << "status_secret_key must not
//         equal false";
//     }
//     hmac_sha256::Keys keys{api_key, secret_key};
//     hmac_sha256::Signer signer(keys);

//     common::TradingPairReverseHashMap pair_reverse =
//     common::InitTPsJR(pairs);

//     boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
//         work_guard(io_context.get_executor());
//     std::thread t([this] { io_context.run(); });

//     binance::BookSnapshotComponent component(
//         boost::asio::make_strand(thread_pool), number_responses,
//         &signer, TypeExchange::TESTNET, pairs, pair_reverse, session_pool,
//         cancel_signal);

//     Exchange::RequestSnapshot request;
//     request.exchange_id  = common::ExchangeId::kBinance;
//     request.trading_pair = {2, 1};
//     request.depth = 1000;

//     Exchange::BusEventRequestNewSnapshotPool mem_pool(2);
//     Exchange::BusEventRequestNewSnapshot bus_event_request(&mem_pool,
//     &request);
//     //bus_event_request.AddReference();

//     uint64_t counter_successfull   = 0;
//     uint64_t counter_unsuccessfull = 0;

//     OnHttpsResponce cb = [&component, &counter_successfull,
//     &counter_unsuccessfull, this,
//                         &pair_reverse](boost::beast::http::response<boost::beast::http::string_body>&
//                         buffer) {
//         const auto& result = buffer.body();

//         binance::detail::FamilyBookSnapshot::ParserResponse parser(
//             pairs[{2, 1}]);
//         auto snapshot = parser.Parse(result);
//         snapshot.trading_pair = {2,1};
//         if (snapshot.exchange_id ==
//                 common::ExchangeId::kBinance &&
//                 snapshot.bids.size() > 0 &&
//                 snapshot.asks.size() > 0) {
//             logi("Successfull snapshot received");
//             counter_successfull++;
//         } else
//         {
//             counter_unsuccessfull++;
//         }
//         logi("snapshot:{}",snapshot.ToString());
//         component.AsyncStop();
//     };
//     component.RegisterCallback(request.trading_pair, &cb);
//     component.AsyncHandleEvent(&bus_event_request);
//     thread_pool.join();
//     session_pool->CloseAllSessions();
//     work_guard.reset();
//     t.join();
//     int x = 0;
//     EXPECT_GE(counter_successfull, 1);
// };
void OnSnapshotCallback(const Exchange::BookSnapshot& snapshot) {
    std::cout << "[SNAPSHOT CALLBACK] Snapshot processed with LastUpdateId: "
              << snapshot.lastUpdateId << "\n";
}

void OnDiffCallback(const Exchange::BookDiffSnapshot2& diff) {
    std::cout << "[DIFF CALLBACK] Diff processed with ID range: ["
              << diff.first_id << "-" << diff.last_id << "]\n";
}
/**
 * @brief Tested Functionalities Summary
 * Component Initialization: Tests the setup of components with correct configuration.
 * Snapshot Handling: Verifies HTTP response parsing and event creation.
 * Diff Handling: Ensures WebSocket response parsing and event creation.
 * Event Bus Communication: Confirms that components can publish and subscribe to events via the event bus.
 * Market Update Generation: Tests bid-ask updates derived from snapshots and diffs.
 * Resource Cleanup(not sure): Validates that resources and threads are properly cleaned up after the test.(not sure)
 */
// TEST_F(BookSnapshotComponentTest, TestLaunchBidAskGeneratorComponent) {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     ASSERT_GE(argc, 2);

//     aot::CoBus bus(thread_pool);

//     config::ApiSecretKey config(argv[1]);
//     auto [status_api_key, api_key] = config.ApiKey();
//     if (!status_api_key) {
//         fmtlog::poll();
//         ASSERT_NE(status_api_key, false)
//             << "status_api_key must not equal false";
//     }

//     auto [status_secret_key, secret_key] = config.SecretKey();
//     if (!status_secret_key) [[unlikely]] {
//         fmtlog::poll();
//         ASSERT_NE(status_secret_key, false)
//             << "status_secret_key must not equal false";
//     }
//     hmac_sha256::Keys keys{api_key, secret_key};
//     hmac_sha256::Signer signer(keys);

//     common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs);

//     boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
//         work_guard(io_context.get_executor());
//     std::thread t([this] { io_context.run(); });

//     //------------------Snapshot component------------------------------------
//     binance::BookSnapshotComponent component(
//         boost::asio::make_strand(thread_pool), number_responses, &signer,
//         TypeExchange::TESTNET, pairs, pair_reverse, session_pool,
//         cancel_signal);

//     Exchange::RequestSnapshot request;
//     request.exchange_id  = common::ExchangeId::kBinance;
//     request.trading_pair = {2, 1};

//     OnHttpsResponce cb =
//         [&component, &bus, this, &pair_reverse](
//             boost::beast::http::response<boost::beast::http::string_body>&
//                 buffer) {
//             const auto& result = buffer.body();

//             binance::detail::FamilyBookSnapshot::ParserResponse parser(
//                 pairs[{2, 1}]);
//             auto snapshot         = parser.Parse(result);
//             snapshot.trading_pair = {2, 1};
//             auto ptr              = component.snapshot_mem_pool_.Allocate(
//                 &component.snapshot_mem_pool_, snapshot.exchange_id,
//                 snapshot.trading_pair, std::move(snapshot.bids),
//                 std::move(snapshot.asks), snapshot.lastUpdateId);
//             auto intr_ptr_snapsot =
//                 boost::intrusive_ptr<Exchange::BookSnapshot2>(ptr);

//             auto bus_event =
//                 component.bus_event_response_snapshot_mem_pool_.Allocate(
//                     &component.bus_event_response_snapshot_mem_pool_,
//                     intr_ptr_snapsot);
//             auto intr_ptr_bus_snapsot =
//                 boost::intrusive_ptr<Exchange::BusEventResponseNewSnapshot>(
//                     bus_event);

//             // fmtlog::poll();
//             bus.AsyncSend(&component, intr_ptr_bus_snapsot);
//         };
//     component.RegisterCallback(request.trading_pair, &cb);
//     //-----------------------------------------------------------------
//     //--------------------------Depth stream component----------------
//     boost::asio::cancellation_signal cancel_signal;

//     binance::BookEventGetterComponent event_getter_component(
//         boost::asio::make_strand(thread_pool), number_responses,
//         TypeExchange::TESTNET, pairs, &session_pool_wss, cancel_signal);

//     Exchange::RequestDiffOrderBook request_diff_order_book;
//     request.exchange_id            = common::ExchangeId::kBinance;
//     request.trading_pair           = {2, 1};

//     uint64_t counter_successfull   = 0;
//     uint64_t counter_unsuccessfull = 0;

//     OnWssResponse cb_wss = [&event_getter_component, &bus, &counter_successfull,
//                             &counter_unsuccessfull, this,
//                             &pair_reverse](boost::beast::flat_buffer& fb) {
//         auto data     = fb.data();  // returns a const_buffer
//         auto response = std::string_view(static_cast<const char*>(data.data()),
//                                          data.size());
//         binance::detail::FamilyBookEventGetter::ParserResponse parser(
//             pairs, pair_reverse);
//         auto result  = parser.Parse(response);

//         auto request = event_getter_component.book_diff_mem_pool_.Allocate(
//             &event_getter_component.book_diff_mem_pool_, result.exchange_id,
//             result.trading_pair, std::move(result.bids), std::move(result.asks),
//             result.first_id, result.last_id);
//         auto intr_ptr_request =
//             boost::intrusive_ptr<Exchange::BookDiffSnapshot2>(request);

//         auto bus_event =
//             event_getter_component.bus_event_book_diff_snapshot_mem_pool_
//                 .Allocate(&event_getter_component
//                                .bus_event_book_diff_snapshot_mem_pool_,
//                           intr_ptr_request);
//         auto intr_ptr_bus_request =
//             boost::intrusive_ptr<Exchange::BusEventBookDiffSnapshot>(bus_event);

//         bus.AsyncSend(&event_getter_component, intr_ptr_bus_request);

//         // if ((result.exchange_id ==
//         //         common::ExchangeId::kBinance) &&
//         //     (result.trading_pair ==
//         //         common::TradingPair(2, 1)))
//         //     counter_successfull++;
//         // else
//         //     counter_unsuccessfull++;
//         // if (counter_successfull == 5) {
//         //     component.AsyncStop();
//         // }
//     };
//     auto cancelation_slot = cancel_signal.slot();
//     event_getter_component.RegisterCallback(request.trading_pair, &cb_wss);
//     //------------------------------------------------------------------------------

//     binance::BidAskGeneratorComponent bid_ask_generator(
//         boost::asio::make_strand(thread_pool), bus, 100, 1000, 10000);
//     // Register the snapshot callback
//     auto ProcessBookEntries = [&bid_ask_generator, &bus](const auto& entries,
//                                                         common::ExchangeId exchange_id,
//                                                         common::TradingPair trading_pair,
//                                                          common::Side side) {
//         boost::for_each(entries, [&](const auto& bid) {
//             Exchange::MEMarketUpdate2* ptr = bid_ask_generator.out_diff_mem_pool_.Allocate(
//                 &bid_ask_generator.out_diff_mem_pool_, exchange_id,
//                 trading_pair, Exchange::MarketUpdateType::DEFAULT,
//                 common::kOrderIdInvalid, side, bid.price, bid.qty);
//             auto intr_ptr =
//                 boost::intrusive_ptr<Exchange::MEMarketUpdate2>(ptr);
//             auto bus_event =
//                 bid_ask_generator.out_bus_event_diff_mem_pool_.Allocate(
//                     &bid_ask_generator.out_bus_event_diff_mem_pool_, intr_ptr);
//             auto intr_ptr_bus_event =
//                 boost::intrusive_ptr<Exchange::BusEventMEMarketUpdate2>(
//                     bus_event);
//             bus.AsyncSend(&bid_ask_generator, intr_ptr_bus_event);
//         });
//     };
//     bid_ask_generator.RegisterSnapshotCallback(
//         [&ProcessBookEntries](const Exchange::BookSnapshot& snapshot) {
//             std::cout << "Lambda Snapshot Callback: Snapshot received with ID: "
//                       << snapshot.lastUpdateId << std::endl;
//             ProcessBookEntries(snapshot.bids, snapshot.exchange_id, snapshot.trading_pair, common::Side::kBid);
//             ProcessBookEntries(snapshot.asks, snapshot.exchange_id, snapshot.trading_pair, common::Side::kAsk);
//         });
//     // Register the diff callback
//     bid_ask_generator.RegisterDiffCallback(
//         [&ProcessBookEntries](const Exchange::BookDiffSnapshot2& diff) {
//             std::cout << "Lambda Diff Callback: Diff received with range ["
//                       << diff.first_id << " - " << diff.last_id << "]"
//                       << std::endl;
//             ProcessBookEntries(diff.bids, diff.exchange_id, diff.trading_pair, common::Side::kBid);
//             ProcessBookEntries(diff.asks, diff.exchange_id, diff.trading_pair, common::Side::kAsk);
//         });

//     bus.Subscribe(&bid_ask_generator, &component);
//     bus.Subscribe(&bid_ask_generator, &event_getter_component);
//     bus.Subscribe(&component, &bid_ask_generator);
//     bus.Subscribe(&event_getter_component, &bid_ask_generator);
//     //------------------------------------------------------------------------------
//     BusEventRequestBBOPrice request_bbo_btc;
//     request_bbo_btc.exchange_id    = common::ExchangeId::kBinance;
//     request_bbo_btc.trading_pair   = {2, 1};
//     request_bbo_btc.snapshot_depth = 1000;
//     auto intr_bus_request =
//         boost::intrusive_ptr<BusEventRequestBBOPrice>(&request_bbo_btc);
//     bid_ask_generator.AsyncHandleEvent(intr_bus_request);

//     thread_pool.join();
//     session_pool->CloseAllSessions();
//     work_guard.reset();
//     t.join();
//     int z = 0;
// }
/**
 * @brief Tested Functionalities Summary
 * Component Initialization: Tests the setup of components with correct configuration.
 * Snapshot Handling: Verifies HTTP response parsing and event creation.
 * Diff Handling: Ensures WebSocket response parsing and event creation.
 * Event Bus Communication: Confirms that components can publish and subscribe to events via the event bus.
 * Market Update Generation: Tests bid-ask updates derived from snapshots and diffs.
 * Resource Cleanup(not sure): Validates that resources and threads are properly cleaned up after the test.(not sure)
 * interaction with order book component
 */
// TEST_F(BookSnapshotComponentTest, TestLaunchBidAskGeneratorComponent) {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     ASSERT_GE(argc, 2);
//     LogPolling log_polling(thread_pool, std::chrono::milliseconds(100));
//     aot::CoBus bus(thread_pool);

//     config::ApiSecretKey config(argv[1]);
//     auto [status_api_key, api_key] = config.ApiKey();
//     if (!status_api_key) {
//         fmtlog::poll();
//         ASSERT_NE(status_api_key, false)
//             << "status_api_key must not equal false";
//     }

//     auto [status_secret_key, secret_key] = config.SecretKey();
//     if (!status_secret_key) [[unlikely]] {
//         fmtlog::poll();
//         ASSERT_NE(status_secret_key, false)
//             << "status_secret_key must not equal false";
//     }
//     hmac_sha256::Keys keys{api_key, secret_key};
//     hmac_sha256::Signer signer(keys);

//     common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs);

//     binance::ApiResponseParser api_response_parser;
//     binance::detail::FamilyBookEventGetter::ParserResponse parser_ob_diff(
//             pairs, pair_reverse);
//     binance::ParserManager parser_manager = InitParserManager(pairs, pair_reverse, api_response_parser, parser_ob_diff);


//     boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
//         work_guard(io_context.get_executor());
//     std::thread t([this] { io_context.run(); });

//     //------------------Snapshot component------------------------------------
//     binance::BookSnapshotComponent component(
//         boost::asio::make_strand(thread_pool), number_responses, &signer,
//         TypeExchange::TESTNET, pairs, pair_reverse, session_pool);

//     OnHttpsResponseExtended cb =
//         [&component, &bus, this, &pair_reverse](
//             boost::beast::http::response<boost::beast::http::string_body>&
//                 buffer, common::TradingPair trading_pair) {
//             const auto& result = buffer.body();
//             std::cout << result << std::endl;

//             binance::detail::FamilyBookSnapshot::ParserResponse parser(
//                 pairs);
//             auto snapshot         = parser.Parse(result, {2,1});
//             auto ptr              = component.snapshot_mem_pool_.Allocate(
//                 &component.snapshot_mem_pool_, snapshot.exchange_id,
//                 snapshot.trading_pair, std::move(snapshot.bids),
//                 std::move(snapshot.asks), snapshot.lastUpdateId);
//             auto intr_ptr_snapsot =
//                 boost::intrusive_ptr<Exchange::BookSnapshot2>(ptr);

//             auto bus_event =
//                 component.bus_event_response_snapshot_mem_pool_.Allocate(
//                     &component.bus_event_response_snapshot_mem_pool_,
//                     intr_ptr_snapsot);
//             auto intr_ptr_bus_snapsot =
//                 boost::intrusive_ptr<Exchange::BusEventResponseNewSnapshot>(
//                     bus_event);

//             bus.AsyncSend(&component, intr_ptr_bus_snapsot);
//         };
//     component.RegisterCallback({2,1}, &cb);
//     //-----------------------------------------------------------------
//     //--------------------------Depth stream component----------------

//     binance::BookEventGetterComponent event_getter_component(
//         thread_pool, number_responses,
//         TypeExchange::TESTNET, pairs, &session_pool_wss);

//     // Exchange::RequestDiffOrderBook request_diff_order_book;
//     // request.exchange_id            = common::ExchangeId::kBinance;
//     // request.trading_pair           = {2, 1};

//     uint64_t counter_successfull   = 0;
//     uint64_t counter_unsuccessfull = 0;
//     uint64_t request_accepted_by_exchange = 0;
//     bool accept_subscribe_successfully = false;
//     bool accept_unsubscribe_successfully = false;


//     OnWssSVTradingPair cb_wss = [&event_getter_component, &bus, &counter_successfull,
//                             &counter_unsuccessfull, this,
//                             &pair_reverse,
//                             &parser_manager,
//                             &request_accepted_by_exchange,
//                             &accept_subscribe_successfully](std::string_view response, common::TradingPair) {
//         //auto data     = fb.data();  // returns a const_buffer
//         // auto response = std::string_view(static_cast<const char*>(data.data()),
//         //                                  data.size());
//         //auto response = std::string_view(static_cast<const char*>(fb.data().data()), fb.size());
//         auto answer = parser_manager.Parse(response);

//         std::cout << response << std::endl;

//         // binance::detail::FamilyBookEventGetter::ParserResponse parser(
//         //     pairs, pair_reverse);
//         // auto result  = parser.Parse(response);
//         //--------------------------------------------
//         if (std::holds_alternative<Exchange::BookDiffSnapshot>(answer)) {
//             std::visit(
//                 [&](auto&& element) {
//                     using T = std::decay_t<decltype(element)>;
//                     if constexpr (std::is_same_v<T,
//                                                         Exchange::BookDiffSnapshot>) {
//                         logi("Received a BookDiffSnapshot!");
//                         Exchange::BookDiffSnapshot& result = element;
//                         auto request = event_getter_component.book_diff_mem_pool_.Allocate(
//                                         &event_getter_component.book_diff_mem_pool_, result.exchange_id,
//                                         result.trading_pair, std::move(result.bids), std::move(result.asks),
//                                         result.first_id, result.last_id);
//                     auto intr_ptr_request =
//                         boost::intrusive_ptr<Exchange::BookDiffSnapshot2>(request);

//                     auto bus_event =
//                         event_getter_component.bus_event_book_diff_snapshot_mem_pool_
//                             .Allocate(&event_getter_component
//                                         .bus_event_book_diff_snapshot_mem_pool_,
//                                     intr_ptr_request);
//                     auto intr_ptr_bus_request =
//                         boost::intrusive_ptr<Exchange::BusEventBookDiffSnapshot>(bus_event);

//                     bus.AsyncSend(&event_getter_component, intr_ptr_bus_request);
//                         // Use diffSnapshot
//                     }
//                 },
//                 answer);
//                 counter_successfull++;
//         }
//         if (std::holds_alternative<ApiResponseData>(answer)) {
//             const auto& result = std::get<ApiResponseData>(answer);
//             logi("{}", result);

//             request_accepted_by_exchange++;
//             if (auto id_value = std::get_if<long int>(&result.id)) {
//                 if (*id_value == 777) {
//                     accept_subscribe_successfully = true;
//                 }
//             } 
//             // if (request_accepted_by_exchange == 1 &&
//             // counter_successfull > 5) component.AsyncStop();
//         }
//     };
//     event_getter_component.RegisterCallback({2,1}, &cb_wss);
//      OnCloseSession cb_on_close_session = [this,
//     &work_guard](){
//         session_pool->CloseAllSessions();
//         work_guard.reset();
//     };

//     event_getter_component.RegisterCallbackOnCloseSession(
//         {2,1}, &cb_on_close_session);
//     //------------------------------------------------------------------------------
//     //--------------------------Order book component--------------------------------
//     Trading::OrderBookComponent order_book_component(boost::asio::make_strand(thread_pool), bus, 1000);
//     order_book_component.AddOrderBook(common::ExchangeId::kBinance, {2,1});
//     //------------------------------------------------------------------------------

//     binance::BidAskGeneratorComponent bid_ask_generator(thread_pool, bus, 100, 1000, 10000);
//     // Register the snapshot callback
//     auto ProcessBookEntries = [&bid_ask_generator, &bus](const auto& entries,
//                                                         common::ExchangeId exchange_id,
//                                                         common::TradingPair trading_pair,
//                                                          common::Side side) {
//         boost::for_each(entries, [&](const auto& bid) {
//             Exchange::MEMarketUpdate2* ptr = bid_ask_generator.out_diff_mem_pool_.Allocate(
//                 &bid_ask_generator.out_diff_mem_pool_, exchange_id,
//                 trading_pair, Exchange::MarketUpdateType::DEFAULT,
//                 common::kOrderIdInvalid, side, bid.price, bid.qty);
//             auto intr_ptr =
//                 boost::intrusive_ptr<Exchange::MEMarketUpdate2>(ptr);
//             auto bus_event =
//                 bid_ask_generator.out_bus_event_diff_mem_pool_.Allocate(
//                     &bid_ask_generator.out_bus_event_diff_mem_pool_, intr_ptr);
//             auto intr_ptr_bus_event =
//                 boost::intrusive_ptr<Exchange::BusEventMEMarketUpdate2>(
//                     bus_event);
//             bus.AsyncSend(&bid_ask_generator, intr_ptr_bus_event);
//         });
//     };
//     bid_ask_generator.RegisterSnapshotCallback(
//         [&ProcessBookEntries](const Exchange::BookSnapshot& snapshot) {
//             ProcessBookEntries(snapshot.bids, snapshot.exchange_id, snapshot.trading_pair, common::Side::kBid);
//             ProcessBookEntries(snapshot.asks, snapshot.exchange_id, snapshot.trading_pair, common::Side::kAsk);
//         });
//     // Register the diff callback
//     bid_ask_generator.RegisterDiffCallback(
//         [&ProcessBookEntries](const Exchange::BookDiffSnapshot2& diff) {
//             ProcessBookEntries(diff.bids, diff.exchange_id, diff.trading_pair, common::Side::kBid);
//             ProcessBookEntries(diff.asks, diff.exchange_id, diff.trading_pair, common::Side::kAsk);
//         });
//     common::TradingPair btc_usdt_trading_pair {2,1};
//     auto& btcusdt_binance_pair_info = pairs[{2,1}];
//     //--------------------------red panda component--------------------------------
//     std::string_view brokers = "localhost:19092";  // Specify your Redpanda broker address here
//     auto redpanda_executor = boost::asio::make_strand(thread_pool);
//     aot::ExchangeTradingPairs exchange_trading_pairs;
//     exchange_trading_pairs.AddOrUpdatePair(common::ExchangeId::kBinance, btc_usdt_trading_pair, btcusdt_binance_pair_info);

//     aot::RedPandaComponent red_panda_component(redpanda_executor, brokers, exchange_trading_pairs);
//     //------------------------------------------------------------------------------
//     bus.Subscribe(&bid_ask_generator, &component);
//     bus.Subscribe(&bid_ask_generator, &event_getter_component);
//     bus.Subscribe(&component, &bid_ask_generator);
//     bus.Subscribe(&event_getter_component, &bid_ask_generator);
//     bus.Subscribe(&bid_ask_generator, &order_book_component);
//     bus.Subscribe(&order_book_component, &red_panda_component);

//     //------------------------------------------------------------------------------
//     BusEventRequestBBOPrice request_bbo_btc;
//     request_bbo_btc.exchange_id    = common::ExchangeId::kBinance;
//     request_bbo_btc.trading_pair   = {2, 1};
//     request_bbo_btc.snapshot_depth = 1000;
//     request_bbo_btc.id = 777L;
//     auto intr_bus_request =
//         boost::intrusive_ptr<BusEventRequestBBOPrice>(&request_bbo_btc);
//     bid_ask_generator.AsyncHandleEvent(intr_bus_request);

//     t.join();
//     log_polling.Stop();
//     thread_pool.join();
// }

int main(int _argc, char** _argv) {
    argv = _argv;
    argc = _argc;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}