#include "aot/strategy/kline.h"

#include <fstream>

#include "re2/re2.h"

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
                           OHLCVILFQueue* external_kline_lfqueue,
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
    Exchange::MEMarketUpdateDouble new_ohlcv[2];
    bool need_fetch_more = true;
    while (run_) {
        if (need_fetch_more) {
            need_fetch_more = ohlcv_getter_->LaunchOne();
        } else
            int x = 0;
        size_t count_new_klines =
            internal_kline_lfqueue_->try_dequeue_bulk(o_h_l_c_v_ext, 1);
        for (uint i = 0; i < count_new_klines; i++) [[likely]] {
            logd("{}", o_h_l_c_v_ext[0].ToString());
            if (market_updates_lfqueue_) [[likely]]
                while (!market_updates_lfqueue_->try_enqueue(clear_event)) {
                    loge("can't clear order book. wait 1s");
                    // using namespace std::literals::chrono_literals;
                    // std::this_thread::sleep_for(1s);
                }
                logi("clear send success");
            // I think that spread equal 0.02. Spread not equal 0
            new_ohlcv[0].side = Common::Side::BUY;
            new_ohlcv[0].qty =
                1;  // no matter qty because price = (best_bid * qty(best_bid) +
                    // best_offer * qty(best_offer))/(qty(best_bid) +
                    // qty(best_offer))
            new_ohlcv[0].price = o_h_l_c_v_ext[i].ohlcv.open * 1.01;

            new_ohlcv[1].side =
                Common::Side::SELL;  // no matter side because price = (best_bid
                                     // * qty(best_bid) + best_offer *
                                     // qty(best_offer))/(qty(best_bid) +
                                     // qty(best_offer))
            new_ohlcv[1].qty =
                1;  // no matter qty because price = (best_bid * qty(best_bid) +
                    // best_offer * qty(best_offer))/(qty(best_bid) +
                    // qty(best_offer))
            new_ohlcv[1].price = o_h_l_c_v_ext[i].ohlcv.open * 0.99;

            if (market_updates_lfqueue_) [[likely]] {
                while (
                    !market_updates_lfqueue_->try_enqueue_bulk(new_ohlcv, 2)) {
                    loge("can't add new ohlcv event. wait 10ms");
                    // using namespace std::literals::chrono_literals;
                    // std::this_thread::sleep_for(10ms);
                }
                logi("send clear 2 events in marketorder book success");

                /**
                 * @brief blocking until market_updates_lfqueue_ not empty
                 *
                 */
                while (market_updates_lfqueue_->size_approx()) {
                    // using namespace std::literals::chrono_literals;
                    // std::this_thread::sleep_for(10ms);
                }
                logi("trade engine process 2 events success");
            }
            if (external_kline_lfqueue_) [[likely]] {
                while (
                    !external_kline_lfqueue_->try_enqueue(o_h_l_c_v_ext[i])) {
                    loge("can't push new ohlcv event in external lfqueu");
                    // using namespace std::literals::chrono_literals;
                    // std::this_thread::sleep_for(10ms);
                }
                logi("send new kline in trade engine success");

                /**
                 * @brief blocking until external_kline_lfqueue_ not empty
                 *
                 */
                while (external_kline_lfqueue_->size_approx()) {
                    // using namespace std::literals::chrono_literals;
                    // std::this_thread::sleep_for(10ms);
                }
                logi("trade engine success process new candle");

            }
            // time_manager_.Update();
        }
    }
}

void OHLCVI::Init(OHLCVILFQueue& lf_queue) {
    lf_queue_ = &lf_queue;
    std::ifstream file(path_to_file_);
    unsigned int number_lines         = 0;
    unsigned int parsed_success_lines = 0;
    try {
        for (std::string line; std::getline(file, line);) {
            ++number_lines;
            std::string open;
            std::string high;
            std::string low;
            std::string close;
            std::string volume;
            if (OHLCVExt new_line;
                RE2::FullMatch(line,
                               ".+\t"
                               "(\\d+\\.\\d+)\t(\\d+\\.\\d+)\t(\\d+\\.\\d+)"
                               "\t(\\d+\\.\\d+)\t(\\d+\\.\\d+)",
                               &open, &high, &low, &close, &volume)) {
                ++parsed_success_lines;
                new_line.ohlcv.open   = stod(open);
                new_line.ohlcv.high   = stod(high);
                new_line.ohlcv.low    = stod(low);
                new_line.ohlcv.close  = stod(close);
                new_line.ohlcv.volume = stod(volume);
                new_line.trading_pair = trading_pair_;
                ohlcv_history_.push_back(new_line);
            }
            fmtlog::poll();
        }
    } catch (const std::ifstream::failure& e) {
        loge("Exception opening/reading file {}", path_to_file_);
    }
    logi("parsed {}/{} lines", parsed_success_lines, number_lines);
    file.close();
    ResetIterator();
};
}  // namespace backtesting