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
#include "aot/common/types.h"
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

// //-----------------------------------------------------------------------------------
/**
 * @brief launch generator bid ask service + order book, that push event to lfqueu
 *
 * @return int
 */

int main() {
    fmtlog::setLogLevel(fmtlog::DBG);
    using namespace binance;
    Exchange::EventLFQueue event_queue;
    prometheus::EventLFQueue prometheus_event_queue;
    Exchange::RequestNewLimitOrderLFQueue request_new_order;
    Exchange::RequestCancelOrderLFQueue request_cancel_order;
    Exchange::ClientResponseLFQueue response;
    OHLCVILFQueue ohlcv_queue;
    DiffDepthStream::ms100 interval;
    TickerHashMap tickers;
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

    GeneratorBidAskService generator_bid_ask(&event_queue, &prometheus_event_queue,
                                     pair[{2, 1}],tickers, TradingPair{2,1}, &interval,
                                     TypeExchange::TESTNET);
    
    strategy::cross_arbitrage::LFQueue queue;
    position_keeper::EventLFQueue orderbook_positionkeeper_channel;
    strategy::cross_arbitrage::OrderBook ob(1, TradingPair{2,1}, pair, &queue, &orderbook_positionkeeper_channel, 1000, 1000, 1000);
    
    Trading::OrderBookService orderbook_service(&ob, &event_queue);

    
    orderbook_service.Start();
    generator_bid_ask.Start();

    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(5s);
    generator_bid_ask.Stop();
    orderbook_service.StopWaitAllQueue();
    //orderbook_service.StopImmediately();
    fmtlog::poll();
}