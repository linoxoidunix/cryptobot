#pragma once

#include <array>
#include <sstream>
#include <vector>

#include "aot/common/types.h"
#include "aot/Logger.h"
#include "aot/third_party/emhash/hash_table7.hpp"
#include "boost/intrusive/avltree.hpp"

struct BBOI {
    virtual common::Price GetWeightedPrice() const = 0;
    virtual ~BBOI() = default;
};

namespace Trading {
/// Used by the trade engine to represent a single order in the limit order
/// book.
struct MarketOrder {
    common::OrderId order_id_ = common::kOrderIdInvalid;
    common::Side side_        = common::Side::INVALID;
    common::Price price_      = common::kPriceInvalid;
    common::Qty qty_          = common::kQtyInvalid;
    // common::Priority priority_ = common::Priority_INVALID;

    /// MarketOrder also serves as a node in a doubly linked list of all orders
    /// at price level arranged in FIFO order.

    /// Only needed for use with MemPool.
    MarketOrder()             = default;

    MarketOrder(common::OrderId order_id, common::Side side,
                common::Price price, common::Qty qty) noexcept
        : order_id_(order_id), side_(side), price_(price), qty_(qty) {}

    auto toString() const -> std::string;
};

/// Hash map from OrderId -> MarketOrder.
// typedef std::vector<MarketOrder*> OrderHashMap;

// typedef std::array<MarketOrder *, common::ME_MAX_ORDER_IDS> OrderHashMap;

/// Used by the trade engine to represent a price level in the limit order book.
/// Internally maintains a list of MarketOrder objects arranged in FIFO order.
struct MarketOrdersAtPrice {
    common::Side side_   = common::Side::INVALID;
    common::Price price_ = common::kPriceInvalid;

    MarketOrder first_mkt_order_;

    /// MarketOrdersAtPrice also serves as a node in a doubly linked list of
    /// price levels arranged in order from most aggressive to least aggressive
    /// price.
    boost::intrusive::avl_set_member_hook<> member_hook_;
    friend bool operator<(const MarketOrdersAtPrice &a,
                          const MarketOrdersAtPrice &b) {
        return a.price_ < b.price_;
    }
    friend bool operator>(const MarketOrdersAtPrice &a,
                          const MarketOrdersAtPrice &b) {
        return a.price_ > b.price_;
    }
    friend bool operator==(const MarketOrdersAtPrice &a,
                           const MarketOrdersAtPrice &b) {
        return a.price_ == b.price_;
    }
    /// Only needed for use with MemPool.
    MarketOrdersAtPrice() = default;

    MarketOrdersAtPrice(common::Side side, common::Price price,
                        const MarketOrder &first_mkt_order,
                        MarketOrdersAtPrice *prev_entry,
                        MarketOrdersAtPrice *next_entry)
        : side_(side),
          price_(price),
          first_mkt_order_(first_mkt_order.order_id_, first_mkt_order.side_,
                           first_mkt_order.price_, first_mkt_order.qty_) {}

    auto toString() const {
        std::stringstream ss;
        ss << "MarketOrdersAtPrice[" << "side:" << sideToString(side_) << " "
           << "price:" << common::priceToString(price_) << " "
           << "first_mkt_order:" << first_mkt_order_.toString() << "]";

        return ss.str();
    }
};
using MemberOption =
    boost::intrusive::member_hook<MarketOrdersAtPrice,
                                  boost::intrusive::avl_set_member_hook<>,
                                  &MarketOrdersAtPrice::member_hook_>;
using BidsatPriceMap = boost::intrusive::avltree<
    MarketOrdersAtPrice,
    boost::intrusive::compare<std::greater<MarketOrdersAtPrice>>, MemberOption>;
using AsksatPriceMap = boost::intrusive::avltree<
    MarketOrdersAtPrice,
    boost::intrusive::compare<std::less<MarketOrdersAtPrice>>, MemberOption>;

/// Hash map from Price -> MarketOrdersAtPrice.
using OrdersAtPriceHashMap =
    emhash7::HashMap<common::Price, MarketOrdersAtPrice *>;

/// Represents a Best Bid Offer (BBO) abstraction for components which only need
/// a small summary of top of book price and liquidity instead of the full order
/// book.

struct BBO;

struct BBO : BBOI {
    common::Price bid_price = common::kPriceInvalid;
    common::Price ask_price = common::kPriceInvalid;
    common::Qty bid_qty     = common::kQtyInvalid;
    common::Qty ask_qty     = common::kQtyInvalid;

    auto ToString() const {
        std::stringstream ss;
        ss << "BBO{" << common::qtyToString(bid_qty) << "@"
           << common::priceToString(bid_price) << "X"
           << common::priceToString(ask_price) << "@"
           << common::qtyToString(ask_qty) << "}";

        return ss.str();
    };

    common::Price GetWeightedPrice() const override {
        if (bid_qty + ask_qty == 0) [[unlikely]]
            return common::kPriceInvalid;
        return std::round(1.0*(bid_price * bid_qty + ask_price * ask_qty) / (bid_qty + ask_qty));
    };

    explicit BBO() = default;
};
}  // namespace Trading

namespace backtesting {

struct BBO : BBOI{
    common::Price price = common::kPriceInvalid;
    common::Qty qty     = common::kQtyInvalid;

    auto ToString() const {
        std::stringstream ss;
        ss << "BBO{" << common::qtyToString(qty) << "@"
           << common::priceToString(price) << "}";

        return ss.str();
    };
    
    common::Price GetWeightedPrice() const override {return price;};

    explicit BBO() = default;
};
}  // namespace backtesting