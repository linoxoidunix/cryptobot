#include "aot/strategy/order_manager.h"
#include "aot/strategy/trade_engine.h"

auto Trading::OrderManager::NewOrder(const Ticker& ticker, PriceD price, Side side,
                                     QtyD qty) noexcept -> void {
    auto ticker_as_string = ticker.symbol->ToString();
    assert(price > 0);
    assert(qty > 0);
    auto order = GetOrder(std::string(ticker_as_string), side);
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
        std::string(ticker_as_string),
        next_order_id_,
        side,
        price,
        qty,
        ticker.info.price_precission,
        ticker.info.qty_precission};
    trade_engine_->SendRequestNewOrder(&new_request);
    *order = {std::string(ticker_as_string), next_order_id_,           side, price,
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
