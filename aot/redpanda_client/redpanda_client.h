#pragma once
#include <string>
#include <vector>
#include <memory>

#include "boost/asio.hpp"

#include "nlohmann/json.hpp"
#include "cppkafka/cppkafka.h"
#include "google/protobuf/message.h"

#include "aot/bus/bus_component.h"
#include "aot/Logger.h"
#include "aot/common/types.h"
#include "aot/common/exchange_trading_pair.h"
#include "aot/strategy/market_order.h"
#include "aot/proto/classes/proto_orderbook.h"
#include "aot/proto/classes/proto_pnl.h"
#include "aot/proto/classes/proto_wallet.h"
namespace aot {

class KafkaClient {
public:
    class Topic : public std::string {
    public:
        explicit Topic(std::string_view topic) : std::string(topic) {}
    };

    class MessageBuilder : public cppkafka::MessageBuilder {
    public:
        explicit MessageBuilder(const Topic& topic) : cppkafka::MessageBuilder(static_cast<std::string>(topic)) {}
    };

    explicit KafkaClient(std::string_view brokers)
        : io_context_(),
          work_guard_(boost::asio::make_work_guard(io_context_)),
          producer_thread_([this] { io_context_.run(); }) {

        cppkafka::Configuration config = {
            {"metadata.broker.list", brokers.data()}
        };

        producer_ = std::make_unique<cppkafka::Producer>(config);
    }

    ~KafkaClient() {
        work_guard_.reset();
        if (producer_thread_.joinable()) {
            producer_thread_.join();
        }
    }

    template <class T>
    void SendMessage(const T& object) {
        if (!producer_) {
            logw("cppkafka::Producer not initialized");
            return;
        }

        std::vector<uint8_t> payload;
        object.SerializeTo(payload);
        boost::asio::post(io_context_, [this, payload]() mutable {
            try {
                if constexpr (std::is_same<T, aot::models::OrderBook>::value) {
                    builder_order_book_.payload(payload);
                    producer_->produce(builder_order_book_);
                } else if constexpr (std::is_same<T, aot::models::Wallet>::value) {
                    builder_wallet_.payload(payload);
                    producer_->produce(builder_wallet_);
                } else if constexpr (std::is_same<T, aot::models::Pnl>::value) {
                    builder_pnl_.payload(payload);
                    producer_->produce(builder_pnl_);
                } else {
                    logw("Unknown object type");
                }
            } catch (const std::exception& ex) {
                loge("Error producing message: {}", std::string(ex.what()));
            }
        });
    }

    template <class T>
    void SendMessageAsJson(const T& object) {
        if (!producer_) {
            logw("cppkafka::Producer not initialized");
            return;
        }

        // Сериализация объекта в JSON
        nlohmann::json json_object;
        try {
            object.SerializeToJson(json_object);
        } catch (const std::exception& ex) {
            loge("Failed to serialize object to JSON: {}", std::string(ex.what()));
            return;
        }

        // Преобразование JSON в строку
        std::string json_string = json_object.dump();
        boost::asio::post(io_context_, [this, json_string]() mutable {
            try {
                // Определение темы на основе типа объекта
                if constexpr (std::is_same<T, aot::models::OrderBook>::value) {
                    builder_order_book_.payload(json_string);
                    producer_->produce(builder_order_book_);
                } else if constexpr (std::is_same<T, aot::models::Wallet>::value) {
                    builder_wallet_.payload(json_string);
                    producer_->produce(builder_wallet_);
                } else if constexpr (std::is_same<T, aot::models::Pnl>::value) {
                    builder_pnl_.payload(json_string);
                    producer_->produce(builder_pnl_);
                } else {
                    logw("Unknown object type for JSON serialization");
                    return;
                }
            } catch (const std::exception& ex) {
                loge("Error producing JSON message: {}", std::string(ex.what()));
            }
        });
    }

    void WaitUntilFinished() {
        if (!producer_) {
            logw("cppkafka::Producer not initialized");
            return;
        }
        boost::asio::post(io_context_, [this]() {
            producer_->flush();
        });
    }

private:
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    std::thread producer_thread_;
    std::unique_ptr<cppkafka::Producer> producer_;

    Topic topic_order_book_{"orderbook"};
    Topic topic_pnl_{"pnl"};
    Topic topic_wallet_{"wallet"};

    MessageBuilder builder_order_book_{topic_order_book_};
    MessageBuilder builder_wallet_{topic_wallet_};
    MessageBuilder builder_pnl_{topic_pnl_};
};

template <typename Executor>
class RedPandaComponent : public bus::Component,
                                 public KafkaClient {
    static constexpr std::string_view name_component_ =
        "RedPandaComponent";
    common::ExchangeIdPrinter exchange_id_printer_;
    Executor& executor_;
    aot::ExchangeTradingPairs& exchange_trading_pairs_;
  public:
    explicit RedPandaComponent(
        Executor& executor,
        std::string_view broker,
        aot::ExchangeTradingPairs& exchange_trading_pairs )
        : KafkaClient(broker),
        executor_(executor),
        exchange_trading_pairs_(exchange_trading_pairs)
         {}
    ~RedPandaComponent() override = default;
    void AsyncHandleEvent(
        boost::intrusive_ptr<Trading::BusEventNewBBO> event) override {
        auto wrapped_event = event->WrappedEventIntrusive();
        boost::asio::co_spawn(executor_,
            [this, wrapped_event]() -> boost::asio::awaitable<void> {
                co_await HandleEventAsync(wrapped_event);
            },
            boost::asio::detached);
    }
    void AsyncStop() override {  }
    private:
     boost::asio::awaitable<void> HandleEventAsync(boost::intrusive_ptr<Trading::NewBBO> wrapped_event) {
        auto exchange_id = wrapped_event->exchange_id;
        auto trading_pair = wrapped_event->trading_pair;

        auto* info = exchange_trading_pairs_.GetPairInfo(exchange_id, trading_pair);
        if (!info) {
            logw("No info for {} {}", exchange_id, trading_pair.ToString());
            co_return;
        }

        if (!ValidatePrices(wrapped_event)) {
            co_return;
        }

        aot::models::OrderBook ob = PrepareOrderBook(wrapped_event, *info);
        SendMessageAsJson(ob);
        co_return;
    }

    bool ValidatePrices(boost::intrusive_ptr<Trading::NewBBO> event) {
        if (event->bbo.bid_price == common::kPriceInvalid) {
            logw("Invalid best bid price. Skipping event");
            return false;
        }
        if (event->bbo.ask_price == common::kPriceInvalid) {
            logw("Invalid best offer price. Skipping event");
            return false;
        }
        return true;
    }

    aot::models::OrderBook PrepareOrderBook(
        boost::intrusive_ptr<Trading::NewBBO> event,
        const common::TradingPairInfo& info) {
        int spread = event->bbo.ask_price - event->bbo.bid_price;
        return aot::models::OrderBook(
            exchange_id_printer_.ToString(event->exchange_id),
            info.https_json_request,
            event->bbo.bid_price,
            event->bbo.ask_price,
            spread);
    }
    
    
    
    
    
    
    
    
    
    
    
    
    // void AsyncHandleEvent(
    //     boost::intrusive_ptr<Trading::BusEventNewBBO> event)
    //     override {
    //     auto wrapped_event = event->WrappedEventIntrusive();
    //     boost::asio::co_spawn(executor_,
    //                           [this, wrapped_event]()-> boost::asio::awaitable<void>{
    //                             auto bb = wrapped_event->bbo.bid_price;
    //                             auto bo = wrapped_event->bbo.ask_price;

    //                             auto exchange_id_base = wrapped_event->exchange_id;
    //                             auto trading_pair_base = wrapped_event->trading_pair;

    //                             auto* info = exchange_trading_pairs_.GetPairInfo(exchange_id_base, trading_pair_base);
    //                             if(info == nullptr){
    //                                 logw("no info for {} {}", exchange_id_base, trading_pair_base.ToString());
    //                                 co_return;
    //                             }
    //                             const auto exchange = fmt::format("{}", wrapped_event->exchange_id);
    //                             const auto trading_pair = info->https_json_request;
    //                             if(bb == common::kPriceInvalid)
    //                             {
    //                                 logw("Invalid best bid price. Skipping event");
    //                                 co_return;
    //                             }
    //                             if(bo == common::kPriceInvalid)
    //                             {
    //                                 logw("Invalid best offer price. Skipping event");
    //                                 co_return;
    //                             }
    //                             auto spread = bo - bb;
    //                             aot::models::OrderBook ob(
    //                                 exchange,
    //                                 trading_pair,
    //                                 bb,
    //                                 bo,
    //                                 spread);
    //                             //SendMessage(ob);
    //                             SendMessageAsJson(ob);
    //                             co_return;
    //                           },
    //                           boost::asio::detached);
    // };

};
}; // namespace aot
