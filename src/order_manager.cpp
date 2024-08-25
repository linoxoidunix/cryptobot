#include "aot/strategy/order_manager.h"
#include "aot/strategy/trade_engine.h"

auto Trading::OrderManager::NewOrder(TickerS ticker_id, PriceD price, Side side,
                                     QtyD qty,  uint8_t price_prec, uint8_t qty_prec) noexcept -> void {
    assert(price > 0);
    assert(qty > 0);
    auto order = GetOrder(ticker_id, side);
    auto OrderIsLive = [order](){
        return !(order->state == OMOrderState::DEAD || order->state == OMOrderState::INVALID); 
    };
    if(OrderIsLive())
    {
        logi("there is live order for ticker:{}. can't create new order", order->ticker);
        fmtlog::poll();
        return;
    }
    const Exchange::RequestNewOrder new_request{
        Exchange::ClientRequestType::NEW,
        ticker_id,
        next_order_id_,
        side,
        price,
        qty,
        price_prec,
        qty_prec};
    trade_engine_->SendRequestNewOrder(&new_request);
    *order = {ticker_id, next_order_id_,           side, price,
              qty,       OMOrderState::PENDING_NEW};
    ++next_order_id_;
}

auto Trading::OrderManager::CancelOrder(TickerS ticker_id,
                                        Side side) noexcept -> void {
    auto order = GetOrder(ticker_id, side);
    const Exchange::RequestCancelOrder cancel_request{
        Exchange::ClientRequestType::CANCEL,
        order->ticker,
        order->order_id};
    trade_engine_->SendRequestCancelOrder(&cancel_request);
    order->state = OMOrderState::PENDING_CANCEL;
}
