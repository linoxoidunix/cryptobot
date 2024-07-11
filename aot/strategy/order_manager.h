#pragma once

#include "aot/Logger.h"
#include "aot/client_response.h"
#include "aot/common/macros.h"
#include "aot/strategy/om_order.h"
// #include "risk_manager.h"

using namespace Common;

namespace Trading {
class TradeEngine;

/**
 * Manages orders for a trading algorithm, hides the complexity of order
 * management to simplify trading strategies. This OrderManager impl supports
 * only 1 active(OMOrderState::LIVE) order for each TickerS !!!!
 */
class OrderManager {
  public:
    explicit OrderManager(
        TradeEngine *trade_engine /*, RiskManager& risk_manager*/)
        : trade_engine_(trade_engine) /*, risk_manager_(risk_manager),*/ {}

    /// Process an order update from a client response and update the state of
    /// the orders being managed.
    auto OnOrderResponse(
        const Exchange::MEClientResponse *client_response) noexcept -> void {
        logd("{}", client_response->ToString());
        auto order = &(ticker_side_order_.at(client_response->ticker)
                           .at(sideToIndex(client_response->side)));
        switch (client_response->type) {
            case Exchange::ClientResponseType::ACCEPTED: {
                order->state = OMOrderState::LIVE;
            } break;
            case Exchange::ClientResponseType::CANCELED: {
                order->state = OMOrderState::DEAD;
            } break;
            case Exchange::ClientResponseType::FILLED: {
                // if client_response->leaves_qty != 0 order still live
                order->qty = client_response->leaves_qty;
                if (order->qty == 0) order->state = OMOrderState::DEAD;
            } break;
            case Exchange::ClientResponseType::CANCEL_REJECTED:
            case Exchange::ClientResponseType::INVALID: {
            } break;
        }
    }

    /**
     * @brief
     * Send a new order with specified attribute, and update the OMOrder
     * object passed here.
     * @param ticker_id ticker as string. For example BTCUSDT
     * @param price
     * @param side
     * @param qty
     */
    auto NewOrder(TickerS ticker_id, PriceD price, Side side,
                  QtyD qty) noexcept -> void;

    /**
     * @brief
     *
     * @param ticker_id
     * @param side
     */
    auto CancelOrder(TickerS ticker_id, Side side) noexcept -> void;

    /// Deleted default, copy & move constructors and assignment-operators.
    OrderManager()                                 = delete;

    OrderManager(const OrderManager &)             = delete;

    OrderManager(const OrderManager &&)            = delete;

    OrderManager &operator=(const OrderManager &)  = delete;

    OrderManager &operator=(const OrderManager &&) = delete;

  private:
    /// The parent trade engine object, used to send out client requests.
    TradeEngine *trade_engine_ = nullptr;

    /// Risk manager to perform pre-trade risk checks.
    // const RiskManager& risk_manager_;

    /// Hash map container from TickerId -> Side -> OMOrder.
    OMOrderTickerSideHashMap ticker_side_order_;

    /// Used to set OrderIds on outgoing new order requests.
    Common::OrderId next_order_id_ = 1;

    Trading::OMOrder *GetOrder(TickerS ticker_id, Side side) {
        if (!ticker_side_order_.count(ticker_id)) [[unlikely]]
            ticker_side_order_.insert({ticker_id, {}});
        return &(ticker_side_order_.at(ticker_id).at(sideToIndex(side)));
    };
};
}  // namespace Trading