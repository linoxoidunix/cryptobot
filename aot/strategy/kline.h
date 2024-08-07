#pragma once

#include <functional>

#include "aot/Exchange.h"
#include "aot/client_request.h"
#include "aot/client_response.h"
#include "aot/common/macros.h"
#include "aot/common/thread_utils.h"
#include "aot/common/time_utils.h"

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