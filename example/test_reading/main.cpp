#include <aot/WS.h>

#include <boost/beast/core.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "aot/Binance.h"
#include "aot/Bybit.h"
#include "aot/Logger.h"
#include "aot/Predictor.h"
#include "aot/strategy/market_order_book.h"
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
//    Trading::MarketOrderBook order_book(12345);
// }

// int main() {
//     using namespace binance;
//     DiffDepthStream::ms100 interval;
//     Symbol btcusdt("BTC", "USDT");
//     BookEventGetter event_capturer(&btcusdt, &interval);
//     BookEvent buffer;
//     event_capturer.Get(buffer);
// }

// int main() {
//     using namespace binance;
//     BookSnapshot::ArgsOrder args{"BTCUSDT", 1000};
//     BookSnapshot book_snapshoter(std::move(args), TypeExchange::TESTNET);
//     book_snapshoter.Exec();
// }

int main() {
    fmtlog::setLogLevel(fmtlog::DBG);

    using namespace binance;
    Exchange::EventLFQueue event_queue;
    DiffDepthStream::ms100 interval;
    Symbol btcusdt("BTC", "USDT");
    GeneratorBidAskService generator(&event_queue, &btcusdt, &interval);
    generator.Start();
    while (generator.GetDownTimeInS() < 120) {
        logd("Waiting till no activity, been silent for {} seconds...",
             generator.GetDownTimeInS());
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(30s);
    }
}
