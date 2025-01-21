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

    EndpointManager exchange_connection_manager;
    
    // Initialize a thread pool for asynchronous tasks
    boost::asio::thread_pool thread_pool;

    LogPolling log_polling(thread_pool, std::chrono::microseconds(1));

    boost::asio::io_context io_context;

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
                                                                    work_guard(io_context.get_executor());
    
    hmac_sha256::Keys keys{"", ""};
    hmac_sha256::Signer signer(keys);
    // std::jthread thread_ioc([&io_context](std::stop_token st) {
    //     while (!st.stop_requested()) {
    //         try {
    //             io_context.run();
    //         } catch (const std::exception& ex) {
    //             std::cerr << "Exception in io_context: " << ex.what() << '\n';
    //         }
    //     }
    // });

    std::thread thread_ioc([&io_context]() {
        io_context.run();
    });
    // Create a connection pool for Bybit WebSocket sessions with a timeout of 30 seconds,
    // a maximum of 3 concurrent connections, and specific WebSocket details.
    ::V2::ConnectionPool<WSSesionType3, const std::string_view&> session_pool_bybit{
        io_context, WSSesionType3::Timeout{30}, 3, "stream.bybit.com",
        "443",      "/v5/public/spot"
    };
    ::V2::ConnectionPool<WSSesionType3, const std::string_view&> session_pool_binance{
        io_context, WSSesionType3::Timeout{30},
                        1,          "stream.binance.com",
                        "9443",      "/ws"};
    binance::HttpsConnectionPoolFactory2 factory_binance;
    ::V2::ConnectionPool<HTTPSesionType3>* session_pool = factory_binance.Create(io_context, HTTPSesionType3::Timeout{30},
                                       5, Network::kMainnet, exchange_connection_manager);
    aot::ExchangeTradingPairs exchange_trading_pairs;
    // Initialize a hash map for trading pair configurations
    common::TradingPairHashMap pairs_bybit;
    common::TradingPairHashMap pairs_binance;

    // Define trading pair information for BTC/USDT on Bybit
    common::TradingPairInfo pair_info_btcusdt_bybit{
        .price_precission     = 2,  // Price precision
        .qty_precission       = 6,  // Quantity precision
        .https_json_request   = "BTCUSDT",  // HTTPS request for JSON responses
        .https_query_request  = "BTCUSDT",  // HTTPS query request string
        .ws_query_request     = "btcusdt",  // WebSocket query request string
        .https_query_response = "BTCUSDT"   // HTTPS query response string
    };
    // Define trading pair information for BTC/USDT on Bybit
    common::TradingPairInfo pair_info_ethusdt_bybit{
        .price_precission     = 2,  // Price precision
        .qty_precission       = 5,  // Quantity precision
        .https_json_request   = "ETHUSDT",  // HTTPS request for JSON responses
        .https_query_request  = "ETHUSDT",  // HTTPS query request string
        .ws_query_request     = "ethusdt",  // WebSocket query request string
        .https_query_response = "ETHUSDT"   // HTTPS query response string
    };
    common::TradingPairInfo pair_info_btcusdt_binance{
        .price_precission     = 2,
        .qty_precission       = 5,
        .https_json_request   = "BTCUSDT",
        .https_query_request  = "BTCUSDT",
        .ws_query_request     = "btcusdt",
        .https_query_response = "BTCUSDT"};

    common::TradingPairInfo pair_info_ethusdt_binance{
        .price_precission     = 2,  // Price precision
        .qty_precission       = 4,  // Quantity precision
        .https_json_request   = "ETHUSDT",  // HTTPS request for JSON responses
        .https_query_request  = "ETHUSDT",  // HTTPS query request string
        .ws_query_request     = "ethusdt",  // WebSocket query request string
        .https_query_response = "ETHUSDT"   // HTTPS query response string
    };
    common::TradingPair btc_usdt_trading_pair {2,1};
    common::TradingPair eth_usdt_trading_pair {3,1};
    // Map the trading pair (Exchange ID: 2, Trading Pair ID: 1) to its configuration
    pairs_bybit[btc_usdt_trading_pair] = pair_info_btcusdt_bybit;
    pairs_bybit[eth_usdt_trading_pair] = pair_info_ethusdt_bybit;
    pairs_binance[btc_usdt_trading_pair] = pair_info_btcusdt_binance;
    pairs_binance[eth_usdt_trading_pair] = pair_info_ethusdt_binance;



    exchange_trading_pairs.AddOrUpdatePair(common::ExchangeId::kBybit, btc_usdt_trading_pair, pair_info_btcusdt_bybit);
    exchange_trading_pairs.AddOrUpdatePair(common::ExchangeId::kBybit, eth_usdt_trading_pair, pair_info_ethusdt_bybit);
    exchange_trading_pairs.AddOrUpdatePair(common::ExchangeId::kBinance, btc_usdt_trading_pair, pair_info_btcusdt_binance);
    exchange_trading_pairs.AddOrUpdatePair(common::ExchangeId::kBinance, eth_usdt_trading_pair, pair_info_ethusdt_binance);


    // Create an asynchronous event bus with the thread pool
    aot::CoBus bus(thread_pool);

    // Initialize a reverse hash map for trading pair IDs to their string representations
    common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs_bybit);
    common::TradingPairReverseHashMap pair_reverse_binance = common::InitTPsJR(pairs_binance);

    // Initialize a response parser for Bybit's API
    bybit::ApiResponseParser api_response_parser_bybit;
    // Initialize a response parser for Binance's API
    binance::ApiResponseParser api_response_parser_binance;

    // Create an order book diff parser for Bybit
    bybit::detail::FamilyBookEventGetter::ParserResponse parser_ob_diff_bybit(
        pairs_bybit, pair_reverse
    );
    // Create an order book diff parser for Binance
    binance::detail::FamilyBookEventGetter::ParserResponse parser_ob_diff_binance(
        pairs_binance, pair_reverse_binance
    );

    // Initialize a parser manager for Bybit
    bybit::ParserManager parser_manager_bybit = InitParserManager(
        pairs_bybit, pair_reverse, api_response_parser_bybit, parser_ob_diff_bybit
    );
    // Initialize a parser manager for Binance
    binance::ParserManager parser_manager_binance = InitParserManager(
        pairs_binance, pair_reverse_binance, api_response_parser_binance, parser_ob_diff_binance
    );

    size_t number_responses = 100;

    binance::BookSnapshotComponent book_snapshot_component_binance(
        boost::asio::make_strand(thread_pool), number_responses, &signer,
        Network::kMainnet, pairs_binance, pair_reverse_binance, session_pool, exchange_connection_manager);

    binance::BookSnapsotCallbackHandler book_snapsot_callback_handler_binance(bus, pairs_binance, book_snapshot_component_binance);
    OnHttpsResponseExtended book_snapshot_calback_handler = book_snapsot_callback_handler_binance.GetCallback();
    book_snapshot_component_binance.RegisterCallback(btc_usdt_trading_pair, &book_snapshot_calback_handler);
    book_snapshot_component_binance.RegisterCallback(eth_usdt_trading_pair, &book_snapshot_calback_handler);

    // Define the maximum number of responses to process in Bybit's book event getter component
    const unsigned int kNumberResponses = 1000;

    // Initialize Bybit's book event getter component
    bybit::BookEventGetterComponent book_event_component_bybit(
        thread_pool, kNumberResponses, TypeExchange::TESTNET, pairs_bybit, &session_pool_bybit
    );
    binance::BookEventGetterComponent book_event_getter_binance(
        thread_pool, kNumberResponses,
        TypeExchange::MAINNET, pairs_binance, &session_pool_binance);

    // Initialize Bybit's bid-ask generator component with event bus and limits for updates
    bybit::BidAskGeneratorComponent bid_ask_generator_bybit(
        thread_pool, bus, 100, 1000, 10000
    );

    // Initialize Binance's bid-ask generator component with event bus and limits for updates
    binance::BidAskGeneratorComponent bid_ask_generator_binance(
        thread_pool, bus, 100, 1000, 10000
    );

    // Create a response handler for Bybit WebSocket messages, connecting it to components
    bybit::OrderBookWebSocketResponseHandler order_book_wb_socket_response_handler(
        book_event_component_bybit,
        bid_ask_generator_bybit,
        bus,
        parser_manager_bybit
    );
    // Create a response handler for Binance WebSocket messages, connecting it to components
    binance::BookEventGetterComponentCallbackHandler book_event_componnent_callback_handler_binance(
        book_event_getter_binance,
        bid_ask_generator_binance,
        bus,
        parser_manager_binance
    );

    // Define a callback to handle WebSocket responses using the response handler
    OnWssResponseTradingPair wss_cb = [&order_book_wb_socket_response_handler](boost::beast::flat_buffer& fb, common::TradingPair trading_pair) {
        order_book_wb_socket_response_handler.HandleResponse(fb, trading_pair);
    };
    // Define a callback to handle WebSocket responses using the response handler
    OnWssResponseTradingPair wss_cb_binance = [&book_event_componnent_callback_handler_binance](boost::beast::flat_buffer& fb, common::TradingPair trading_pair) {
        book_event_componnent_callback_handler_binance.HandleResponse(fb, trading_pair);
    };
    // Register the callback for the trading pair (Exchange ID: 2, Trading Pair ID: 1)
    book_event_component_bybit.RegisterCallback(btc_usdt_trading_pair, &wss_cb);
    book_event_component_bybit.RegisterCallback(eth_usdt_trading_pair, &wss_cb);
    // Register the callback for the trading pair (Exchange ID: 2, Trading Pair ID: 1)
    book_event_getter_binance.RegisterCallback(btc_usdt_trading_pair, &wss_cb_binance);
    book_event_getter_binance.RegisterCallback(eth_usdt_trading_pair, &wss_cb_binance);

    // // Register all callbacks for the bid-ask generator. process each murket update event
    bybit::BidAskGeneratorCallbackHandler bid_ask_generator_callback_handler(
        bus, bid_ask_generator_bybit
    );
    
    // Register all callbacks for the bid-ask generator. process batched from excgange
    binance::BidAskGeneratorCallbackBatchHandler bid_ask_generator_callback_handler_binance(
        bus, bid_ask_generator_binance
    );
    // --------------------------Order Book Component--------------------------------
    // Initialize the order book component with its own strand and event bus
    Trading::OrderBookComponent order_book_component(
        boost::asio::make_strand(thread_pool), bus, 1000
    );

    // Add an order book for Bybit's BTC/USDT trading pair
    order_book_component.AddOrderBook(common::ExchangeId::kBybit, btc_usdt_trading_pair);
    order_book_component.AddOrderBook(common::ExchangeId::kBybit, eth_usdt_trading_pair);
    order_book_component.AddOrderBook(common::ExchangeId::kBinance, btc_usdt_trading_pair);
    order_book_component.AddOrderBook(common::ExchangeId::kBinance, eth_usdt_trading_pair);

    std::string_view brokers = "localhost:19092";  // Specify your Redpanda broker address here
    auto redpanda_executor = boost::asio::make_strand(thread_pool);
    aot::RedPandaComponent red_panda_component(redpanda_executor, brokers, exchange_trading_pairs);


    // ----------------------Register Connections Between Components-----------------
    // Establish bidirectional communication between the bid-ask generator and book event getter
    // This allows the bid-ask generator to send data to the book event component, and vice versa.
    bus.Subscribe(&bid_ask_generator_bybit, &book_event_component_bybit);
    bus.Subscribe(&book_event_component_bybit, &bid_ask_generator_bybit);

    // Establish bidirectional communication between the bid-ask generator and book snapshot component for Binance
    // The bid-ask generator communicates with the snapshot component to exchange bid-ask and snapshot updates.
    bus.Subscribe(&bid_ask_generator_binance, &book_snapshot_component_binance);
    bus.Subscribe(&book_snapshot_component_binance, &bid_ask_generator_binance);

    // Establish bidirectional communication between the bid-ask generator and book event getter for Binance
    // This ensures the bid-ask generator and book event getter work together for real-time event updates.
    bus.Subscribe(&bid_ask_generator_binance, &book_event_getter_binance);
    bus.Subscribe(&book_event_getter_binance, &bid_ask_generator_binance);

    // Subscribe the order book component to updates from the bid-ask generator for Bybit
    // This allows the order book component to receive bid-ask updates directly from the generator.
    bus.Subscribe(&bid_ask_generator_bybit, &order_book_component);

    // Subscribe the order book component to updates from the bid-ask generator for Binance
    // The order book component processes bid-ask data generated for Binance.
    bus.Subscribe(&bid_ask_generator_binance, &order_book_component);

    // Subscribe the order book component to updates from the Red Panda component
    // The order book component forwards processed data to Red Panda for further usage or logging.
    bus.Subscribe(&order_book_component, &red_panda_component);


    BusEventRequestBBOPrice request_bbo_btc_sub;
    request_bbo_btc_sub.exchange_id    = common::ExchangeId::kBybit;
    request_bbo_btc_sub.trading_pair   = btc_usdt_trading_pair;
    request_bbo_btc_sub.snapshot_depth = 50;
    request_bbo_btc_sub.subscribe = true;
    request_bbo_btc_sub.id = 777L;

    auto intr_bus_request_btcusdt_sub =
    boost::intrusive_ptr<BusEventRequestBBOPrice>(&request_bbo_btc_sub);
    bid_ask_generator_bybit.AsyncHandleEvent(intr_bus_request_btcusdt_sub);

    BusEventRequestBBOPrice request_bbo_eth_sub;
    request_bbo_eth_sub.exchange_id    = common::ExchangeId::kBybit;
    request_bbo_eth_sub.trading_pair   = eth_usdt_trading_pair;
    request_bbo_eth_sub.snapshot_depth = 50;
    request_bbo_eth_sub.subscribe = true;
    request_bbo_eth_sub.id = 888L;

    auto intr_bus_request_ethusdt_sub =
    boost::intrusive_ptr<BusEventRequestBBOPrice>(&request_bbo_eth_sub);
    bid_ask_generator_bybit.AsyncHandleEvent(intr_bus_request_ethusdt_sub);
    
    BusEventRequestBBOPrice request_bbo_btc_sub_binance;
    request_bbo_btc_sub_binance.exchange_id    = common::ExchangeId::kBinance;
    request_bbo_btc_sub_binance.trading_pair   = btc_usdt_trading_pair;
    request_bbo_btc_sub_binance.snapshot_depth = 1000;
    request_bbo_btc_sub_binance.subscribe = true;
    request_bbo_btc_sub_binance.id = 999L;

    auto intr_bus_request_btcusdt_sub_binance =
    boost::intrusive_ptr<BusEventRequestBBOPrice>(&request_bbo_btc_sub_binance);

    bid_ask_generator_binance.AsyncHandleEvent(intr_bus_request_btcusdt_sub_binance);

    BusEventRequestBBOPrice request_eth_usdt_sub_binance;
    request_eth_usdt_sub_binance.exchange_id    = common::ExchangeId::kBinance;
    request_eth_usdt_sub_binance.trading_pair   = eth_usdt_trading_pair;
    request_eth_usdt_sub_binance.snapshot_depth = 1000;
    request_eth_usdt_sub_binance.subscribe = true;
    request_eth_usdt_sub_binance.id = 1010L;

    auto intr_bus_request_ethusdt_sub_binance =
    boost::intrusive_ptr<BusEventRequestBBOPrice>(&request_eth_usdt_sub_binance);

    bid_ask_generator_binance.AsyncHandleEvent(intr_bus_request_ethusdt_sub_binance);

    thread_ioc.join();
    log_polling.Stop();
    thread_pool.join();

    return 0; // End of main function

}