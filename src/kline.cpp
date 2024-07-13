#include "aot/strategy/kline.h"

KLineService::KLineService(OHLCVGetter* ohlcv_getter,
                                    OHLCVILFQueue* kline_lfqueue) : 
                                    ohlcv_getter_(ohlcv_getter),
                                    kline_lfqueue_(kline_lfqueue) {
                                        ohlcv_getter_->Init(*kline_lfqueue_);
                                    }

auto KLineService::Run() noexcept -> void {
     while (run_) {
        ohlcv_getter_->LaunchOne();
        time_manager_.Update();
    }
}