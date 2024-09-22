#pragma once

#include <functional>

#include "aot/Logger.h"
#include "aot/client_request.h"
#include "aot/client_response.h"
#include "aot/common/macros.h"
#include "aot/common/thread_utils.h"
#include "aot/common/time_utils.h"
#include "aot/market_data/market_update.h"
#include "aot/prometheus/event.h"
#include "aot/strategy/base_strategy.h"
#include "aot/strategy/market_order_book.h"
#include "aot/strategy/order_manager.h"
#include "aot/strategy/position_keeper.h"

#include "aot/strategy/cross_arbitrage/signals.h"

namespace Trading {
const auto kMeasureTForTradeEngine =
    MEASURE_T_FOR_TRADE_ENGINE;  // MEASURE_T_FOR_TRADE_ENGINE define in
                                 // cmakelists.txt;

class BaseStrategy;
class TradeEngine {
  public:
    explicit TradeEngine(
        Exchange::EventLFQueue *market_updates,
        Exchange::RequestNewLimitOrderLFQueue *request_new_order,
        Exchange::RequestCancelOrderLFQueue *request_cancel_order,
        Exchange::ClientResponseLFQueue *response, OHLCVILFQueue *klines,
        prometheus::EventLFQueue *latency_event_lfqueue,
        const common::TradingPair trading_pair,
        common::TradingPairHashMap &pairs, base_strategy::Strategy *predictor);

    virtual ~TradeEngine();
    void SetStrategy(Trading::BaseStrategy* strategy){strategy_ = strategy;};
    void SetOrderManager(Trading::OrderManager* om){order_manager_ = om;};
    /// Start and stop the trade engine main thread.
    auto Start() -> void {
        run_    = true;
        thread_ = std::unique_ptr<std::thread>(common::createAndStartThread(
            -1, "Trading/TradeEngine", [this] { Run(); }));
        ASSERT(thread_ != nullptr, "Failed to start TradeEngine thread.");
    };

    auto Stop() -> void {
        logi("stop trade engine");
        if(incoming_md_updates_)
            while (incoming_md_updates_->size_approx()) {
                logi("Sleeping till all updates are consumed md-size:{}",
                    incoming_md_updates_->size_approx());
                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(10ms);
            }
        run_ = false;
    }
    /**
     * @brief launch strategy for generate trade signals
     *
     * @param new_kline
     */
    virtual auto OnNewKLine(const OHLCVExt *new_kline) noexcept -> void;

    common::Delta GetDownTimeInS() const { return time_manager_.GetDeltaInS(); }

    /// Write a client request to the lock free queue for the order server to
    /// consume and send to the exchange.
    auto SendRequestNewOrder(
        const Exchange::RequestNewOrder *request_new_order) noexcept -> void {
        auto status_op = request_new_order_->try_enqueue(*request_new_order);
        if (!status_op) [[unlikely]]
            loge("my queue is full");
    }
    auto SendRequestCancelOrder(const Exchange::RequestCancelOrder
                                    *request_cancel_order) noexcept -> void {
        auto status_op =
            request_cancel_order_->try_enqueue(*request_cancel_order);
        if (!status_op) [[unlikely]]
            loge("my queue is full");
    }

    /// Process changes to the order book - updates the position keeper, feature
    /// engine and informs the trading algorithm about the update.
    auto OnOrderBookUpdate() noexcept -> void;
    Trading::OrderManager* OrderManager(){return order_manager_;}
    std::string GetStatistics() const { return position_keeper_.ToString(); }

    /// Deleted default, copy & move constructors and assignment-operators.
    TradeEngine()                                = delete;

    TradeEngine(const TradeEngine &)             = delete;

    TradeEngine(const TradeEngine &&)            = delete;

    TradeEngine &operator=(const TradeEngine &)  = delete;

    TradeEngine &operator=(const TradeEngine &&) = delete;

  protected:
    Exchange::EventLFQueue *incoming_md_updates_               = nullptr;
    Exchange::RequestNewLimitOrderLFQueue *request_new_order_  = nullptr;
    Exchange::RequestCancelOrderLFQueue *request_cancel_order_ = nullptr;
    Exchange::ClientResponseLFQueue *response_                 = nullptr;
    prometheus::EventLFQueue *latency_event_lfqueue_           = nullptr;
    const TradingPair trading_pair_;
    TradingPairHashMap pairs_;
    PositionKeeper position_keeper_;

    std::unique_ptr<std::thread> thread_;
    TradeEngineCfgHashMap config_;
    Trading::MarketOrderBook order_book_;
    Trading::OrderManager* order_manager_;
    /**
    * Process client responses - updates the position keeper and informs the
    trading algorithm about the response.
    */
    auto OnOrderResponse(
        const Exchange::MEClientResponse *client_response) noexcept -> void;

  protected:
    volatile bool run_ = false;
    common::TimeManager time_manager_;
    OHLCVILFQueue *klines_ = nullptr;
    Trading::BaseStrategy* strategy_;

    /// Main loop for this thread - processes incoming client responses and
    /// market data updates which in turn may generate client requests.
    virtual auto Run() noexcept -> void;
};
}  // namespace Trading

namespace backtesting {
class TradeEngine : public Trading::TradeEngine {
    backtesting::MarketOrderBook order_book_;

  public:
    explicit TradeEngine(OHLCVILFQueue *klines,
                         const common::TradingPair trading_pair,
                         common::TradingPairHashMap &pairs,
                         base_strategy::Strategy *predictor)
        : Trading::TradeEngine(nullptr, nullptr, nullptr, nullptr, klines,
                               nullptr, trading_pair, pairs, predictor),
          order_book_(trading_pair, pairs) {};
    ~TradeEngine() override = default;

  private:
    auto Run() noexcept -> void override;
    auto OnNewKLine(const OHLCVExt *new_kline) noexcept -> void override;
};
};  // namespace backtesting