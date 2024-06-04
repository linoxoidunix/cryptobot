#pragma once

#include <functional>

#include "aot/common/thread_utils.h"
#include "aot/common/time_utils.h"
#include "aot/common/macros.h"

// #include "exchange/order_server/client_request.h"
// #include "exchange/order_server/client_response.h"
#include "aot/market_data/market_update.h"

#include "aot/strategy/market_order_book.h"
#include "aot/Logger.h"
//#include "feature_engine.h"
#include "aot/strategy/position_keeper.h"
//#include "order_manager.h"
//#include "risk_manager.h"

//#include "market_maker.h"
//#include "liquidity_taker.h"

namespace Trading {
  class TradeEngine {
  public:
    explicit TradeEngine(Exchange::EventLFQueue *market_updates, const Ticker& ticker);

    ~TradeEngine();

    /// Start and stop the trade engine main thread.
    auto Start() -> void {
      run_ = true;
      ASSERT(common::createAndStartThread(-1, "Trading/TradeEngine", [this] { Run(); }) != nullptr, "Failed to start TradeEngine thread.");
    };

    auto Stop() -> void {
      while(incoming_md_updates_->size_approx()) {
        logi("Sleeping till all updates are consumed md-size:%\n", incoming_md_updates_->size_approx());
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(10ms);
      }
      run_ = false;
    }

    common::Delta GetDownTimeInS() const { return time_manager_.GetDeltaInS(); }


    /// Main loop for this thread - processes incoming client responses and market data updates which in turn may generate client requests.
    auto Run() noexcept -> void;

    /// Write a client request to the lock free queue for the order server to consume and send to the exchange.
    //auto sendClientRequest(const Exchange::MEClientRequest *client_request) noexcept -> void;

    /// Process changes to the order book - updates the position keeper, feature engine and informs the trading algorithm about the update.
    auto OnOrderBookUpdate(std::string ticker, Price price, Side side, MarketOrderBookDouble *book) noexcept -> void;

    /// Process trade events - updates the  feature engine and informs the trading algorithm about the trade event.
    //auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *book) noexcept -> void;

    /// Process client responses - updates the position keeper and informs the trading algorithm about the response.
    auto OnOrderResponse(const Exchange::MEClientResponse *client_response) noexcept -> void;

    /// Function wrappers to dispatch order book updates, trade events and client responses to the trading algorithm.
    //std::function<void(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *book)> algoOnTradeUpdate_;
  



    /// Deleted default, copy & move constructors and assignment-operators.
    TradeEngine() = delete;

    TradeEngine(const TradeEngine &) = delete;

    TradeEngine(const TradeEngine &&) = delete;

    TradeEngine &operator=(const TradeEngine &) = delete;

    TradeEngine &operator=(const TradeEngine &&) = delete;

  private:
    
    Exchange::EventLFQueue *incoming_md_updates_ = nullptr;
    common::TimeManager time_manager_;
    Trading::MarketOrderBookDouble order_book_;
    volatile bool run_ = false;
    const Ticker& ticker_;
    PositionKeeper position_keeper_;
  };
}