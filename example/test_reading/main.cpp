#include <bybit/WS.h>

#include <iostream>
#include <memory>
// #include <bybit/root_certificates.hpp>
#include <bybit/Binance.h>
#include <bybit/Bybit.h>
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

// int main() {
//     std::function<void(std::string)> func;
//     func = [](std::string) {};
//     net::io_context ioc;
//     using klsb = binance::KLineStreamBinance;
//     Symbol btcusdt("BTC", "USDT");
//     auto chart_interval = binance::m1();
//     klsb channel(btcusdt, &chart_interval);
//     std::make_shared<WS>(ioc
//                          // func, func, func, func
//                          )
//         ->Run("stream.binance.com", "9443",
//               fmt::format("/ws/{0}", channel.ToString()));
//     ioc.run();
//     return 0;
// }

int main() {
    std::function<void(std::string)> func;
    func = [](std::string) {};
    net::io_context ioc;
    using klsb = bybit::KLineStreamBybit;
    Symbol btcusdt("BTC", "USDT");
    auto chart_interval = bybit::m1();
    klsb channel(btcusdt, &chart_interval);
    std::make_shared<WS>(ioc
                         // func, func, func, func
                         )
        ->Run("stream-testnet.bybit.com/v5/public/spot", "9443",
              fmt::format("/{0}", channel.ToString()));
    ioc.run();
    return 0;
}