#pragma once

#include "aot/Exchange.h"
#include "aot/Logger.h"
#include "aot/common/macros.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/order_manager.h"
#include "aot/wallet_asset.h"

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
                          const Common::TradingPair trading_pairs,
                          Common::TradingPairHashMap& pairs);

    /**
     * Launch OnOrderBookUpdate callback when there are changes in
     * marketorderbookdouble for BaseStrategy is None
     */
    auto OnOrderBookUpdate(const Common::TradingPair &trading_pair, PriceD price, Side side,
                           Trading::MarketOrderBookDouble *order_book) noexcept
        -> void {
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

    /// Deleted default, copy & move constructors and assignment-operators.
    BaseStrategy()                                 = delete;

    BaseStrategy(const BaseStrategy &)             = delete;

    BaseStrategy(const BaseStrategy &&)            = delete;

    BaseStrategy &operator=(const BaseStrategy &)  = delete;

    BaseStrategy &operator=(const BaseStrategy &&) = delete;

  private:
    base_strategy::Strategy *strategy_;
    TradeEngine *trade_engine_;
    Trading::OrderManager *order_manager_ = nullptr;
    const TradeEngineCfgHashMap &ticker_cfg_;
    TradingPair trading_pairs_;
    TradingPairHashMap& pairs_;
    Wallet wallet_;
    Trading::MarketOrderBookDouble *order_book_ = nullptr;
    std::vector<std::function<void(const Common::TradingPair& trading_pair)>> actions_;
    /**
     * @brief if strategy want buy qty asset with price_asset=price it calls
     * this BuySomeAsset. Used for long operation
     *
     * @param pair
     * @param price
     * @param qty
     */
    auto BuySomeAsset(const Common::TradingPair& pair) noexcept -> void {
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (!ticker_cfg_.count(pair)) [[unlikely]] {
            logw("fail buy because tickercfg not contain {}", pair.ToString());
            return;
        }
        auto buy_price = order_book_->getBBO()->ask_price;
        if(buy_price == Common::kPRICE_DOUBLE_INVALID)[[unlikely]]
        {
            logw("skip BuySomeAsset. BBO ask_price=INVALID. please wait more time for update BBO");
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
    auto SellAllAsset(const Common::TradingPair& trading_pair) noexcept -> void {
        logi("launch long sell action for {}", trading_pair.ToString());
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (auto number_asset = wallet_.SafetyGetNumberAsset(trading_pair);
            number_asset > 0) {
            auto price = order_book_->getBBO()->bid_price;
            order_manager_->NewOrder(trading_pair, price, Side::SELL, std::abs(number_asset));
        } else
            logw("fail because number_asset={} <= 0", number_asset);
    };
    /**
     * @brief if strategy want sell qty asset with price_asset=price it calls.
     * Used for short operation this SellSomeAsset
     *
     * @param trading_pair
     */
    auto SellSomeAsset(const Common::TradingPair& trading_pair) noexcept -> void {
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (!ticker_cfg_.count(trading_pair)) [[unlikely]] {
            logw("fail sell because tickercfg not contain {}", trading_pair.ToString());
            return;
        }
        auto sell_price = order_book_->getBBO()->bid_price;
        if(sell_price == Common::kPRICE_DOUBLE_INVALID)
        {
            logw("skip SellSomeAsset. BBO bid_price=INVALID. please wait more time for update BBO");
            return;
        }
        auto qty = ticker_cfg_.at(trading_pair).clip;
        logi("launch short sell action for {} price:{} qty:{}",
             trading_pair.ToString(), sell_price, qty);
        order_manager_->NewOrder(trading_pair, sell_price, Side::SELL, std::abs(qty));
    };
    /**
     * @brief if strategy want buy all asset that early sold than it calls
     * BuyAllAsset. Used for short operation
     *
     * @param trading_pair 
     */
    auto BuyAllAsset(const Common::TradingPair& trading_pair) noexcept -> void {
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
        actions_[(int)TradeAction::kEnterLong] = [this](const Common::TradingPair& trading_pair) {
            BuySomeAsset(trading_pair);
        };
        actions_[(int)TradeAction::kEnterShort] = [this](const Common::TradingPair& trading_pair) {
            SellSomeAsset(trading_pair);
        };
        actions_[(int)TradeAction::kExitLong] = [this](const Common::TradingPair& trading_pair) {
            SellAllAsset(trading_pair);
        };
        actions_[(int)TradeAction::kExitShort] = [this](const Common::TradingPair& trading_pair) {
            BuyAllAsset(trading_pair);
        };
        actions_[(int)TradeAction::kNope] = [this](const Common::TradingPair& trading_pair) {
        };
    };
};
}  // namespace Trading