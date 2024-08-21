#include "aot/strategy/kline.h"

KLineService::KLineService(OHLCVGetter* ohlcv_getter,
                           OHLCVILFQueue* kline_lfqueue)
    : ohlcv_getter_(ohlcv_getter), kline_lfqueue_(kline_lfqueue) {
    ohlcv_getter_->Init(*kline_lfqueue_);
}

auto KLineService::Run() noexcept -> void {
    while (run_) {
        ohlcv_getter_->LaunchOne();
        time_manager_.Update();
    }
}
namespace backtesting {
KLineService::KLineService(OHLCVGetter* ohlcv_getter,
                           OHLCVILFQueue* internal_kline_lfqueue,
                           OHLCVILFQueue *external_kline_lfqueue,
                           Exchange::EventLFQueue* market_updates_lfqueue)
    : ohlcv_getter_(ohlcv_getter),
      internal_kline_lfqueue_(internal_kline_lfqueue),
      external_kline_lfqueue_(external_kline_lfqueue),
      market_updates_lfqueue_(market_updates_lfqueue) {
    ohlcv_getter_->Init(*internal_kline_lfqueue);
}

auto KLineService::Run() noexcept -> void {
    OHLCVExt o_h_l_c_v_ext[50];
    Exchange::MEMarketUpdateDouble clear_event;
    clear_event.type = Exchange::MarketUpdateType::CLEAR;
    Exchange::MEMarketUpdateDouble new_ohlcv;
    while (run_) {
        ohlcv_getter_->LaunchOne();
        time_manager_.Update();
        size_t count_new_klines =
            internal_kline_lfqueue_->try_dequeue_bulk(o_h_l_c_v_ext, 50);
        for (uint i = 0; i < count_new_klines; i++) [[likely]] {
            auto status_clear_order_book = market_updates_lfqueue_->try_enqueue(clear_event);
            if(!status_clear_order_book)[[unlikely]]
            {
                loge("can't clear order book");
                continue;
            }
            new_ohlcv.side = Common::Side::BUY;//no matter side because price = (best_bid * qty(best_bid) + best_offer * qty(best_offer))/(qty(best_bid) + qty(best_offer))
            new_ohlcv.qty = 1;//no matter qty because price = (best_bid * qty(best_bid) + best_offer * qty(best_offer))/(qty(best_bid) + qty(best_offer))
            new_ohlcv.price = o_h_l_c_v_ext[i].ohlcv.open;
            auto status_new_ohlcv = market_updates_lfqueue_->try_enqueue(new_ohlcv);
            if(!status_new_ohlcv)[[unlikely]]
            {
                loge("can't add new ohlcv event");
                continue;
            }
            auto status_push_new_ohlcv_eternal = external_kline_lfqueue_->try_enqueue(o_h_l_c_v_ext[i]);
            if(!status_push_new_ohlcv_eternal)[[unlikely]]
            {
                loge("can't push new ohlcv event in external lfqueu");
                continue;
            }
        }
        time_manager_.Update();
    }
}
}  // namespace backtesting