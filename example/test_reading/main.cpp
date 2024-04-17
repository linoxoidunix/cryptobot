#include <iostream>
#include <memory>
#include <bybit/WS.h>
#include <bybit/root_certificates.hpp>

int main()
{
    std::function<void(std::string)> func;
    func = [](std::string){};
    net::io_context ioc;
    std::make_shared<WS>(
        ioc
        // func, func, func, func
        )->Run("stream.binance.com", "9443", "/ws/bnbusdt@depth@100ms");
    ioc.run();
    return 0;
}