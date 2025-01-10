#define PY_SSIZE_T_CLEAN
#include <aot/WS.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "aot/Binance.h"
#include "aot/Bybit.h"
#include "aot/Logger.h"
#include "aot/common/types.h"
#include "aot/redpanda_client/redpanda_client.h"
#include "aot/strategy/market_order_book.h"
#include "aot/common/exchange_trading_pair.h"


/**
 * @brief In this test, I will subscribe to multiple trading pairs for the Binance and Bybit exchanges.
 *
 * @return int
 */

int main() {
    fmtlog::setLogLevel(fmtlog::DBG);

    // Initialize a thread pool for asynchronous tasks
    boost::asio::thread_pool thread_pool;

    LogPolling log_polling(thread_pool, std::chrono::microseconds(1));

    boost::asio::io_context io_context;

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
                                                                    work_guard(io_context.get_executor());
    std::thread thread_ioc([&io_context] { io_context.run(); });

    // Create a connection pool for Bybit WebSocket sessions with a timeout of 30 seconds,
    // a maximum of 3 concurrent connections, and specific WebSocket details.
    ::V2::ConnectionPool<WSSesionType3, const std::string_view&> session_pool_bybit{
        io_context, WSSesionType3::Timeout{30}, 3, "stream.bybit.com",
        "443",      "/v5/public/spot"
    };
    aot::ExchangeTradingPairs exchange_trading_pairs;
    // Initialize a hash map for trading pair configurations
    common::TradingPairHashMap pairs;

    // Define trading pair information for BTC/USDT on Bybit
    common::TradingPairInfo pair_info_btcusdt_bybit{
        .price_precission     = 2,  // Price precision
        .qty_precission       = 6,  // Quantity precision
        .https_json_request   = "BTCUSDT",  // HTTPS request for JSON responses
        .https_query_request  = "BTCUSDT",  // HTTPS query request string
        .ws_query_request     = "btcusdt",  // WebSocket query request string
        .https_query_response = "BTCUSDT"   // HTTPS query response string
    };

    // Map the trading pair (Exchange ID: 2, Trading Pair ID: 1) to its configuration
    pairs[{2, 1}] = pair_info_btcusdt_bybit;
    common::TradingPair btc_usdt_trading_pair {2,1};

    exchange_trading_pairs.AddOrUpdatePair(common::ExchangeId::kBybit, btc_usdt_trading_pair, pair_info_btcusdt_bybit);


    // Create an asynchronous event bus with the thread pool
    aot::CoBus bus(thread_pool);

    // Initialize a reverse hash map for trading pair IDs to their string representations
    common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs);

    // Initialize a response parser for Bybit's API
    bybit::ApiResponseParser api_response_parser_bybit;

    // Create an order book diff parser for Bybit
    bybit::detail::FamilyBookEventGetter::ParserResponse parser_ob_diff_bybit(
        pairs, pair_reverse
    );

    // Initialize a parser manager for Bybit
    bybit::ParserManager parser_manager_bybit = InitParserManager(
        pairs, pair_reverse, api_response_parser_bybit, parser_ob_diff_bybit
    );

    // Define the maximum number of responses to process in Bybit's book event getter component
    const unsigned int kNumberResponses = 1000;

    // Initialize Bybit's book event getter component
    bybit::BookEventGetterComponent book_event_component_bybit(
        thread_pool, kNumberResponses, TypeExchange::TESTNET, pairs, &session_pool_bybit
    );

    // Initialize Bybit's bid-ask generator component with event bus and limits for updates
    bybit::BidAskGeneratorComponent bid_ask_generator_bybit(
        thread_pool, bus, 100, 1000, 10000
    );

    // Create a response handler for Bybit WebSocket messages, connecting it to components
    bybit::OrderBookWebSocketResponseHandler order_book_wb_socket_response_handler(
        book_event_component_bybit,
        bid_ask_generator_bybit,
        bus,
        parser_manager_bybit
    );

    // Define a callback to handle WebSocket responses using the response handler
    OnWssResponse wss_cb = [&order_book_wb_socket_response_handler](boost::beast::flat_buffer& fb) {
        order_book_wb_socket_response_handler.HandleResponse(fb);
    };

    // Register the callback for the trading pair (Exchange ID: 2, Trading Pair ID: 1)
    book_event_component_bybit.RegisterCallback({2, 1}, &wss_cb);

    // Register all callbacks for the bid-ask generator
    bybit::BidAskGeneratorCallbackHandler bid_ask_generator_callback_handler(
        bus, bid_ask_generator_bybit
    );

    // --------------------------Order Book Component--------------------------------
    // Initialize the order book component with its own strand and event bus
    Trading::OrderBookComponent order_book_component(
        boost::asio::make_strand(thread_pool), bus, 1000
    );

    // Add an order book for Bybit's BTC/USDT trading pair
    common::TradingPair trading_pair{2, 1};
    order_book_component.AddOrderBook(common::ExchangeId::kBybit, trading_pair);

     std::string_view brokers = "localhost:19092";  // Specify your Redpanda broker address here
    auto redpanda_executor = boost::asio::make_strand(thread_pool);
    aot::RedPandaComponent red_panda_component(redpanda_executor, brokers, exchange_trading_pairs);


    // ----------------------Register Connections Between Components-----------------
    // Establish bidirectional communication between the bid-ask generator and book event getter
    bus.Subscribe(&bid_ask_generator_bybit, &book_event_component_bybit);
    bus.Subscribe(&book_event_component_bybit, &bid_ask_generator_bybit);

    // Subscribe the order book component to updates from the bid-ask generator
    bus.Subscribe(&bid_ask_generator_bybit, &order_book_component);
    bus.Subscribe(&order_book_component, &red_panda_component);


    BusEventRequestBBOPrice request_bbo_btc_sub;
    request_bbo_btc_sub.exchange_id    = common::ExchangeId::kBybit;
    request_bbo_btc_sub.trading_pair   = {2, 1};
    request_bbo_btc_sub.snapshot_depth = 50;
    request_bbo_btc_sub.subscribe = true;
    request_bbo_btc_sub.id = 777L;

    auto intr_bus_request_sub =
    boost::intrusive_ptr<BusEventRequestBBOPrice>(&request_bbo_btc_sub);

    bid_ask_generator_bybit.AsyncHandleEvent(intr_bus_request_sub);

    thread_ioc.join();
    log_polling.Stop();
    thread_pool.join();

    return 0; // End of main function

}