#include "aot/strategy/market_order_book.h"
#include "aot/strategy/trade_engine.h"
#include <algorithm>

#include "aot/Logger.h"

// #include "trade_engine.h"

namespace Trading {
MarketOrderBook::MarketOrderBook()
    : orders_at_price_pool_(Common::ME_MAX_ORDERS_AT_PRICE) {
    // oid_to_order_.resize(Common::ME_MAX_ORDER_IDS);
    // price_orders_at_price_.resize(Common::ME_MAX_PRICE_LEVELS);
}

MarketOrderBook::~MarketOrderBook() {
//    logi("call ~MarketOrderBook() OrderBook\n{}", toString(false, true));
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
         market_update->side == Common::Side::SELL ||
         bids_at_price_map_.size() &&
         market_update->side == Common::Side::SELL &&
         market_update->price >= bids_at_price_map_.begin()->price_);
    const auto ask_will_be_updated =
        (!asks_at_price_map_.size() &&
         market_update->side == Common::Side::BUY ||
         asks_at_price_map_.size() &&
         market_update->side == Common::Side::BUY &&
         market_update->price <= asks_at_price_map_.begin()->price_);

    if (market_update->qty != 0) {
        MarketOrder order(Common::OrderId_INVALID, market_update->side,
                          market_update->price, market_update->qty);
        addOrder(&order);
    } else {
        removeOrdersAtPrice(market_update->side, market_update->price);
    }

    updateBBO(bid_will_be_updated, ask_will_be_updated);
    if (bid_will_be_updated || ask_will_be_updated) logi("{}", bbo_.toString());
    logd("{}", market_update->ToString());
}

auto MarketOrderBook::toString(bool detailed,
                               bool validity_check) const -> std::string {
    std::stringstream ss;
    return ss.str();
};

auto Trading::MarketOrderBookDouble::OnMarketUpdate(
    const Exchange::MEMarketUpdateDouble *market_update) noexcept -> void {
    const Exchange::MEMarketUpdate buf(market_update, precission_price_,
                                       precission_qty_);
    book_.onMarketUpdate(&buf);
    trade_engine_->OnOrderBookUpdate(
        ticker_, market_update->price, market_update->side, this);
};

};  // namespace Trading