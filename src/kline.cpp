#include "aot/strategy/kline.h"

KLineService::KLineService(OHLCVGetter* ohlcv_getter,
                                    OHLCVLFQueue* kline_lfqueue) : 
                                    ohlcv_getter_(ohlcv_getter),
                                    kline_lfqueue_(kline_lfqueue) {}

auto KLineService::Run() noexcept -> void {
     while (run_) {

    }
}