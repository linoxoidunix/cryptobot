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
#include "aot/common/types.h"
#include "aot/order_gw/order_gw.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/trade_engine.h"
#include "aot/third_party/emhash/hash_table7.hpp"
//#include "aot/strategy/position_keeper.h"
#include "moodycamel/concurrentqueue.h"
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
//     //using namespace bybit;
//     using klsb = binance::KLineStream;
//     binance::Symbol btcusdt("BTC", "USDT");
//     auto chart_interval = binance::m1();
//     OHLCVIStorage storage;
//     binance::OHLCVI fetcher(&btcusdt, &chart_interval);
//     fetcher.Get(storage);
//     return 0;
// }

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
// std::array<Trading::MarketOrder *, Common::ME_MAX_ORDER_IDS> map;
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
// orders_at_price_pool_(Common::ME_MAX_PRICE_LEVELS);
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

// int main() {
//     fmtlog::setLogLevel(fmtlog::INF);
//     using namespace binance;
//     Exchange::EventLFQueue event_queue;
//     DiffDepthStream::ms100 interval;
//     TickerInfo info{2, 5};
//     Symbol btcusdt("BTC", "USDT");
//     Ticker ticker(&btcusdt, info);
//     GeneratorBidAskService generator(&event_queue, ticker, &interval,
//                                      TypeExchange::TESTNET);
//     generator.Start();
//     Trading::TradeEngine trade_engine_service(&event_queue, ticker);
//     trade_engine_service.Start();
//     while (trade_engine_service.GetDownTimeInS() < 120) {
//         logd("Waiting till no activity, been silent for {} seconds...",
//              generator.GetDownTimeInS());
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(30s);
//     }
// }

// int main()
// {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     // auto x = 12345;
//     // logd("for x={}, Digits10={}", x, Common::Digits10(x));
//     int exp;
//     auto x = 12345.567000001;
//     std::frexp(x, &exp);
//     logd("for x={}, length_fractional_part={}", x,
//     Common::LeengthFractionalPart(x));

//     return 0;
// }

// int main()
// {
//     fmtlog::setLogLevel(fmtlog::DBG);
//     emhash7::HashMap<int, double> map;
//     map.emplace_unique(1, 0.1);
//     map.emplace_unique(2, 0.2);
//     logd("size:{} value:{}", map.size(), map.at(2));
//     return 0;
// }

int main(int argc, char** argv) {
    fmtlog::setLogLevel(fmtlog::DBG);

    using namespace binance;
    OrderNewLimit::ArgsOrder args{
        "BTCUSDT",         0.001,      40000, TimeInForce::FOK,
        Common::Side::BUY, Type::LIMIT};
    hmac_sha256::Keys keys{argv[1], argv[2]};
    hmac_sha256::Signer signer(keys);
    auto type = TypeExchange::TESTNET;
    OrderNewLimit new_order(&signer, type);
    using namespace Trading;
    Exchange::RequestNewLimitOrderLFQueue requests_new_order;
    Exchange::RequestCancelOrderLFQueue requests_cancel_order;
    Exchange::ClientResponseLFQueue client_responses;
    Exchange::RequestNewOrder request_new_order;
    request_new_order.ticker   = "BTCUSDT";
    request_new_order.order_id = 6;
    request_new_order.side     = Common::Side::BUY;
    request_new_order.price    = 40000;
    request_new_order.qty      = 0.001;

    requests_new_order.enqueue(request_new_order);
    OrderGateway gw(&new_order, &requests_new_order, &requests_cancel_order,
                    &client_responses);
    gw.start();
    while (gw.GetDownTimeInS() < 40) {
        logd("Waiting till no activity, been silent for {} seconds...",
             gw.GetDownTimeInS());
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(3s);
    }

    Exchange::MEClientResponse response[50];  // Could also be any iterator

    size_t count_new_order = client_responses.try_dequeue_bulk(response, 50);
    for (int i = 0; i < count_new_order; i++) {
        logd("{}", response[i].ToString());
    }

    return 0;
}