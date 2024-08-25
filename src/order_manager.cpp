#include "aot/strategy/order_manager.h"
#include "aot/strategy/trade_engine.h"

auto Trading::OrderManager::NewOrder(const Common::TradingPair trading_pair, PriceD price, Side side,
                                     QtyD qty) noexcept -> void {
    //auto ticker_as_string = trading_pair.ToString();
    assert(price > 0);
    assert(qty > 0);
    auto order = GetOrder(trading_pair, side);
    auto OrderIsLive = [order](){
        return !(order->state == OMOrderState::DEAD || order->state == OMOrderState::INVALID); 
    };
    if(OrderIsLive())
    {
        logi("there is live order for ticker:{}. can't create new order", trading_pair.ToString());
        fmtlog::poll();
        return;
    }
    const Exchange::RequestNewOrder new_request{
        Exchange::ClientRequestType::NEW,
        trading_pair,
        next_order_id_,
        side,
        price,
        qty};
    trade_engine_->SendRequestNewOrder(&new_request);
    *order = {trading_pair, next_order_id_,           side, price,
              qty,       OMOrderState::PENDING_NEW};
    ++next_order_id_;
}

auto Trading::OrderManager::CancelOrder(Common::TradingPair trading_pair,
                                        Side side) noexcept -> void {
    auto order = GetOrder(trading_pair, side);
    const Exchange::RequestCancelOrder cancel_request{
        Exchange::ClientRequestType::CANCEL,
        order->trading_pair,
        order->order_id};
    trade_engine_->SendRequestCancelOrder(&cancel_request);
    order->state = OMOrderState::PENDING_CANCEL;
}
