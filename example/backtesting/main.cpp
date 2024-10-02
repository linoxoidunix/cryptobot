#define PY_SSIZE_T_CLEAN


#include <boost/beast/core.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include "magic_enum.hpp"
//#include "moodycamel/concurrentqueue.h"//if link as 3rd party
#include "concurrentqueue.h"//if link form source

#include "aot/Binance.h"
#include "aot/Bybit.h"
#include "aot/Logger.h"
#include "aot/Predictor.h"
#include "aot/common/types.h"
#include "aot/config/config.h"
#include "aot/launcher_predictor.h"
#include "aot/order_gw/order_gw.h"
#include "aot/prometheus/service.h"
#include "aot/strategy/kline.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/trade_engine.h"
#include "aot/third_party/emhash/hash_table7.hpp"
#include "aot/WS.h"

int main(int argc, char** argv) {
    config::BackTesting config(argv[1]);
    using namespace binance;
    fmtlog::setLogLevel(fmtlog::DBG);
    common::TickerHashMap tickers;
    tickers[1] = "usdt";
    tickers[2] = "btc";
    
    TradingPairHashMap pair;
    common::TradingPairInfo pair_info{
        .price_precission = 2,
        .qty_precission = 5,
        .https_json_request = "BTCUSDT",
        .https_query_request = "BTCUSDT",
        .ws_query_request = "btcusdt",
        .https_query_response = "BTCUSDT"
        };
    pair[{2, 1}] = pair_info;
    
    OHLCVILFQueue internal_ohlcv_queue;
    OHLCVILFQueue external_ohlcv_queue;
    

    //fmtlog::setLogFile("888.txt");
    
    //init python predictor
    // const auto python_path = argv[1];
    // std::string path_where_models =
    //     "/home/linoxoidunix/Programming/cplusplus/cryptobot";
    // auto predictor_module = "strategy.py";
    // auto class_module     = "Predictor";
    // auto method_module    = "predict";
    // base_strategy::Strategy predictor(python_path, path_where_models,
    //                                   predictor_module, class_module,
    //                                   method_module);

    backtesting::TradeEngine trade_engine_service(
        &external_ohlcv_queue, TradingPair{2,1}, pair, nullptr);


    auto chart_interval = binance::m1();
    
    auto [status, path_to_history_data] = config.PathToHistoryData();
    if(!status)[[unlikely]]{
        loge("path to history data is not correct");
        fmtlog::poll();
        return 0;
    }
    backtesting::OHLCVI fetcher(
        path_to_history_data,
        &trade_engine_service, TradingPair{2,1}, pair[{2, 1}]);
        
    backtesting::KLineService kline_service(
        &fetcher, &internal_ohlcv_queue, &external_ohlcv_queue);

    trade_engine_service.Start();
    kline_service.start();

    common::TimeManager time_manager;
    while (trade_engine_service.GetDownTimeInS() < 4) {
        // while (trade_engine_service.GetDownTimeInS() < 120) {
        logd("Waiting till no activity, been silent for {} seconds...",
             trade_engine_service.GetDownTimeInS());
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
    logi("{}", trade_engine_service.GetStatistics());
    fmtlog::poll();
}