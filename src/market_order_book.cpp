#include "aot/strategy/market_order_book.h"

#include <algorithm>

#include "aot/Logger.h"
#include "aot/strategy/trade_engine.h"

// #include "trade_engine.h"

namespace Trading {
MarketOrderBook::MarketOrderBook()
    : orders_at_price_pool_(common::ME_MAX_ORDERS_AT_PRICE) {}

MarketOrderBook::~MarketOrderBook() {
    logi("call ~MarketOrderBook()");
    ClearOrderBook();
}

/// Process market data update and update the limit order book.
auto MarketOrderBook::onMarketUpdate(
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
        MarketOrder order(common::OrderId_INVALID, market_update->side,
                          market_update->price, market_update->qty);
        addOrder(&order);
    } else {
        removeOrdersAtPrice(market_update->side, market_update->price);
    }

    updateBBO(bid_will_be_updated, ask_will_be_updated);
    if (bid_will_be_updated || ask_will_be_updated) logi("{}", bbo_.toString());
    logd("{}", market_update->ToString());
}

auto MarketOrderBookDouble::OnMarketUpdate(
    const Exchange::MEMarketUpdateDouble *market_update) noexcept -> void {
    const Exchange::MEMarketUpdate buf(market_update, precission_price_,
                                       precission_qty_);
    book_.onMarketUpdate(&buf);
    if(trade_engine_)
        trade_engine_->OnOrderBookUpdate(trading_pair_, market_update->price,
                                     market_update->side, this);
};

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
    Exchange::MEMarketUpdateDouble results[50];

    while (run_) {
        size_t count = queue_->try_dequeue_bulk(results, 50);
        for (uint i = 0; i < count; i++) [[likely]] {
            ob_->OnMarketUpdate(&results[i]);
        }
    }
}
}  // namespace Trading

namespace backtesting {

MarketOrderBook::~MarketOrderBook() { logi("call ~MarketOrderBook()"); }

/// Process market data update and update the limit order book.
auto MarketOrderBook::onMarketUpdate(
    const Exchange::MEMarketUpdate *market_update) noexcept -> void {
    bbo_.price = market_update->price;
    bbo_.qty   = market_update->qty;
}

auto MarketOrderBookDouble::OnNewKLine(const OHLCVExt *new_kline) noexcept
    -> void {
    bbo_double_.price = new_kline->ohlcv.open;
    bbo_double_.qty   = new_kline->ohlcv.volume / bbo_double_.price;
}
}  // namespace backtesting