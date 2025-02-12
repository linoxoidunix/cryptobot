#pragma once

#include <string_view>
#include <vector>

#include "aot.pb.h"
#include "aot/common/types.h"
#include "aot/strategy/arbitrage/arbitrage_step.h"

namespace aot {
namespace models {
// Wallet class to represent wallet data
class Transaction {
  public:
    Transaction(std::string_view trading_pair, common::ExchangeId exchange_id,
                common::MarketType market_type, aot::Operation operation)
        : trading_pair_(trading_pair),
          exchange_id_(exchange_id),
          market_type_(market_type),
          operation_(operation) {}
    Transaction(const aot::Step& step, const common::TradingPairInfo& info)
        : trading_pair_(info.https_json_request),
          exchange_id_(step.exchange_id),
          market_type_(step.market_type),
          operation_(step.operation) {}

    // Serialize to protobuf
    void SerializeTo(std::vector<uint8_t>& output) const {
        // Create a protobuf message
        aot::proto::Transaction message;
        message.set_trading_pair(std::string(trading_pair_));
        message.set_exchange_id(ConvertToProtoEnum(exchange_id_));
        message.set_market_type(ConvertToProtoEnum(market_type_));
        message.set_transaction_action(ConvertToProtoEnum(operation_));

        // message.set_exchange(exchange);
        // message.set_ticker(ticker);
        // message.set_balance(balance);

        // // Get the size of the message and resize the output vector
        int size = message.ByteSizeLong();
        output.resize(size);

        // // Serialize the message into the output vector
        message.SerializeToArray(output.data(), size);
    }
    static inline aot::proto::ExchangeId ConvertToProtoEnum(
        common::ExchangeId exchange_id) {
        switch (exchange_id) {
            case common::ExchangeId::kBinance:
                return aot::proto::ExchangeId::BINANCE;
            case common::ExchangeId::kBybit:
                return aot::proto::ExchangeId::BYBIT;
            case common::ExchangeId::kMexc:
                return aot::proto::ExchangeId::MEXC;
        }
        return aot::proto::ExchangeId::EXCHANGE_ID_INVALID;
    }
    static inline aot::proto::MarketType ConvertToProtoEnum(
        common::MarketType market_type) {
        switch (market_type) {
            case common::MarketType::kSpot:
                return aot::proto::MarketType::SPOT;
            case common::MarketType::kFutures:
                return aot::proto::MarketType::FUTURES;
            case common::MarketType::kOptions:
                return aot::proto::MarketType::OPTIONS;
        }
        return aot::proto::MarketType::MARKET_TYPE_INVALID;
    }
    static inline aot::proto::TransactionAction ConvertToProtoEnum(
        aot::Operation operation) {
        switch (operation) {
            case aot::Operation::kBuy:
                return aot::proto::TransactionAction::BUY;
            case aot::Operation::kSell:
                return aot::proto::TransactionAction::SELL;
        }
        return aot::proto::TransactionAction::SELL;
    }

  private:
    std::string_view trading_pair_;
    common::ExchangeId exchange_id_;
    common::MarketType market_type_;
    aot::Operation operation_;
};
};  // namespace models
};  // namespace aot