#pragma once

#include <array>
#include <vector>
#include <sstream>

#include "aot/common/types.h"

// using namespace Common;

namespace Trading {
/// Used by the trade engine to represent a single order in the limit order
/// book.
struct MarketOrder {
    Common::OrderId order_id_ = Common::OrderId_INVALID;
    Common::Side side_        = Common::Side::INVALID;
    Common::Price price_      = Common::Price_INVALID;
    Common::Qty qty_          = Common::Qty_INVALID;
    // Common::Priority priority_ = Common::Priority_INVALID;

    /// MarketOrder also serves as a node in a doubly linked list of all orders
    /// at price level arranged in FIFO order.
    MarketOrder *prev_order_  = nullptr;
    MarketOrder *next_order_  = nullptr;

    /// Only needed for use with MemPool.
    MarketOrder()             = default;

    MarketOrder(Common::OrderId order_id, Common::Side side,
                Common::Price price, Common::Qty qty, MarketOrder *prev_order,
                MarketOrder *next_order) noexcept
        : order_id_(order_id),
          side_(side),
          price_(price),
          qty_(qty),
          prev_order_(prev_order),
          next_order_(next_order) {}

    auto toString() const -> std::string;
};

/// Hash map from OrderId -> MarketOrder.
typedef std::vector<MarketOrder*> OrderHashMap;

//typedef std::array<MarketOrder *, Common::ME_MAX_ORDER_IDS> OrderHashMap;

/// Used by the trade engine to represent a price level in the limit order book.
/// Internally maintains a list of MarketOrder objects arranged in FIFO order.
struct MarketOrdersAtPrice {
    Common::Side side_               = Common::Side::INVALID;
    Common::Price price_             = Common::Price_INVALID;

    MarketOrder *first_mkt_order_    = nullptr;

    /// MarketOrdersAtPrice also serves as a node in a doubly linked list of
    /// price levels arranged in order from most aggressive to least aggressive
    /// price.
    MarketOrdersAtPrice *prev_entry_ = nullptr;
    MarketOrdersAtPrice *next_entry_ = nullptr;

    /// Only needed for use with MemPool.
    MarketOrdersAtPrice()            = default;

    MarketOrdersAtPrice(Common::Side side, Common::Price price,
                        MarketOrder *first_mkt_order,
                        MarketOrdersAtPrice *prev_entry,
                        MarketOrdersAtPrice *next_entry)
        : side_(side),
          price_(price),
          first_mkt_order_(first_mkt_order),
          prev_entry_(prev_entry),
          next_entry_(next_entry) {}

    auto toString() const {
        std::stringstream ss;
        ss << "MarketOrdersAtPrice[" << "side:" << sideToString(side_) << " "
           << "price:" << Common::priceToString(price_) << " "
           << "first_mkt_order:"
           << (first_mkt_order_ ? first_mkt_order_->toString() : "null") << " "
           << "prev:"
           << Common::priceToString(prev_entry_ ? prev_entry_->price_
                                                : Common::Price_INVALID)
           << " " << "next:"
           << Common::priceToString(next_entry_ ? next_entry_->price_
                                                : Common::Price_INVALID)
           << "]";

        return ss.str();
    }
};

/// Hash map from Price -> MarketOrdersAtPrice.
using OrdersAtPriceHashMap = std::vector<MarketOrdersAtPrice *>;
// typedef std::array<MarketOrdersAtPrice *, Common::ME_MAX_PRICE_LEVELS>
//     OrdersAtPriceHashMap;

/// Represents a Best Bid Offer (BBO) abstraction for components which only need
/// a small summary of top of book price and liquidity instead of the full order
/// book.

struct BBO;
struct BBODouble {
    double bid_price = std::numeric_limits<double>::max();
    double ask_price = std::numeric_limits<double>::max();
    double bid_qty   = std::numeric_limits<double>::max();
    double ask_qty   = std::numeric_limits<double>::max();
    auto ToString() const {
        return fmt::format(
            "BBODouble[{}@{}X{}@{}]", Common::qtyToString(bid_qty),
            Common::priceToString(bid_price), Common::priceToString(ask_price),
            Common::qtyToString(ask_qty));
    };
    explicit BBODouble(const BBO* bbo, uint precission_price, uint precission_qty);
    explicit BBODouble() = default;
};

struct BBO {
    Common::Price bid_price = Common::Price_INVALID;
    Common::Price ask_price = Common::Price_INVALID;
    Common::Qty bid_qty     = Common::Qty_INVALID;
    Common::Qty ask_qty     = Common::Qty_INVALID;

    auto toString() const {
        std::stringstream ss;
        ss << "BBO{" << Common::qtyToString(bid_qty) << "@"
           << Common::priceToString(bid_price) << "X"
           << Common::priceToString(ask_price) << "@"
           << Common::qtyToString(ask_qty) << "}";

        return ss.str();
    };
    explicit BBO(const BBODouble* bbo_double, uint precission_price, uint precission_qty);
    explicit BBO() = default;
};
}  // namespace Trading