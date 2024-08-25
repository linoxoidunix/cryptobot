#pragma once
#include <vector>

#include "aot/Exchange.h"
#include "aot/Logger.h"
#include "aot/common/mem_pool.h"
#include "aot/common/types.h"
#include "aot/market_data/market_update.h"
#include "aot/strategy/market_order.h"

namespace Trading {
class TradeEngine;

class MarketOrderBook final {
  public:
    explicit MarketOrderBook();

    ~MarketOrderBook();

    /// Process market data update and update the limit order book.
    auto onMarketUpdate(const Exchange::MEMarketUpdate *market_update) noexcept
        -> void;

    //  auto setTradeEngine(TradeEngine *trade_engine) {
    //    trade_engine_ = trade_engine;
    //  }

    auto getBBO() const noexcept -> const BBO * { return &bbo_; }

    auto toString(bool detailed, bool validity_check) const -> std::string;

    /// Deleted default, copy & move constructors and assignment-operators.
    // MarketOrderBook() = delete;

    /// Update the BBO abstraction, the two boolean parameters represent if the
    /// buy or the sekk (or both) sides or both need to be updated.
    auto updateBBO(bool update_bid, bool update_ask) noexcept {
        if (update_bid) {
            if (bids_at_price_map_.size()) {
                bbo_.bid_price =
                    bids_at_price_map_.begin()->first_mkt_order_.price_;
                bbo_.bid_qty =
                    bids_at_price_map_.begin()->first_mkt_order_.qty_;
            } else {
                bbo_.bid_price = Common::Price_INVALID;
                bbo_.bid_qty   = Common::Qty_INVALID;
            }
        }

        if (update_ask) {
            if (asks_at_price_map_.size()) {
                bbo_.ask_price =
                    asks_at_price_map_.begin()->first_mkt_order_.price_;
                bbo_.ask_qty =
                    asks_at_price_map_.begin()->first_mkt_order_.qty_;
            } else {
                bbo_.ask_price = Common::Price_INVALID;
                bbo_.ask_qty   = Common::Qty_INVALID;
            }
        }
    }

    void ClearOrderBook() {
        bids_at_price_map_.clear();
        asks_at_price_map_.clear();
        for (const auto &order_at_price : price_orders_at_price_)
            orders_at_price_pool_.deallocate(order_at_price.second);
        price_orders_at_price_.clear();
    }

    MarketOrderBook(const MarketOrderBook &)             = delete;

    MarketOrderBook(const MarketOrderBook &&)            = delete;

    MarketOrderBook &operator=(const MarketOrderBook &)  = delete;

    MarketOrderBook &operator=(const MarketOrderBook &&) = delete;

  private:
    // const Common::TickerId ticker_id_;

    /// Parent trade engine that owns this limit order book, used to send
    /// notifications when book changes or trades occur.

    /// Hash map from OrderId -> MarketOrder.
    // OrderHashMap oid_to_order_;

    /// Memory pool to manage MarketOrdersAtPrice objects.
    common::MemPool<Trading::MarketOrdersAtPrice> orders_at_price_pool_;

    /// Pointers to beginning / best prices / top of book of buy and sell price
    /// levels.
    /// Hash map from Price -> MarketOrdersAtPrice.
    OrdersAtPriceHashMap price_orders_at_price_;
    BidsatPriceMap bids_at_price_map_;
    AsksatPriceMap asks_at_price_map_;
    /// Memory pool to manage MarketOrder objects.
    BBO bbo_;

  private:
    /// Fetch and return the MarketOrdersAtPrice corresponding to the provided
    /// price.
    auto getOrdersAtPrice(Common::Price price) noexcept
        -> Trading::MarketOrdersAtPrice * {
        return price_orders_at_price_.at(price);
    }

    /// Add a new MarketOrdersAtPrice at the correct price into the containers -
    /// the hash map and the doubly linked list of price levels.
    auto addOrdersAtPrice(MarketOrdersAtPrice *new_orders_at_price) noexcept {
        price_orders_at_price_.emplace_unique(new_orders_at_price->price_,
                                              new_orders_at_price);

        if (new_orders_at_price->side_ == Common::Side::BUY)
            asks_at_price_map_.insert_equal(*new_orders_at_price);
        if (new_orders_at_price->side_ == Common::Side::SELL)
            bids_at_price_map_.insert_equal(*new_orders_at_price);
    }

    /// Remove the MarketOrdersAtPrice from the containers - the hash map and
    /// the doubly linked list of price levels.
    auto removeOrdersAtPrice(Common::Side side, Common::Price price) noexcept {
        auto order_at_price = price_orders_at_price_.at(price);
        if (!order_at_price) {
            // https://github.com/binance/binance-spot-api-docs/blob/20f752900a3a7a63c72f5a1b18d762a1d5b001bd/web-socket-streams.md#how-to-manage-a-local-order-book-correctly
            // How to manage a local order book correctly
            // 9.Receiving an event that removes a price level that is not in
            // your local order book can happen and is normal.
            logw("order_book not contain such price");
            return;
        }
        if (side == Common::Side::BUY) {
            if (asks_at_price_map_.count(*order_at_price)) [[likely]]
                asks_at_price_map_.erase(*order_at_price);
            else
                loge("critical error asks_at_price_map_");
        }
        if (side == Common::Side::SELL) {
            if (bids_at_price_map_.count(*order_at_price)) [[likely]]
                bids_at_price_map_.erase(*order_at_price);
            else
                loge("critical error bids_at_price_map_");
        }
        fmtlog::poll();
        price_orders_at_price_.at(price) = nullptr;
        price_orders_at_price_.erase(price);

        orders_at_price_pool_.deallocate(order_at_price);
    }

    /// Add a single order at the end of the FIFO queue at the price level that
    /// this order belongs in.
    auto addOrder(MarketOrder *order) noexcept -> void {
        const auto orders_at_price = getOrdersAtPrice(order->price_);

        if (!orders_at_price) {
            auto new_orders_at_price = orders_at_price_pool_.allocate(
                order->side_, order->price_, *order, nullptr, nullptr);
            addOrdersAtPrice(new_orders_at_price);
        } else {
            orders_at_price->first_mkt_order_.order_id_ = order->order_id_;
            if (orders_at_price->first_mkt_order_.side_ != order->side_)
                [[unlikely]]
                ASSERT(true,
                       "try change asks_at_price_map_ or bids_at_price_map_");
            orders_at_price->first_mkt_order_.price_ = order->price_;
            orders_at_price->first_mkt_order_.qty_   = order->qty_;
        }
    }
};

class MarketOrderBookDouble {
  public:
    explicit MarketOrderBookDouble(Common::TradingPair trading_pair,
                                   Common::TradingPairHashMap &pairs)
        : trading_pair_(trading_pair),
          pairs_(pairs),
          precission_price_(pairs[trading_pair].price_precission),
          precission_qty_(pairs[trading_pair].qty_precission) {};

    ~MarketOrderBookDouble() = default;

    /// Process market data update and update the limit order book.
    auto OnMarketUpdate(
        const Exchange::MEMarketUpdateDouble *market_update) noexcept -> void;

    auto SetTradeEngine(TradeEngine *trade_engine) {
        trade_engine_ = trade_engine;
    }

    /// Update the BBO abstraction, the two boolean parameters represent if the
    /// buy or the sekk (or both) sides or both need to be updated.
    auto updateBBO(bool update_bid, bool update_ask) noexcept {
        book_.updateBBO(update_bid, update_ask);
    }

    auto getBBO() noexcept -> const BBODouble * {
        bbo_double_ =
            BBODouble(book_.getBBO(), precission_price_, precission_qty_);
        return &bbo_double_;
    }

    // auto toString(bool detailed, bool validity_check) const -> std::string;

    /// Deleted default, copy & move constructors and assignment-operators.
    // MarketOrderBook() = delete;

    MarketOrderBookDouble(const MarketOrderBook &)             = delete;

    MarketOrderBookDouble(const MarketOrderBook &&)            = delete;

    MarketOrderBookDouble &operator=(const MarketOrderBook &)  = delete;

    MarketOrderBookDouble &operator=(const MarketOrderBook &&) = delete;

  private:
    Common::TradingPair trading_pair_;
    Common::TradingPairHashMap &pairs_;
    uint precission_price_;
    uint precission_qty_;
    BBODouble bbo_double_;
    MarketOrderBook book_;
    TradeEngine *trade_engine_ = nullptr;
};

/// Hash map from TickerId -> MarketOrderBook.
using MarketOrderBookHashMap =
    std::array<MarketOrderBook *, Common::ME_MAX_TICKERS>;
}  // namespace Trading