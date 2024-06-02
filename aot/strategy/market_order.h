#pragma once

#include <array>
#include <sstream>
#include <vector>

#include "aot/common/types.h"
#include "aot/third_party/emhash/hash_table7.hpp"
#include "boost/intrusive/avltree.hpp"

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

    /// Only needed for use with MemPool.
    MarketOrder()             = default;

    MarketOrder(Common::OrderId order_id, Common::Side side,
                Common::Price price, Common::Qty qty) noexcept
        : order_id_(order_id), side_(side), price_(price), qty_(qty) {}

    auto toString() const -> std::string;
};

/// Hash map from OrderId -> MarketOrder.
// typedef std::vector<MarketOrder*> OrderHashMap;

// typedef std::array<MarketOrder *, Common::ME_MAX_ORDER_IDS> OrderHashMap;

/// Used by the trade engine to represent a price level in the limit order book.
/// Internally maintains a list of MarketOrder objects arranged in FIFO order.
struct MarketOrdersAtPrice {
    Common::Side side_   = Common::Side::INVALID;
    Common::Price price_ = Common::Price_INVALID;

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

    MarketOrdersAtPrice(Common::Side side, Common::Price price,
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
           << "price:" << Common::priceToString(price_) << " "
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
    emhash7::HashMap<Common::Price, MarketOrdersAtPrice *>;

/// Represents a Best Bid Offer (BBO) abstraction for components which only need
/// a small summary of top of book price and liquidity instead of the full order
/// book.

struct BBO;

struct BBODouble {

    double bid_price   = Common::kPRICE_DOUBLE_INVALID;
    double ask_price   = Common::kPRICE_DOUBLE_INVALID;
    double bid_qty     = Common::kQTY_DOUBLE_INVALID;
    double ask_qty     = Common::kQTY_DOUBLE_INVALID;
    uint8_t price_prec = 0;
    uint8_t qty_prec   = 0;

    auto ToString() const {
        auto bid_qty_string = (bid_qty != Common::kQTY_DOUBLE_INVALID)? fmt::format("{:.{}f}", bid_qty, qty_prec) : "INVALID";
        auto ask_qty_string = (ask_qty != Common::kQTY_DOUBLE_INVALID)? fmt::format("{:.{}f}", ask_qty, qty_prec) : "INVALID";
        auto bid_price_string = (bid_price != Common::kPRICE_DOUBLE_INVALID)? fmt::format("{:.{}f}", bid_price, price_prec) : "INVALID";
        auto ask_price_string = (ask_qty != Common::kPRICE_DOUBLE_INVALID)? fmt::format("{:.{}f}", ask_price, price_prec) : "INVALID";
        return fmt::format("BBODouble[{}@{}X{}@{}]", bid_qty_string, bid_price_string,
                           ask_price_string, ask_qty_string);
    };
    explicit BBODouble(const BBO *bbo, uint8_t precission_price,
                       uint8_t precission_qty);
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
    explicit BBO(const BBODouble *bbo_double, uint8_t precission_price,
                 uint8_t precission_qty);
    explicit BBO() = default;
};
}  // namespace Trading