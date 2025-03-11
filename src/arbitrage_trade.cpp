#include "aot/strategy/arbitrage/arbitrage_trade.h"

#include <type_traits>  // Для static_assert

cross_exchange::ArbitrageTrade::ArbitrageTrade(
    aot::TradingPairContext trading_pair_context, aot::Step step)
    : trading_pair_context_(trading_pair_context), step_(step) {}

void cross_exchange::ArbitrageTrade::SerializeToJson(nlohmann::json& j) const {
    cross_exchange::ArbitrageTradeHash hasher;
    nlohmann::json trading_pair_context_json;
    trading_pair_context_.SerializeToJson(trading_pair_context_json);
    nlohmann::json transaction_json;
    step_.SerializeToJson(transaction_json);
    j = {
        {"id", hasher(*this)},
        {"trading_pair_context", trading_pair_context_json},
        {"step", transaction_json},
    };
}

std::size_t cross_exchange::ArbitrageTradeHash::operator()(
    const cross_exchange::ArbitrageTrade& trade) const {
    std::size_t seed = 0;
    aot::TradingPairContextHash context_hash;
    aot::StepHash transaction_hash;
    // Комбинируем хэши всех полей объекта с использованием boost::hash_combine
    boost::hash_combine(seed, context_hash(trade.trading_pair_context_));
    boost::hash_combine(seed, transaction_hash(trade.step_));

    return seed;
};

namespace cross_exchange {
bool operator==(const ArbitrageTrade& lhs, const ArbitrageTrade& rhs) {
    return lhs.trading_pair_context_ == rhs.trading_pair_context_ &&
           lhs.step_ == rhs.step_;
};
};  // namespace cross_exchange