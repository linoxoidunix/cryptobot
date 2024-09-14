#include "aot/strategy/market_order_book.h"

#include <algorithm>

#include "aot/Logger.h"
#include "aot/strategy/trade_engine.h"

// #include "trade_engine.h"

namespace Trading {
MarketOrderBook::MarketOrderBook(common::TradingPair trading_pair, common::TradingPairHashMap& pairs)
    : orders_at_price_pool_(common::ME_MAX_ORDERS_AT_PRICE),trading_pair_(trading_pair),pairs_(pairs) {}

MarketOrderBook::~MarketOrderBook() {
    logi("call ~MarketOrderBook()");
    ClearOrderBook();
}

/// Process market data update and update the limit order book.
auto MarketOrderBook::OnMarketUpdate(
    const Exchange::MEMarketUpdate *market_update) noexcept -> void {
    if (market_update->type == Exchange::MarketUpdateType::CLEAR) [[unlikely]] {
        ClearOrderBook();
        return;
    }

    const auto bid_will_be_updated =
        (!bids_at_price_map_.size() &&
             market_update->side == common::Side::SELL ||
         bids_at_price_map_.size() &&
             market_update->side == common::Side::SELL &&
             market_update->price >= bids_at_price_map_.begin()->price_);
    const auto ask_will_be_updated =
        (!asks_at_price_map_.size() &&
             market_update->side == common::Side::BUY ||
         asks_at_price_map_.size() &&
             market_update->side == common::Side::BUY &&
             market_update->price <= asks_at_price_map_.begin()->price_);

    if (market_update->qty != 0) {
        MarketOrder order(common::kOrderIdInvalid, market_update->side,
                          market_update->price, market_update->qty);
        addOrder(&order);
    } else {
        removeOrdersAtPrice(market_update->side, market_update->price);
    }

    updateBBO(bid_will_be_updated, ask_will_be_updated);
    if (bid_will_be_updated || ask_will_be_updated) logi("{}", bbo_.ToString());
    logd("{}", market_update->ToString());
}

void OrderBookService::Run() {
    if (!ob_) [[unlikely]] {
        logw("Can't run OrderBookService. ob_ = nullptr");
        return;
    }
    if (!queue_) [[unlikely]] {
        logw("Can't run OrderBookService. queue_ = nullptr");
        return;
    }
    logi("OrderBookService start");
    Exchange::MEMarketUpdate results[50];

    while (run_) {
        size_t count = queue_->try_dequeue_bulk(results, 50);
        for (uint i = 0; i < count; i++) [[likely]] {
            ob_->OnMarketUpdate(&results[i]);
        }
    }
}
}  // namespace Trading

namespace backtesting {

MarketOrderBook::~MarketOrderBook() { logi("call ~backtestin::MarketOrderBook()"); }

}  // namespace backtesting