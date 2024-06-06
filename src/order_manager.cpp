#include "aot/strategy/order_manager.h"
#include "aot/strategy/trade_engine.h"
#include "order_manager.h"

namespace Trading {
  /// Send a new order with specified attribute, and update the OMOrder object passed here.
   }  // namespace Trading
  auto Trading::OrderManager::NewOrder(OMOrder *order, TickerS ticker_id,
                                       PriceD price, Side side,
                                       QtyD qty) noexcept -> void {

  const Exchange::MEClientRequest new_request{Exchange::ClientRequestType::NEW, trade_engine_->clientId(), ticker_id,
                                                next_order_id_, side, price, qty};
    trade_engine_->sendClientRequest(&new_request);

    *order = {ticker_id, next_order_id_, side, price, qty, OMOrderState::PENDING_NEW};
    ++next_order_id_;

    logger_->log("%:% %() % Sent new order % for %\n", __FILE__, __LINE__, __FUNCTION__,
                 Common::getCurrentTimeStr(&time_str_),
                 new_request.toString().c_str(), order->toString().c_str());
  }


                                       }

  auto Trading::OrderManager::CancelOrder(OMOrder *order) noexcept -> void {}
