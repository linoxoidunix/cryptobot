#include "aot/strategy/base_strategy.h"



auto Trading::BaseStrategy::OnNewKLine(const OHLCV * new_kline) noexcept -> void
{
}


// НЕОБХОДИМО НАПИСАТЬ ГЕНЕРАЦИЮ СИГНАЛОВ КУПИТЬ И ПРОДАТЬ
// ОБРАБОТКА ENTER_LONG, ENTER_SHORT, EXIT_LONG, EXIT_SHORT,