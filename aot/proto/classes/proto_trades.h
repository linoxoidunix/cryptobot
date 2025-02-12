#pragma once

#include <string_view>
#include <vector>

#include "aot.pb.h"
#include "aot/common/exchange_trading_pair.h"
#include "aot/common/types.h"
#include "aot/proto/classes/proto_trade.h"
#include "aot/proto/classes/proto_transaction.h"
#include "aot/strategy/arbitrage/arbitrage_cycle.h"
#include "aot/strategy/arbitrage/trade_dictionary.h"

namespace aot {
namespace models {
// Wallet class to represent wallet data
class Trades {
  public:
    Trades(const aot::TradeDictionary& trades,
           aot::ExchangeTradingPairs& exchange_trading_pairs)
        : trades_(trades), exchange_trading_pairs_(exchange_trading_pairs) {}

    // Serialize to protobuf
    void SerializeTo(std::vector<uint8_t>& output) const {
        // Create a protobuf message
        aot::proto::Trades message;
        auto unique_cycles = trades_.GetUniqueCycles();
        for (auto& trade : unique_cycles) {
            auto id         = hasher_(trade);
            auto& new_trade = (*message.mutable_trades())[id];
            new_trade.set_id(id);
            // Создаем экземпляр модели Trade и сериализуем её
            aot::models::Trade trade_model(trade, exchange_trading_pairs_);
            std::vector<uint8_t> trade_data;
            trade_model.SerializeTo(trade_data);

            // Преобразуем бинарные данные обратно для Proto
            if (!new_trade.ParseFromArray(trade_data.data(),
                                          trade_data.size())) {
                logw("Failed to parse trade data for id: {}", id);
            }
        }
        // // Get the size of the message and resize the output vector
        int size = message.ByteSizeLong();
        output.resize(size);

        // // Serialize the message into the output vector
        message.SerializeToArray(output.data(), size);
    }

  private:
    const aot::TradeDictionary& trades_;
    aot::ExchangeTradingPairs& exchange_trading_pairs_;
    ArbitrageCycleHash hasher_;
};
};  // namespace models
};  // namespace aot