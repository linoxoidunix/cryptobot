#pragma once

#include <functional>

#include "aot/Exchange.h"
#include "aot/client_request.h"
#include "aot/client_response.h"
#include "aot/common/macros.h"
#include "aot/common/thread_utils.h"
#include "aot/common/time_utils.h"
#include "aot/market_data/market_update.h"
#include "aot/strategy/trade_engine.h"

namespace inner {
class OrderNewI;
};

class KLineService {
  public:
    explicit KLineService(OHLCVGetter *ohlcv_getter,
                          OHLCVILFQueue *kline_lfqueue);

    ~KLineService() {
        stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
        if (thread_) [[likely]]
            thread_->join();
    }

    /// Start and stop the order gateway main thread.
    auto start() {
        run_    = true;
        thread_ = std::unique_ptr<std::thread>(common::createAndStartThread(
            -1, "Trading/KLineService", [this]() { Run(); }));
        ASSERT(thread_ != nullptr, "Failed to start KLineService thread.");
    }
    common::Delta GetDownTimeInS() { return time_manager_.GetDeltaInS(); }
    auto stop() -> void { run_ = false; }

    /// Deleted default, copy & move constructors and assignment-operators.
    KLineService()                                 = delete;

    KLineService(const KLineService &)             = delete;

    KLineService(const KLineService &&)            = delete;

    KLineService &operator=(const KLineService &)  = delete;

    KLineService &operator=(const KLineService &&) = delete;

  private:
    std::unique_ptr<std::thread> thread_;
    OHLCVGetter *ohlcv_getter_;
    /// Lock free queue on which we consume client requests from the trade
    /// engine and forward them to the exchange's order server.
    OHLCVILFQueue *kline_lfqueue_ = nullptr;
    volatile bool run_            = false;

  private:
    /// Main thread loop - sends out client requests to the exchange and reads
    /// and dispatches incoming client responses.
    auto Run() noexcept -> void;
    common::TimeManager time_manager_;
};

namespace backtesting {
/**
 * @brief class that get new data from ohlcv_getter than process than redirect
 * to kline_lfqueue and
 *
 */
class KLineService {
  public:
    explicit KLineService(OHLCVGetter *ohlcv_getter,
                          OHLCVILFQueue *internal_kline_lfqueue,
                          OHLCVILFQueue *external_kline_lfqueue,
                          Exchange::EventLFQueue *market_updates_lfqueue);

    ~KLineService() {
        stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
        if (thread_) [[likely]]
            thread_->join();
    }

    /// Start and stop the order gateway main thread.
    auto start() {
        run_    = true;
        thread_ = std::unique_ptr<std::thread>(common::createAndStartThread(
            -1, "Trading/KLineService", [this]() { Run(); }));
        ASSERT(thread_ != nullptr, "Failed to start KLineService thread.");
    }
    common::Delta GetDownTimeInS() { return time_manager_.GetDeltaInS(); }
    auto stop() -> void { run_ = false; }

    /// Deleted default, copy & move constructors and assignment-operators.
    KLineService()                                 = delete;

    KLineService(const KLineService &)             = delete;

    KLineService(const KLineService &&)            = delete;

    KLineService &operator=(const KLineService &)  = delete;

    KLineService &operator=(const KLineService &&) = delete;

  private:
    std::unique_ptr<std::thread> thread_;
    OHLCVGetter *ohlcv_getter_;
    OHLCVILFQueue *internal_kline_lfqueue_          = nullptr;
    OHLCVILFQueue *external_kline_lfqueue_          = nullptr;
    Exchange::EventLFQueue *market_updates_lfqueue_ = nullptr;
    volatile bool run_                              = false;

  private:
    /// Main thread loop - sends out client requests to the exchange and reads
    /// and dispatches incoming client responses.
    auto Run() noexcept -> void;
    common::TimeManager time_manager_;
};

/**
 * @brief class for reading ohlcv history file
 *
 */
class OHLCVI : public OHLCVGetter {
  public:
    explicit OHLCVI(std::string_view path_to_file,
                    Trading::TradeEngine *trade_engine)
        : path_to_file_(path_to_file.data()), trade_engine_(trade_engine) {};
    void Init(OHLCVILFQueue &lf_queue) override;
    bool LaunchOne() override {
        if (lf_queue_ == nullptr) [[unlikely]]
            return false;
        if (iterator_ohlcv_history == ohlcv_history_.end()) [[unlikely]] {
            logw("iterator_ohlcv_history = ohlcv_history_.end()");
            if (trade_engine_) [[likely]]
                trade_engine_->Stop();
            return false;
        }
        auto status = lf_queue_->try_enqueue(*iterator_ohlcv_history);
        if (status) [[likely]] {
            ++iterator_ohlcv_history;
        } else {
            logw("can't push data to queue. probably queue is full");
        }
        return true;
    };
    void ResetIterator() { iterator_ohlcv_history = ohlcv_history_.begin(); }

  private:
    const std::string path_to_file_;
    Trading::TradeEngine *trade_engine_ = nullptr;
    std::list<OHLCVExt> ohlcv_history_;
    OHLCVILFQueue *lf_queue_ = nullptr;
    std::list<OHLCVExt>::iterator iterator_ohlcv_history =
        ohlcv_history_.begin();
};
}  // namespace backtesting