#pragma once

#include <string_view>
#include <vector>

#include "aot.pb.h"
#include "aot/common/exchange_trading_pair.h"
#include "aot/common/types.h"
#include "aot/proto/classes/proto_transaction.h"
#include "aot/strategy/arbitrage/arbitrage_cycle.h"
namespace aot {
namespace models {
// Wallet class to represent wallet data
class Trade {
    ArbitrageCycleHash hasher;

  public:
    Trade(const aot::ArbitrageCycle& cycle,
          aot::ExchangeTradingPairs& exchange_trading_pairs)
        : cycle_(cycle), exchange_trading_pairs_(exchange_trading_pairs) {}

    // Serialize to protobuf
    void SerializeTo(std::vector<uint8_t>& output) const {
        // Create a protobuf message
        aot::proto::Trade message;
        message.set_id(hasher(cycle_));

        for (auto& step : cycle_) {
            auto* info = exchange_trading_pairs_.GetPairInfo(step.exchange_id,
                                                             step.trading_pair);
            if (!info) {
                logw("No info for {} {}", step.exchange_id, step.trading_pair);
                continue;
            }
            aot::proto::Transaction* transaction = message.add_transactions();

            transaction->set_trading_pair(info->https_json_request);
            transaction->set_exchange_id(
                aot::models::Transaction::ConvertToProtoEnum(step.exchange_id));
            transaction->set_market_type(
                aot::models::Transaction::ConvertToProtoEnum(step.market_type));
            transaction->set_transaction_action(
                aot::models::Transaction::ConvertToProtoEnum(step.operation));
        }
        // // Get the size of the message and resize the output vector
        int size = message.ByteSizeLong();
        output.resize(size);

        // // Serialize the message into the output vector
        message.SerializeToArray(output.data(), size);
    }

  private:
    const aot::ArbitrageCycle& cycle_;
    aot::ExchangeTradingPairs& exchange_trading_pairs_;
};
};  // namespace models
};  // namespace aot