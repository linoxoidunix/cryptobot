#include "aot/strategy/market_order.h"

#include "aot/common/types.h"

namespace Trading {
auto MarketOrder::toString() const -> std::string {
    std::stringstream ss;
    ss << "MarketOrder" << "[" << "oid:" << Common::orderIdToString(order_id_)
       << " " << "side:" << Common::sideToString(side_) << " "
       << "price:" << Common::priceToString(price_) << " "
       << "qty:" << Common::qtyToString(qty_)
       <<  "]";

    return ss.str();
};

// namespace Trading
BBODouble::BBODouble(const BBO* bbo, uint precission_price,
                     uint precission_qty)
    : bid_price((bbo->bid_price * std::pow(10, -precission_price))),
      ask_price((bbo->ask_price * std::pow(10, -precission_price))),
      bid_qty((bbo->bid_qty * std::pow(10, -precission_qty))),
      ask_qty((bbo->ask_qty * std::pow(10, -precission_qty))){};

BBO::BBO(const BBODouble* bbo_double, uint precission_price,
                  uint precission_qty)
    : bid_price((Common::Price)(bbo_double->bid_price *
                                std::pow(10, precission_price))),
      ask_price((Common::Price)(bbo_double->ask_price *
                                std::pow(10, precission_price))),
      bid_qty((Common::Qty)(bbo_double->bid_qty * std::pow(10, precission_qty))),
      ask_qty((Common::Qty)(bbo_double->ask_qty * std::pow(10, precission_qty))){};

}  // namespace Trading