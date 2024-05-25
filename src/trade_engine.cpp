#include "aot/strategy/trade_engine.h"

namespace Trading {
  TradeEngine::TradeEngine(
                           Exchange::EventLFQueue *market_updates)
      : incoming_md_updates_(market_updates)
         {
    // for (size_t i = 0; i < ticker_order_book_.size(); ++i) {
    //   ticker_order_book_[i] = new MarketOrderBook(i, &logger_);
    //   ticker_order_book_[i]->setTradeEngine(this);
    // }
  };

  TradeEngine::~TradeEngine() {
    run_ = false;

    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);



    // for (auto &order_book: ticker_order_book_) {
    //   delete order_book;
    //   order_book = nullptr;
    // }
    incoming_md_updates_ = nullptr;
  };

  /// Write a client request to the lock free queue for the order server to consume and send to the exchange.
  

  /// Main loop for this thread - processes incoming client responses and market data updates which in turn may generate client requests.
  auto TradeEngine::Run() noexcept -> void {
    logi("TradeEngineService start");
    while (run_) {
        Exchange::MEMarketUpdate event;
        if (bool found = incoming_md_updates_->try_dequeue(event); !found) [[unlikely]]
            continue;
        logd("Processing in trade engine {}", event.ToString());
        
        //order_book.onMarketUpdate(&event);
        time_manager_.Update();
      }
    }
  }

//   /// Process changes to the order book - updates the position keeper, feature engine and informs the trading algorithm about the update.
//   auto TradeEngine::onOrderBookUpdate(TickerId ticker_id, Price price, Side side, MarketOrderBook *book) noexcept -> void {
//     logger_.log("%:% %() % ticker:% price:% side:%\n", __FILE__, __LINE__, __FUNCTION__,
//                 Common::getCurrentTimeStr(&time_str_), ticker_id, Common::priceToString(price).c_str(),
//                 Common::sideToString(side).c_str());

//     auto bbo = book->getBBO();

//     position_keeper_.updateBBO(ticker_id, bbo);

//     feature_engine_.onOrderBookUpdate(ticker_id, price, side, book);

//     algoOnOrderBookUpdate_(ticker_id, price, side, book);
//   }

//   /// Process trade events - updates the  feature engine and informs the trading algorithm about the trade event.
//   auto TradeEngine::onTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *book) noexcept -> void {
//     logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
//                 market_update->toString().c_str());

//     feature_engine_.onTradeUpdate(market_update, book);

//     algoOnTradeUpdate_(market_update, book);
//   }

//   /// Process client responses - updates the position keeper and informs the trading algorithm about the response.
//   auto TradeEngine::onOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept -> void {
//     logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
//                 client_response->toString().c_str());

//     if (UNLIKELY(client_response->type_ == Exchange::ClientResponseType::FILLED)) {
//       position_keeper_.addFill(client_response);
//     }

//     algoOnOrderUpdate_(client_response);
//   }
