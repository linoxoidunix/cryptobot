#include "aot/strategy/order_manager.h"
#include "aot/strategy/trade_engine.h"

auto Trading::OrderManager::NewOrder(const common::TradingPair trading_pair, common::Price price, Side side,
                                     common::Qty qty) noexcept -> void {
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

auto Trading::OrderManager::CancelOrder(common::TradingPair trading_pair,
                                        Side side) noexcept -> void {
    auto order = GetOrder(trading_pair, side);
    const Exchange::RequestCancelOrder cancel_request{
        Exchange::ClientRequestType::CANCEL,
        order->trading_pair,
        order->order_id};
    trade_engine_->SendRequestCancelOrder(&cancel_request);
    order->state = OMOrderState::PENDING_CANCEL;
}

auto backtesting::OrderManager::NewOrder(const common::TradingPair trading_pair, common::Price price, Side side,
                                     common::Qty qty) noexcept -> void {
    assert(price > 0);
    assert(qty > 0);
    auto order = GetOrder(trading_pair, side);
    auto OrderIsLive = [order](){
        return !(order->state == Trading::OMOrderState::DEAD || order->state == Trading::OMOrderState::INVALID); 
    };
    if(OrderIsLive())
    {
        logi("there is live order for ticker:{}. can't create new order", trading_pair.ToString());
        return;
    }
    *order = {trading_pair, next_order_id_,           side, price,
              qty,       Trading::OMOrderState::LIVE};
    ++next_order_id_;
}

auto backtesting::OrderManager::CancelOrder(common::TradingPair trading_pair,
                                        Side side) noexcept -> void {
    auto order = GetOrder(trading_pair, side);
    order->state = Trading::OMOrderState::DEAD;
}
