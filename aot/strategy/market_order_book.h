#pragma once
#include <vector>

#include "aot/Exchange.h"
#include "aot/Logger.h"
#include "aot/common/mem_pool.h"
#include "aot/common/thread_utils.h"
#include "aot/common/types.h"
#include "aot/market_data/market_update.h"
#include "aot/strategy/cross_arbitrage/signals.h"
#include "aot/strategy/market_order.h"

namespace Trading {
class TradeEngine;

class MarketOrderBook {
    /// Memory pool to manage MarketOrdersAtPrice objects.
    common::MemPool<Trading::MarketOrdersAtPrice> orders_at_price_pool_;

    /// Pointers to beginning / best prices / top of book of buy and sell price
    /// levels.
    /// Hash map from Price -> MarketOrdersAtPrice.
    OrdersAtPriceHashMap price_orders_at_price_;
    BidsatPriceMap bids_at_price_map_;
    AsksatPriceMap asks_at_price_map_;
    BBO bbo_;
    common::TradingPair trading_pair_;
    common::TradingPairHashMap &pairs_;

  public:
    explicit MarketOrderBook(common::TradingPair trading_pair,
                             common::TradingPairHashMap &pairs);

    virtual ~MarketOrderBook();

    /// Process market data update and update the limit order book.
    virtual void OnMarketUpdate(const Exchange::MEMarketUpdate *market_update) noexcept;

    auto getBBO() const noexcept -> const BBO * { return &bbo_; }

    /// Update the BBO abstraction, the two boolean parameters represent if the
    /// buy or the sekk (or both) sides or both need to be updated.
    virtual void updateBBO(bool update_bid, bool update_ask) noexcept {
        if (update_bid) {
            if (bids_at_price_map_.size()) {
                bbo_.bid_price =
                    bids_at_price_map_.begin()->first_mkt_order_.price_;
                bbo_.bid_qty =
                    bids_at_price_map_.begin()->first_mkt_order_.qty_;
            } else {
                bbo_.bid_price = common::kPriceInvalid;
                bbo_.bid_qty   = common::kQtyInvalid;
            }
        }

        if (update_ask) {
            if (asks_at_price_map_.size()) {
                bbo_.ask_price =
                    asks_at_price_map_.begin()->first_mkt_order_.price_;
                bbo_.ask_qty =
                    asks_at_price_map_.begin()->first_mkt_order_.qty_;
            } else {
                bbo_.ask_price = common::kPriceInvalid;
                bbo_.ask_qty   = common::kQtyInvalid;
            }
        }
    }

    void ClearOrderBook() {
        logi("found {} bids in order book", bids_at_price_map_.size());
        logi("found {} asks in order book", asks_at_price_map_.size());
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

/// Hash map from TickerId -> MarketOrderBook.
using MarketOrderBookHashMap =
    std::array<MarketOrderBook *, common::ME_MAX_TICKERS>;

class OrderBookService : public common::ServiceI {
  public:
    explicit OrderBookService(MarketOrderBook *ob,
                              Exchange::EventLFQueue *book_update)
        : ob_(ob), queue_(book_update) {};
    ~OrderBookService() override{
        run_ = false;
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    };
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
    void StopImmediately() override { run_ = false; };

    void Run();

  private:
    volatile bool run_ = false;
    std::unique_ptr<std::jthread> thread_;

    MarketOrderBook *ob_           = nullptr;
    Exchange::EventLFQueue *queue_ = nullptr;
};
}  // namespace Trading

namespace strategy {
namespace cross_arbitrage {
/**
 * @brief publish driven event for startegy in lfqueu
 *
 */
class OrderBook : public Trading::MarketOrderBook {
    common::ExchangeId exchange_;
    strategy::cross_arbitrage::LFQueue *queue_ = nullptr;
    BBUPool bbu_pool_;
    BAUPool bau_pool_;

  public:
    explicit OrderBook(common::ExchangeId exchange,
                        common::TradingPair trading_pair,
                       common::TradingPairHashMap &pairs,
                       strategy::cross_arbitrage::LFQueue *queue,
                       uint bbu_mempool_size, uint bau_mempool_size)
        : MarketOrderBook(trading_pair, pairs),
          exchange_(exchange),
          queue_(queue),
          bbu_pool_(bbu_mempool_size),
          bau_pool_(bau_mempool_size) {};
    void updateBBO(bool update_bid, bool update_ask) noexcept override {
        Trading::MarketOrderBook::updateBBO(update_bid, update_ask);
        if (!queue_) return;
        auto bbo = getBBO();
        if (update_bid) {
            logi("push BBidUpdated event");
            auto ptr = bbu_pool_.allocate(BBidUpdated(exchange_, common::TradingPair{2, 1}, bbo->bid_price, bbo->bid_qty, &bbu_pool_));
            auto status = queue_->try_enqueue(ptr);
            if(!status)
                loge("can't push new event to queue");
            return;
        }
        if (update_ask) {
            logi("push BAskUpdated event");
            auto ptr = bau_pool_.allocate(BAskUpdated(exchange_,common::TradingPair{2, 1}, bbo->ask_price, bbo->ask_qty, &bau_pool_));
            auto status = queue_->try_enqueue(ptr);
            if(!status)
                loge("can't push new event to queue");
            return;
        }
    };
    ~OrderBook() override{
        logi("call Order Book d'tor");
    };
};
};  // namespace cross_arbitrage
};  // namespace strategy

namespace backtesting {
class TradeEngine;

class MarketOrderBook : public Trading::MarketOrderBook {
  public:
    explicit MarketOrderBook(common::TradingPair trading_pair,
                             common::TradingPairHashMap &pairs)
        : Trading::MarketOrderBook(trading_pair, pairs) {};

    ~MarketOrderBook() override;

    /// Process market data update and update the limit order book.
    auto OnNewKLine(const OHLCVExt *new_kline) noexcept -> void {
        bbo_.price = new_kline->ohlcv.open;
        bbo_.qty   = new_kline->ohlcv.volume / bbo_.price;
    }

    void OnMarketUpdate(const Exchange::MEMarketUpdate *market_update) noexcept override{
        bbo_.price = market_update->price;
        bbo_.qty   = market_update->qty;
    }
        

    auto GetBBO() const noexcept -> const backtesting::BBO * { return &bbo_; }

    MarketOrderBook(const MarketOrderBook &)             = delete;

    MarketOrderBook(const MarketOrderBook &&)            = delete;

    MarketOrderBook &operator=(const MarketOrderBook &)  = delete;

    MarketOrderBook &operator=(const MarketOrderBook &&) = delete;

  private:
    backtesting::BBO bbo_;
};

/// Hash map from TickerId -> MarketOrderBook.
using MarketOrderBookHashMap =
    std::array<MarketOrderBook *, common::ME_MAX_TICKERS>;
}  // namespace backtesting
