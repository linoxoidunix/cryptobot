#include "aot/strategy/market_order.h"

#include "aot/common/types.h"

namespace Trading {
auto MarketOrder::toString() const -> std::string {
    std::stringstream ss;
    ss << "MarketOrder" << "[" << "oid:" << common::orderIdToString(order_id_)
       << " " << "side:" << common::sideToString(side_) << " "
       << "price:" << common::priceToString(price_) << " "
       << "qty:" << common::qtyToString(qty_) << "]";

    return ss.str();
};

// namespace Trading
BBODouble::BBODouble(const BBO* bbo, uint8_t precission_price,
                     uint8_t precission_qty)
    : price_prec(precission_price), qty_prec(precission_qty) {
    if (bbo->bid_price != common::kPriceInvalid) [[likely]]
        bid_price =
            (double)bbo->bid_price * 1.0 * std::pow(10, -precission_price);
    else
        bid_price = common::kPRICE_DOUBLE_INVALID;

    if (bbo->ask_price != common::kPriceInvalid) [[likely]]
        ask_price =
            (double)bbo->ask_price * 1.0 * std::pow(10, -precission_price);
    else
        ask_price = common::kPRICE_DOUBLE_INVALID;
    if (bbo->bid_qty != common::Qty_INVALID) [[likely]]
        bid_qty = (double)bbo->bid_qty * 1.0 * std::pow(10, -precission_qty);
    else
        bid_qty = common::kQTY_DOUBLE_INVALID;
    if (bbo->ask_qty != common::Qty_INVALID) [[likely]]
        ask_qty = (double)bbo->ask_qty * 1.0 * std::pow(10, -precission_qty);
    else
        ask_qty = common::kQTY_DOUBLE_INVALID;
};

BBO::BBO(const BBODouble* bbo_double, uint8_t precission_price,
         uint8_t precission_qty)
    : bid_price((common::Price)(bbo_double->bid_price *
                                std::pow(10, precission_price))),
      ask_price((common::Price)(bbo_double->ask_price *
                                std::pow(10, precission_price))),
      bid_qty(
          (common::Qty)(bbo_double->bid_qty * std::pow(10, precission_qty))),
      ask_qty(
          (common::Qty)(bbo_double->ask_qty * std::pow(10, precission_qty))) {};
}  // namespace Trading

namespace backtesting {
// namespace Trading
BBODouble::BBODouble(const BBO* bbo, uint8_t precission_price,
                     uint8_t precission_qty)
    : price_prec(precission_price), qty_prec(precission_qty) {
    if (bbo->price != common::kPriceInvalid) [[likely]]
        price = (double)bbo->price * 1.0 * std::pow(10, -precission_price);
    else
        price = common::kPRICE_DOUBLE_INVALID;

    if (bbo->qty != common::Qty_INVALID) [[likely]]
        qty = (double)bbo->qty * 1.0 * std::pow(10, -precission_qty);
    else
        qty = common::kQTY_DOUBLE_INVALID;
};

BBO::BBO(const BBODouble* bbo_double, uint8_t precission_price,
         uint8_t precission_qty)
    : price(
          (common::Price)(bbo_double->price * std::pow(10, precission_price))),
      qty((common::Qty)(bbo_double->qty * std::pow(10, precission_qty))) {};
}  // namespace backtesting