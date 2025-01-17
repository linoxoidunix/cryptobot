#pragma once
#include <typeinfo>
#include <vector>

#include "aot/Exchange.h"
#include "aot/Logger.h"
#include "aot/bus/bus.h"
#include "aot/common/mem_pool.h"
#include "aot/common/thread_utils.h"
#include "aot/common/types.h"
#include "aot/market_data/market_update.h"
#include "aot/strategy/cross_arbitrage/signals.h"
#include "aot/strategy/market_order.h"
#include "aot/strategy/position_keeper.h"

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
    virtual void OnMarketUpdate(
        const Exchange::MEMarketUpdate *market_update) noexcept;

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
    const common::TradingPair &GetTradingPair() const noexcept {
        return trading_pair_;
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

        if (new_orders_at_price->side_ == common::Side::kAsk) {
            logi("add new ask with p:{} q{}",
                 new_orders_at_price->first_mkt_order_.price_,
                 new_orders_at_price->first_mkt_order_.qty_);
            asks_at_price_map_.insert_equal(*new_orders_at_price);
        }
        if (new_orders_at_price->side_ == common::Side::kBid) {
            logi("add new bid with p:{} q:{}",
                 new_orders_at_price->first_mkt_order_.price_,
                 new_orders_at_price->first_mkt_order_.qty_);
            bids_at_price_map_.insert_equal(*new_orders_at_price);
        }
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
        if (side == common::Side::kAsk) {
            if (asks_at_price_map_.count(*order_at_price)) [[likely]]
                asks_at_price_map_.erase(*order_at_price);
            else
                loge("critical error asks_at_price_map_");
        }
        if (side == common::Side::kBid) {
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

class MarketOrderBook2 {
    // template class to store all subscribers on MarketOrderBook2 signals
    template <typename SignalType>
    class SignalEmitter {
        using SignalCallback = std::function<void(const SignalType &)>;
        std::vector<SignalCallback> subscribers_;

      public:
        void Subscribe(SignalCallback callback) {
            subscribers_.emplace_back(std::move(callback));
        }

        void Emit(const SignalType &signal) {
            for (const auto &subscriber : subscribers_) {
                if (subscriber) {
                    subscriber(signal);
                }
            }
        }
    };
    /// Pointers to beginning / best prices / top of book of buy and sell price
    /// levels.
    /// Hash map from Price -> MarketOrdersAtPrice.
    OrdersAtPriceHashMap price_orders_at_price_bids_;
    OrdersAtPriceHashMap price_orders_at_price_asks_;
    BidsatPriceMap bids_at_price_map_;
    AsksatPriceMap asks_at_price_map_;
    BBO bbo_;
    common::ExchangeId exchange_id_;
    common::TradingPair trading_pair_;
    /// Memory pool to manage MarketOrdersAtPrice objects.
    common::MemoryPool<Trading::MarketOrdersAtPrice> orders_at_price_pool_bids_{
        common::ME_MAX_ORDERS_AT_PRICE};
    common::MemoryPool<Trading::MarketOrdersAtPrice> orders_at_price_pool_asks_{
        common::ME_MAX_ORDERS_AT_PRICE};
    SignalEmitter<BBO> bbo_signal_emitter_;

  public:
    explicit MarketOrderBook2(common::ExchangeId exchange_id,
                              common::TradingPair trading_pair)
        : exchange_id_(exchange_id), trading_pair_(trading_pair) {};

    virtual ~MarketOrderBook2() {
        logi("call ~MarketOrderBook2()");
        ClearOrderBook();
    };

    /// Process market data update and update the limit order book.
    virtual void OnMarketUpdate(
        const Exchange::MEMarketUpdate2 *market_update) noexcept;

    /// Process market data snapshot and update the limit order book.
    virtual void OnMarketUpdate(
        const Exchange::BookSnapshot2 *market_snapshot) noexcept;

    /// Process market data diff and update the limit order book.
    virtual void OnMarketUpdate(
        const Exchange::BookDiffSnapshot2 *market_diff) noexcept;

    auto GetBBO() const noexcept -> const BBO * { return &bbo_; }

    /// Update the BBO abstraction, the two boolean parameters represent if the
    /// buy or the sekk (or both) sides or both need to be updated.
    virtual void UpdateBBO(bool update_bid, bool update_ask) noexcept {
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
        if (bbo_.bid_price > bbo_.ask_price)
            loge("bid:{} > ask:{}", bbo_.bid_price, bbo_.ask_price);
    }
    const common::TradingPair &GetTradingPair() const noexcept {
        return trading_pair_;
    }
    // Подписчики на BBO
    void SubscribeToBBO(std::function<void(const BBO &)> callback) {
        bbo_signal_emitter_.Subscribe(std::move(callback));
    }
    void ClearOrderBook() {
        logi("found {} bids in order book", bids_at_price_map_.size());
        logi("found {} asks in order book", asks_at_price_map_.size());
        bids_at_price_map_.clear();
        asks_at_price_map_.clear();
        for (const auto &order_at_price : price_orders_at_price_bids_)
            orders_at_price_pool_bids_.Deallocate(order_at_price.second);
        price_orders_at_price_bids_.clear();
        for (const auto &order_at_price : price_orders_at_price_asks_)
            orders_at_price_pool_asks_.Deallocate(order_at_price.second);
        price_orders_at_price_asks_.clear();
    }

    MarketOrderBook2(const MarketOrderBook2 &)             = delete;

    MarketOrderBook2(const MarketOrderBook2 &&)            = delete;

    MarketOrderBook2 &operator=(const MarketOrderBook2 &)  = delete;

    MarketOrderBook2 &operator=(const MarketOrderBook2 &&) = delete;

  private:
    /// Fetch and return the MarketOrdersAtPrice corresponding to the provided
    /// price.
    auto GetOrdersAtPrice(common::Price price, bool is_bids) noexcept
        -> Trading::MarketOrdersAtPrice * {
        if (is_bids) return price_orders_at_price_bids_.at(price);
        return price_orders_at_price_asks_.at(price);
    }

    /// Add a new MarketOrdersAtPrice at the correct price into the containers -
    /// the hash map and the doubly linked list of price levels.
    auto AddOrdersAtPrice(MarketOrdersAtPrice *new_orders_at_price) noexcept {
        if (new_orders_at_price->side_ == common::Side::kAsk)
            price_orders_at_price_asks_.emplace_unique(
                new_orders_at_price->price_, new_orders_at_price);
        if (new_orders_at_price->side_ == common::Side::kBid)
            price_orders_at_price_bids_.emplace_unique(
                new_orders_at_price->price_, new_orders_at_price);

        if (new_orders_at_price->side_ == common::Side::kAsk) {
            logi("add new ask with p:{}", new_orders_at_price->price_);
            asks_at_price_map_.insert_equal(*new_orders_at_price);
        }
        if (new_orders_at_price->side_ == common::Side::kBid) {
            logi("add new bid with p:{}", new_orders_at_price->price_);
            bids_at_price_map_.insert_equal(*new_orders_at_price);
        }
    }

    /// Remove the MarketOrdersAtPrice from the containers - the hash map and
    /// the doubly linked list of price levels.
    auto RemoveOrdersAtPrice(common::Side side, common::Price price) noexcept {
        auto order_at_price =
            (side == common::Side::kBid)
                ? price_orders_at_price_bids_.at(price)
                : price_orders_at_price_asks_.at(price);
        if (!order_at_price) {
            // https://github.com/binance/binance-spot-api-docs/blob/20f752900a3a7a63c72f5a1b18d762a1d5b001bd/web-socket-streams.md#how-to-manage-a-local-order-book-correctly
            // How to manage a local order book correctly
            // 9.Receiving an event that removes a price level that is not in
            // your local order book can happen and is normal.
            logw("order_book not contain such price");
            return;
        }
        if (side == common::Side::kAsk) {
            if (asks_at_price_map_.count(*order_at_price)) [[likely]]
                asks_at_price_map_.erase(*order_at_price);
            else
                loge("critical error asks_at_price_map_");
        }
        if (side == common::Side::kBid) {
            if (bids_at_price_map_.count(*order_at_price)) [[likely]]
                bids_at_price_map_.erase(*order_at_price);
            else
                loge("critical error bids_at_price_map_");
        }
        if (side == common::Side::kBid) {
            price_orders_at_price_bids_.at(price) = nullptr;
            price_orders_at_price_bids_.erase(price);

            orders_at_price_pool_bids_.Deallocate(order_at_price);
        } else {
            price_orders_at_price_asks_.at(price) = nullptr;
            price_orders_at_price_asks_.erase(price);

            orders_at_price_pool_asks_.Deallocate(order_at_price);
        }
    }

    /// Add a single order at the end of the FIFO queue at the price level that
    /// this order belongs in.
    auto AddOrder(MarketOrder *order) noexcept -> void {
        bool is_bids = (order->side_ == common::Side::kBid)? true : false;
        auto orders_at_price = GetOrdersAtPrice(order->price_, is_bids);

        if (!orders_at_price) {
            auto& orders_at_price_pool_ = (order->side_ == common::Side::kBid)
                                             ? orders_at_price_pool_bids_
                                             : orders_at_price_pool_asks_;
            auto new_orders_at_price   = orders_at_price_pool_.Allocate(
                order->side_, order->price_, *order, nullptr, nullptr);
            AddOrdersAtPrice(new_orders_at_price);
        } else {
            orders_at_price->first_mkt_order_.order_id_ = order->order_id_;
            if (orders_at_price->first_mkt_order_.side_ != order->side_)
                [[unlikely]]
                ASSERT(true,
                       "try change asks_at_price_map_ or bids_at_price_map_");
            logi(
                "update position old price:{} old qty:{} new price:{} new "
                "qty:{}",
                orders_at_price->first_mkt_order_.price_,
                orders_at_price->first_mkt_order_.qty_, order->price_,
                order->qty_);
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
    ~OrderBookService() override {
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

template <typename Executor>
class OrderBookComponent : public bus::Component {
    using OrderBookMap = std::unordered_map<
        common::ExchangeId,
        std::unordered_map<common::TradingPair, MarketOrderBook2,
                           common::TradingPairHash, common::TradingPairEqual>>;

    Executor executor_;
    aot::CoBus &bus_;
    OrderBookMap order_books_;
    Trading::NewBBOPool new_bbo_pool_;
    Trading::BusEventNewBBOPool bus_event_new_bbo_pool_;

  public:
    explicit OrderBookComponent(Executor &&executor, aot::CoBus &bus,
                                uint64_t max_new_bbo_)
        : executor_(std::move(executor)),
          bus_(bus),
          new_bbo_pool_{max_new_bbo_},
          bus_event_new_bbo_pool_{max_new_bbo_} {}
    ~OrderBookComponent() override = default;

    void AsyncHandleEvent(
        boost::intrusive_ptr<Exchange::BusEventMEMarketUpdate2> event)
        override {
        // need logic co choose right executor depends on trading pair
        // and exchange
        boost::asio::co_spawn(executor_, HandleNewMEMarketUpdate(event),
                              boost::asio::detached);
    };

    void AsyncHandleEvent(
        boost::intrusive_ptr<Exchange::BusEventResponseNewSnapshot> event)
        override {
        // Обработка событий типа ResponseNewSnapshot
        boost::asio::co_spawn(executor_, HandleResponseNewSnapshot(event),
                              boost::asio::detached);
    }

    void AsyncHandleEvent(
        boost::intrusive_ptr<Exchange::BusEventBookDiffSnapshot> event)
        override {
        // Обработка событий типа MEMarketUpdate
        boost::asio::co_spawn(executor_, HandleNewDiff(event),
                              boost::asio::detached);
    }

    // Method to add a new order book
    void AddOrderBook(common::ExchangeId exchange_id,
                      const common::TradingPair &trading_pair) {
        // Ensure the outer map contains the desired ExchangeId
        auto &inner_map =
            order_books_
                .try_emplace(
                    exchange_id,
                    std::unordered_map<common::TradingPair, MarketOrderBook2,
                                       common::TradingPairHash,
                                       common::TradingPairEqual>())
                .first->second;

        auto result = inner_map.try_emplace(
            trading_pair,  // Key (TradingPair)
            exchange_id,
            trading_pair  // Constructor arguments for MarketOrderBook2
        );

        if (result.second) {
            logi("[MarketOrderBook2 inserted successfully!] with {} {}",
                 exchange_id, trading_pair.ToString());
            MarketOrderBook2 &order_book = result.first->second;

            // Подписка на сигналы BBO
            order_book.SubscribeToBBO(
                [this, exchange_id, trading_pair](const BBO &bbo) {
                    SendBBOToBus(exchange_id, trading_pair, bbo);
                });
            logi("add subscription to BBO for ob with {} {}", exchange_id,
                 trading_pair.ToString());

        } else {
            logi(
                "[MarketOrderBook2 already exists in the inner map!] with {} "
                "{}",
                exchange_id, trading_pair.ToString());
        }
    }

  private:
    boost::asio::awaitable<void> HandleNewMEMarketUpdate(
        boost::intrusive_ptr<Exchange::BusEventMEMarketUpdate2> event) {
        // Extract the necessary information from the event
        const auto *wrapped_event = event->WrappedEvent();
        if (!wrapped_event) {
            co_return;  // Exit if the event is invalid
        }

        const auto exchange_id   = wrapped_event->exchange_id;
        const auto &trading_pair = wrapped_event->trading_pair;

        // Check if the exchange and trading pair exist in the order books
        if (!order_books_.contains(exchange_id)) {
            loge("[EXCHANGE NOT FOUND] {}, {}", exchange_id,
                 trading_pair.ToString());
            co_return;
        }
        if (!order_books_[exchange_id].contains(trading_pair)) {
            loge("[TRADING PAIR NOT FOUND] {}, {}", exchange_id,
                 trading_pair.ToString());
            co_return;
        }

        auto &inner_map =
            order_books_
                .try_emplace(
                    exchange_id,
                    std::unordered_map<common::TradingPair, MarketOrderBook2,
                                       common::TradingPairHash,
                                       common::TradingPairEqual>())
                .first->second;

        auto &order_book =
            inner_map
                .try_emplace(
                    trading_pair, exchange_id,
                    trading_pair  // Arguments for constructing MarketOrderBook2
                    )
                .first->second;

        logi("[PROCESSING MARKET UPDATE] {}, {}, Price: {}, Qty: {}",
             exchange_id, trading_pair.ToString(), wrapped_event->price,
             wrapped_event->qty);

        order_book.OnMarketUpdate(wrapped_event);

        co_return;
    }

    boost::asio::awaitable<void> HandleResponseNewSnapshot(
        boost::intrusive_ptr<Exchange::BusEventResponseNewSnapshot> event) {
        logi("[ORDERBOOK] processing new snapshot");
        // Extract the necessary information from the event
        const auto *wrapped_event = event->WrappedEvent();
        if (!wrapped_event) {
            co_return;  // Exit if the event is invalid
        }

        const auto exchange_id   = wrapped_event->exchange_id;
        const auto &trading_pair = wrapped_event->trading_pair;

        // Check if the exchange and trading pair exist in the order books
        if (!order_books_.contains(exchange_id)) {
            loge("[EXCHANGE NOT FOUND] {}, {}", exchange_id,
                 trading_pair.ToString());
            co_return;
        }
        if (!order_books_[exchange_id].contains(trading_pair)) {
            loge("[TRADING PAIR NOT FOUND] {}, {}", exchange_id,
                 trading_pair.ToString());
            co_return;
        }

        auto &inner_map =
            order_books_
                .try_emplace(
                    exchange_id,
                    std::unordered_map<common::TradingPair, MarketOrderBook2,
                                       common::TradingPairHash,
                                       common::TradingPairEqual>())
                .first->second;

        auto &order_book =
            inner_map
                .try_emplace(
                    trading_pair, exchange_id,
                    trading_pair  // Arguments for constructing MarketOrderBook2
                    )
                .first->second;

        logi("[PROCESSING MARKET UPDATE] {}, {}, b_size: {}, a_size: {}",
             exchange_id, trading_pair.ToString(), wrapped_event->bids.size(),
             wrapped_event->asks.size());

        order_book.OnMarketUpdate(wrapped_event);

        co_return;
    }

    boost::asio::awaitable<void> HandleNewDiff(
        boost::intrusive_ptr<Exchange::BusEventBookDiffSnapshot> event) {
        logi("[ORDERBOOK] processing new diff");
        // Extract the necessary information from the event
        const auto *wrapped_event = event->WrappedEvent();
        if (!wrapped_event) {
            co_return;  // Exit if the event is invalid
        }

        const auto exchange_id   = wrapped_event->exchange_id;
        const auto &trading_pair = wrapped_event->trading_pair;

        // Check if the exchange and trading pair exist in the order books
        if (!order_books_.contains(exchange_id)) {
            loge("[EXCHANGE NOT FOUND] {}, {}", exchange_id,
                 trading_pair.ToString());
            co_return;
        }
        if (!order_books_[exchange_id].contains(trading_pair)) {
            loge("[TRADING PAIR NOT FOUND] {}, {}", exchange_id,
                 trading_pair.ToString());
            co_return;
        }

        auto &inner_map =
            order_books_
                .try_emplace(
                    exchange_id,
                    std::unordered_map<common::TradingPair, MarketOrderBook2,
                                       common::TradingPairHash,
                                       common::TradingPairEqual>())
                .first->second;

        auto &order_book =
            inner_map
                .try_emplace(
                    trading_pair, exchange_id,
                    trading_pair  // Arguments for constructing MarketOrderBook2
                    )
                .first->second;

        logi("[PROCESSING MARKET UPDATE] {}, {}, b_size: {}, a_size: {}",
             exchange_id, trading_pair.ToString(), wrapped_event->bids.size(),
             wrapped_event->asks.size());

        order_book.OnMarketUpdate(wrapped_event);

        co_return;
    }

    void SendBBOToBus(common::ExchangeId exchange_id,
                      const common::TradingPair &trading_pair, const BBO &bbo) {
        auto request = new_bbo_pool_.Allocate(&new_bbo_pool_, exchange_id,
                                              trading_pair, bbo);
        auto intr_ptr_request = boost::intrusive_ptr<NewBBO>(request);

        auto bus_event        = bus_event_new_bbo_pool_.Allocate(
            &bus_event_new_bbo_pool_, intr_ptr_request);
        auto intr_ptr_bus_request =
            boost::intrusive_ptr<BusEventNewBBO>(bus_event);

        bus_.AsyncSend(this, intr_ptr_bus_request);
    }
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
    strategy::cross_arbitrage::LFQueue *orderbook_tradeengine_channel_ =
        nullptr;
    position_keeper::EventLFQueue *orderbook_positionkeeper_channel_ = nullptr;

    BBUPool bbu_pool_;
    BAUPool bau_pool_;
    position_keeper::UpdateBBOPool update_bbo_pool_;

  public:
    explicit OrderBook(
        common::ExchangeId exchange, common::TradingPair trading_pair,
        common::TradingPairHashMap &pairs,
        strategy::cross_arbitrage::LFQueue *orderbook_tradeengine_channel,
        position_keeper::EventLFQueue *orderbook_positionkeeper_channel,
        uint bbu_mempool_size, uint bau_mempool_size,
        uint updatebbo_mempool_size)
        : MarketOrderBook(trading_pair, pairs),
          exchange_(exchange),
          orderbook_tradeengine_channel_(orderbook_tradeengine_channel),
          orderbook_positionkeeper_channel_(orderbook_positionkeeper_channel),
          bbu_pool_(bbu_mempool_size),
          bau_pool_(bau_mempool_size),
          update_bbo_pool_(updatebbo_mempool_size) {};
    void updateBBO(bool update_bid, bool update_ask) noexcept override {
        Trading::MarketOrderBook::updateBBO(update_bid, update_ask);
        if (!orderbook_tradeengine_channel_) {
            loge("orderbook_tradeengine_channel_ = nullptr");
            return;
        }
        if (!orderbook_positionkeeper_channel_) {
            loge("orderbook_positionkeeper_channel_ = nullptr");
            return;
        }
        auto bbo = getBBO();
        if (update_bid) {
            SendMessageToChannel(bbu_pool_, orderbook_tradeengine_channel_,
                                 exchange_, GetTradingPair(), bbo->bid_price,
                                 bbo->bid_qty);
            SendMessageToChannel(update_bbo_pool_,
                                 orderbook_positionkeeper_channel_, exchange_,
                                 GetTradingPair(), bbo);
            return;
        }
        if (update_ask) {
            SendMessageToChannel(bau_pool_, orderbook_tradeengine_channel_,
                                 exchange_, GetTradingPair(), bbo->ask_price,
                                 bbo->ask_qty);
            SendMessageToChannel(update_bbo_pool_,
                                 orderbook_positionkeeper_channel_, exchange_,
                                 GetTradingPair(), bbo);
            return;
        }
    };
    ~OrderBook() override { logi("call Order Book d'tor"); };

  private:
    template <class Pool, class Channel, class... Args>
    void SendMessageToChannel(Pool &mem_pool, Channel *channel, Args... args) {
        auto tuple_args =
            std::forward<std::tuple<Args...>>(std::tuple<Args...>(args...));

        using ValueType = Pool::value_type;
        logi("push {} event to {} channel", typeid(ValueType).name(),
             typeid(Pool).name());
        auto ptr = mem_pool.allocate(ValueType(args..., &mem_pool));
        if (auto status = channel->try_enqueue(ptr); !status)
            loge("can't push new event to queue");
    }
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

    void OnMarketUpdate(
        const Exchange::MEMarketUpdate *market_update) noexcept override {
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
