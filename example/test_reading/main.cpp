#define PY_SSIZE_T_CLEAN
#include <aot/WS.h>

#include <boost/beast/core.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "aot/Binance.h"
#include "aot/Bybit.h"
#include "aot/Logger.h"
#include "aot/Predictor.h"
#include "aot/Https.h"
#include "aot/common/types.h"
#include "aot/config/config.h"
#include "aot/order_gw/order_gw.h"
#include "aot/prometheus/service.h"
#include "aot/strategy/kline.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/trade_engine.h"
#include "aot/third_party/emhash/hash_table7.hpp"
// #include "aot/strategy/position_keeper.h"
// #include "aot/strategy/om_order.h"
#include "aot/launcher_predictor.h"
#include "magic_enum.hpp"
//#include "moodycamel/concurrentqueue.h"//if link as 3rd party
#include "concurrentqueue.h"//if link form source
// #define FMT_HEADER_ONLY
// #include <bybit/third_party/fmt/core.h>
//  #define FMTLOG_HEADER_ONLY
//  #include <bybit/third_party/fmtlog.h>
//  int main()
//  {
//      std::function<void(std::string)> func;
//      func = [](std::string){};
//      net::io_context ioc;
//      std::make_shared<WS>(
//          ioc
//          // func, func, func, func
//          )->Run("stream.binance.com", "9443", "/ws/bnbusdt@depth@100ms");
//      ioc.run();
//      return 0;
//  }

// int main() {
//     using namespace binance;
//     net::io_context ioc;
//     //fmtlog::setLogFile("log", true);
//     fmtlog::setLogLevel(fmtlog::DBG);

//     std::function<void(boost::beast::flat_buffer & buffer)> OnMessageCB;
//     OnMessageCB = [](boost::beast::flat_buffer& buffer) {
//         auto resut = boost::beast::buffers_to_string(buffer.data());
//         logi("{}", resut);
//         fmtlog::poll();
//     };

//     using klsb = KLineStream;
//     Symbol btcusdt("BTC", "USDT");
//     auto chart_interval = m1();
//     klsb channel(btcusdt, &chart_interval);
//     std::string empty_request = "{}";
//     std::make_shared<WS>(ioc, empty_request, OnMessageCB)
//         ->Run("stream.binance.com", "9443",
//               fmt::format("/ws/{0}", channel.ToString()));
//     ioc.run();
//     return 0;
// }

// int main() {
//     std::function<void(boost::beast::flat_buffer& buffer)> func;
//     func = [](boost::beast::flat_buffer& buffer) {
//         std::cout << boost::beast::buffers_to_string(buffer.data());
//     };
//     net::io_context ioc;
//     using klsb = bybit::KLineStreamBybit;
//     Symbol btcusdt("BTC", "USDT");
//     auto chart_interval = bybit::m1();
//     klsb channel(btcusdt, &chart_interval);
//     std::make_shared<WS>(ioc,
//                         func
//                          // func, func, func, func
//                          )
//         ->Run("stream-testnet.bybit.com/v5/public/spot", "9443",
//               fmt::format("/{0}", channel.ToString()));
//     ioc.run();
//     return 0;
// }

// int main()
// {
//     //using namespace bybit;
//     using klsb = bybit::KLineStream;
//     bybit::Symbol btcusdt("BTC", "USDT");
//     auto chart_interval = bybit::m1();
//     OHLCVIStorage storage;
//     bybit::OHLCVI fetcher(&btcusdt, &chart_interval);
//     fetcher.Get(storage);
//     return 0;
// }

// int main()
// {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     using klsb = binance::KLineStream;
//     binance::Symbol btcusdt("BTC", "USDT");
//     auto chart_interval = binance::m1();
//     OHLCVIStorage storage;
//     OHLCVLFQueue queue;
//     binance::OHLCVI fetcher(&btcusdt, &chart_interval,
//     TypeExchange::TESTNET); fetcher.Init(queue); while(true)
//     {
//         fetcher.LaunchOne();
//         OHLCV results[50];  // Could also be any iterator

//         size_t count_klines = queue.try_dequeue_bulk(results, 50);
//         for (int i = 0; i < count_klines; i++) {
//             logd("{}", results[i].ToString());
//         }
//         fmtlog::poll();
//     }
//     return 0;
// }
//----------------------------------------------------------------
/**
 * @brief kline service performance check
 *
 * @return int
 */
// int main() {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     using klsb = binance::KLineStream;
//     binance::Symbol btcusdt("BTC", "USDT");
//     auto chart_interval = binance::m1();
//     OHLCVIStorage storage;
//     OHLCVLFQueue queue;
//     binance::OHLCVI fetcher(&btcusdt, &chart_interval,
//     TypeExchange::TESTNET); KLineService service(&fetcher, &queue);
//     service.start();
//     while (service.GetDownTimeInS() < chart_interval.Seconds()) {
//         OHLCV results[50];  // Could also be any iterator
//         size_t count_klines = queue.try_dequeue_bulk(results, 50);
//         for (int i = 0; i < count_klines; i++) {
//             logd("{} countklines:{}", results[i].ToString(), count_klines);
//         }
//         fmtlog::poll();
//     }
//     return 0;
// }
//----------------------------------------------------------------
// int main()
// {
//     try
//     {
//         uint a = 2;
//         Predictor predictor(a);
//         OHLCV data;
//         predictor.Predict(data);
//     }
//     catch(const std::exception& e)
//     {
//         std::cerr << e.what() << '\n';
//     }
//     return 0;
// }

// int main(int argc, char** argv)
// {
//     using namespace binance;
//     Side buy = Side::BUY;
//     OrderNewLimit::ArgsOrder args{"BTCUSDT", 0.001, 30000, TimeInForce::FOK,
//     buy, Type::LIMIT};
//     hmac_sha256::Keys keys{argv[1], argv[2]};
//     hmac_sha256::Signer signer(keys);
//     OrderNewLimit order (std::move(args), &signer, TypeExchange::TESTNET);
//     order.Exec();
//     return 0;
// };

// int main(int argc, char** argv)
// {
//     using namespace bybit;
//     Side buy = Side::BUY;
//     OrderNewLimit::ArgsOrder args{"BTCUSDT", 0.001, 40000,
//     TimeInForce::POST_ONLY, buy, Type::LIMIT}; hmac_sha256::Keys
//     keys{argv[1],
//             argv[2]};
//     hmac_sha256::Signer signer(keys);
//     OrderNewLimit order (std::move(args), &signer, TypeExchange::TESTNET);
//     order.Exec();
//     return 0;
// };

// void sender(moodycamel::ConcurrentQueue<int>& q)
// {
//     for (int i = 0; i < 100; ++i)
//     {
//         q.enqueue(i);
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//     }
// }

// void receiver(moodycamel::ConcurrentQueue<int>& q)
// {
//     for (int i = 0; i < 100; ++i)
//     {
//         int item;
//         bool found = q.try_dequeue(item);
//         if(found)
//             logi("{}", item);
//         std::this_thread::sleep_for(std::chrono::milliseconds(5));

//     }
// }

// int main(int argc, char** argv)
// {
//     moodycamel::ConcurrentQueue<int> q;
//     std::thread t1(sender, std::ref(q)); // pass by value
//     std::thread t2(receiver, std::ref(q)); // pass by value
//     t1.join();
//     t2.join();

// };

// int main()
// {
// std::array<int, 1048 * 1048> board;
// logi("sizeof int ={}byte sizeof map elem={}", sizeof(int), 1048 * 1048);
// std::vector<int> board(10048*2048);
// logi("sizeof board elem={} size={}", sizeof(board), board.size());

// int x = 0;
// Trading::MarketOrderBook order_book;
// std::array<Trading::MarketOrder *, common::ME_MAX_ORDER_IDS> map;
// std::array<Trading::MarketOrder *, 1'000'000> map;
//  logi("sizeof market_order ptr ={}byte sizeof map byte={} KByte={} MByte={}
//  Gbyte={} ", sizeof(Trading::MarketOrder*), sizeof(map), 1.0*
//  sizeof(map)/1024, 1.0* sizeof(map)/1024/1024, 1.0*
//  sizeof(map)/1024/1024/1024);
// Trading::OrderHashMap map(8048*8048);
// logi("sizeof board elem={} size={}", sizeof(map), map.size());

// map.fill(nullptr);
// int x = 0;
// common::MemPool<Trading::MarketOrdersAtPrice>
// orders_at_price_pool_(common::ME_MAX_PRICE_LEVELS);
//}

// int main() {
//     using namespace binance;
//     DiffDepthStream::ms100 interval;
//     Symbol btcusdt("BTC", "USDT");
//     BookEventGetter event_capturer(&btcusdt, &interval);
//     Exchange::BookDiffLFQueue queue;
//     event_capturer.Init(queue);
//     while(true)
//         event_capturer.LaunchOne();
//     fmtlog::poll();
// }

// int main() {
//     using namespace binance;
//     BookSnapshot::ArgsOrder args{"BTCUSDT", 200};
//     Exchange::BookSnapshot snapshot;
//     BookSnapshot book_snapshoter(std::move(args), TypeExchange::TESTNET,
//                                  &snapshot);
//     book_snapshoter.Exec();
//     logd("{}", snapshot);
//     //Exchange::BookSnapshotElem elem(1, 2);
//     //auto ddd = fmt::format("{}", elem);
//     //logd("{}", elem.ToString());

// }
// //-----------------------------------------------------------------------------------
// /**
//  * @brief launch only generator bid ask service
//  *
//  * @return int
//  */
// int main() {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     using namespace binance;
//     Exchange::EventLFQueue event_queue;
//     prometheus::EventLFQueue prometheus_event_queue;
//     Exchange::RequestNewLimitOrderLFQueue request_new_order;
//     Exchange::RequestCancelOrderLFQueue request_cancel_order;
//     Exchange::ClientResponseLFQueue response;
//     OHLCVILFQueue ohlcv_queue;
//     DiffDepthStream::ms100 interval;
//     TickerHashMap tickers;
//     tickers[1] = "usdt";
//     tickers[2] = "btc";
    
//     TradingPairHashMap pair;
//     binance::Symbol symbol(tickers[2], tickers[1]);
//     TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
//     pair[{2, 1}] = pair_info;

//     GeneratorBidAskService generator(&event_queue, &prometheus_event_queue,
//                                      pair[{2, 1}],tickers, TradingPair{2,1}, &interval,
//                                      TypeExchange::TESTNET);
//     generator.Start();
//     using namespace std::literals::chrono_literals;
//     std::this_thread::sleep_for(5s);
//     generator.Stop();
//     fmtlog::poll();

//     // trade_engine_service.Stop();
// }
// //-----------------------------------------------------------------------------------
// /**
//  * @brief launch generator bid ask service + market order book
//  *
//  * @return int
//  */
// int main() {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     using namespace binance;
//     Exchange::EventLFQueue event_queue;
//     prometheus::EventLFQueue prometheus_event_queue;
//     Exchange::RequestNewLimitOrderLFQueue request_new_order;
//     Exchange::RequestCancelOrderLFQueue request_cancel_order;
//     Exchange::ClientResponseLFQueue response;
//     OHLCVILFQueue ohlcv_queue;
//     DiffDepthStream::ms100 interval;
//     TickerHashMap tickers;
//     tickers[1] = "usdt";
//     tickers[2] = "btc";
    
//     TradingPairHashMap pair;
//     binance::Symbol symbol(tickers[2], tickers[1]);
//     TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
//     pair[{2, 1}] = pair_info;

//     GeneratorBidAskService generator_bid_ask(&event_queue, &prometheus_event_queue,
//                                      pair[{2, 1}],tickers, TradingPair{2,1}, &interval,
//                                      TypeExchange::TESTNET);

//     Trading::MarketOrderBook ob(TradingPair{2,1}, pair);
//     Trading::OrderBookService orderbook_service(&ob, &event_queue);
    
//     orderbook_service.Start();
//     generator_bid_ask.Start();

//     using namespace std::literals::chrono_literals;
//     std::this_thread::sleep_for(5s);
//     generator_bid_ask.Stop();
//     orderbook_service.StopWaitAllQueue();
//     fmtlog::poll();
// }
// //-----------------------------------------------------------------------------------
/**
 * @brief launch generator bid ask service + order book, that push event to lfqueu
 *
 * @return int
 */
// int main() {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     using namespace binance;
//     Exchange::EventLFQueue event_queue;
//     prometheus::EventLFQueue prometheus_event_queue;
//     Exchange::RequestNewLimitOrderLFQueue request_new_order;
//     Exchange::RequestCancelOrderLFQueue request_cancel_order;
//     Exchange::ClientResponseLFQueue response;
//     OHLCVILFQueue ohlcv_queue;
//     DiffDepthStream::ms100 interval;
//     TickerHashMap tickers;
//     tickers[1] = "usdt";
//     tickers[2] = "btc";
    
//     TradingPairHashMap pair;
//     binance::Symbol symbol(tickers[2], tickers[1]);
//     TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
//     pair[{2, 1}] = pair_info;

//     GeneratorBidAskService generator_bid_ask(&event_queue, &prometheus_event_queue,
//                                      pair[{2, 1}],tickers, TradingPair{2,1}, &interval,
//                                      TypeExchange::TESTNET);
    
//     strategy::cross_arbitrage::LFQueue queue;
//     position_keeper::EventLFQueue orderbook_positionkeeper_channel;

//     strategy::cross_arbitrage::OrderBook ob(1, TradingPair{2,1}, pair, &queue, &orderbook_positionkeeper_channel, 1000, 1000, 1000);
//     Trading::OrderBookService orderbook_service(&ob, &event_queue);
    
//     orderbook_service.Start();
//     generator_bid_ask.Start();

//     using namespace std::literals::chrono_literals;
//     std::this_thread::sleep_for(5s);
//     generator_bid_ask.Stop();
//     orderbook_service.StopWaitAllQueue();
//     //orderbook_service.StopImmediately();
//     fmtlog::poll();
// }
//-----------------------------------------------------------------------------------
// /**
//  * @brief testing trade engine + generator bid ask service + check update BBO
//  *
//  * @return int
//  */
// int main() {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     using namespace binance;
//     Exchange::EventLFQueue event_queue;
//     prometheus::EventLFQueue prometheus_event_queue;
//     Exchange::RequestNewLimitOrderLFQueue request_new_order;
//     Exchange::RequestCancelOrderLFQueue request_cancel_order;
//     Exchange::ClientResponseLFQueue response;
//     OHLCVILFQueue ohlcv_queue;
//     DiffDepthStream::ms100 interval;
//     TickerHashMap tickers;
//     tickers[1] = "usdt";
//     tickers[2] = "btc";
    
//     TradingPairHashMap pair;
//     binance::Symbol symbol(tickers[2], tickers[1]);
//     TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
//     pair[{2, 1}] = pair_info;

//     GeneratorBidAskService generator(&event_queue, &prometheus_event_queue,
//                                      pair[{2, 1}],tickers, TradingPair{2,1}, &interval,
//                                      TypeExchange::TESTNET);
//     generator.Start();
//     Trading::TradeEngine trade_engine_service(
//         &event_queue, &request_new_order, &request_cancel_order, &response,
//         &ohlcv_queue, &prometheus_event_queue, ticker, nullptr);
//     std::string host  = "localhost";
//     unsigned int port = 6060;
//     prometheus::Service prometheus_service(host, port,
//     &prometheus_event_queue); prometheus_service
//         .Start();  // launch prometheus server that send data to prometheus

//     trade_engine_service.Start();
//     common::TimeManager time_manager;
//     while (trade_engine_service.GetDownTimeInS() < 10) {
//         logd("Waiting till no activity, been silent for {} seconds...",
//              generator.GetDownTimeInS());
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(1s);
//     }
//     using namespace std::literals::chrono_literals;
//     std::this_thread::sleep_for(5s);
//     generator.Stop();
//     fmtlog::poll();

//     trade_engine_service.Stop();
// }
//-----------------------------------------------------------------------------------
// /**
//  * @brief testing binance response for new order
//  *
//  * @param argc
//  * @param argv
//  * @return int
//  */
// int main(int argc, char** argv) {
//     fmtlog::setLogLevel(fmtlog::DBG);

//     using namespace binance;
//     hmac_sha256::Keys keys{argv[2], argv[3]};
//     hmac_sha256::Signer signer(keys);
//     auto type = TypeExchange::TESTNET;
//     OrderNewLimit new_order(&signer, type);
//     CancelOrder executor_cancel_order(&signer, type);
//     using namespace Trading;
//     Exchange::RequestNewLimitOrderLFQueue requests_new_order;
//     Exchange::RequestCancelOrderLFQueue requests_cancel_order;
//     Exchange::ClientResponseLFQueue client_responses;
//     Exchange::RequestNewOrder request_new_order;
//     request_new_order.ticker   = "BTCUSDT";
//     request_new_order.order_id = 6;
//     request_new_order.side     = common::Side::BUY;
//     request_new_order.price    = 40000;
//     request_new_order.qty      = 0.001;

//     requests_new_order.enqueue(request_new_order);
//     OrderGateway gw(&new_order, &executor_cancel_order, &requests_new_order,
//     &requests_cancel_order,
//                     &client_responses);
//     gw.start();
//     while (gw.GetDownTimeInS() < 7) {
//         logd("Waiting till no activity, been silent for {} seconds...",
//              gw.GetDownTimeInS());
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(3s);
//     }

//     Exchange::MEClientResponse response[50];

//     size_t count_new_order = client_responses.try_dequeue_bulk(response, 50);
//     for (int i = 0; i < count_new_order; i++) {
//         logd("{}", response[i].ToString());
//     }

//     return 0;
// }
//-----------------------------------------------------------------------------------

// /**
//  * @brief test add and cancel order for binance without connection pool
//  *
//  * @param argc
//  * @param argv
//  * @return int
//  */
// int main(int argc, char** argv) {
//     //boost::asio::io_context ioc;
//     // std::thread t([&ioc] {
//     //     auto work_guard = boost::asio::make_work_guard(ioc);
        
//     //     ioc.run();
//     // });
//     binance::testnet::HttpsExchange exchange;
//     fmtlog::setLogLevel(fmtlog::DBG);
//     config::ApiSecretKey config(argv[1]);

//     auto [status_api_key, api_key] = config.ApiKey();
//     if(!status_api_key){
//         fmtlog::poll();
//         return 0;
//     }

//     auto [status_secret_key, secret_key] = config.SecretKey();
//     if(!status_secret_key)[[unlikely]]{
//         fmtlog::poll();
//         return 0;
//     }

//     hmac_sha256::Keys keys{api_key, secret_key};
//     hmac_sha256::Signer signer(keys);
//     auto type = TypeExchange::TESTNET;
//     fmtlog::setLogLevel(fmtlog::DBG);
//     using namespace binance;

//     common::TickerHashMap tickers;
//     tickers[1] = "usdt";
//     tickers[2] = "btc";
    
//     TradingPairHashMap pairs;
//     binance::Symbol symbol(tickers[2], tickers[1]);
//     TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
//     pairs[{2, 1}] = pair_info;

//     // HTTPSSessionPool session_pools;
//     // binance::ConnectionPoolFactory factory;
//     // auto pool = factory.Create(ioc, &exchange, 15, HTTPSesionType::Timeout{30});
//     // session_pools[1] = pool;
    

//     OrderNewLimit new_order(&signer, type, pairs);

//     //NewOrderExecutors new_order_executors;
//     //new_order_executors[1] = &new_order;  

//     CancelOrder executor_cancel_order(&signer, type, pairs);







//     using namespace Trading;
//     Exchange::RequestNewLimitOrderLFQueue requests_new_order;
//     Exchange::RequestCancelOrderLFQueue requests_cancel_order;
//     Exchange::ClientResponseLFQueue client_responses;

//     Exchange::RequestNewOrder request_new_order;
//     request_new_order.exchange_id = 1;
//     request_new_order.trading_pair   = {2, 1};
//     request_new_order.order_id = 6;
//     request_new_order.side     = common::Side::BUY;
//     request_new_order.price    = 4000000;
//     request_new_order.qty      = 100;

//     requests_new_order.enqueue(request_new_order);
//     Exchange::RequestCancelOrder order_for_cancel;
//     order_for_cancel.exchange_id = 1;
//     order_for_cancel.trading_pair   = {2, 1};
//     order_for_cancel.order_id = 6;

//     requests_cancel_order.enqueue(order_for_cancel);
//     OrderGateway gw(&new_order, &executor_cancel_order, &requests_new_order,
//                     &requests_cancel_order, &client_responses);
//     gw.Start();
//     while (gw.GetDownTimeInS() < 7) {
//         logd("Waiting till no activity, been silent for {} seconds...",
//              gw.GetDownTimeInS());
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(3s);
//     }

//     Exchange::MEClientResponse response[50];

//     size_t count_new_order = client_responses.try_dequeue_bulk(response, 50);
//     for (int i = 0; i < count_new_order; i++) {
//         logd("{}", response[i].ToString());
//     }

//     fmtlog::poll();
//     return 0;
// }
//-----------------------------------------------------------------------------------
/**
 * @brief test add and cancel order for binance with connection pool
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char** argv) {
    boost::asio::io_context ioc;

    binance::testnet::HttpsExchange exchange;
    fmtlog::setLogLevel(fmtlog::DBG);
    fmtlog::setLogFile("888.txt");
    config::ApiSecretKey config(argv[1]);

    auto [status_api_key, api_key] = config.ApiKey();
    if(!status_api_key){
        fmtlog::poll();
        return 0;
    }

    auto [status_secret_key, secret_key] = config.SecretKey();
    if(!status_secret_key)[[unlikely]]{
        fmtlog::poll();
        return 0;
    }

    hmac_sha256::Keys keys{api_key, secret_key};
    hmac_sha256::Signer signer(keys);
    auto type = TypeExchange::TESTNET;
    fmtlog::setLogLevel(fmtlog::DBG);
    using namespace binance;

    common::TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";
    
    TradingPairHashMap pairs;
    binance::Symbol symbol(tickers[2], tickers[1]);
    TradingPairInfo pair_info{std::string(symbol.ToString()), 2, 5};
    pairs[{2, 1}] = pair_info;

    HTTPSSessionPool session_pools;
    binance::ConnectionPoolFactory factory;
    auto pool = factory.Create(ioc, &exchange, 5, HTTPSesionType::Timeout{30});
    
    std::thread t([&ioc] {
        auto work_guard = boost::asio::make_work_guard(ioc);
        
        ioc.run();
        int x = 0;
    });
    //using namespace std::literals::chrono_literals;
    //std::this_thread::sleep_for(15s);
    
    session_pools[1] = pool;
    

    OrderNewLimit2 new_order(&signer, type, pairs, pool);

    NewLimitOrderExecutors new_limit_order_executors;
    new_limit_order_executors[1] = &new_order;  

    CancelOrder2 executor_cancel_order(&signer, type, pairs, pool);

    CancelOrderExecutors cancel_order_executors;
    cancel_order_executors[1] = &executor_cancel_order;





    using namespace Trading;
    Exchange::RequestNewLimitOrderLFQueue requests_new_order;
    Exchange::RequestCancelOrderLFQueue requests_cancel_order;
    Exchange::ClientResponseLFQueue client_responses;

    Exchange::RequestNewOrder request_new_order;
    request_new_order.exchange_id = 1;
    request_new_order.trading_pair   = {2, 1};
    request_new_order.order_id = 6;
    request_new_order.side     = common::Side::BUY;
    request_new_order.price    = 4000000;
    request_new_order.qty      = 100;

    requests_new_order.enqueue(request_new_order);
    Exchange::RequestCancelOrder order_for_cancel;
    order_for_cancel.exchange_id = 1;
    order_for_cancel.trading_pair   = {2, 1};
    order_for_cancel.order_id = 6;

    requests_cancel_order.enqueue(order_for_cancel);
    OrderGateway2 gw(new_limit_order_executors, cancel_order_executors, &requests_new_order,
                    &requests_cancel_order, &client_responses);
    gw.Start();
    while (gw.GetDownTimeInS() < 30) {
        logd("Waiting till no activity, been silent for {} seconds...",
             gw.GetDownTimeInS());
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
    }

    Exchange::MEClientResponse response[50];

    size_t count_new_order = client_responses.try_dequeue_bulk(response, 50);
    for (int i = 0; i < count_new_order; i++) {
        logd("{}", response[i].ToString());
    }
    ioc.stop();
    fmtlog::poll();
    t.join();
    return 0;
}
//-----------------------------------------------------------------------------------
// /**
//  * @brief testing cpp wrapper for python strategy predict class
//  *
//  * @param argc
//  * @param argv
//  * @return int
//  */
// int main(int argc, char** argv)
// {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     const auto python_path = argv[1];
//     std::string path_where_models =
//     "/home/linoxoidunix/Programming/cplusplus/cryptobot";
//     auto predictor_module = "strategy.py";
//     auto class_module = "Predictor";
//     auto method_module = "predict";
//     {
//         base_strategy::Strategy predictor(python_path, path_where_models,
//         predictor_module, class_module, method_module);
//     }
//     // fmtlog::poll();
//     // for(int i =0 ; i < 100; i++){
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // predictor.Predict(40000.0, 70000.0, 50000.0, 60000.0, 10000);
//     // //     base_strategy::Strategy predictor1(python_path,
//     path_where_models,
//     // // predictor_module, class_module, method_module);
//     // //     base_strategy::Strategy predictor2(python_path,
//     path_where_models,
//     // // predictor_module, class_module, method_module);
//     base_strategy::Strategy predictor3(python_path, path_where_models,
//     // // predictor_module, class_module, method_module);
//     base_strategy::Strategy predictor4(python_path, path_where_models,
//     // // predictor_module, class_module, method_module);
//     base_strategy::Strategy predictor5(python_path, path_where_models,
//     // // predictor_module, class_module, method_module);
//     base_strategy::Strategy predictor6(python_path, path_where_models,
//     // // predictor_module, class_module, method_module);
//     base_strategy::Strategy predictor7(python_path, path_where_models,
//     // // predictor_module, class_module, method_module);
//     base_strategy::Strategy predictor8(python_path, path_where_models,
//     // // predictor_module, class_module, method_module);
//     base_strategy::Strategy predictor9(python_path, path_where_models,
//     // // predictor_module, class_module, method_module);
//     base_strategy::Strategy predictor10(python_path, path_where_models,
//     // // predictor_module, class_module, method_module);
//     base_strategy::Strategy predictor11(python_path, path_where_models,
//     // // predictor_module, class_module, method_module);
//     // }
//     fmtlog::poll();
// }
//----------------------------------------------------------------------------------------
/**
 * @brief test launch trade engine service with
 * 1)KLineService service
 * 2)GeneratorBidAskService service
 * 3)OrderGateway service
 * @param argc
 * @param argv
 * @return int
 */
// int main(int argc, char** argv) {
//     hmac_sha256::Keys keys{argv[2], argv[3]};
//     hmac_sha256::Signer signer(keys);
//     auto type = TypeExchange::TESTNET;
//     fmtlog::setLogLevel(fmtlog::OFF);
//     using namespace binance;
//     Exchange::EventLFQueue event_queue;
//     Exchange::RequestNewLimitOrderLFQueue requests_new_order;
//     Exchange::RequestCancelOrderLFQueue requests_cancel_order;
//     Exchange::ClientResponseLFQueue client_responses;
//     OHLCVILFQueue ohlcv_queue;
//     prometheus::EventLFQueue prometheus_event_queue;
//     OrderNewLimit new_order(&signer, type);
//     CancelOrder executor_cancel_order(&signer, type);
//     DiffDepthStream::ms100 interval;
//     TickerInfo info{2, 5};
//     Symbol btcusdt("BTC", "USDT");
//     Ticker ticker(&btcusdt, info);

//     std::string host  = "localhost";
//     unsigned int port = 6060;
//     prometheus::Service prometheus_service(host, port,
//     &prometheus_event_queue); prometheus_service
//         .Start();  // launch prometheus server that send data to prometheus

//     GeneratorBidAskService generator_bid_ask_service(
//         &event_queue, &prometheus_event_queue, ticker, &interval,
//         TypeExchange::TESTNET);
//     generator_bid_ask_service.Start();

//     Trading::OrderGateway gw(&new_order, &executor_cancel_order,
//                              &requests_new_order, &requests_cancel_order,
//                              &client_responses);
//     gw.start();

//     auto chart_interval = binance::m1();
//     binance::OHLCVI fetcher(&btcusdt, &chart_interval,
//     TypeExchange::TESTNET); KLineService kline_service(&fetcher,
//     &ohlcv_queue); kline_service.start();

//     // init python predictor
//     const auto python_path = argv[1];
//     std::string path_where_models =
//         "/home/linoxoidunix/Programming/cplusplus/cryptobot";
//     auto predictor_module = "strategy.py";
//     auto class_module = "Predictor";
//     auto method_module = "predict";
//     base_strategy::Strategy predictor(python_path, path_where_models,
//                                       predictor_module, class_module,
//                                       method_module);

//     Trading::TradeEngine trade_engine_service(
//         &event_queue, &requests_new_order, &requests_cancel_order,
//         &client_responses, &ohlcv_queue, &prometheus_event_queue, ticker,
//         &predictor);
//     trade_engine_service.Start();

//     common::TimeManager time_manager;
//     while (trade_engine_service.GetDownTimeInS() < 10) {
//     //while (trade_engine_service.GetDownTimeInS() < 120) {
//         logd("Waiting till no activity, been silent for {} seconds...",
//              trade_engine_service.GetDownTimeInS());
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(30s);
//     }
// }
//----------------------------------------------------------------------------------------
/**
 * @brief launch prometheus service
 *
 * @param argc
 * @param argv
 * @return int
 */

// int main(int argc, char** argv) {
//     std::string host = "localhost";
//     unsigned int port = 6060;
//     prometheus::EventLFQueue event_lfqueue;
//     prometheus::Service service (host, port, &event_lfqueue);
//     service.Start();
//     constexpr bool tt = true;
//     //std::cout << a << std::endl;
//     common::TimeManager time_manager;
//     while (service.GetDownTimeInS() < 5) {
//         // logd("Waiting till no activity, been silent for {} seconds...",
//         //      trade_engine_service.GetDownTimeInS());
//         using namespace std::literals::chrono_literals;
//         event_lfqueue.enqueue(prometheus::Event(prometheus::EventType::kLFQueuePushNewBidAsksEvents,
//         common::getCurrentNanoS())); std::this_thread::sleep_for(1s);
//     }
// }

// enum class Color : int { RED = 0, BLUE = 1, GREEN = 2 };
// #include <random>
// int main() {
//     Color color = Color::RED;
//     auto s      = magic_enum::enum_name(color);

//     std::random_device rd;   // a seed source for the random number engine
//     std::mt19937 gen(rd());  // mersenne_twister_engine seeded with rd()
//     std::uniform_int_distribution<> distrib(0, 2);

//     for (int i = 0; i < 10; i++) {
//         const auto random_value = distrib(gen);
//         auto color              = magic_enum::enum_cast<Color>(random_value);
//         if (color.has_value()) {
//             s = magic_enum::enum_name(color.value());
//             std::cout << s.data() << std::endl;
//         }
//     }
//     auto ccc = 0;
// }
//----------------------------------------------------------------------------------------
// #include "aot/common/time_utils.h"
// int main(){
//     fmtlog::setLogLevel(fmtlog::DBG);
//     auto t11 = common::getCurNano();
//     for(int i = 0; i < 1000000; i++)
//     {
//         auto v = 0;
//         v++;
//     }
//     auto t21 = common::getCurNano();
//     logd("new:{}", t21-t11);
//     auto t12 = common::getCurrentNanoS();
//     for(int i = 0; i < 1000000; i++)
//     {
//         auto v = 0;
//         v++;
//     }
//     auto t22 = common::getCurrentNanoS();
//     logd("old:{}",t22-t12);
// }
//----------------------------------------------------------------------------------------
// #include <aot/strategy/market_order_book.h>
// #include <random>
// int main(){
//     fmtlog::setLogLevel(fmtlog::OFF);
//     std::vector<Exchange::MEMarketUpdate> diffs;
//     diffs.reserve(100000);
//     std::random_device dev;
//     std::mt19937 rng(dev());
//     std::uniform_int_distribution<std::mt19937::result_type> dist(1,256); //
//     distribution in range [1, 6]
//     std::uniform_int_distribution<std::mt19937::result_type> dist_qty(0,1);
//     // distribution in range [1, 6]
//     std::uniform_int_distribution<std::mt19937::result_type>
//     dist_action(0,1); // distribution in range [1, 6]

//     for(int i = 0; i < 10000000; i++)
//     {
//         Exchange::MEMarketUpdate market_update;
//         market_update.order_id = 1;
//         market_update.ticker = "USDT";
//         market_update.side = (common::Side) dist_action(rng);
//         market_update.price = dist(rng);
//         market_update.qty = dist_qty(rng);
//         diffs.push_back(market_update);
//     }
//     Trading::MarketOrderBook book;
//     auto begin = common::getCurrentNanoS();
//     auto buffer = 0;
//     for(int i = 0; i < 10000000; i++)
//     {
//         book.onMarketUpdate(&diffs[i]);
//     }
//     auto end = common::getCurrentNanoS();
//     std::cout << (end-begin) / 10000000.0 << "ns" << std::endl;
//     //std::cout << buffer / 100000.0 << "ns" << std::endl;

// }
//----------------------------------------------------------------------------------------
// #include <regex>
// #include <fstream>
// #include <regex>
// #include "aot/market_data/market_update.h"
// int main(){
//     using namespace binance;
//     fmtlog::setLogLevel(fmtlog::OFF);
//     std::ifstream infile(fmt::format("999.txt"));
//     std::string line;
//     std::regex word_regex(".+ MEMarketUpdateDouble\\[ticker:(\\w*)
//     type:(\\w*) side:(\\w+) qty:(\\d+\\.\\d+) price:(\\d+\\.\\d+)\\]");
//     std::smatch pieces_match;
//     std::vector<Exchange::MEMarketUpdate> diffs;
//     diffs.reserve(200000);
//     std::vector<Exchange::MEMarketUpdateDouble> diffs_double;
//     diffs_double.reserve(200000);
//     while (std::getline(infile, line))
//     {
//         //std::cout << line << std::endl;

//         if (std::regex_match(line, pieces_match, word_regex)){

//             Exchange::MEMarketUpdate update;
//             Exchange::MEMarketUpdateDouble update_double;

//             std::string color_name{"GREEN"};
//             //std::cout << pieces_match[1] << " " << pieces_match[2] << " "
//             << pieces_match[3] << " " << pieces_match[4] << " " <<
//             pieces_match[5] << std::endl; auto _type =
//             magic_enum::enum_cast<Exchange::MarketUpdateType>(std::string(pieces_match[2]));
//             if(!_type.has_value())
//                 continue;
//             update.type = _type.value();
//             update_double.type = _type.value();
//             auto _side =
//             magic_enum::enum_cast<common::Side>(std::string(pieces_match[3]));
//             if(!_side.has_value())
//                 continue;
//             update.side = _side.value();
//             update_double.side = _side.value();

//             double qty = stod(pieces_match[4]);
//             update_double.qty = qty;
//             double price = stod(pieces_match[5]);
//             update_double.price = price;

//             update.qty =  qty * 10000;
//             update.price =  price * 100;
//             diffs.push_back(update);
//             diffs_double.push_back(update_double);

//         }
//     }
//     Trading::MarketOrderBook book;
//     auto begin = common::getCurrentNanoS();
//     for(int i = 0; i < diffs.size(); i++)
//     {
//         book.onMarketUpdate(&diffs[i]);
//     }
//     auto end = common::getCurrentNanoS();
//     std::cout << (end-begin)*1.0 / diffs.size() << "ns" << std::endl;
//     std::cout << diffs.size() << std::endl;

//     TickerInfo info{2, 5};
//     Symbol btcusdt("BTC", "USDT");
//     Ticker ticker(&btcusdt, info);
//     Trading::MarketOrderBookDouble book_double(ticker);
//     auto begin1 = common::getCurrentNanoS();
//     for(int i = 0; i < diffs_double.size(); i++)
//     {
//         book_double.OnMarketUpdate(&diffs_double[i]);
//     }
//     auto end1 = common::getCurrentNanoS();
//     std::cout << (end1-begin1)*1.0 / diffs_double.size() << "ns" <<
//     std::endl; std::cout << diffs_double.size() << std::endl;

//     Trading::MarketOrderBook book2;
//     auto begin2 = common::getCurrentNanoS();
//     for(int i = 0; i < diffs_double.size(); i++)
//     {
//         const Exchange::MEMarketUpdate buf(&diffs_double[i], 2,
//                                        5);
//         book2.onMarketUpdate(&buf);
//     }
//     auto end2 = common::getCurrentNanoS();
//     std::cout << (end2-begin2)*1.0 / diffs_double.size() << "ns" <<
//     std::endl; std::cout << diffs_double.size() << std::endl;
// }
//----------------------------------------------------------------------------------------
/**
 * @brief test market order book without GeneratorBidAskService
 * KLineService will generate events for order book
 * @param argc
 * @param argv
 * @return int
 */
// int main(int argc, char** argv) {
//     hmac_sha256::Keys keys{argv[2], argv[3]};
//     hmac_sha256::Signer signer(keys);
//     auto type = TypeExchange::TESTNET;
//     fmtlog::setLogLevel(fmtlog::DBG);
//     using namespace binance;
//     Exchange::EventLFQueue event_queue;
//     Exchange::RequestNewLimitOrderLFQueue requests_new_order;
//     Exchange::RequestCancelOrderLFQueue requests_cancel_order;
//     Exchange::ClientResponseLFQueue client_responses;
//     OHLCVILFQueue internal_ohlcv_queue;
//     OHLCVILFQueue external_ohlcv_queue;
//     prometheus::EventLFQueue prometheus_event_queue;
//     OrderNewLimit new_order(&signer, type);
//     CancelOrder executor_cancel_order(&signer, type);
//     DiffDepthStream::ms100 interval;
//     TickerInfo info{2, 5};
//     Symbol btcusdt("BTC", "USDT");
//     Ticker ticker(&btcusdt, info);

//     auto chart_interval = binance::m1();
//     binance::OHLCVI fetcher(&btcusdt, &chart_interval,
//     TypeExchange::TESTNET); backtesting::KLineService kline_service(&fetcher,
//     &internal_ohlcv_queue, &external_ohlcv_queue, &event_queue);
//     kline_service.start();

//     Trading::TradeEngine trade_engine_service(
//         &event_queue, &requests_new_order, &requests_cancel_order,
//         &client_responses, &external_ohlcv_queue, &prometheus_event_queue,
//         ticker, nullptr);
//     trade_engine_service.Start();

//     common::TimeManager time_manager;
//     while (trade_engine_service.GetDownTimeInS() < 10) {
//         // while (trade_engine_service.GetDownTimeInS() < 120) {
//         logd("Waiting till no activity, been silent for {} seconds...",
//              trade_engine_service.GetDownTimeInS());
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(30s);
//     }
// }
//----------------------------------------------------------------------------------------
/**
 * @brief test market order book without GeneratorBidAskService
 * KLineService will generate events for order book
 * @param argc
 * @param argv
 * @return int
 */
// int main(int argc, char** argv) {
//     fmtlog::setLogLevel(fmtlog::INF);
//     backtesting::OHLCVI
//     ohlcv_history("/home/linoxoidunix/Programming/cplusplus/cryptobot/aot_data/ohlcv_history/ohlcv.csv");
//     OHLCVILFQueue ohlcv_queue;
//     ohlcv_history.Init(ohlcv_queue);
//     for(int i = 0; i < 5900; i++)
//         ohlcv_history.LaunchOne();
//     fmtlog::poll();
// }
//----------------------------------------------------------------------------------------
/**
 * @brief test market order book with backtesting::OHLCV
 * KLineService will generate events for order book
 * @param argc
 * @param argv
 * @return int
 */
// int main(int argc, char** argv) {
//     hmac_sha256::Keys keys{argv[2], argv[3]};
//     hmac_sha256::Signer signer(keys);
//     auto type = TypeExchange::TESTNET;
//     fmtlog::setLogLevel(fmtlog::OFF);
//     //fmtlog::setLogFile("111.txt");
//     using namespace binance;
//     Exchange::EventLFQueue event_queue;
//     Exchange::RequestNewLimitOrderLFQueue requests_new_order;
//     Exchange::RequestCancelOrderLFQueue requests_cancel_order;
//     Exchange::ClientResponseLFQueue client_responses;
//     OHLCVILFQueue internal_ohlcv_queue;
//     OHLCVILFQueue external_ohlcv_queue;
//     prometheus::EventLFQueue prometheus_event_queue;
//     OrderNewLimit new_order(&signer, type);
//     CancelOrder executor_cancel_order(&signer, type);
//     DiffDepthStream::ms100 interval;
//     TickerInfo info{2, 5};
//     Symbol btcusdt("BTC", "USDT");
//     Ticker ticker(&btcusdt, info);

//     Trading::TradeEngine trade_engine_service(
//         &event_queue, &requests_new_order, &requests_cancel_order,
//         &client_responses, &external_ohlcv_queue, &prometheus_event_queue,
//         ticker, nullptr);

//     auto chart_interval = binance::m1();
//     backtesting::OHLCVI fetcher(
//         "/home/linoxoidunix/Programming/cplusplus/cryptobot/aot_data/ohlcv_history/ohlcv.csv",
//         &trade_engine_service);
//     backtesting::KLineService kline_service(
//         &fetcher, &internal_ohlcv_queue, &external_ohlcv_queue,
//         &event_queue);

//     trade_engine_service.Start();
//     kline_service.start();

//     common::TimeManager time_manager;
//     while (trade_engine_service.GetDownTimeInS() < 10) {
//         // while (trade_engine_service.GetDownTimeInS() < 120) {
//         logd("Waiting till no activity, been silent for {} seconds...",
//              trade_engine_service.GetDownTimeInS());
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(30s);
//     }
// }
//----------------------------------------------------------------------------------------
// int main(int argc, char** argv) {

//     // std::string path_where_models =
//     //     "/home/linoxoidunix/Programming/cplusplus/cryptobot";
//     // std::string path_where_models =
//     //     "/home/linoxoidunix/.venv/lib/python3.11/site-packages:/home/linoxoidunix/Programming/cplusplus/cryptobot/aot/python";
//     std::string path_where_models = argv[1];
//     auto predictor_module = "strategy.py";
//     auto class_module = "Predictor";
//     auto method_module = "predict";

//     setenv("PYTHONPATH", path_where_models.c_str(), 1);
//     Py_Initialize();
//     PyRun_SimpleString("import sys\nprint(sys.path)");
//     auto file_predictor_ = std::string(predictor_module);
//     size_t lastindex                 = file_predictor_.find_last_of(".");
//     auto file_name_without_extension = file_predictor_.substr(0, lastindex);
//     auto file_name_ = PyUnicode_DecodeFSDefault(file_name_without_extension.c_str());
//     auto module_name_ = PyImport_Import(file_name_);
//     if(module_name_)
//         std::cout << "success" << std::endl;
//     int x = 0;
// }
//----------------------------------------------------------------------------------------
// int main(){
//     return 0;
// }


