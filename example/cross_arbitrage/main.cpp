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
#include "magic_enum/magic_enum.hpp"
//#include "moodycamel/concurrentqueue.h"//if link as 3rd party
#include "concurrentqueue.h"//if link form source

// //-----------------------------------------------------------------------------------
/**
 * @brief launch generator bid ask service + order book, that push event to lfqueu
 *
 * @return int
 */

int main() {
    return 0;
}