#pragma once

#include <string_view>
#include <vector>

#include "aot.pb.h"
namespace aot {
namespace models {
// OrderBook class to represent market order book data
class OrderBook {
  public:
    OrderBook(std::string_view exchange, std::string_view tradingPair,
              double bestBid, double bestOffer, double spread)
        : exchange(exchange),
          tradingPair(tradingPair),
          bestBid(bestBid),
          bestOffer(bestOffer),
          spread(spread) {}

    // Serialize to protobuf
    void SerializeTo(std::vector<uint8_t>& output) const {
        aot::proto::OrderBook message;
        message.set_exchange(exchange);
        message.set_trading_pair(tradingPair);
        message.set_best_bid(bestBid);
        message.set_best_offer(bestOffer);
        message.set_spread(spread);

        // Get the size of the message and resize the output vector
        int size = message.ByteSizeLong();
        output.resize(size);

        // Serialize the message into the output vector
        message.SerializeToArray(output.data(), size);
    }
    void SerializeToJson(nlohmann::json& json) const {
    json = {
        {"orderbook", {
            {"exchange", exchange},
            {"trading_pair", tradingPair},
            {"best_bid", bestBid},
            {"best_offer", bestOffer},
            {"spread", spread},
        }}
    };

}

  private:
    std::string exchange;
    std::string tradingPair;
    double bestBid;
    double bestOffer;
    double spread;
};
};  // namespace models
};  // namespace aot