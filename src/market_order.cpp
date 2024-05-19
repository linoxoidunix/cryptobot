#include "aot/strategy/market_order.h"
#include "aot/common/types.h"

namespace Trading {
  auto MarketOrder::toString() const -> std::string {
    std::stringstream ss;
    ss << "MarketOrder" << "["
       << "oid:" << Common::orderIdToString(order_id_) << " "
       << "side:" << Common::sideToString(side_) << " "
       << "price:" << Common::priceToString(price_) << " "
       << "qty:" << Common::qtyToString(qty_) << " "
       //<< "prio:" << priorityToString(priority_) << " "
       << "prev:" << Common::orderIdToString(prev_order_ ? prev_order_->order_id_ : Common::OrderId_INVALID) << " "
       << "next:" << Common::orderIdToString(next_order_ ? next_order_->order_id_ : Common::OrderId_INVALID) << "]";

    return ss.str();
  }
}