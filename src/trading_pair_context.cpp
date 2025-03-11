#include "aot/strategy/arbitrage/trading_pair_context.h"

std::size_t aot::TradingPairContextHash::operator()(
    const aot::TradingPairContext& step) const {
    std::size_t seed = 0;

    // Комбинируем хэши полей
    boost::hash_combine(seed,
                        std::hash<common::ExchangeId>()(step.exchange_id));
    boost::hash_combine(seed,
                        std::hash<common::MarketType>()(step.market_type));
    boost::hash_combine(seed, common::TradingPairHash()(step.trading_pair));

    return seed;
}
