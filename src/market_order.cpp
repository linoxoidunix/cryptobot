#include "aot/strategy/market_order.h"

#include "aot/common/types.h"

namespace Trading {
auto MarketOrder::toString() const -> std::string {
    std::stringstream ss;
    ss << "MarketOrder" << "[" << "oid:" << Common::orderIdToString(order_id_)
       << " " << "side:" << Common::sideToString(side_) << " "
       << "price:" << Common::priceToString(price_) << " "
       << "qty:" << Common::qtyToString(qty_) << "]";

    return ss.str();
};

// namespace Trading
BBODouble::BBODouble(const BBO* bbo, uint8_t precission_price,
                     uint8_t precission_qty)
    : price_prec(precission_price), qty_prec(precission_qty) {
    if (bbo->bid_price != Common::Price_INVALID) [[likely]]
        bid_price =
            (double)bbo->bid_price * 1.0 * std::pow(10, -precission_price);
    else
        bid_price = Common::kPRICE_DOUBLE_INVALID;

    if (bbo->ask_price != Common::Price_INVALID) [[likely]]
        ask_price =
            (double)bbo->ask_price * 1.0 * std::pow(10, -precission_price);
    else
        ask_price = Common::kPRICE_DOUBLE_INVALID;
    if (bbo->bid_qty != Common::Qty_INVALID) [[likely]]
        bid_qty = (double)bbo->bid_qty * 1.0 * std::pow(10, -precission_qty);
    else
        bid_qty = Common::kQTY_DOUBLE_INVALID;
    if (bbo->ask_qty != Common::Qty_INVALID) [[likely]]
        ask_qty = (double)bbo->ask_qty * 1.0 * std::pow(10, -precission_qty);
    else
        ask_qty = Common::kQTY_DOUBLE_INVALID;
};

BBO::BBO(const BBODouble* bbo_double, uint8_t precission_price,
         uint8_t precission_qty)
    : bid_price((Common::Price)(bbo_double->bid_price *
                                std::pow(10, precission_price))),
      ask_price((Common::Price)(bbo_double->ask_price *
                                std::pow(10, precission_price))),
      bid_qty(
          (Common::Qty)(bbo_double->bid_qty * std::pow(10, precission_qty))),
      ask_qty(
          (Common::Qty)(bbo_double->ask_qty * std::pow(10, precission_qty))){};

}  // namespace Trading