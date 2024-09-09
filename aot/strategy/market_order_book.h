#pragma once
#include <vector>

#include "aot/Exchange.h"
#include "aot/Logger.h"
#include "aot/common/mem_pool.h"
#include "aot/common/thread_utils.h"
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

    auto getBBO() const noexcept -> const BBO * { return &bbo_; }

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
                bbo_.bid_price = common::Price_INVALID;
                bbo_.bid_qty   = common::Qty_INVALID;
            }
        }

        if (update_ask) {
            if (asks_at_price_map_.size()) {
                bbo_.ask_price =
                    asks_at_price_map_.begin()->first_mkt_order_.price_;
                bbo_.ask_qty =
                    asks_at_price_map_.begin()->first_mkt_order_.qty_;
            } else {
                bbo_.ask_price = common::Price_INVALID;
                bbo_.ask_qty   = common::Qty_INVALID;
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
    auto getOrdersAtPrice(common::Price price) noexcept
        -> Trading::MarketOrdersAtPrice * {
        return price_orders_at_price_.at(price);
    }

    /// Add a new MarketOrdersAtPrice at the correct price into the containers -
    /// the hash map and the doubly linked list of price levels.
    auto addOrdersAtPrice(MarketOrdersAtPrice *new_orders_at_price) noexcept {
        price_orders_at_price_.emplace_unique(new_orders_at_price->price_,
                                              new_orders_at_price);

        if (new_orders_at_price->side_ == common::Side::BUY)
            asks_at_price_map_.insert_equal(*new_orders_at_price);
        if (new_orders_at_price->side_ == common::Side::SELL)
            bids_at_price_map_.insert_equal(*new_orders_at_price);
    }

    /// Remove the MarketOrdersAtPrice from the containers - the hash map and
    /// the doubly linked list of price levels.
    auto removeOrdersAtPrice(common::Side side, common::Price price) noexcept {
        auto order_at_price = price_orders_at_price_.at(price);
        if (!order_at_price) {
            // https://github.com/binance/binance-spot-api-docs/blob/20f752900a3a7a63c72f5a1b18d762a1d5b001bd/web-socket-streams.md#how-to-manage-a-local-order-book-correctly
            // How to manage a local order book correctly
            // 9.Receiving an event that removes a price level that is not in
            // your local order book can happen and is normal.
            logw("order_book not contain such price");
            return;
        }
        if (side == common::Side::BUY) {
            if (asks_at_price_map_.count(*order_at_price)) [[likely]]
                asks_at_price_map_.erase(*order_at_price);
            else
                loge("critical error asks_at_price_map_");
        }
        if (side == common::Side::SELL) {
            if (bids_at_price_map_.count(*order_at_price)) [[likely]]
                bids_at_price_map_.erase(*order_at_price);
            else
                loge("critical error bids_at_price_map_");
        }
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
    explicit MarketOrderBookDouble(common::TradingPair trading_pair,
                                   common::TradingPairHashMap &pairs)
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
        bbo_double_ = BBODouble(book_.getBBO(), precission_price_, precission_qty_);
        return &bbo_double_;
    }

    MarketOrderBookDouble(const MarketOrderBook &)             = delete;

    MarketOrderBookDouble(const MarketOrderBook &&)            = delete;

    MarketOrderBookDouble &operator=(const MarketOrderBook &)  = delete;

    MarketOrderBookDouble &operator=(const MarketOrderBook &&) = delete;

  private:
    common::TradingPair trading_pair_;
    common::TradingPairHashMap &pairs_;
    uint8_t precission_price_;
    uint8_t precission_qty_;
    BBODouble bbo_double_;
    MarketOrderBook book_;
    TradeEngine *trade_engine_ = nullptr;
};

/// Hash map from TickerId -> MarketOrderBook.
using MarketOrderBookHashMap =
    std::array<MarketOrderBook *, common::ME_MAX_TICKERS>;

class OrderBookService : public common::ServiceI {
  public:
    explicit OrderBookService(MarketOrderBookDouble *ob,
                              Exchange::EventLFQueue *book_update)
        : ob_(ob), queue_(book_update) {};
    ~OrderBookService() override = default;
    void Start() override {
        run_    = true;
        thread_ = std::unique_ptr<std::jthread>(common::createAndStartJThread(
            -1, "Trading/OrderBookService", [this] { Run(); }));
        ASSERT(thread_ != nullptr, "Failed to start OrderBookService thread.");
    }
    void StopWaitAllQueue() override {
        logi("stop Trading/OrderBookService");
        while (queue_->size_approx()) {
            logi("Sleeping till all updates are consumed md-size:{}",
                 queue_->size_approx());
            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(10ms);
        }
        run_ = false;
    };
    void StopImmediately() override {run_ = false;};

    void Run();

  private:
    volatile bool run_ = false;
    std::unique_ptr<std::jthread> thread_;

    MarketOrderBookDouble *ob_ = nullptr;
    Exchange::EventLFQueue *queue_ = nullptr;
};
}  // namespace Trading

namespace backtesting {
class TradeEngine;

class MarketOrderBook final {
  public:
    explicit MarketOrderBook() = default;

    ~MarketOrderBook();

    /// Process market data update and update the limit order book.
    auto onMarketUpdate(const Exchange::MEMarketUpdate *market_update) noexcept
        -> void;

    auto GetBBO() const noexcept -> const backtesting::BBO * { return &bbo_; }

    MarketOrderBook(const MarketOrderBook &)             = delete;

    MarketOrderBook(const MarketOrderBook &&)            = delete;

    MarketOrderBook &operator=(const MarketOrderBook &)  = delete;

    MarketOrderBook &operator=(const MarketOrderBook &&) = delete;

  private:
    backtesting::BBO bbo_;
};

class MarketOrderBookDouble {
  public:
    explicit MarketOrderBookDouble(common::TradingPair trading_pair,
                                   common::TradingPairHashMap &pairs)
        : trading_pair_(trading_pair),
          pairs_(pairs),
          precission_price_(pairs[trading_pair].price_precission),
          precission_qty_(pairs[trading_pair].qty_precission) {};

    ~MarketOrderBookDouble() = default;

    auto OnNewKLine(const OHLCVExt *market_update) noexcept -> void;

    auto SetTradeEngine(TradeEngine *trade_engine) {
        trade_engine_ = trade_engine;
    }

    auto getBBO() noexcept -> const BBODouble * { return &bbo_double_; }

    MarketOrderBookDouble(const MarketOrderBook &)             = delete;

    MarketOrderBookDouble(const MarketOrderBook &&)            = delete;

    MarketOrderBookDouble &operator=(const MarketOrderBook &)  = delete;

    MarketOrderBookDouble &operator=(const MarketOrderBook &&) = delete;

  private:
    common::TradingPair trading_pair_;
    common::TradingPairHashMap &pairs_;
    uint precission_price_;
    uint precission_qty_;
    BBODouble bbo_double_;
    TradeEngine *trade_engine_ = nullptr;
};

/// Hash map from TickerId -> MarketOrderBook.
using MarketOrderBookHashMap =
    std::array<MarketOrderBook *, common::ME_MAX_TICKERS>;
}  // namespace backtesting
