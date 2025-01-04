#include "aot/strategy/market_order.h"

#include "aot/common/types.h"
#include "aot/bus/bus_component.h"

namespace Trading {
auto MarketOrder::toString() const -> std::string {
    std::stringstream ss;
    ss << "MarketOrder" << "[" << "oid:" << common::orderIdToString(order_id_)
       << " " << "side:" << common::sideToString(side_) << " "
       << "price:" << common::priceToString(price_) << " "
       << "qty:" << common::qtyToString(qty_) << "]";

    return ss.str();
};

void BusEventNewBBO::Accept(bus::Component* comp) {
     comp->AsyncHandleEvent(this); 
}

}  // namespace Trading
