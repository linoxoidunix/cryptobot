#include "aot/strategy/trade_engine.h"

#include "aot/strategy/base_strategy.h"

namespace Trading {
TradeEngine::TradeEngine(
    Exchange::EventLFQueue* market_updates,
    Exchange::RequestNewLimitOrderLFQueue* request_new_order,
    Exchange::RequestCancelOrderLFQueue* request_cancel_order,
    Exchange::ClientResponseLFQueue* response, 
    OHLCVILFQueue* klines,
    prometheus::EventLFQueue* latency_event_lfqueue,
    const Ticker& ticker, base_strategy::Strategy* predictor)
    : incoming_md_updates_(market_updates),
      request_new_order_(request_new_order),
      request_cancel_order_(request_cancel_order),
      response_(response),
      klines_(klines),
      latency_event_lfqueue_(latency_event_lfqueue),
      ticker_(ticker),
      order_book_(ticker),
      order_manager_(this),
      strategy_(predictor, this, &order_manager_, config_) {
    Common::TradeEngineCfg btcusdt_cfg;
    btcusdt_cfg.clip   = 0.0001;
    config_["BTCUSDT"] = btcusdt_cfg;
    order_book_.SetTradeEngine(this);
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
    Exchange::MEMarketUpdateDouble results[50];
    OHLCVExt new_klines[50];
    
    common::Delta delta = 0;
    unsigned int number_messages = 0;

    while (run_) {
        size_t count_responses =
            response_->try_dequeue_bulk(results_responses, 50);
        for (uint i = 0; i < count_responses; i++) [[likely]] {
            OnOrderResponse(&results_responses[i]);
        }
        time_manager_.Update();

        size_t count = incoming_md_updates_->try_dequeue_bulk(results, 50);
        auto start = common::getCurrentNanoS();
        for (uint i = 0; i < count; i++) [[likely]] {
            order_book_.OnMarketUpdate(&results[i]);
        }
        auto end = common::getCurrentNanoS();

        if(count)[[likely]]
        {
            delta += end - start;
            number_messages += count;
            prometheus::LogEvent<kMeasureTForTradeEngine>(prometheus::EventType::kUpdateSpeedOrderBook, delta*1.0/number_messages, latency_event_lfqueue_);
        }
        time_manager_.Update();

        size_t count_new_klines = klines_->try_dequeue_bulk(new_klines, 50);
        auto begin_on_new_line = common::getCurrentNanoS();
        for (uint i = 0; i < count_new_klines; i++) [[likely]] {
            OnNewKLine(&new_klines[i]);
        }
        auto end_on_new_line = common::getCurrentNanoS();
        if(count_new_klines)[[likely]]
            prometheus::LogEvent<kMeasureTForTradeEngine>(prometheus::EventType::kStrategyOnNewKLineRate, (end_on_new_line-begin_on_new_line)*1.0/count_new_klines, latency_event_lfqueue_);
        time_manager_.Update();

        
        if (count) [[likely]] {
            auto bbo = order_book_.getBBO();
            logi("process {} operations {}", count, bbo->ToString());
        }
        fmtlog::poll();
    }
}
}  // namespace Trading

auto Trading::TradeEngine::OnOrderBookUpdate(
    const std::string& ticker, PriceD price, Side side,
    MarketOrderBookDouble* book) noexcept -> void {
    auto bbo = order_book_.getBBO();
    position_keeper_.UpdateBBO(ticker, bbo);
    strategy_.OnOrderBookUpdate(ticker, price, side, &order_book_);
}

auto Trading::TradeEngine::OnOrderResponse(
    const Exchange::MEClientResponse* client_response) noexcept -> void {
    if (client_response->type == Exchange::ClientResponseType::FILLED)
        [[unlikely]] {
        position_keeper_.AddFill(client_response);
        strategy_.OnOrderResponse(client_response);
    }
}

auto Trading::TradeEngine::OnNewKLine(const OHLCVExt* new_kline) noexcept
    -> void {
    // launch algorithm prediction, that generate signals
    logi("launch algorithm prediction for {}", new_kline->ToString());
    //AddEventForPrometheus<kMeasureTForTradeEngine>(prometheus::EventType::kStrategyOnNewKLineBefore, latency_event_lfqueue_);
    strategy_.OnNewKLine(new_kline);
    //AddEventForPrometheus<kMeasureTForTradeEngine>(prometheus::EventType::kStrategyOnNewKLineAfter, latency_event_lfqueue_);
}

//   /// Process changes to the order book - updates the position keeper,
//   feature engine and informs the trading algorithm about the update. auto
//   TradeEngine::onOrderBookUpdate(TickerId ticker_id, Price price, Side
//   side, MarketOrderBook *book) noexcept -> void {
//     logger_.log("%:% %() % ticker:% price:% side:%\n", __FILE__,
//     __LINE__,
//     __FUNCTION__,
//                 Common::getCurrentTimeStr(&time_str_), ticker_id,
//                 Common::priceToString(price).c_str(),
//                 Common::sideToString(side).c_str());

//     auto bbo = book->getBBO();

//     position_keeper_.updateBBO(ticker_id, bbo);

//     feature_engine_.onOrderBookUpdate(ticker_id, price, side, book);

//     algoOnOrderBookUpdate_(ticker_id, price, side, book);
//   }

//   /// Process trade events - updates the  feature engine and informs the
//   trading algorithm about the trade event. auto
//   TradeEngine::onTradeUpdate(const Exchange::MEMarketUpdate
//   *market_update, MarketOrderBook *book) noexcept -> void {
//     logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__,
//     Common::getCurrentTimeStr(&time_str_),
//                 market_update->toString().c_str());

//     feature_engine_.onTradeUpdate(market_update, book);

//     algoOnTradeUpdate_(market_update, book);
//   }

//   /// Process client responses - updates the position keeper and informs
//   the trading algorithm about the response. auto
//   TradeEngine::onOrderUpdate(const Exchange::MEClientResponse
//   *client_response) noexcept -> void {
//     logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__,
//     Common::getCurrentTimeStr(&time_str_),
//                 client_response->toString().c_str());

//     if (UNLIKELY(client_response->type_ ==
//     Exchange::ClientResponseType::FILLED)) {
//       position_keeper_.addFill(client_response);
//     }

//     algoOnOrderUpdate_(client_response);
//   }
