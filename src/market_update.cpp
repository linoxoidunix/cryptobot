#include "aot/market_data/market_update.h"
#include "aot/Logger.h"
#include <cmath>
#include <string>

Exchange::MEMarketUpdate::MEMarketUpdate(
    const MEMarketUpdateDouble *market_update_double, uint precission_price,
    uint precission_qty)
    : type(market_update_double->type),
      order_id(common::OrderId_INVALID),
      //ticker_id(market_update_double->ticker_id),
      //ticker(market_update_double->ticker),

      side(market_update_double->side),
      price(std::round(market_update_double->price * std::pow(10, precission_price))),
      qty(std::round(market_update_double->qty * std::pow(10, precission_qty))) {}

Exchange::MEMarketUpdateDouble::MEMarketUpdateDouble(
    const MEMarketUpdate *market_update, uint precission_price,
    uint precission_qty)
    : type(market_update->type),
      //ticker(market_update->ticker),
      side(market_update->side),
      price(market_update->price * std::pow(10, -precission_price)),
      qty(market_update->qty * std::pow(10, -precission_qty)) {}

