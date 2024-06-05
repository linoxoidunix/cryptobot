#pragma once

#include <array>
#include <sstream>
#include "ankerl/unordered_dense.h"
#include "aot/common/types.h"

using namespace Common;

namespace Trading {
  /// Represents the type / action in the order structure in the order manager.
  enum class OMOrderState : int8_t {
    INVALID = 0,
    PENDING_NEW = 1,
    LIVE = 2,
    PENDING_CANCEL = 3,
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
    OMOrderState order_state = OMOrderState::INVALID;

    auto toString() const {
      std::stringstream ss;
      ss << "OMOrder" << "["
         << "tid:" << ticker << " "
         << "oid:" << Common::orderIdToString(order_id) << " "
         << "side:" << Common::sideToString(side) << " "
         << "price:" << price << " "
         << "qty:" << qty << " "
         << "state:" << OMOrderStateToString(order_state) << "]";

      return ss.str();
    }
  };

  /// Hash map from Side -> OMOrder.
  using OMOrderSideHashMap = std::array<OMOrder, sideToIndex(Side::MAX) + 1>;

  /// Hash map from TickerId -> Side -> OMOrder.
  //typedef std::array<OMOrderSideHashMap, ME_MAX_TICKERS> OMOrderTickerSideHashMap;
  using OMOrderTickerSideHashMap = ankerl::unordered_dense::map<std::string, OMOrderSideHashMap>;
}