#pragma once

#include "aot/common/types.h"
#include "aot/common/mem_pool.h"
#include "aot/Logger.h"

#include "aot/strategy/market_order.h"
#include "aot/market_data/market_update.h"
#include "aot/common/mem_pool.h"

namespace Trading {
  class TradeEngine;

  class MarketOrderBook final {
  public:
    explicit MarketOrderBook(Common::TickerId ticker_id);

    ~MarketOrderBook();

    /// Process market data update and update the limit order book.
    auto onMarketUpdate(const Exchange::MEMarketUpdate *market_update) noexcept -> void;

    // auto setTradeEngine(TradeEngine *trade_engine) {
    //   trade_engine_ = trade_engine;
    // }

    /// Update the BBO abstraction, the two boolean parameters represent if the buy or the sekk (or both) sides or both need to be updated.
    auto updateBBO(bool update_bid, bool update_ask) noexcept {
      if(update_bid) {
        if(bids_by_price_) {
          bbo_.bid_price_ = bids_by_price_->price_;
          bbo_.bid_qty_ = bids_by_price_->first_mkt_order_->qty_;
          for(auto order = bids_by_price_->first_mkt_order_->next_order_; order != bids_by_price_->first_mkt_order_; order = order->next_order_)
            bbo_.bid_qty_ += order->qty_;
        }
        else {
          bbo_.bid_price_ = Common::Price_INVALID;
          bbo_.bid_qty_ = Common::Qty_INVALID;
        }
      }

      if(update_ask) {
        if(asks_by_price_) {
          bbo_.ask_price_ = asks_by_price_->price_;
          bbo_.ask_qty_ = asks_by_price_->first_mkt_order_->qty_;
          for(auto order = asks_by_price_->first_mkt_order_->next_order_; order != asks_by_price_->first_mkt_order_; order = order->next_order_)
            bbo_.ask_qty_ += order->qty_;
        }
        else {
          bbo_.ask_price_ = Common::Price_INVALID;
          bbo_.ask_qty_ = Common::Qty_INVALID;
        }
      }
    }

    auto getBBO() const noexcept -> const BBO* {
      return &bbo_;
    }

    auto toString(bool detailed, bool validity_check) const -> std::string;

    /// Deleted default, copy & move constructors and assignment-operators.
    MarketOrderBook() = delete;

    MarketOrderBook(const MarketOrderBook &) = delete;

    MarketOrderBook(const MarketOrderBook &&) = delete;

    MarketOrderBook &operator=(const MarketOrderBook &) = delete;

    MarketOrderBook &operator=(const MarketOrderBook &&) = delete;

  private:
    const Common::TickerId ticker_id_;

    /// Parent trade engine that owns this limit order book, used to send notifications when book changes or trades occur.
    //TradeEngine *trade_engine_ = nullptr;

    /// Hash map from OrderId -> MarketOrder.
    OrderHashMap oid_to_order_;

    /// Memory pool to manage MarketOrdersAtPrice objects.
    Common::MemPool<Trading::MarketOrdersAtPrice> orders_at_price_pool_;

    /// Pointers to beginning / best prices / top of book of buy and sell price levels.
    MarketOrdersAtPrice *bids_by_price_ = nullptr;
    MarketOrdersAtPrice *asks_by_price_ = nullptr;

    /// Hash map from Price -> MarketOrdersAtPrice.
    OrdersAtPriceHashMap price_orders_at_price_;

    /// Memory pool to manage MarketOrder objects.
    Common::MemPool<Trading::MarketOrder> order_pool_;

    BBO bbo_;

    std::string time_str_;

  private:
    auto priceToIndex(Common::Price price) const noexcept {
      return (price % Common::ME_MAX_PRICE_LEVELS);
    }

    /// Fetch and return the MarketOrdersAtPrice corresponding to the provided price.
    auto getOrdersAtPrice(Common::Price price) const noexcept -> Trading::MarketOrdersAtPrice * {
      return price_orders_at_price_.at(priceToIndex(price));
    }

    /// Add a new MarketOrdersAtPrice at the correct price into the containers - the hash map and the doubly linked list of price levels.
    auto addOrdersAtPrice(MarketOrdersAtPrice *new_orders_at_price) noexcept {
      price_orders_at_price_.at(priceToIndex(new_orders_at_price->price_)) = new_orders_at_price;

      const auto best_orders_by_price = (new_orders_at_price->side_ == Common::Side::BUY ? bids_by_price_ : asks_by_price_);
      if (UNLIKELY(!best_orders_by_price)) {
        (new_orders_at_price->side_ == Common::Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
        new_orders_at_price->prev_entry_ = new_orders_at_price->next_entry_ = new_orders_at_price;
      } else {
        auto target = best_orders_by_price;
        bool add_after = ((new_orders_at_price->side_ == Common::Side::SELL && new_orders_at_price->price_ > target->price_) ||
                          (new_orders_at_price->side_ == Common::Side::BUY && new_orders_at_price->price_ < target->price_));
        if (add_after) {
          target = target->next_entry_;
          add_after = ((new_orders_at_price->side_ == Common::Side::SELL && new_orders_at_price->price_ > target->price_) ||
                       (new_orders_at_price->side_ == Common::Side::BUY && new_orders_at_price->price_ < target->price_));
        }
        while (add_after && target != best_orders_by_price) {
          add_after = ((new_orders_at_price->side_ == Common::Side::SELL && new_orders_at_price->price_ > target->price_) ||
                       (new_orders_at_price->side_ == Common::Side::BUY && new_orders_at_price->price_ < target->price_));
          if (add_after)
            target = target->next_entry_;
        }

        if (add_after) { // add new_orders_at_price after target.
          if (target == best_orders_by_price) {
            target = best_orders_by_price->prev_entry_;
          }
          new_orders_at_price->prev_entry_ = target;
          target->next_entry_->prev_entry_ = new_orders_at_price;
          new_orders_at_price->next_entry_ = target->next_entry_;
          target->next_entry_ = new_orders_at_price;
        } else { // add new_orders_at_price before target.
          new_orders_at_price->prev_entry_ = target->prev_entry_;
          new_orders_at_price->next_entry_ = target;
          target->prev_entry_->next_entry_ = new_orders_at_price;
          target->prev_entry_ = new_orders_at_price;

          if ((new_orders_at_price->side_ == Common::Side::BUY && new_orders_at_price->price_ > best_orders_by_price->price_) ||
              (new_orders_at_price->side_ == Common::Side::SELL &&
               new_orders_at_price->price_ < best_orders_by_price->price_)) {
            target->next_entry_ = (target->next_entry_ == best_orders_by_price ? new_orders_at_price
                                                                               : target->next_entry_);
            (new_orders_at_price->side_ == Common::Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
          }
        }
      }
    }

    /// Remove the MarketOrdersAtPrice from the containers - the hash map and the doubly linked list of price levels.
    auto removeOrdersAtPrice(Common::Side side, Common::Price price) noexcept {
      const auto best_orders_by_price = (side == Common::Side::BUY ? bids_by_price_ : asks_by_price_);
      auto orders_at_price = getOrdersAtPrice(price);

      if (UNLIKELY(orders_at_price->next_entry_ == orders_at_price)) { // empty side of book.
        (side == Common::Side::BUY ? bids_by_price_ : asks_by_price_) = nullptr;
      } else {
        orders_at_price->prev_entry_->next_entry_ = orders_at_price->next_entry_;
        orders_at_price->next_entry_->prev_entry_ = orders_at_price->prev_entry_;

        if (orders_at_price == best_orders_by_price) {
          (side == Common::Side::BUY ? bids_by_price_ : asks_by_price_) = orders_at_price->next_entry_;
        }

        orders_at_price->prev_entry_ = orders_at_price->next_entry_ = nullptr;
      }

      price_orders_at_price_.at(priceToIndex(price)) = nullptr;

      orders_at_price_pool_.deallocate(orders_at_price);
    }

    /// Remove and de-allocate provided order from the containers.
    auto removeOrder(MarketOrder *order) noexcept -> void {
      auto orders_at_price = getOrdersAtPrice(order->price_);

      if (order->prev_order_ == order) { // only one element.
        removeOrdersAtPrice(order->side_, order->price_);
      } else { // remove the link.
        const auto order_before = order->prev_order_;
        const auto order_after = order->next_order_;
        order_before->next_order_ = order_after;
        order_after->prev_order_ = order_before;

        if (orders_at_price->first_mkt_order_ == order) {
          orders_at_price->first_mkt_order_ = order_after;
        }

        order->prev_order_ = order->next_order_ = nullptr;
      }

      oid_to_order_.at(order->order_id_) = nullptr;
      order_pool_.deallocate(order);
    }

    /// Add a single order at the end of the FIFO queue at the price level that this order belongs in.
    auto addOrder(MarketOrder *order) noexcept -> void {
      const auto orders_at_price = getOrdersAtPrice(order->price_);

      if (!orders_at_price) {
        order->next_order_ = order->prev_order_ = order;

        auto new_orders_at_price = orders_at_price_pool_.allocate(order->side_, order->price_, order, nullptr, nullptr);
        addOrdersAtPrice(new_orders_at_price);
      } else {
        auto first_order = (orders_at_price ? orders_at_price->first_mkt_order_ : nullptr);

        first_order->prev_order_->next_order_ = order;
        order->prev_order_ = first_order->prev_order_;
        order->next_order_ = first_order;
        first_order->prev_order_ = order;
      }

      oid_to_order_.at(order->order_id_) = order;
    }

    void ClearOrderBook() {
       for (auto &order: oid_to_order_) {
          if (order)
            order_pool_.deallocate(order);
        }
        oid_to_order_.fill(nullptr);

        if(bids_by_price_) {
          for(auto bid = bids_by_price_->next_entry_; bid != bids_by_price_; bid = bid->next_entry_)
            orders_at_price_pool_.deallocate(bid);
          orders_at_price_pool_.deallocate(bids_by_price_);
        }

        if(asks_by_price_) {
          for(auto ask = asks_by_price_->next_entry_; ask != asks_by_price_; ask = ask->next_entry_)
            orders_at_price_pool_.deallocate(ask);
          orders_at_price_pool_.deallocate(asks_by_price_);
        }

        bids_by_price_ = asks_by_price_ = nullptr;
    }
  };

  /// Hash map from TickerId -> MarketOrderBook.
using MarketOrderBookHashMap = std::array<MarketOrderBook *, Common::ME_MAX_TICKERS>;
}