#include "aot/market_data/market_update.h"
#include "aot/Logger.h"
#include <cmath>
#include <string>


Exchange::BookSnapshot& Exchange::BookSnapshot::operator=(Exchange::BookSnapshot2&& element){
        exchange_id = std::move(element.exchange_id);
        trading_pair = std::move(element.trading_pair);
        bids         = std::move(element.bids);
        asks         = std::move(element.asks);
        lastUpdateId = element.lastUpdateId;
        return *this;
    }