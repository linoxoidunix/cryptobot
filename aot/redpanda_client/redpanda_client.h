#pragma once
#include <string>
#include <vector>
#include <memory>

#include "boost/asio.hpp"

#include "nlohmann/json.hpp"
#include "cppkafka/cppkafka.h"
#include "google/protobuf/message.h"

#include "aot/Logger.h"
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

    explicit KafkaClient(const std::string& brokers)
        : io_context_(),
          work_guard_(boost::asio::make_work_guard(io_context_)),
          producer_thread_([this] { io_context_.run(); }) {

        cppkafka::Configuration config = {
            {"metadata.broker.list", brokers}
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

        io_context_.post([this, &payload]() mutable {
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

    void WaitUntilFinished() {
        if (!producer_) {
            logw("cppkafka::Producer not initialized");
            return;
        }

        io_context_.post([this] {
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

} // namespace aot
