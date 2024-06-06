#include "aot/strategy/order_manager.h"

#include "aot/strategy/trade_engine.h"
#include "order_manager.h"

auto Trading::OrderManager::NewOrder(OMOrder *order, TickerS ticker,
                                     PriceD price, Side side,
                                     QtyD qty) noexcept -> void {
    const Exchange::RequestNewOrder new_request{
        Exchange::ClientRequestType::NEW,
        ticker,
        next_order_id_,
        side,
        price,
        qty,
        0,
        0};
    trade_engine_->sendClientRequest(&new_request);
    *order = {ticker, next_order_id_,           side, price,
              qty,    OMOrderState::PENDING_NEW};
    ++next_order_id_;
}

auto Trading::OrderManager::CancelOrder(OMOrder *order) noexcept -> void {}
