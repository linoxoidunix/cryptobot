#pragma once

#include "aot/common/macros.h"
#include "aot/Logger.h"

#include "aot/client_response.h"

#include "aot/strategy/om_order.h"
//#include "risk_manager.h"

using namespace Common;

namespace Trading {
  class TradeEngine;

  /// Manages orders for a trading algorithm, hides the complexity of order management to simplify trading strategies.
  class OrderManager {
  public:
    explicit OrderManager(TradeEngine *trade_engine/*, RiskManager& risk_manager*/)
        : trade_engine_(trade_engine)/*, risk_manager_(risk_manager),*/{
    }

    /// Process an order update from a client response and update the state of the orders being managed.
    auto OnOrderResponse(const Exchange::MEClientResponse *client_response) noexcept -> void {
      logd("{}", client_response->ToString());
      auto order = &(ticker_side_order_.at(client_response->ticker).at(sideToIndex(client_response->side)));
      logd("{}", order->ToString());
    //   logger_->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
    //                order->toString().c_str());

      switch (client_response->type) {
        case Exchange::ClientResponseType::ACCEPTED: {
          order->order_state = OMOrderState::LIVE;
        }
          break;
        case Exchange::ClientResponseType::CANCELED: {
          order->order_state = OMOrderState::DEAD;
        }
          break;
        case Exchange::ClientResponseType::FILLED: {
          order->qty = client_response->leaves_qty;
          if(!order->qty)
            order->order_state = OMOrderState::DEAD;
        }
          break;
        case Exchange::ClientResponseType::CANCEL_REJECTED:
        case Exchange::ClientResponseType::INVALID: {
        }
          break;
      }
    }

    /// Send a new order with specified attribute, and update the OMOrder object passed here.
    auto NewOrder(OMOrder *order, TickerS ticker_id, PriceD price, Side side, QtyD qty) noexcept -> void;

    /// Send a cancel for the specified order, and update the OMOrder object passed here.
    auto CancelOrder(OMOrder *order) noexcept -> void;

    /// Move a single order on the specified side so that it has the specified price and quantity.
    /// This will perform risk checks prior to sending the order, and update the OMOrder object passed here.
    auto moveOrder(OMOrder *order, Common::TickerS ticker, Common::PriceD price, Common::Side side, Common::QtyD qty) noexcept {
      switch (order->order_state) {
        case OMOrderState::LIVE: {
          if(order->price != price) {
            //START_MEASURE(Trading_OrderManager_cancelOrder);
            CancelOrder(order);
            //END_MEASURE(Trading_OrderManager_cancelOrder, (*logger_));
          }
        }
          break;
        case OMOrderState::INVALID:
        case OMOrderState::DEAD: {
          if(price != Price_INVALID)[[likely]] {
            // START_MEASURE(Trading_RiskManager_checkPreTradeRisk);
            // const auto risk_result = risk_manager_.checkPreTradeRisk(ticker_id, side, qty);
            // END_MEASURE(Trading_RiskManager_checkPreTradeRisk, (*logger_));
            // if(LIKELY(risk_result == RiskCheckResult::ALLOWED)) {
            //   START_MEASURE(Trading_OrderManager_newOrder);
             NewOrder(order, ticker, price, side, qty);
            //   END_MEASURE(Trading_OrderManager_newOrder, (*logger_));
            // } else
            //   logger_->log("%:% %() % Ticker:% Side:% Qty:% RiskCheckResult:%\n", __FILE__, __LINE__, __FUNCTION__,
            //                Common::getCurrentTimeStr(&time_str_),
            //                tickerIdToString(ticker_id), sideToString(side), qtyToString(qty),
            //                riskCheckResultToString(risk_result));
          }
        }
          break;
        case OMOrderState::PENDING_NEW:
        case OMOrderState::PENDING_CANCEL:
          break;
      }
    }

    /// Have orders of quantity clip at the specified buy and sell prices.
    /// This can result in new orders being sent if there are none.
    /// This can result in existing orders being cancelled if they are not at the specified price or of the specified quantity.
    /// Specifying Price_INVALID for the buy or sell prices indicates that we do not want an order there.
    auto moveOrders(Common::TickerS ticker, Price bid_price, Price ask_price, Qty clip) noexcept {
      {
        auto bid_order = &(ticker_side_order_.at(ticker).at(Common::sideToIndex(Common::Side::BUY)));
        //START_MEASURE(Trading_OrderManager_moveOrder);
        moveOrder(bid_order, ticker, bid_price, Side::BUY, clip);
        //END_MEASURE(Trading_OrderManager_moveOrder, (*logger_));
      }

      {
        auto ask_order = &(ticker_side_order_.at(ticker).at(Common::sideToIndex(Common::Side::SELL)));
        //START_MEASURE(Trading_OrderManager_moveOrder);
        moveOrder(ask_order, ticker, ask_price, Side::SELL, clip);
        //END_MEASURE(Trading_OrderManager_moveOrder, (*logger_));
      }
    }

    /// Helper method to fetch the buy and sell OMOrders for the specified TickerId.
    auto getOMOrderSideHashMap(const Common::TickerS& ticker) const {
      return &(ticker_side_order_.at(ticker));
    }

    /// Deleted default, copy & move constructors and assignment-operators.
    OrderManager() = delete;

    OrderManager(const OrderManager &) = delete;

    OrderManager(const OrderManager &&) = delete;

    OrderManager &operator=(const OrderManager &) = delete;

    OrderManager &operator=(const OrderManager &&) = delete;

  private:
    /// The parent trade engine object, used to send out client requests.
    TradeEngine *trade_engine_ = nullptr;

    /// Risk manager to perform pre-trade risk checks.
    //const RiskManager& risk_manager_;

    //std::string time_str_;
    //Common::Logger *logger_ = nullptr;

    /// Hash map container from TickerId -> Side -> OMOrder.
    OMOrderTickerSideHashMap ticker_side_order_;

    /// Used to set OrderIds on outgoing new order requests.
    Common::OrderId next_order_id_ = 1;
  };
}