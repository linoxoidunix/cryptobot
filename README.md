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

thanks to https://martin.ankerl.com/2022/08/27/hashmap-bench-01/

# TODO
1. for order manager need consider riskmanager 

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

# Detail aot/python
1. aot/python/downloader.py - download raw kline from binance exchange
2. aot/python/evaluate_trading_signals cpp.py - save X(open, high, low, close, volume and some features) dataset as features_df.h5 for Predictor class. Save top 3 best LightGBM model in .txt for evaluate y
3. aot/python/evaluate_trading_signals.py - test file for learning lightgbm
4. aot/python/machine_learning.py - test file for learning C++ wrapper for python
5. aot/python/my_types.py - module that define enter_long, enter_short, exit_long, exit_short signals as string
6. aot/python/prepare_features_target cpp.py - module that calculate some features from raw klines as Python notebook
7. aot/python/prepare_features_target.py - test file for learning calculate features
8. aot/python/prepare_model_for_bot.py - module that calculate some features from raw klines as Python class
9. aot/python/prepare_model.py - test file for learning LightGBM
10. aot/python/strategy.py - Python class with simple API that generate enter_long, enter_short, exit_long, exit_short signals as string
11. aot/python/utils.py - utils that help split dataset for cross validation


