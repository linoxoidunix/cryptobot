#pragma once

#include <string_view>
#include <vector>

#include "aot.pb.h"
namespace aot {
namespace models {
// OrderBook class to represent market order book data
class OrderBook {
  public:
    OrderBook(std::string_view exchange, std::string_view market_type, std::string_view trading_pair,
              double best_bid, double best_ask, double spread, double best_bid_qty, double best_ask_qty)
        : exchange_(exchange),
          market_type_(market_type),
          trading_pair_(trading_pair),
          best_bid_(best_bid),
          best_ask_(best_ask),
          spread_(spread),
          best_bid_qty_(best_bid_qty),
          best_ask_qty_(best_ask_qty){}
    OrderBook(){}

    // Serialize to protobuf
    void SerializeTo(std::vector<uint8_t>& output) const {
        // aot::proto::OrderBook message;
        // message.set_exchange(exchange_);
        // message.set_trading_pair(tradingPair_);
        // message.set_best_bid(bestBid_);
        // message.set_best_offer(bestOffer_);
        // message.set_spread(spread_);

        // // Get the size of the message and resize the output vector
        // int size = message.ByteSizeLong();
        // output.resize(size);

        // // Serialize the message into the output vector
        // message.SerializeToArray(output.data(), size);
    }
    void SerializeToJson(nlohmann::json& json) const {
    json = {
        {"orderbook", {
            {"exchange", exchange_},
            {"market_type", market_type_},
            {"trading_pair", trading_pair_},
            {"best_bid", best_bid_},
            {"best_ask", best_ask_},
            {"spread", spread_},
            {"best_bid_qty", best_bid_qty_},
            {"best_ask_qty", best_ask_qty_},
        }}
    };

}

  private:
    std::string exchange_;
    std::string trading_pair_;
    std::string market_type_;
    double best_bid_;
    double best_ask_;
    double spread_;
    double best_bid_qty_;
    double best_ask_qty_;

};
};  // namespace models
};  // namespace aot