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
        logi("add order");
        MarketOrder order(common::kOrderIdInvalid, market_update->side,
                          market_update->price, market_update->qty);
        addOrder(&order);
    } else {
        logi("remove order");
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

/// Process market data update and update the limit order book.
void MarketOrderBook2::OnMarketUpdate(
    const Exchange::MEMarketUpdate2 *market_update) noexcept {
    if (market_update->type == Exchange::MarketUpdateType::CLEAR) [[unlikely]] {
        ClearOrderBook();
    }

    const auto bid_will_be_updated =
        (!bids_at_price_map_.size() &&
             market_update->side == common::Side::SELL ||
         bids_at_price_map_.size() &&
             market_update->side == common::Side::SELL &&
             market_update->price >= bids_at_price_map_.begin()->price_);
    if(bid_will_be_updated){
        logi("bids_at_price_map_size:{} {} price:{} cur_best_bid:{}", bids_at_price_map_.size(),
        market_update->side,
        market_update->price,
        bids_at_price_map_.begin()->price_);
    }
    const auto ask_will_be_updated =
        (!asks_at_price_map_.size() &&
             market_update->side == common::Side::BUY ||
         asks_at_price_map_.size() &&
             market_update->side == common::Side::BUY &&
             market_update->price <= asks_at_price_map_.begin()->price_);
    if(ask_will_be_updated){
        logi("bids_at_price_map_size:{} {} price:{} cur_best_ask:{}", bids_at_price_map_.size(),
        market_update->side,
        market_update->price,
        asks_at_price_map_.begin()->price_);
    }
    if (market_update->qty != 0) {
        MarketOrder order(common::kOrderIdInvalid, market_update->side,
                          market_update->price, market_update->qty);
        AddOrder(&order);
    } else {
        RemoveOrdersAtPrice(market_update->side, market_update->price);
    }

    UpdateBBO(bid_will_be_updated, ask_will_be_updated);
    if (bid_will_be_updated || ask_will_be_updated){
        logi("{}", bbo_.ToString());
        bbo_signal_emitter_.Emit(bbo_);
    }
    //logd("{}", market_update->ToString());
}

/// Process market data snapsot and update the limit order book.
void MarketOrderBook2::OnMarketUpdate(
    const Exchange::BookSnapshot2 *market_snapshot) noexcept {

   if (!market_snapshot) {
        loge("Received null market snapshot");
        return;
    }

    // Очистка текущей книги ордеров
    ClearOrderBook();
    
    bool bid_will_be_updated = false;
    bool ask_will_be_updated = false;
    // Обработка bid (покупок)
    // Обработка bid (покупок)
    for (const auto& bid : market_snapshot->bids) {
        auto qty = bid.qty;
        if (qty > 0) {
            // Если новый bid улучшает лучшую цену
            if (!bid_will_be_updated &&
                (!bids_at_price_map_.size() || bids_at_price_map_.size() && bid.price >= bids_at_price_map_.begin()->price_)) {
                bid_will_be_updated = true;
            }
            logi("add bid order price:{} qty:{}", bid.price, bid.qty);
            MarketOrder order(common::kOrderIdInvalid, common::Side::SELL, bid.price, bid.qty);
            AddOrder(&order);
        } else {
            logi("rm bid price:{}", bid.price);
            RemoveOrdersAtPrice(common::Side::SELL, bid.price);
        }
    }

    // Обработка ask (продаж)
    for (const auto& ask : market_snapshot->asks) {
        auto qty = ask.qty;
        if (qty > 0) {
            // Если новый ask улучшает лучшую цену
            if (!ask_will_be_updated &&
                (!asks_at_price_map_.size() || asks_at_price_map_.size() && ask.price <= asks_at_price_map_.begin()->price_)) {
                ask_will_be_updated = true;
            }
            logi("add ask order price:{} qty:{}", ask.price, ask.qty);
            MarketOrder order(common::kOrderIdInvalid, common::Side::BUY, ask.price, ask.qty);
            AddOrder(&order);
        } else {
            logi("rm ask price:{}", ask.price);
            RemoveOrdersAtPrice(common::Side::BUY, ask.price);
        }
    }

    // Обновление и эмиссия сигнала BBO
    UpdateBBO(bid_will_be_updated, ask_will_be_updated);
    if (bid_will_be_updated || ask_will_be_updated) {
        logi("BBO updated: {}", bbo_.ToString());
        bbo_signal_emitter_.Emit(bbo_);
    }
}

/// Process market data snapsot and update the limit order book.
void MarketOrderBook2::OnMarketUpdate(
    const Exchange::BookDiffSnapshot2 *market_diff) noexcept {

   if (!market_diff) {
        loge("Received null market snapshot");
        return;
    }

   
    bool bid_will_be_updated = false;
    bool ask_will_be_updated = false;
    // Обработка bid (покупок)
    // Обработка bid (покупок)
    for (const auto& bid : market_diff->bids) {
        auto qty = bid.qty;
        if (qty > 0) {
            // Если новый bid улучшает лучшую цену
            if (!bid_will_be_updated &&
                (bids_at_price_map_.empty() || bid.price >= bids_at_price_map_.begin()->price_)) {
                bid_will_be_updated = true;
            }
            logi("add bid order price:{} qty:{}", bid.price, bid.qty);
            MarketOrder order(common::kOrderIdInvalid, common::Side::SELL, bid.price, bid.qty);
            AddOrder(&order);
        } else {
            logi("rm bid price:{}", bid.price);
            RemoveOrdersAtPrice(common::Side::SELL, bid.price);
        }
    }

    // Обработка ask (продаж)
    for (const auto& ask : market_diff->asks) {
        auto qty = ask.qty;
        if (qty > 0) {
            // Если новый ask улучшает лучшую цену
            if (!ask_will_be_updated &&
                (asks_at_price_map_.empty() || ask.price <= asks_at_price_map_.begin()->price_)) {
                ask_will_be_updated = true;
            }
            logi("add ask order price:{} qty:{}", ask.price, ask.qty);
            MarketOrder order(common::kOrderIdInvalid, common::Side::BUY, ask.price, ask.qty);
            AddOrder(&order);
        } else {
            logi("rm ask price:{}", ask.price);
            RemoveOrdersAtPrice(common::Side::BUY, ask.price);
        }
    }

    // Обновление и эмиссия сигнала BBO
    UpdateBBO(bid_will_be_updated, ask_will_be_updated);
    if (bid_will_be_updated || ask_will_be_updated) {
        logi("BBO updated: {}", bbo_.ToString());
        bbo_signal_emitter_.Emit(bbo_);
    }
}
}  // namespace Trading

namespace backtesting {

MarketOrderBook::~MarketOrderBook() { logi("call ~backtestin::MarketOrderBook()"); }

}  // namespace backtesting