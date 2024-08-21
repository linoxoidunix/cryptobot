#include "aot/strategy/base_strategy.h"

#include "aot/common/types.h"
#include "aot/launcher_predictor.h"

Trading::BaseStrategy::BaseStrategy(base_strategy::Strategy* strategy,
                                    TradeEngine* trade_engine,
                                    OrderManager* order_manager,
                                    const TradeEngineCfgHashMap& ticker_cfg)
    : strategy_(strategy),
      trade_engine_(trade_engine),
      order_manager_(order_manager),
      ticker_cfg_(ticker_cfg) {
    InitActions();
}

auto Trading::BaseStrategy::OnNewKLine(const OHLCVExt* new_kline) noexcept
    -> void {
    if(!strategy_)[[unlikely]]
        return;    
    auto result = strategy_->Predict(
        new_kline->ohlcv.open, new_kline->ohlcv.high, new_kline->ohlcv.low,
        new_kline->ohlcv.close, new_kline->ohlcv.volume);
    base_strategy::Strategy::Parser parser;
    auto number_action = parser.Parse(result);
    actions_[(int)number_action](new_kline->ticker);     //i confident that number_action <= actions_size()
}

// НЕОБХОДИМО НАПИСАТЬ ГЕНЕРАЦИЮ СИГНАЛОВ КУПИТЬ И ПРОДАТЬ
// ОБРАБОТКА ENTER_LONG, ENTER_SHORT, EXIT_LONG, EXIT_SHORT,
