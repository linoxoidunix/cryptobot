#include "aot/strategy/base_strategy.h"

#include "aot/common/types.h"
#include "aot/launcher_predictor.h"

Trading::BaseStrategy::BaseStrategy(base_strategy::Strategy* strategy,
                                    TradeEngine* trade_engine,
                                    OrderManager* order_manager,
                                    const TradeEngineCfgHashMap& ticker_cfg,
                                    const Common::TradingPair trading_pairs,
                                    Common::TradingPairHashMap& pairs)
    : strategy_(strategy),
      trade_engine_(trade_engine),
      order_manager_(order_manager),
      ticker_cfg_(ticker_cfg),
      trading_pairs_(trading_pairs),
      pairs_(pairs) {
    InitActions();
}

auto Trading::BaseStrategy::OnNewKLine(const OHLCVExt* new_kline) noexcept
    -> void {
    if (!strategy_) [[unlikely]]
        return;
    auto result = strategy_->Predict(
        new_kline->ohlcv.open, new_kline->ohlcv.high, new_kline->ohlcv.low,
        new_kline->ohlcv.close, new_kline->ohlcv.volume);
    base_strategy::Strategy::Parser parser;
    auto number_action = parser.Parse(result);
    logd("{}", new_kline->trading_pair.ToString());
    actions_[(int)number_action](
        new_kline->trading_pair);  // i confident that number_action <= actions_size()
}
