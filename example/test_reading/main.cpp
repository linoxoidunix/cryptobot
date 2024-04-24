#include <bybit/WS.h>

#include <iostream>
#include <memory>
// #include <bybit/root_certificates.hpp>
#include <bybit/Binance.h>
#include <bybit/Bybit.h>

#include <boost/beast/core.hpp>
#include <thread>
#include <fstream>
//#define FMT_HEADER_ONLY
//#include <bybit/third_party/fmt/core.h>
// #define FMTLOG_HEADER_ONLY
// #include <bybit/third_party/fmtlog.h>
// int main()
// {
//     std::function<void(std::string)> func;
//     func = [](std::string){};
//     net::io_context ioc;
//     std::make_shared<WS>(
//         ioc
//         // func, func, func, func
//         )->Run("stream.binance.com", "9443", "/ws/bnbusdt@depth@100ms");
//     ioc.run();
//     return 0;
// }

int main() {
    net::io_context ioc;
    fmtlog::setLogFile("log", true);
    fmtlog::setLogLevel(fmtlog::DBG);

    std::function<void(boost::beast::flat_buffer & buffer)> OnMessageCB;
    OnMessageCB = [](boost::beast::flat_buffer& buffer) {
        auto resut = boost::beast::buffers_to_string(buffer.data());
        logi("{}", resut);
        fmtlog::poll();
    };

    using klsb = binance::KLineStreamBinance;
    Symbol btcusdt("BTC", "USDT");
    auto chart_interval = binance::m1();
    klsb channel(btcusdt, &chart_interval);
    std::make_shared<WS>(ioc, OnMessageCB
                         // func, func, func, func
                         )
        ->Run("stream.binance.com", "9443",
              fmt::format("/ws/{0}", channel.ToString()));
    ioc.run();
    return 0;
}

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