#pragma once

#include <string_view>
#include <vector>

#include "aot.pb.h"
namespace aot {
namespace models {
// Pnl class to represent profit and loss data
class Pnl {
  public:
    Pnl(std::string_view& exchange, std::string_view& tradingPair,
        double realized, double unrealized)
        : exchange(exchange),
          tradingPair(tradingPair),
          realized(realized),
          unrealized(unrealized) {}

    // Serialize to protobuf
    void SerializeTo(std::vector<uint8_t>& output) const {
        aot::proto::Pnl message;
        message.set_exchange(exchange);
        message.set_trading_pair(tradingPair);
        message.set_realized(realized);
        message.set_unrealized(unrealized);

        // Get the size of the message and resize the output vector
        int size = message.ByteSizeLong();
        output.resize(size);

        // Serialize the message into the output vector
        message.SerializeToArray(output.data(), size);
    }

  private:
    std::string exchange;
    std::string tradingPair;
    double realized;
    double unrealized;
};
};  // namespace models
};  // namespace aot