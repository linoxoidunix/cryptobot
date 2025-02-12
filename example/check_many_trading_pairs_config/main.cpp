#define PY_SSIZE_T_CLEAN
#include <aot/WS.h>

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <thread>

#include "aot/Binance.h"
#include "aot/Bybit.h"
#include "aot/Logger.h"
#include "aot/common/exchange_trading_pair.h"
#include "aot/common/types.h"
#include "aot/config/config.h"
#include "aot/redpanda_client/redpanda_client.h"
#include "aot/strategy/arbitrage/arbitrage_strategy.h"
#include "aot/strategy/market_order_book.h"

std::list<common::TradingPair> FindCrossExchangePairs(
    const common::TradingPairHashMap& exchange1,
    const common::TradingPairHashMap& exchange2) {
    std::list<common::TradingPair> arbitrage_pairs;

    // Перебираем пары из первой биржи
    for (const auto& [info1, _, pair] : exchange1) {
        // Ищем соответствующую пару на второй бирже
        auto it = exchange2.find(pair);
        if (it != exchange2.end()) {
            const auto& info2 = it->second;

            // Если пара найдена на обеих биржах — добавляем в результат
            arbitrage_pairs.emplace_back(pair);
        }
    }

    return arbitrage_pairs;
}

/**
 * @brief In this test, I will subscribe to multiple trading pairs for the
 * Binance and Bybit exchanges.
 *
 * @return int
 */

int main(int argc, char** argv) {
    fmtlog::setLogLevel(fmtlog::DBG);

    std::string_view file_name = argv[1];
    auto toml_parser           = std::make_shared<config::TomlFileParser>();
    auto ticker_manager        = std::make_shared<config::TickerManager>();
    config::TickerParser parser(toml_parser, ticker_manager);
    parser.Parse(file_name);
    auto& pairs_binance = parser.PairsBinance();
    auto& pairs_bybit   = parser.PairsBybit();
    for (const auto& [info, _, pair] : pairs_binance) {
        fmt::print("{} {} {}\n", common::ExchangeId::kBinance, pair, info);
    }
    for (const auto& [info, _, pair] : pairs_bybit) {
        fmt::print("{} {} {}\n", common::ExchangeId::kBybit, pair, info);
    }
    EndpointManager exchange_connection_manager;

    // Initialize a thread pool for asynchronous tasks
    boost::asio::thread_pool thread_pool;

    LogPolling log_polling(thread_pool, std::chrono::microseconds(1));

    boost::asio::io_context io_context_bybit;
    boost::asio::io_context io_context_binance;

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard_bybit(io_context_bybit.get_executor());
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard_binance(io_context_binance.get_executor());
    hmac_sha256::Keys keys{"", ""};
    hmac_sha256::Signer signer(keys);
    // std::jthread thread_ioc([&io_context](std::stop_token st) {
    //     while (!st.stop_requested()) {
    //         try {
    //             io_context.run();
    //         } catch (const std::exception& ex) {
    //             std::cerr << "Exception in io_context: " << ex.what() <<
    //             '\n';
    //         }
    //     }
    // });

    std::thread thread_ioc_bybit(
        [&io_context_bybit]() { io_context_bybit.run(); });
    std::thread thread_ioc_binance(
        [&io_context_binance]() { io_context_binance.run(); });
    // Create a connection pool for Bybit WebSocket sessions with a timeout of
    // 30 seconds, a maximum of 3 concurrent connections, and specific WebSocket
    // details.
    ::V2::ConnectionPool<WSSesionType3, const std::string_view&>
        session_pool_spot_bybit{io_context_bybit,
                                WSSesionType3::Timeout{30},
                                3,
                                "stream.bybit.com",
                                "443",
                                "/v5/public/spot"};
    ::V2::ConnectionPool<WSSesionType3, const std::string_view&>
        session_pool_futures_bybit{io_context_bybit,
                                   WSSesionType3::Timeout{30},
                                   3,
                                   "stream.bybit.com",
                                   "443",
                                   "/v5/public/linear"};
    ::V2::ConnectionPool<WSSesionType3, const std::string_view&>
        session_pool_binance{io_context_binance,
                             WSSesionType3::Timeout{30},
                             3,
                             "stream.binance.com",
                             "9443",
                             "/ws"};
    binance::HttpsConnectionPoolFactory2 factory_binance;
    ::V2::ConnectionPool<HTTPSesionType3>* session_pool =
        factory_binance.Create(io_context_binance, HTTPSesionType3::Timeout{30},
                               5, Network::kMainnet,
                               exchange_connection_manager);

    // this section for redpanda.
    // begin----------------------------------------------------------------------
    aot::ExchangeTradingPairs exchange_trading_pairs;
    for (const auto& [info, _, trading_pair] : pairs_binance) {
        exchange_trading_pairs.AddOrUpdatePair(common::ExchangeId::kBinance,
                                               trading_pair, info);
    }
    for (const auto& [info, _, trading_pair] : pairs_bybit) {
        exchange_trading_pairs.AddOrUpdatePair(common::ExchangeId::kBybit,
                                               trading_pair, info);
    }
    // this section for redpanda.
    // end----------------------------------------------------------------------

    // Create an asynchronous event bus with the thread pool
    aot::CoBus bus(thread_pool);

    // Initialize a reverse hash map for trading pair IDs to their string
    // representations
    common::TradingPairReverseHashMap pair_reverse =
        common::InitTPsJR(pairs_bybit);
    common::TradingPairReverseHashMap pair_reverse_binance =
        common::InitTPsJR(pairs_binance);

    // Initialize a response parser for Bybit's API
    bybit::ApiResponseParser api_response_parser_bybit;
    // Initialize a response parser for Binance's API
    binance::ApiResponseParser api_response_parser_binance;

    // Create an order book diff parser for Bybit
    bybit::detail::FamilyBookEventGetter::ParserResponse parser_ob_diff_bybit(
        pairs_bybit, pair_reverse);
    // Create an order book diff parser for Binance
    binance::detail::FamilyBookEventGetter::ParserResponse
        parser_ob_diff_binance(pairs_binance, pair_reverse_binance);

    // Initialize a parser manager for Bybit
    bybit::ParserManager parser_manager_bybit =
        InitParserManager(pairs_bybit, pair_reverse, api_response_parser_bybit,
                          parser_ob_diff_bybit);
    // Initialize a parser manager for Binance
    binance::ParserManager parser_manager_binance =
        InitParserManager(pairs_binance, pair_reverse_binance,
                          api_response_parser_binance, parser_ob_diff_binance);

    size_t number_responses = 100;

    binance::BookSnapshotComponent book_snapshot_component_binance(
        boost::asio::make_strand(thread_pool), number_responses, &signer,
        Network::kMainnet, pairs_binance, pair_reverse_binance, session_pool,
        exchange_connection_manager);

    binance::BookSnapsotCallbackHandler book_snapsot_callback_handler_binance(
        bus, pairs_binance, book_snapshot_component_binance);
    OnHttpsResponseExtended book_snapshot_calback_handler =
        book_snapsot_callback_handler_binance.GetCallback();
    for (const auto& [ignored1, ignored2, trading_pair] : pairs_binance) {
        book_snapshot_component_binance.RegisterCallback(
            trading_pair, &book_snapshot_calback_handler);
    }

    // Define the maximum number of responses to process in Bybit's book event
    // getter component
    const unsigned int kNumberResponses = 1000;

    // Initialize Bybit's book event getter component
    bybit::BookEventGetterComponent<
        boost::asio::thread_pool,
        bybit::detail::FamilyBookEventGetter::ArgsBody>
        book_event_component_bybit(thread_pool, kNumberResponses, pairs_bybit,
                                   &session_pool_spot_bybit,
                                   common::ExchangeId::kBybit);
    bybit::BookEventGetterComponent<
        boost::asio::thread_pool,
        bybit::detail::FamilyBookEventGetter::ArgsBody>
        book_event_component_futures_bybit(
            thread_pool, kNumberResponses, pairs_bybit,
            &session_pool_futures_bybit, common::ExchangeId::kBybit);
    binance::BookEventGetterComponent<
        boost::asio::thread_pool,
        binance::detail::FamilyBookEventGetter::ArgsBody>
        book_event_getter_binance(thread_pool, kNumberResponses, pairs_binance,
                                  &session_pool_binance,
                                  common::ExchangeId::kBinance);

    // Initialize Bybit's bid-ask generator component with event bus and limits
    // for updates
    bybit::BidAskGeneratorComponent bid_ask_generator_bybit(thread_pool, bus,
                                                            100, 1000, 10000);
    // Initialize Bybit's bid-ask generator component with event bus and limits
    // for updates
    bybit::BidAskGeneratorComponent bid_ask_generator_futures_bybit(
        thread_pool, bus, 100, 1000, 10000);
    // Initialize Binance's bid-ask generator component with event bus and
    // limits for updates
    binance::BidAskGeneratorComponent bid_ask_generator_binance(
        thread_pool, bus, 100, 1000, 10000);

    // Create a response handler for Bybit WebSocket messages, connecting it to
    // components
    bybit::OrderBookWebSocketResponseHandler<
        boost::asio::thread_pool, boost::asio::thread_pool,
        bybit::detail::FamilyBookEventGetter::ArgsBody>
        order_book_wb_socket_response_handler(book_event_component_bybit,
                                              bid_ask_generator_bybit, bus,
                                              parser_manager_bybit);
    bybit::OrderBookWebSocketResponseHandler<
        boost::asio::thread_pool, boost::asio::thread_pool,
        bybit::detail::FamilyBookEventGetter::ArgsBody>
        order_book_wb_socket_response_handler_futures_bybit(
            book_event_component_futures_bybit, bid_ask_generator_futures_bybit,
            bus, parser_manager_bybit);
    // Create a response handler for Binance WebSocket messages, connecting it
    // to components
    binance::BookEventGetterComponentCallbackHandler
        book_event_componnent_callback_handler_binance(
            book_event_getter_binance, bid_ask_generator_binance, bus,
            parser_manager_binance);

    // Define a callback to handle WebSocket responses using the response
    // handler
    OnWssFBTradingPair wss_cb = [&order_book_wb_socket_response_handler](
                                    boost::beast::flat_buffer& fb,
                                    common::TradingPair trading_pair) {
        order_book_wb_socket_response_handler.HandleResponse(fb, trading_pair);
    };
    // Define a callback to handle WebSocket responses using the response
    // handler
    OnWssFBTradingPair wss_futures_cb =
        [&order_book_wb_socket_response_handler_futures_bybit](
            boost::beast::flat_buffer& fb, common::TradingPair trading_pair) {
            order_book_wb_socket_response_handler_futures_bybit.HandleResponse(
                fb, trading_pair);
        };
    // Define a callback to handle WebSocket responses using the response
    // handler
    OnWssFBTradingPair wss_cb_binance =
        [&book_event_componnent_callback_handler_binance](
            boost::beast::flat_buffer& fb, common::TradingPair trading_pair) {
            book_event_componnent_callback_handler_binance.HandleResponse(
                fb, trading_pair);
        };
    // Register the callback for the trading pair (Exchange ID: 2, Trading Pair
    // ID: 1)
    for (const auto& [ignored1, ignored2, trading_pair] : pairs_bybit) {
        book_event_component_bybit.RegisterCallback(trading_pair, &wss_cb);
        book_event_component_futures_bybit.RegisterCallback(trading_pair,
                                                            &wss_futures_cb);
    }
    // Register the callback for the trading pair (Exchange ID: 2, Trading Pair
    // ID: 1)
    for (const auto& [ignored1, ignored2, trading_pair] : pairs_binance) {
        book_event_getter_binance.RegisterCallback(trading_pair,
                                                   &wss_cb_binance);
    }

    // // Register all callbacks for the bid-ask generator. process each murket
    // update event
    bybit::BidAskGeneratorCallbackHandler bid_ask_generator_callback_handler(
        bus, bid_ask_generator_bybit);
    bybit::BidAskGeneratorCallbackHandler
        bid_ask_generator_futures_callback_handler(
            bus, bid_ask_generator_futures_bybit);
    // Register all callbacks for the bid-ask generator. process batched from
    // excgange
    binance::BidAskGeneratorCallbackBatchHandler
        bid_ask_generator_callback_handler_binance(bus,
                                                   bid_ask_generator_binance);
    // --------------------------Order Book
    // Component-------------------------------- Initialize the spot order book
    // component with its own strand and event bus
    Trading::OrderBookComponent order_book_component(
        boost::asio::make_strand(thread_pool), bus, 1000,
        common::MarketType::kSpot);
    // Initialize the spot order book component with its own strand and event
    // bus
    Trading::OrderBookComponent order_book_futures_component(
        boost::asio::make_strand(thread_pool), bus, 1000,
        common::MarketType::kFutures);
    // Add an order book for Bybit's BTC/USDT trading pair
    for (const auto& [ignored1, ignored2, trading_pair] : pairs_bybit) {
        order_book_component.AddOrderBook(common::ExchangeId::kBybit,
                                          trading_pair);
        order_book_futures_component.AddOrderBook(common::ExchangeId::kBybit,
                                                  trading_pair);
    }
    for (const auto& [ignored1, ignored2, trading_pair] : pairs_binance) {
        order_book_component.AddOrderBook(common::ExchangeId::kBinance,
                                          trading_pair);
    }

    std::string_view brokers =
        "localhost:19092";  // Specify your Redpanda broker address here
    auto redpanda_executor = boost::asio::make_strand(thread_pool);
    aot::RedPandaComponent red_panda_component(redpanda_executor, brokers,
                                               exchange_trading_pairs);
    // --------------------------ArbitrageStrategyComponent--------------------------------
    auto cross_pairs = FindCrossExchangePairs(pairs_binance, pairs_bybit);
    aot::ArbitrageStrategyComponent arbitrage_strategy_component(thread_pool,
                                                                 bus, 1000);
    for (auto& cross_pair : cross_pairs) {
        aot::Step buy_on_binance(cross_pair, common::ExchangeId::kBinance,
                                 common::MarketType::kSpot,
                                 aot::Operation::kBuy);
        aot::Step sell_on_bybit_futures(cross_pair, common::ExchangeId::kBybit,
                                        common::MarketType::kFutures,
                                        aot::Operation::kSell);
        aot::ArbitrageCycle cycle_binance_bybit_spot_futures;
        cycle_binance_bybit_spot_futures.push_back(buy_on_binance);
        cycle_binance_bybit_spot_futures.push_back(sell_on_bybit_futures);
        aot::Step sell_on_bybit_spot(cross_pair, common::ExchangeId::kBybit,
                                     common::MarketType::kSpot,
                                     aot::Operation::kSell);
        aot::ArbitrageCycle cycle_binance_bybit_spot_spot;
        cycle_binance_bybit_spot_spot.push_back(buy_on_binance);
        cycle_binance_bybit_spot_spot.push_back(sell_on_bybit_spot);

        aot::Step sell_on_binance_futures(
            cross_pair, common::ExchangeId::kBinance,
            common::MarketType::kFutures, aot::Operation::kSell);
        aot::Step sell_on_binance_spot(cross_pair, common::ExchangeId::kBinance,
                                       common::MarketType::kSpot,
                                       aot::Operation::kSell);
        aot::Step buy_on_bybit(cross_pair, common::ExchangeId::kBybit,
                               common::MarketType::kSpot, aot::Operation::kBuy);
        aot::ArbitrageCycle cycle_bybit_binance_spot_futures;
        cycle_bybit_binance_spot_futures.push_back(sell_on_binance_futures);
        cycle_bybit_binance_spot_futures.push_back(buy_on_bybit);
        aot::ArbitrageCycle cycle_bybit_binance_spot_spot;
        cycle_bybit_binance_spot_spot.push_back(sell_on_binance_spot);
        cycle_bybit_binance_spot_spot.push_back(buy_on_bybit);

        arbitrage_strategy_component.AddArbitrageCycle(
            cycle_binance_bybit_spot_futures);
        arbitrage_strategy_component.AddArbitrageCycle(
            cycle_binance_bybit_spot_spot);
        arbitrage_strategy_component.AddArbitrageCycle(
            cycle_bybit_binance_spot_futures);
        arbitrage_strategy_component.AddArbitrageCycle(
            cycle_bybit_binance_spot_spot);
    }
    const auto& trade_dictionary =
        arbitrage_strategy_component.GetTradeDictionary();
    red_panda_component.AsyncHandleEvent(trade_dictionary);
    nlohmann::json j;
    auto unique_items = trade_dictionary.GetUniqueCycles();
    for (auto& it : unique_items) {
        nlohmann::json j;
        it.SerializeToJson(j);
        logi("{}", j.dump(4));
    }
    // ----------------------Register Connections Between
    // Components----------------- Establish bidirectional communication between
    // the bid-ask generator and book event getter This allows the bid-ask
    // generator to send data to the book event component, and vice versa.
    bus.Subscribe(&bid_ask_generator_bybit, &book_event_component_bybit);
    bus.Subscribe(&book_event_component_bybit, &bid_ask_generator_bybit);
    // Establish bidirectional communication between the bid-ask generator and
    // book event getter This allows the bid-ask generator to send data to the
    // book event component, and vice versa.
    bus.Subscribe(&bid_ask_generator_futures_bybit,
                  &book_event_component_futures_bybit);
    bus.Subscribe(&book_event_component_futures_bybit,
                  &bid_ask_generator_futures_bybit);
    // Establish bidirectional communication between the bid-ask generator and
    // book snapshot component for Binance The bid-ask generator communicates
    // with the snapshot component to exchange bid-ask and snapshot updates.
    bus.Subscribe(&bid_ask_generator_binance, &book_snapshot_component_binance);
    bus.Subscribe(&book_snapshot_component_binance, &bid_ask_generator_binance);

    // Establish bidirectional communication between the bid-ask generator and
    // book event getter for Binance This ensures the bid-ask generator and book
    // event getter work together for real-time event updates.
    bus.Subscribe(&bid_ask_generator_binance, &book_event_getter_binance);
    bus.Subscribe(&book_event_getter_binance, &bid_ask_generator_binance);

    // Subscribe the order book component to updates from the bid-ask generator
    // for Bybit This allows the order book component to receive bid-ask updates
    // directly from the generator.
    bus.Subscribe(&bid_ask_generator_bybit, &order_book_component);
    // Subscribe the order book component to updates from the bid-ask generator
    // for Bybit This allows the order book component to receive bid-ask updates
    // directly from the generator.
    bus.Subscribe(&bid_ask_generator_futures_bybit,
                  &order_book_futures_component);
    // Subscribe the order book component to updates from the bid-ask generator
    // for Binance The order book component processes bid-ask data generated for
    // Binance.
    bus.Subscribe(&bid_ask_generator_binance, &order_book_component);

    // Subscribe the Red Panda component to updates from the order book
    // component The order book component forwards processed data to Red Panda
    // for further usage or logging.
    bus.Subscribe(&order_book_component, &red_panda_component);
    // Subscribe the Red Panda component to updates from the order book
    // component The order book component forwards processed data to Red Panda
    // for further usage or logging.
    bus.Subscribe(&order_book_futures_component, &red_panda_component);
    // Subscribe the ArbitrageStrategyComponent to updates from the order book
    // component The order book component forwards processed data to
    // ArbitrageStrategyComponent for further usage or logging.
    bus.Subscribe(&order_book_component, &arbitrage_strategy_component);
    bus.Subscribe(&arbitrage_strategy_component, &red_panda_component);

    //
    BusEventRequestBBOPricePool request_bbo_pool{1000};
    long unsigned int id_request = 0;
    for (const auto& [ignored1, ignored2, trading_pair] : pairs_bybit) {
        auto* request_bbo = request_bbo_pool.Allocate(
            &request_bbo_pool, common::ExchangeId::kBybit, trading_pair, 50,
            true, id_request);
        auto intr_bus_request_sub =
            boost::intrusive_ptr<BusEventRequestBBOPrice>(request_bbo);
        logi("[bid ask generator] bybit start subscribe to {}", trading_pair);
        bid_ask_generator_bybit.AsyncHandleEvent(intr_bus_request_sub);
        bid_ask_generator_futures_bybit.AsyncHandleEvent(intr_bus_request_sub);
        id_request++;
    }
    for (const auto& [ignored1, ignored2, trading_pair] : pairs_binance) {
        auto* request_bbo = request_bbo_pool.Allocate(
            &request_bbo_pool, common::ExchangeId::kBinance, trading_pair, 1000,
            true, id_request);
        auto intr_bus_request_sub =
            boost::intrusive_ptr<BusEventRequestBBOPrice>(request_bbo);
        logi("[bid ask generator] binance start subscribe to {}", trading_pair);
        bid_ask_generator_binance.AsyncHandleEvent(intr_bus_request_sub);
        id_request++;
    }

    thread_ioc_binance.join();
    thread_ioc_bybit.join();
    log_polling.Stop();
    thread_pool.join();

    return 0;  // End of main function
}