#include "aot/strategy/trade_engine.h"

#include "aot/strategy/base_strategy.h"
#include "aot/strategy/cross_arbitrage/signals.h"

namespace Trading {
TradeEngine::TradeEngine(
    Exchange::EventLFQueue* market_updates,
    Exchange::RequestNewLimitOrderLFQueue* request_new_order,
    Exchange::RequestCancelOrderLFQueue* request_cancel_order,
    Exchange::ClientResponseLFQueue* response, OHLCVILFQueue* klines,
    prometheus::EventLFQueue* latency_event_lfqueue,
    const common::TradingPair trading_pair, common::TradingPairHashMap& pairs,
    base_strategy::Strategy* predictor)
    : incoming_md_updates_(market_updates),
      request_new_order_(request_new_order),
      request_cancel_order_(request_cancel_order),
      response_(response),
      klines_(klines),
      latency_event_lfqueue_(latency_event_lfqueue),
      trading_pair_(trading_pair),
      pairs_(pairs),
      order_book_(trading_pair, pairs)//,
      //order_manager_(this)
    //   strategy_(predictor, this, &order_manager_, config_, trading_pair,
    //             pairs)
                 {
    common::TradeEngineCfg btcusdt_cfg;
    btcusdt_cfg.clip      = 1;
    config_[trading_pair] = btcusdt_cfg;
};

TradeEngine::~TradeEngine() {
    run_ = false;

    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);

    incoming_md_updates_  = nullptr;
    request_new_order_    = nullptr;
    request_cancel_order_ = nullptr;
    response_             = nullptr;
    klines_               = nullptr;
    if (thread_) [[likely]]
        thread_->join();
};

/// Write a client request to the lock free queue for the order server to
/// consume and send to the exchange.

/// Main loop for this thread - processes incoming client responses and market
/// data updates which in turn may generate client requests.
auto TradeEngine::Run() noexcept -> void {
    logi("TradeEngineService start");
    Exchange::MEClientResponse results_responses[50];
    Exchange::MEMarketUpdate results[50];
    OHLCVExt new_klines[50];
    strategy::cross_arbitrage::Event* strategy_events[50];
    common::Delta delta          = 0;
    unsigned int number_messages = 0;
    unsigned int dequed_elements = 0;
    unsigned int dequed_mb_elements = 0;
    while (run_) {
        size_t count_responses =
            response_->try_dequeue_bulk(results_responses, 50);
        for (uint i = 0; i < count_responses; i++){
            OnOrderResponse(&results_responses[i]);
        }
        if (count_responses) time_manager_.Update();
    }
    logd("dequed klines={}", dequed_elements);
    logd("dequed diffs_mb={}", dequed_mb_elements);

}
}  // namespace Trading

auto Trading::TradeEngine::OnOrderBookUpdate() noexcept -> void {
    auto bbo = order_book_.getBBO();
    position_keeper_.UpdateBBO(trading_pair_, bbo);
}

auto Trading::TradeEngine::OnOrderResponse(
    const Exchange::MEClientResponse* client_response) noexcept -> void {
    if (client_response->type == Exchange::ClientResponseType::FILLED)
        [[unlikely]] {
        position_keeper_.AddFill(client_response);
        strategy_->OnOrderResponse(client_response);
    }
}

auto Trading::TradeEngine::OnNewKLine(const OHLCVExt* new_kline) noexcept
    -> void {
    logi("launch algorithm prediction for {}", new_kline->ToString());
    strategy_->OnNewKLine(new_kline);
}

auto backtesting::TradeEngine::OnNewKLine(const OHLCVExt* new_kline) noexcept
    -> void {
    logi("launch algorithm prediction for {}", new_kline->ToString());
    order_book_.OnNewKLine(new_kline);
    //strategy_.OnNewKLine(new_kline);
}

auto backtesting::TradeEngine::Run() noexcept -> void{
    logi("TradeEngineService start");
    OHLCVExt new_klines[50];

    while (run_){
        size_t count_new_klines = klines_->try_dequeue_bulk(new_klines, 50);
        for (uint i = 0; i < count_new_klines; i++) [[likely]] {
            OnNewKLine(&new_klines[i]);
        }
        if (count_new_klines) {
            time_manager_.Update();
        }
    }
}