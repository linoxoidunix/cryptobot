#pragma once

#include "aot/Exchange.h"
#include "aot/Logger.h"
#include "aot/client_response.h"
#include "aot/common/macros.h"
#include "aot/strategy/om_order.h"
// #include "risk_manager.h"

using namespace common;

namespace Trading {
class TradeEngine;
class PositionKeeper;
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
        if (!ticker_side_order_.count(client_response->trading_pair))
            [[unlikely]] {
            loge("critical error in OrderManager");
            return;
        }
        auto order = &(ticker_side_order_.at(client_response->trading_pair)
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
    virtual auto NewOrder(common::ExchangeId exchange_id, common::TradingPair trading_pair, common::Price price,
                          Side side, common::Qty qty) noexcept -> void;

    /**
     * @brief
     *
     * @param ticker_id
     * @param side
     */
    virtual auto CancelOrder(common::ExchangeId exchange_id, common::TradingPair trading_pair,
                             Side side) noexcept -> void;

    /// Deleted default, copy & move constructors and assignment-operators.
    OrderManager()                                 = delete;
    OrderManager(const OrderManager &)             = delete;
    OrderManager(const OrderManager &&)            = delete;
    OrderManager &operator=(const OrderManager &)  = delete;
    OrderManager &operator=(const OrderManager &&) = delete;
    virtual ~OrderManager()                        = default;

  private:
    /// The parent trade engine object, used to send out client requests.
    TradeEngine *trade_engine_ = nullptr;

    /// Risk manager to perform pre-trade risk checks.
    // const RiskManager& risk_manager_;

    /// Hash map container from TickerId -> Side -> OMOrder.
    OMOrderTickerSideHashMap ticker_side_order_;

    /// Used to set OrderIds on outgoing new order requests.
    common::OrderId next_order_id_ = 1;

  protected:
    Trading::OMOrder *GetOrder(common::TradingPair trading_pair, Side side) {
        if (!ticker_side_order_.count(trading_pair)) [[unlikely]]
            ticker_side_order_.insert({trading_pair, {}});
        return &(ticker_side_order_.at(trading_pair).at(sideToIndex(side)));
    };
};
}  // namespace Trading

namespace backtesting {
class OrderManager : public Trading::OrderManager {
    Trading::PositionKeeper* keeper_;
    using Trading::OrderManager::OrderManager;

    Trading::OMOrderTickerSideHashMap ticker_side_order_;

    common::OrderId next_order_id_ = 1;

  public:
    auto NewOrder(common::ExchangeId exchange_id,  common::TradingPair trading_pair, common::Price price, Side side,
                  common::Qty qty) noexcept -> void override;
    auto CancelOrder(common::ExchangeId exchange_id, common::TradingPair trading_pair,
                     Side side) noexcept -> void override;

    Trading::OMOrder *TestGetOrder(common::TradingPair trading_pair,
                                   Side side) {
        return GetOrder(trading_pair, side);
    };
    void SetCustomPositionKeeper(Trading::PositionKeeper* keeper){keeper_ = keeper;};
    OrderManager()                                 = delete;
    OrderManager(const OrderManager &)             = delete;
    OrderManager(const OrderManager &&)            = delete;
    OrderManager &operator=(const OrderManager &)  = delete;
    OrderManager &operator=(const OrderManager &&) = delete;
    ~OrderManager() override                       = default;

  private:
    auto OnOrderResponse(const Exchange::MEClientResponse *) const {};
};

}  // namespace backtesting