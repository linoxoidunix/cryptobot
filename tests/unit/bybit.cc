#include "aot/Bybit.h"

#include <gtest/gtest.h>

#include "aot/Exchange.h"
#include "aot/WS.h"
#include "aot/common/types.h"
#include "aot/strategy/market_order_book.h"
#include "boost/asio.hpp"
#include "boost/asio/io_context.hpp"

// Helper function to initialize the parser manager


// Google Test Fixture for the BookEventGetterComponent test
// class BookEventGetterComponentTest : public ::testing::Test {
//   protected:
//     boost::asio::io_context io_context;
//     ::V2::ConnectionPool<WSSesionType3, const std::string_view&> session_pool{
//         io_context, WSSesionType3::Timeout{30}, 1, "stream.bybit.com",
//         "443",      "/v5/public/spot"};
//     size_t number_responses = 100;
//     common::TradingPairHashMap pairs;
//     boost::asio::thread_pool thread_pool;

//     // The component we are going to test

//     void SetUp() override {
//         common::TradingPairInfo pair_info{.price_precission     = 2,
//                                           .qty_precission       = 6,
//                                           .https_json_request   = "BTCUSDT",
//                                           .https_query_request  = "BTCUSDT",
//                                           .ws_query_request     = "btcusdt",
//                                           .https_query_response = "BTCUSDT"};
//         pairs[{2, 1}] = pair_info;
//     }

//     void TearDown() override {
//         // Any cleanup needed after each test
//     }
// };

//                                                             // Test case for AsyncHandleEvent
//                                                             /**
//                                                             * @brief Construct a new test f object
//                                                             *
//                                                             *
//                                                             */
//                                                             TEST_F(BookEventGetterComponentTest, TestAsyncHandleEventBybit) {
//                                                                 // boost::asio::steady_timer timer(io_context, std::chrono::seconds(20));

//                                                                 fmtlog::setLogLevel(fmtlog::DBG);
//                                                                 LogPolling log_polling(thread_pool, std::chrono::microseconds(1));
//                                                                 aot::CoBus bus(thread_pool);

//                                                                 common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs);
//                                                                 bybit::ApiResponseParser api_response_parser;
//                                                                 bybit::detail::FamilyBookEventGetter::ParserResponse parser_ob_diff(
//                                                                         pairs, pair_reverse);
//                                                                 bybit::ParserManager parser_manager = InitParserManager(pairs, pair_reverse, api_response_parser, parser_ob_diff);

//                                                                 boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
//                                                                     work_guard(io_context.get_executor());
//                                                                 std::thread t([this] { io_context.run(); });

//                                                                 bybit::BookEventGetterComponent<boost::asio::thread_pool, bybit::detail::FamilyBookEventGetter::ArgsBody> component(
//                                                                     thread_pool, number_responses,
//                                                                     pairs, &session_pool, common::ExchangeId::kBybit);

//                                                                 BusEventRequestBBOPrice request_bbo_btc_sub;
//                                                                 request_bbo_btc_sub.exchange_id    = common::ExchangeId::kBybit;
//                                                                 request_bbo_btc_sub.trading_pair   = {2, 1};
//                                                                 request_bbo_btc_sub.snapshot_depth = 50;
//                                                                 request_bbo_btc_sub.subscribe = true;
//                                                                 request_bbo_btc_sub.id = 777L;

//                                                                 auto intr_bus_request_sub =
//                                                                     boost::intrusive_ptr<BusEventRequestBBOPrice>(&request_bbo_btc_sub);

//                                                                 BusEventRequestBBOPrice request_bbo_btc_unsub;
//                                                                 request_bbo_btc_unsub.exchange_id    = common::ExchangeId::kBybit;
//                                                                 request_bbo_btc_unsub.trading_pair   = {2, 1};
//                                                                 request_bbo_btc_unsub.snapshot_depth = 50;
//                                                                 request_bbo_btc_unsub.subscribe = false;
//                                                                 //problem with double initialization. TODO
//                                                                 //i use previous 777 id. that is stored in 
//                                                                 //BidAskGeneratorComponent
//                                                                 request_bbo_btc_unsub.id = 888L;

//                                                                 auto intr_bus_request_unsub =
//                                                                     boost::intrusive_ptr<BusEventRequestBBOPrice>(&request_bbo_btc_unsub);

//                                                                 uint64_t counter_successful   = 0;
//                                                                 uint64_t counter_unsuccessful = 0;
//                                                                 uint64_t request_accepted_by_exchange = 0;
//                                                                 bool is_unsubscribed_happened = false;
//                                                                 bool accept_subscribe_successfully = false;
//                                                                 bool accept_unsubscribe_successfully = false;

//                                                                 bybit::BidAskGeneratorComponent bid_ask_generator(
//                                                                     thread_pool, bus, 100, 1000, 10000);
                                                                    
//                                                                 OnWssFBTradingPair wss_cb           = [&component,
//                                                                                                 &bid_ask_generator,
//                                                                                                         &counter_successful,
//                                                                                         &counter_unsuccessful, this, &pair_reverse,
//                                                                                         &bus,
//                                                                                         &intr_bus_request_unsub,
//                                                                                         &accept_subscribe_successfully,
//                                                                                         &accept_unsubscribe_successfully,
//                                                                                         &request_accepted_by_exchange,
//                                                                                         &is_unsubscribed_happened,
//                                                                                         &parser_manager](boost::beast::flat_buffer& fb, common::TradingPair) {
//                                                                     auto response = std::string_view(static_cast<const char*>(fb.data().data()), fb.size());
//                                                                     //std::cout << response << std::endl;
//                                                                     auto answer = parser_manager.Parse(response);

//                                                                     if (std::holds_alternative<Exchange::BookSnapshot>(answer) ||
//                                                                         std::holds_alternative<Exchange::BookDiffSnapshot>(answer)) {
//                                                                         std::visit(
//                                                                             [&](auto&& snapshot) {
//                                                                                 using T = std::decay_t<decltype(snapshot)>;
//                                                                                 if constexpr (std::is_same_v<T,
//                                                                                                                     Exchange::BookDiffSnapshot>) {
//                                                                                     logi("Received a BookDiffSnapshot!");
//                                                                                     Exchange::BookDiffSnapshot& diffSnapshot = snapshot;
//                                                                                     auto request = component.book_diff_mem_pool_.Allocate(
//                                                                                         &component.book_diff_mem_pool_,
//                                                                                         diffSnapshot.exchange_id, diffSnapshot.trading_pair,
//                                                                                         std::move(diffSnapshot.bids),
//                                                                                         std::move(diffSnapshot.asks), diffSnapshot.first_id,
//                                                                                         diffSnapshot.last_id);
//                                                                                     auto intr_ptr_request =
//                                                                                         boost::intrusive_ptr<Exchange::BookDiffSnapshot2>(
//                                                                                             request);

//                                                                                     auto bus_event =
//                                                                                         component.bus_event_book_diff_snapshot_mem_pool_
//                                                                                             .Allocate(
//                                                                                                 &component
//                                                                                                     .bus_event_book_diff_snapshot_mem_pool_,
//                                                                                                 intr_ptr_request);
//                                                                                     auto intr_ptr_bus_request = boost::intrusive_ptr<
//                                                                                                 Exchange::BusEventBookDiffSnapshot>(bus_event);
//                                                                                     logi(
//                                                                                         "SEND DIFFSNAPSOT EVENT LAST_ID:{} TO BUS FROM CB "
//                                                                                                 "EVENTGETTER", diffSnapshot.last_id);
//                                                                                     bus.AsyncSend(&component, intr_ptr_bus_request);
                                                                                    
//                                                                                     if (!is_unsubscribed_happened && counter_successful > 5) {
//                                                                                                     logi("Unsubscribing...");
//                                                                                                     bid_ask_generator.AsyncHandleEvent(intr_bus_request_unsub);
//                                                                                                     is_unsubscribed_happened = true;
//                                                                                                 }
//                                                                                     // Use diffSnapshot
//                                                                                 } else if constexpr (std::is_same_v<
//                                                                                                                 T, Exchange::BookSnapshot>) {
//                                                                                     logi("Received a BookSnapshot!");
//                                                                                     Exchange::BookSnapshot& book_snapshot = snapshot;
//                                                                                     // Use bookSnapshot
//                                                                                     book_snapshot.trading_pair            = {2, 1};
//                                                                                     auto ptr = component.snapshot_mem_pool_.Allocate(
//                                                                                         &component.snapshot_mem_pool_,
//                                                                                         book_snapshot.exchange_id,
//                                                                                         book_snapshot.trading_pair,
//                                                                                         std::move(book_snapshot.bids),
//                                                                                         std::move(book_snapshot.asks),
//                                                                                         book_snapshot.lastUpdateId);
//                                                                                     auto intr_ptr_snapsot =
//                                                                                         boost::intrusive_ptr<Exchange::BookSnapshot2>(ptr);

//                                                                                     auto bus_event =
//                                                                                         component.bus_event_response_snapshot_mem_pool_
//                                                                                             .Allocate(
//                                                                                                 &component
//                                                                                                     .bus_event_response_snapshot_mem_pool_,
//                                                                                                 intr_ptr_snapsot);
//                                                                                     auto intr_ptr_bus_snapshot = boost::intrusive_ptr<
//                                                                                                 Exchange::BusEventResponseNewSnapshot>(bus_event);
//                                                                                     logi("SEND SNAPSHOT EVENT LAST_ID:{} TO BUS FROM CB EVENTGETTER", book_snapshot.lastUpdateId);
//                                                                                     bus.AsyncSend(&component, intr_ptr_bus_snapshot);
//                                                                                 }
//                                                                             },
//                                                                             answer);
//                                                                             counter_successful++;
//                                                                     }
//                                                                     if (std::holds_alternative<ApiResponseData>(answer)) {
//                                                                         const auto& result = std::get<ApiResponseData>(answer);
//                                                                         logi("{}", result);

//                                                                         request_accepted_by_exchange++;
//                                                                         if (auto id_value = std::get_if<long int>(&result.id)) {
//                                                                             if (*id_value == 777) {
//                                                                                 accept_subscribe_successfully = true;
//                                                                             }
//                                                                         } 
//                                                                         if (auto value = std::get_if<long int>(&result.id)) {
//                                                                             if (*value == 888L) {
//                                                                                 accept_unsubscribe_successfully = true;
//                                                                             }
//                                                                         }
//                                                                         if (request_accepted_by_exchange == 2) 
//                                                                             bid_ask_generator.AsyncStop();   
//                                                                     }
//                                                                 };
//                                                                 component.RegisterCallback({2, 1}, &wss_cb);
//                                                                 OnCloseSession cb_on_close_session = [this,
//                                                                 &work_guard](){
//                                                                     session_pool.CloseAllSessions();
//                                                                     work_guard.reset();
//                                                                 };

//                                                                 component.RegisterCallbackOnCloseSession(
//                                                                     {2,1}, &cb_on_close_session);
//                                                                 //--------------------------Order book
//                                                                 //component--------------------------------
//                                                                 Trading::OrderBookComponent order_book_component(
//                                                                     boost::asio::make_strand(thread_pool), bus, 1000, common::MarketType::kSpot);
//                                                                 common::TradingPair trading_pair{2, 1};
//                                                                 order_book_component.AddOrderBook(common::ExchangeId::kBybit, trading_pair);
//                                                                 //------------------------------------------------------------------------------
//                                                                 //------------------------------------------------------------------------------

//                                                                 // Register the snapshot callback
//                                                                 std::function<void(const std::list<Exchange::BookSnapshotElem>& entries,
//                                                                                 common::ExchangeId exchange_id,
//                                                                                 common::TradingPair trading_pair, common::Side side,
//                                                                                 Exchange::MarketUpdateType type)>
//                                                                     ProcessBookEntries;
//                                                                 ProcessBookEntries = [&bid_ask_generator, &bus](
//                                                                                         const auto& entries,
//                                                                                         common::ExchangeId exchange_id,
//                                                                                         common::TradingPair trading_pair,
//                                                                                         common::Side side,
//                                                                                         Exchange::MarketUpdateType type) {
//                                                                     boost::for_each(entries, [&](const auto& bid) {
//                                                                         Exchange::MEMarketUpdate2* ptr =
//                                                                         bid_ask_generator.out_diff_mem_pool_.Allocate(
//                                                                             &bid_ask_generator.out_diff_mem_pool_, exchange_id,
//                                                                             trading_pair, type,
//                                                                             common::kOrderIdInvalid, side, bid.price, bid.qty);
//                                                                         auto intr_ptr =
//                                                                             boost::intrusive_ptr<Exchange::MEMarketUpdate2>(ptr);
//                                                                         auto bus_event =
//                                                                             bid_ask_generator.out_bus_event_diff_mem_pool_.Allocate(
//                                                                                 &bid_ask_generator.out_bus_event_diff_mem_pool_,
//                                                                                 intr_ptr);
//                                                                         auto intr_ptr_bus_event =
//                                                                             boost::intrusive_ptr<Exchange::BusEventMEMarketUpdate2>(
//                                                                                 bus_event);
//                                                                         bus.AsyncSend(&bid_ask_generator, intr_ptr_bus_event);
//                                                                     });
//                                                                     // for (const auto& bid : entries) {
//                                                                     //     Exchange::MEMarketUpdate2* ptr =
//                                                                     //         bid_ask_generator.out_diff_mem_pool_.Allocate(
//                                                                     //             &bid_ask_generator.out_diff_mem_pool_, exchange_id,
//                                                                     //             trading_pair, Exchange::MarketUpdateType::DEFAULT,
//                                                                     //             common::kOrderIdInvalid, side, bid.price, bid.qty);

//                                                                     //     auto intr_ptr =
//                                                                     //         boost::intrusive_ptr<Exchange::MEMarketUpdate2>(ptr);

//                                                                     //     auto bus_event =
//                                                                     //         bid_ask_generator.out_bus_event_diff_mem_pool_.Allocate(
//                                                                     //             &bid_ask_generator.out_bus_event_diff_mem_pool_, intr_ptr);

//                                                                     //     auto intr_ptr_bus_event =
//                                                                     //         boost::intrusive_ptr<Exchange::BusEventMEMarketUpdate2>(
//                                                                     //             bus_event);

//                                                                     //     bus.AsyncSend(&bid_ask_generator, intr_ptr_bus_event);
//                                                                     // }
//                                                                 };
//                                                                 bid_ask_generator.RegisterSnapshotCallback(
//                                                                     [&ProcessBookEntries](const Exchange::BookSnapshot& snapshot) {
//                                                                         Exchange::MarketUpdateType type = Exchange::MarketUpdateType::DEFAULT;
//                                                                         if(snapshot.lastUpdateId == 1)
//                                                                             type = Exchange::MarketUpdateType::CLEAR;
//                                                                         ProcessBookEntries(snapshot.bids, snapshot.exchange_id,
//                                                                                         snapshot.trading_pair, common::Side::kBid, type);
//                                                                         ProcessBookEntries(snapshot.asks, snapshot.exchange_id,
//                                                                                         snapshot.trading_pair, common::Side::kAsk, type);
//                                                                     });
//                                                                 // Register the diff callback
//                                                                 bid_ask_generator.RegisterDiffCallback(
//                                                                     [&ProcessBookEntries](const Exchange::BookDiffSnapshot2& diff) {
//                                                                         Exchange::MarketUpdateType type = Exchange::MarketUpdateType::DEFAULT;
//                                                                         ProcessBookEntries(diff.bids, diff.exchange_id, diff.trading_pair,
//                                                                                         common::Side::kBid, type);
//                                                                         ProcessBookEntries(diff.asks, diff.exchange_id, diff.trading_pair,
//                                                                                         common::Side::kAsk, type);
//                                                                     });
//                                                                 //---------------------------init
//                                                                 //bus-------------------------------------------
//                                                                 bus.Subscribe(&bid_ask_generator, &component);
//                                                                 bus.Subscribe(&component, &bid_ask_generator);
//                                                                 bus.Subscribe(&bid_ask_generator, &order_book_component);
//                                                                 //------------------------------------------------------------------------------
//                                                                 BusEventRequestBBOPrice request_bbo_btc;
//                                                                 request_bbo_btc.exchange_id    = common::ExchangeId::kBybit;
//                                                                 request_bbo_btc.trading_pair   = {2, 1};
//                                                                 request_bbo_btc.snapshot_depth = 50;
//                                                                 request_bbo_btc.id = 777;

//                                                                 // auto intr_bus_request =
//                                                                 //     boost::intrusive_ptr<BusEventRequestBBOPrice>(&request_bbo_btc);
//                                                                 bid_ask_generator.AsyncHandleEvent(intr_bus_request_sub);

//                                                                 //session_pool.CloseAllSessions();
//                                                                 //work_guard.reset();
//                                                                 t.join();
//                                                                 log_polling.Stop();
//                                                                 thread_pool.join();
//                                                                 int z = 0;
//                                                                 EXPECT_GE(counter_successful, 5);
//                                                                 EXPECT_EQ(request_accepted_by_exchange, 2);

//                                                             }

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}