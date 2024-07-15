# cryptobot
bot for algo trading

# Features
1. Post order for Binance and Bybit
2. Capture __raw__ exchange market order book update events for Binance
3. Capture __raw__ Klines for Binance and Bybit  
4. Capture __raw__ order book snapshot for Binance
5. Order book sync snap and diff for binance
6. Get bbo using order book for binance 
7. Order gateway [PARTITIAL DEBUG for BINANCE] 

thanks to https://martin.ankerl.com/2022/08/27/hashmap-bench-01/

# TODO
1. for order manager need consider riskmanager 

# Definitions
1. Service - entity that runs in a separate thread
    * GeneratorBidAskService - a service that generate new bid and new ask events in lock-free queue
    * KLineService - a service that generate new klines in lock-free queue
    * OrderGateway - a service that sends buy and sell orders to the exchange. It also processes messages from the exchange about the status of execution of these orders
    * TradeEngine - a service that processes incoming client responses and market data updates(klines or bid/ask events) which in turn may generate client requests.
2. Trading::BaseStrategy is entity that generates enter_long, enter_short, exit_long, exit_short signal on new klines from exchange.
3. Trading::OrderManager have 2 public methods NewOrder and CancelOrder and OnOrderResponse callback which must be called when a new Exchange::MEClientResponse * event arrives from the exchange
4. MarketOrderBook is entity that maintains the local order book based on bid and ask events received from the exchange

+ 123123
- 123123
# Restrictions
+ 123123
- 123123
* C++20
* Boost 1.65.0
* CMake 3.29
1. OrderManager impl supports only 1 active order for each side!!!!
2. OrderBook: 
    * uses boost::intrusive::avltree to sort prices into levels in ascending or descending order
    * at each price level has a pointer to the structure, which stores the price, side, amount of asset at this level. At each price level there is no ordered list of orders depending on their priority

# Third party library
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

