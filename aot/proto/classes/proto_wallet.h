#pragma once

#include <string_view>
#include <vector>

#include "aot.pb.h"

namespace aot{
    namespace models{
        // Wallet class to represent wallet data
        class Wallet {
        public:
            Wallet(std::string_view exchange, std::string_view ticker, double balance)
                : exchange(exchange), ticker(ticker), balance(balance) {}

            // Serialize to protobuf
            void SerializeTo(std::vector<uint8_t>& output) const {
                // Create a protobuf message
                aot::proto::Wallet message;
                message.set_exchange(exchange);
                message.set_ticker(ticker);
                message.set_balance(balance);

                // Get the size of the message and resize the output vector
                int size = message.ByteSizeLong();
                output.resize(size);

                // Serialize the message into the output vector
                message.SerializeToArray(output.data(), size);
            }

        private:
            std::string exchange;
            std::string ticker;
            double balance;
        };
    };
};