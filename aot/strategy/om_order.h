#pragma once

#include <array>
#include <sstream>
#include "ankerl/unordered_dense.h"
#include "aot/common/types.h"

namespace Trading {
  /// Represents the type / action in the order structure in the order manager.
  enum class OMOrderState : int8_t {
    /**
     * @brief The INVALID state represents an invalid order state
     * 
     */
    INVALID = 0,
    /**
     * @brief The PENDING_NEW state signifies that a new order has been sent out by OrderManager but it has not been accepted by the electronic trading exchange yet
     * 
     */
    PENDING_NEW = 1,
    /**
     * @brief When we receive a response from the exchange to signify acceptance, the order goes from PENDING_NEW to LIVE
     * 
     */
    LIVE = 2,
    /**
     * @brief Like PENDING_NEW, the PENDING_CANCEL state represents the state of an order when a cancellation for an order has been sent to the exchange but has not been processed by the exchange or the response has not been received back
     * 
     */
    PENDING_CANCEL = 3,
    /**
     * @brief The DEAD state represents an order that does not exist – it has either not been sent yet or fully executed or successfully cancelled:
     * 
     */
    DEAD = 4
  };

  inline auto OMOrderStateToString(OMOrderState side) -> std::string {
    switch (side) {
      case OMOrderState::PENDING_NEW:
        return "PENDING_NEW";
      case OMOrderState::LIVE:
        return "LIVE";
      case OMOrderState::PENDING_CANCEL:
        return "PENDING_CANCEL";
      case OMOrderState::DEAD:
        return "DEAD";
      case OMOrderState::INVALID:
        return "INVALID";
    }
    return "UNKNOWN";
  }

  /// Internal structure used by the order manager to represent a single strategy order.
  struct OMOrder {
    std::string ticker;
    Common::OrderId order_id = Common::OrderId_INVALID;
    Common::Side side = Common::Side::INVALID;
    double price = Common::kPRICE_DOUBLE_INVALID;
    double qty = Common::kQTY_DOUBLE_INVALID;
    OMOrderState state = OMOrderState::INVALID;

    auto ToString() const {
      std::stringstream ss;
      ss << "OMOrder" << "["
         << "tid:" << ticker << " "
         << "oid:" << Common::orderIdToString(order_id) << " "
         << "side:" << Common::sideToString(side) << " "
         << "price:" << price << " "
         << "qty:" << qty << " "
         << "state:" << OMOrderStateToString(state) << "]";

      return ss.str();
    }
  };

  /// Hash map from Side -> OMOrder.
  using OMOrderSideHashMap = std::array<Trading::OMOrder, sideToIndex(Common::Side::MAX) + 1>;

  /// Hash map from TickerId -> Side -> OMOrder.
  using OMOrderTickerSideHashMap = ankerl::unordered_dense::map<std::string, OMOrderSideHashMap>;
  using OMOrders = ankerl::unordered_dense::map<Common::OrderId, Trading::OMOrder>;
}