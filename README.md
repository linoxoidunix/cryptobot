# cryptobot
bot for algo trading on crypto exchanges

# Features
1. Post order for Binance and Bybit
2. Capture __raw__ exchange market order book update events for Binance
3. Capture __raw__ Klines for Binance and Bybit  
4. Capture __raw__ order book snapshot for Binance
5. Order book sync snap and diff for binance
6. Get bbo using order book for binance 
7. Order gateway [testing for BINANCE] 

# Definitions
1. Service - entity that runs in a separate thread
    * GeneratorBidAskService - a service that generate new bid and new ask events in lock-free queue
    * KLineService - a service that generate new klines in lock-free queue
    * Trading::OrderGateway - a service that sends buy and sell orders to the exchange. It also processes messages from the exchange about the status of execution of these orders
    * Trading::TradeEngine - a service that processes incoming client responses and market data updates(klines or bid/ask events) which in turn may generate client requests.
2. In strategy.py Predictor is class that generates enter_long, enter_short, exit_long, exit_short signals on new klines from exchange using LightGBM
3. base_strategy::Strategy is cpp wrapper for Predictor class 
4. Trading::BaseStrategy is entity that can buy or sell asset using enter_long, enter_short, exit_long, exit_short signals.
5. Trading::OrderManager have 2 public methods NewOrder and CancelOrder and OnOrderResponse callback which must be called when a new Exchange::MEClientResponse* event arrives from the exchange
6. MarketOrderBook is entity that maintains the local order book based on bid and ask events received from the exchange
7. Common::TradeEngineCfgHashMap is hashmap that contains clip for each ticker  
8. Trading::PositionKeeper is entity that controls realized or unrealized pnls

# Bot's scheme
![Alt Text](/doc/ForREADME/scheme_project.png)

# Restrictions
* C++20
* Boost 1.65.0
* CMake 3.29
1. OrderManager impl supports only 1 active order for each side!!!!
2. OrderBook: 
    * uses boost::intrusive::avltree to sort prices into levels in ascending or descending order
    * at each price level has a pointer to the structure, which stores the price, side, amount of asset at this level. At each price level there is no ordered list of orders depending on their priority

# Third party C++ library
1. https://github.com/cameron314/concurrentqueue
2. https://github.com/fmtlib/fmt
3. https://github.com/MengRao/fmtlog
4. https://github.com/ktprime/emhash
5. https://github.com/simdjson/simdjson
6. https://github.com/nlohmann/json
7. https://github.com/boostorg/boost
8. https://github.com/openssl/openssl
9. Extending Python with C or C++
10. https://github.com/google/benchmark

# Third party Python library
1. https://github.com/microsoft/LightGBM/tree/d73c6b530b39a18a3cacaafc4e42550be853c036
2. https://pypi.org/project/binance-ohlcv/
3. https://pypi.org/project/TA-Lib/
4. https://pypi.org/project/numpy/
5. https://pypi.org/project/pandas/

# Details aot/python
1. aot/python/downloader.py - download raw kline from binance exchange
2. aot/python/evaluate_trading_signals cpp.py - save X(open, high, low, close, volume and some features) dataset as features_df.h5 for Predictor class. Save top 3 best LightGBM model in .txt for evaluate y
3. aot/python/evaluate_trading_signals.py - test file for learning lightgbm
4. aot/python/machine_learning.py - test file for learning C++ wrapper for python
5. aot/python/my_types.py - module that define enter_long, enter_short, exit_long, exit_short signals as string
6. aot/python/prepare_features_target cpp.py - module that calculate some features from raw klines as Python notebook
7. aot/python/prepare_features_target.py - test file for learning calculate features
8. __aot/python/prepare_model_for_bot.py - module that calculate some features from raw klines as Python class__
9. aot/python/prepare_model.py - test file for learning LightGBM
10. __aot/python/strategy.py - Python class with simple API that generate enter_long, enter_short, exit_long, exit_short signals as string__
11. aot/python/utils.py - utils that help split dataset for cross validation

# Basic usage
* Cmake
```
set (PROJECT_NAME test_reading)
project(${PROJECT_NAME})


file(GLOB SRC
     "*.cpp"
)


add_executable (${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
aot
concurrentqueue::concurrentqueue
Python::Python
${Boost_LIBRARIES}
unordered_dense::unordered_dense
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
```
1. For Binance launch trade engine + generator bid/ask events service. check update BBO and BBODouble
```c++
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
#include "aot/strategy/kline.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/trade_engine.h"
#include "aot/third_party/emhash/hash_table7.hpp"
#include "aot/launcher_predictor.h"
#include "moodycamel/concurrentqueue.h"

int main() {
    fmtlog::setLogLevel(fmtlog::INF);
    using namespace binance;
    Exchange::EventLFQueue event_queue;//this queue contains bids and asks from the bid ask generator
    Exchange::RequestNewLimitOrderLFQueue request_new_order;//this queue contains requests for new limit order
    Exchange::RequestCancelOrderLFQueue request_cancel_order;//this queue contains requests for cancel order by id
    Exchange::ClientResponseLFQueue response;//this queue contains response from exchange when you send new order or send request cancel order
    OHLCVILFQueue ohlcv_queue;//this queue contains klines from exchange. in this example this queue always empty
    DiffDepthStream::ms100 interval;
    TickerInfo info{2, 5};//set manual price precission and qty precission for ticker
    Symbol btcusdt("BTC", "USDT");
    Ticker ticker(&btcusdt, info);
    GeneratorBidAskService generator(&event_queue, ticker, &interval,
                                    TypeExchange::TESTNET);
    generator.Start();
    Trading::TradeEngine trade_engine_service(&event_queue,
    &request_new_order, &request_cancel_order,  &response, &ohlcv_queue, ticker, nullptr);
    trade_engine_service.Start();
    while (trade_engine_service.GetDownTimeInS() < 120) {
        logd("Waiting till no activity, been silent for {} seconds...",
            generator.GetDownTimeInS());
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(30s);
    }
}
```
![Alt Text](/doc/ForREADME/check_bbo.gif)

2. For Binance check response when send new limit order
```c++
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
#include "aot/strategy/kline.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/trade_engine.h"
#include "aot/third_party/emhash/hash_table7.hpp"
#include "aot/launcher_predictor.h"
#include "moodycamel/concurrentqueue.h"

int main(int argc, char** argv) {
    fmtlog::setLogLevel(fmtlog::DBG);

    using namespace binance;
    hmac_sha256::Keys keys{argv[2], argv[3]};//set api key and secret key
    hmac_sha256::Signer signer(keys); //init hmac_sha256 signer
    auto type = TypeExchange::TESTNET;
    OrderNewLimit new_order(&signer, type);//init executor for send new order limit
    CancelOrder executor_cancel_order(&signer, type);//init executor for cancel order by id
    using namespace Trading;
    Exchange::RequestNewLimitOrderLFQueue requests_new_order;
    Exchange::RequestCancelOrderLFQueue requests_cancel_order;
    Exchange::ClientResponseLFQueue client_responses;
    Exchange::RequestNewOrder request_new_order;//start init manual request new order
    request_new_order.ticker   = "BTCUSDT";
    request_new_order.order_id = 6;
    request_new_order.side     = Common::Side::BUY;
    request_new_order.price    = 40000;
    request_new_order.qty      = 0.001;

    requests_new_order.enqueue(request_new_order);
    OrderGateway gw(&new_order, &executor_cancel_order, &requests_new_order,
    &requests_cancel_order,
                    &client_responses);
    gw.start();//start order gateway for process RequestNewOrder 
    while (gw.GetDownTimeInS() < 7) {
        logd("Waiting till no activity, been silent for {} seconds...",
            gw.GetDownTimeInS());
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(3s);
    }

    Exchange::MEClientResponse response[50]; 

    size_t count_new_order = client_responses.try_dequeue_bulk(response, 50);
    for (int i = 0; i < count_new_order; i++) {
        logd("{}", response[i].ToString());//check response on new order
    }

    return 0;
}
```
![Alt Text](/doc/ForREADME/check_response.png)

3. For Binance test add and cancel order
```c++
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
#include "aot/strategy/kline.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/trade_engine.h"
#include "aot/third_party/emhash/hash_table7.hpp"
#include "aot/launcher_predictor.h"
#include "moodycamel/concurrentqueue.h"

int main(int argc, char** argv) {
    hmac_sha256::Keys keys{argv[2], argv[3]};
    hmac_sha256::Signer signer(keys);
    auto type = TypeExchange::TESTNET;
    fmtlog::setLogLevel(fmtlog::DBG);
    using namespace binance;
    OrderNewLimit new_order(&signer, type);
    CancelOrder executor_cancel_order(&signer, type);
    using namespace Trading;
    Exchange::RequestNewLimitOrderLFQueue requests_new_order;
    Exchange::RequestCancelOrderLFQueue requests_cancel_order;
    Exchange::ClientResponseLFQueue client_responses;

    Exchange::RequestNewOrder request_new_order;
    request_new_order.ticker   = "BTCUSDT";
    request_new_order.order_id = 6;//set manual unique id for new buy order 
    request_new_order.side     = Common::Side::BUY;
    request_new_order.price    = 40000;
    request_new_order.qty      = 0.001;

    requests_new_order.enqueue(request_new_order);
    Exchange::RequestCancelOrder order_for_cancel;
    order_for_cancel.ticker   = "BTCUSDT";
    order_for_cancel.order_id = 6;//cancel order by manual id

    requests_cancel_order.enqueue(order_for_cancel);
    OrderGateway gw(&new_order, &executor_cancel_order, &requests_new_order,
                    &requests_cancel_order, &client_responses);
    gw.start();
    while (gw.GetDownTimeInS() < 7) {
        logd("Waiting till no activity, been silent for {} seconds...",
            gw.GetDownTimeInS());
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(3s);
    }

    Exchange::MEClientResponse response[50];  /

    size_t count_new_order = client_responses.try_dequeue_bulk(response, 50);
    for (int i = 0; i < count_new_order; i++) {
        logd("{}", response[i].ToString());
    }

    return 0;
}
```
![Alt Text](/doc/ForREADME/add_and_cancel_order.png)

# TODO
1. for order manager need consider riskmanager

# Acknowledgment
* https://martin.ankerl.com/2022/08/27/hashmap-bench-01/



