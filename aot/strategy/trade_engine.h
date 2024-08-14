#pragma once

#include <functional>

#include "aot/Logger.h"
#include "aot/client_request.h"
#include "aot/client_response.h"
#include "aot/common/macros.h"
#include "aot/common/thread_utils.h"
#include "aot/common/time_utils.h"
#include "aot/market_data/market_update.h"
#include "aot/strategy/base_strategy.h"
#include "aot/strategy/market_order_book.h"
// #include "feature_engine.h"
#include "aot/prometheus/event.h"
#include "aot/strategy/order_manager.h"
#include "aot/strategy/position_keeper.h"

// #include "order_manager.h"
// #include "risk_manager.h"

// #include "market_maker.h"
// #include "liquidity_taker.h"

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
        prometheus::EventLFQueue *latency_event_lfqueue, const Ticker &ticker,
        base_strategy::Strategy *predictor);

    ~TradeEngine();

    /// Start and stop the trade engine main thread.
    auto Start() -> void {
        run_    = true;
        thread_ = std::unique_ptr<std::thread>(common::createAndStartThread(
            -1, "Trading/TradeEngine", [this] { Run(); }));
        ASSERT(thread_ != nullptr, "Failed to start TradeEngine thread.");
    };

    auto Stop() -> void {
        while (incoming_md_updates_->size_approx()) {
            logi("Sleeping till all updates are consumed md-size:%\n",
                 incoming_md_updates_->size_approx());
            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(10ms);
        }
        run_ = false;
    }

    common::Delta GetDownTimeInS() const { return time_manager_.GetDeltaInS(); }

    /// Main loop for this thread - processes incoming client responses and
    /// market data updates which in turn may generate client requests.
    auto Run() noexcept -> void;

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
        auto status_op = request_cancel_order_->try_enqueue(*request_cancel_order);
        if (!status_op) [[unlikely]]
            loge("my queue is full");
    }

    /// Process changes to the order book - updates the position keeper, feature
    /// engine and informs the trading algorithm about the update.
    auto OnOrderBookUpdate(std::string ticker, PriceD price, Side side,
                           MarketOrderBookDouble *book) noexcept -> void;

    /// Process trade events - updates the  feature engine and informs the
    /// trading algorithm about the trade event.
    // auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update,
    // MarketOrderBook *book) noexcept -> void;

    /// Deleted default, copy & move constructors and assignment-operators.
    TradeEngine()                                = delete;

    TradeEngine(const TradeEngine &)             = delete;

    TradeEngine(const TradeEngine &&)            = delete;

    TradeEngine &operator=(const TradeEngine &)  = delete;

    TradeEngine &operator=(const TradeEngine &&) = delete;

  private:
    Exchange::EventLFQueue *incoming_md_updates_               = nullptr;
    Exchange::RequestNewLimitOrderLFQueue *request_new_order_  = nullptr;
    Exchange::RequestCancelOrderLFQueue *request_cancel_order_ = nullptr;
    Exchange::ClientResponseLFQueue *response_                 = nullptr;
    OHLCVILFQueue *klines_                                     = nullptr;
    prometheus::EventLFQueue *latency_event_lfqueue_           = nullptr;
    const Ticker &ticker_;
    PositionKeeper position_keeper_;

    volatile bool run_ = false;
    std::unique_ptr<std::thread> thread_;
    TradeEngineCfgHashMap config_;
    common::TimeManager time_manager_;
    Trading::MarketOrderBookDouble order_book_;
    Trading::OrderManager order_manager_;
    Trading::BaseStrategy strategy_;
    /**
    * Process client responses - updates the position keeper and informs the
    trading algorithm about the response.
    */
    auto OnOrderResponse(
        const Exchange::MEClientResponse *client_response) noexcept -> void;
    /**
     * @brief launch strategy for generate trade signals
     *
     * @param new_kline
     */
    auto OnNewKLine(const OHLCVExt *new_kline) noexcept -> void;
    template <bool need_measure_latency, class LFQueuePtr>
    void AddEventForPrometheus(prometheus::EventType type, LFQueuePtr queue) {
        if constexpr (need_measure_latency == true) {
            if (queue) [[likely]] {
                auto status = queue->try_enqueue(
                    prometheus::Event(type, common::getCurrentNanoS()));
                    if(!status)[[unlikely]]
                        loge("my queue is full");
            }
        }
    }
};
}  // namespace Trading