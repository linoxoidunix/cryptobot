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
                          const TradeEngineCfgHashMap &ticker_cfg);

    /**
     * Launch OnOrderBookUpdate callback when there are changes in
     * marketorderbookdouble for BaseStrategy is None
     */
    auto OnOrderBookUpdate(const TickerS &ticker_id, PriceD price, Side side,
                           Trading::MarketOrderBookDouble *order_book) noexcept
        -> void {
        if (!order_book_) [[unlikely]]
            order_book_ = order_book;
    }

    /**
     * Process client responses for the strategy's orders.
     */
    auto OnOrderUpdate(
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
    Wallet wallet_;
    Trading::MarketOrderBookDouble *order_book_ = nullptr;
    std::vector<std::function<void(const TickerS &ticker)>> actions_;
    /**
     * @brief if strategy want buy qty asset with price_asset=price it calls
     * this BuySomeAsset. Used for long operation
     *
     * @param ticker_id
     * @param price
     * @param qty
     */
    auto BuySomeAsset(const TickerS &ticker_id) noexcept -> void {
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (!ticker_cfg_.count(ticker_id)) [[unlikely]] {
            logw("fail buy because tickercfg not contain {}", ticker_id);
            return;
        }
        auto buy_price = order_book_->getBBO()->ask_price;
        if(buy_price == Common::kPRICE_DOUBLE_INVALID)[[unlikely]]
        {
            logw("skip BuySomeAsset. BBO ask_price=INVALID. please wait more time for update BBO");
            return;
        }
        auto qty = ticker_cfg_.at(ticker_id).clip;
        logi("launch long buy action for ticker:{} price:{} qty:{}", ticker_id,
             buy_price, qty);
        order_manager_->NewOrder(ticker_id, buy_price, Side::BUY, qty);
    }
    /**
     * @brief if strategy want sell all asset that early buyed than it calls
     * SellAllAsset. Used for long operation
     *
     * @param ticker_id ticker asset
     * @param price
     */
    auto SellAllAsset(const TickerS &ticker_id) noexcept -> void {
        logi("launch long sell action for ticker:{}", ticker_id);
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (auto number_asset = wallet_.SafetyGetNumberAsset(ticker_id);
            number_asset > 0) {
            auto price = order_book_->getBBO()->bid_price;
            order_manager_->NewOrder(ticker_id, price, Side::SELL,
                                     number_asset);
        } else
            logw("fail because number_asset={} <= 0", number_asset);
    };
    /**
     * @brief if strategy want sell qty asset with price_asset=price it calls.
     * Used for short operation this SellSomeAsset
     *
     * @param ticker_id
     * @param price
     * @param qty
     */
    auto SellSomeAsset(const TickerS &ticker_id) noexcept -> void {
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (!ticker_cfg_.count(ticker_id)) [[unlikely]] {
            logw("fail sell because tickercfg not contain {}", ticker_id);
            return;
        }
        auto sell_price = order_book_->getBBO()->bid_price;
        if(sell_price == Common::kPRICE_DOUBLE_INVALID)
        {
            logw("skip SellSomeAsset. BBO bid_price=INVALID. please wait more time for update BBO");
            return;
        }
        auto qty = ticker_cfg_.at(ticker_id).clip;
        logi("launch short sell action for ticker:{} price:{} qty:{}",
             ticker_id, sell_price, qty);
        order_manager_->NewOrder(ticker_id, sell_price, Side::SELL, qty);
    };
    /**
     * @brief if strategy want buy all asset that early sold than it calls
     * BuyAllAsset. Used for short operation
     *
     * @param ticker_id ticker asset
     * @param price
     */
    auto BuyAllAsset(const TickerS &ticker_id) noexcept -> void {
        logi("launch short buy action for ticker:{}", ticker_id);
        if (!order_book_) [[unlikely]] {
            logi("order_book_ ptr not updated in strategy");
            return;
        }
        if (auto number_asset = wallet_.SafetyGetNumberAsset(ticker_id);
            number_asset < 0) {
            auto buy_price = order_book_->getBBO()->ask_price;
            order_manager_->NewOrder(ticker_id, buy_price, Side::BUY,
                                     number_asset);
        } else
            logw("fail because number_asset={} >= 0", number_asset);
    };
    /**
     * @brief init inner variable actions_
     *
     */
    void InitActions() {
        actions_.resize((int)TradeAction::kNope + 1);
        actions_[(int)TradeAction::kEnterLong] = [this](const TickerS &ticker) {
            BuySomeAsset(ticker);
        };
        actions_[(int)TradeAction::kEnterShort] = [this](const TickerS &ticker) {
            SellSomeAsset(ticker);
        };
        actions_[(int)TradeAction::kExitLong] = [this](const TickerS &ticker) {
            SellAllAsset(ticker);
        };
        actions_[(int)TradeAction::kExitShort] = [this](const TickerS &ticker) {
            BuyAllAsset(ticker);
        };
        actions_[(int)TradeAction::kNope] = [this](const TickerS &ticker) {
        };
    };
};
}  // namespace Trading