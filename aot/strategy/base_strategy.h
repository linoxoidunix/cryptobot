#pragma once
#include <list>
#include <unordered_map>

#include "aot/Exchange.h"
#include "aot/Logger.h"
#include "aot/common/macros.h"
#include "aot/strategy/market_order.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/order_manager.h"
#include "aot/wallet_asset.h"

namespace startegy{
    namespace cross_arbitrage{
        class Event;
    }
}
namespace base_strategy {
class Strategy;
};

namespace Trading {
class TradeEngine;
class BaseStrategy {
  public:
    explicit BaseStrategy(base_strategy::Strategy *strategy,
                          TradeEngine *trade_engine,
                          OrderManager *order_manager,
                          const TradeEngineCfgHashMap &ticker_cfg,
                          const common::TradingPair trading_pairs,
                          common::TradingPairHashMap &pairs);
    virtual ~BaseStrategy() = default;
    /**
     * Launch OnOrderBookUpdate callback when there are changes in
     * marketorderbookdouble for BaseStrategy is None
     */
    auto OnOrderBookUpdate(
        const common::TradingPair &trading_pair, common::Price price, Side side,
        Trading::MarketOrderBook *order_book) noexcept -> void {
        if (!order_book_) [[unlikely]]
            order_book_ = order_book;
    }

    /**
     * Process client responses for the strategy's orders.
     */
    auto OnOrderResponse(
        const Exchange::MEClientResponse *client_response) noexcept -> void {
        order_manager_->OnOrderResponse(client_response);
        wallet_.Update(client_response);
    }

    /**
     * @brief Process new kline. Launch order manager
     *
     */
    auto OnNewKLine(const OHLCVExt *new_kline) noexcept -> void;

    virtual void OnNewSignal(strategy::cross_arbitrage::Event *signal) {
       logi("doing nothing");
    };

    /// Deleted default, copy & move constructors and assignment-operators.
    BaseStrategy()                                 = delete;

    BaseStrategy(const BaseStrategy &)             = delete;

    BaseStrategy(const BaseStrategy &&)            = delete;

    BaseStrategy &operator=(const BaseStrategy &)  = delete;

    BaseStrategy &operator=(const BaseStrategy &&) = delete;

  protected:
    base_strategy::Strategy *strategy_;
    TradeEngine *trade_engine_;
    Trading::OrderManager *order_manager_ = nullptr;
    const TradeEngineCfgHashMap &ticker_cfg_;
    TradingPair trading_pairs_;
    TradingPairHashMap &pairs_;
    Trading::MarketOrderBook *order_book_ = nullptr;
    std::vector<std::function<void(const common::TradingPair &trading_pair)>>
        actions_;
    Wallet wallet_;
    /**
     * @brief if strategy want buy qty asset with price_asset=price it calls
     * this BuySomeAsset. Used for long operation
     *
     * @param pair
     * @param price
     * @param qty
     */
    auto BuySomeAsset(const common::TradingPair &pair) noexcept -> void {
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (!ticker_cfg_.count(pair)) [[unlikely]] {
            logw("fail buy because tickercfg not contain {}", pair.ToString());
            return;
        }
        auto buy_price = order_book_->getBBO()->ask_price;
        if (buy_price == common::kPriceInvalid) [[unlikely]] {
            logw(
                "skip BuySomeAsset. BBO ask_price=INVALID. please wait more "
                "time for update BBO");
            return;
        }
        auto qty = ticker_cfg_.at(pair).clip;
        logi("launch long buy action for {} price:{} qty:{}", pair.ToString(),
             buy_price, qty);
        order_manager_->NewOrder(pair, buy_price, Side::BUY, std::abs(qty));
    }
    /**
     * @brief if strategy want sell all asset that early buyed than it calls
     * SellAllAsset. Used for long operation
     *
     * @param trading_pair
     * @param price
     */
    auto SellAllAsset(const common::TradingPair &trading_pair) noexcept
        -> void {
        logi("launch long sell action for {}", trading_pair.ToString());
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (auto number_asset = wallet_.SafetyGetNumberAsset(trading_pair);
            number_asset > 0) {
            auto price = order_book_->getBBO()->bid_price;
            order_manager_->NewOrder(trading_pair, price, Side::SELL,
                                     std::abs(number_asset));
        } else
            logw("fail because number_asset={} <= 0", number_asset);
    };
    /**
     * @brief if strategy want sell qty asset with price_asset=price it calls.
     * Used for short operation this SellSomeAsset
     *
     * @param trading_pair
     */
    auto SellSomeAsset(const common::TradingPair &trading_pair) noexcept
        -> void {
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (!ticker_cfg_.count(trading_pair)) [[unlikely]] {
            logw("fail sell because tickercfg not contain {}",
                 trading_pair.ToString());
            return;
        }
        auto sell_price = order_book_->getBBO()->bid_price;
        if (sell_price == common::kPriceInvalid) {
            logw(
                "skip SellSomeAsset. BBO bid_price=INVALID. please wait more "
                "time for update BBO");
            return;
        }
        auto qty = ticker_cfg_.at(trading_pair).clip;
        logi("launch short sell action for {} price:{} qty:{}",
             trading_pair.ToString(), sell_price, qty);
        order_manager_->NewOrder(trading_pair, sell_price, Side::SELL,
                                 std::abs(qty));
    };
    /**
     * @brief if strategy want buy all asset that early sold than it calls
     * BuyAllAsset. Used for short operation
     *
     * @param trading_pair
     */
    auto BuyAllAsset(const common::TradingPair &trading_pair) noexcept -> void {
        logi("launch short buy action for {}", trading_pair.ToString());
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (auto number_asset = wallet_.SafetyGetNumberAsset(trading_pair);
            number_asset < 0) {
            auto buy_price = order_book_->getBBO()->ask_price;
            order_manager_->NewOrder(trading_pair, buy_price, Side::BUY,
                                     std::abs(number_asset));
        } else
            logw("fail because number_asset={} >= 0", number_asset);
    };
    /**
     * @brief init inner variable actions_
     *
     */
    void InitActions() {
        actions_.resize((int)TradeAction::kNope + 1);
        actions_[(int)TradeAction::kEnterLong] =
            [this](const common::TradingPair &trading_pair) {
                BuySomeAsset(trading_pair);
            };
        actions_[(int)TradeAction::kEnterShort] =
            [this](const common::TradingPair &trading_pair) {
                SellSomeAsset(trading_pair);
            };
        actions_[(int)TradeAction::kExitLong] =
            [this](const common::TradingPair &trading_pair) {
                SellAllAsset(trading_pair);
            };
        actions_[(int)TradeAction::kExitShort] =
            [this](const common::TradingPair &trading_pair) {
                BuyAllAsset(trading_pair);
            };
        actions_[(int)TradeAction::kNope] =
            [this](const common::TradingPair &trading_pair) {};
    };
};
};  // namespace Trading

namespace strategy {
namespace cross_arbitrage {

class CrossArbitrage : public Trading::BaseStrategy {
    std::unordered_map<common::ExchangeId, Trading::BBO> prices_;
    std::unordered_map<common::ExchangeId, common::TradingPair> &working_pairs_;
    std::list<common::ExchangeId> &exchanges_;
    common::ExchangeId exch1;
    common::ExchangeId exch2;
  public:
    explicit CrossArbitrage(
        std::unordered_map<common::ExchangeId, common::TradingPair>
            &working_pairs,
        std::list<common::ExchangeId> &exchanges,
        Trading::TradeEngine *trade_engine,
        Trading::OrderManager *order_manager,
        const TradeEngineCfgHashMap &ticker_cfg,
        common::TradingPairHashMap &pairs);
    ~CrossArbitrage() override = default;
    void OnNewSignal (strategy::cross_arbitrage::Event *signal) override{
        auto type = signal->GetType();
        if (type == strategy::cross_arbitrage::EventType::kBidUpdate) {
            {
                auto event =
                    static_cast<strategy::cross_arbitrage::BBidUpdated *>(
                        signal);
                prices_[event->exchange].bid_price = event->price;
                prices_[event->exchange].bid_qty  = event->qty;
                event->Deallocate();
            }
        } else if (type == strategy::cross_arbitrage::EventType::kAskUpdate) {
            auto event =
                static_cast<strategy::cross_arbitrage::BAskUpdated *>(signal);
            prices_[event->exchange].ask_price = event->price;
            prices_[event->exchange].ask_qty  = event->qty;
            event->Deallocate();
        } else {
            loge("Unknown signal type");
            return;
        }
        bool condition1 = prices_[exch2].bid_price == common::kPriceInvalid ||
                          prices_[exch1].ask_price == common::kPriceInvalid;
        if (!condition1)
            if (prices_[exch2].bid_price - prices_[exch1].ask_price > 0) {
                logi("buy on exch1 and sell on exch2");
                return;
            }
        bool condition2 = prices_[exch1].bid_price == common::kPriceInvalid ||
                          prices_[exch2].ask_price == common::kPriceInvalid;

        if (!condition2)
            if (prices_[exch1].bid_price - prices_[exch2].ask_price > 0) {
                logi("buy on exch2 and sell on exch1");
                return;
            }
        if (!condition1 && !condition2) {
            logw("there aren't conditions for cross arbitrage");
        }
    };
    const Wallet* GetWallet() const{return &wallet_;};
};
};  // namespace cross_arbitrage
};  // namespace strategy