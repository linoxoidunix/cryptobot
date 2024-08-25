#pragma once

#include <functional>

#include "aot/client_request.h"
#include "aot/client_response.h"
#include "aot/common/macros.h"
#include "aot/common/thread_utils.h"
#include "aot/common/time_utils.h"

namespace inner {
class OrderNewI;
class CancelOrderI;
};  // namespace inner

namespace Trading {
class OrderGateway {
  public:
    OrderGateway(inner::OrderNewI *executor_new_orders,
                 inner::CancelOrderI *executor_canceled_orders,
                 Exchange::RequestNewLimitOrderLFQueue *requests_new_order,
                 Exchange::RequestCancelOrderLFQueue *requests_cancel_order,
                 Exchange::ClientResponseLFQueue *client_responses);

    ~OrderGateway() {
        Stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
        if (thread_) [[likely]]
            thread_->join();
    }

    /// Start and stop the order gateway main thread.
    auto Start() {
        run_    = true;
        thread_ = std::unique_ptr<std::thread>(common::createAndStartThread(
            -1, "Trading/OrderGateway", [this]() { Run(); }));
        ASSERT(thread_ != nullptr, "Failed to start OrderGateway thread.");
    }
    common::Delta GetDownTimeInS() { return time_manager_.GetDeltaInS(); }
    auto Stop() -> void { run_ = false; }

    /// Deleted default, copy & move constructors and assignment-operators.
    OrderGateway()                                 = delete;

    OrderGateway(const OrderGateway &)             = delete;

    OrderGateway(const OrderGateway &&)            = delete;

    OrderGateway &operator=(const OrderGateway &)  = delete;

    OrderGateway &operator=(const OrderGateway &&) = delete;

  private:
    inner::OrderNewI *executor_new_orders_;
    inner::CancelOrderI *executor_canceled_orders_;

    /// Lock free queue on which we consume client requests from the trade
    /// engine and forward them to the exchange's order server.
    Exchange::RequestNewLimitOrderLFQueue *requests_new_order_  = nullptr;
    Exchange::RequestCancelOrderLFQueue *requests_cancel_order_ = nullptr;
    Exchange::ClientResponseLFQueue *incoming_responses_        = nullptr;

    volatile bool run_                                          = false;

  private:
    std::unique_ptr<std::thread> thread_;
    /// Main thread loop - sends out client requests to the exchange and reads
    /// and dispatches incoming client responses.
    auto Run() noexcept -> void;
    common::TimeManager time_manager_;

    /// Callback when an incoming client response is read, we perform some
    /// checks and forward it to the lock free queue connected to the trade
    /// engine.
    auto RecvCallback() noexcept -> void {};
};
}  // namespace Trading

namespace backtesting {
class OrderGateway {
  public:
    OrderGateway(

        Exchange::RequestNewLimitOrderLFQueue *requests_new_order,
        Exchange::RequestCancelOrderLFQueue *requests_cancel_order,
        Exchange::ClientResponseLFQueue *client_responses);

    ~OrderGateway() {
        Stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
        if (thread_) [[likely]]
            thread_->join();
    }

    /// Start and stop the order gateway main thread.
    auto Start() {
        run_    = true;
        thread_ = std::unique_ptr<std::thread>(common::createAndStartThread(
            -1, "Trading/OrderGateway", [this]() { Run(); }));
        ASSERT(thread_ != nullptr, "Failed to start OrderGateway thread.");
    }
    common::Delta GetDownTimeInS() { return time_manager_.GetDeltaInS(); }
    auto Stop() -> void { run_ = false; }

    /// Deleted default, copy & move constructors and assignment-operators.
    OrderGateway()                                 = delete;

    OrderGateway(const OrderGateway &)             = delete;

    OrderGateway(const OrderGateway &&)            = delete;

    OrderGateway &operator=(const OrderGateway &)  = delete;

    OrderGateway &operator=(const OrderGateway &&) = delete;

  private:
    /// Lock free queue on which we consume client requests from the trade
    /// engine and forward them to the exchange's order server.
    Exchange::RequestNewLimitOrderLFQueue *requests_new_order_  = nullptr;
    Exchange::RequestCancelOrderLFQueue *requests_cancel_order_ = nullptr;
    Exchange::ClientResponseLFQueue *incoming_responses_        = nullptr;

    volatile bool run_                                          = false;

  private:
    std::unique_ptr<std::thread> thread_;
    /// Main thread loop - sends out client requests to the exchange and reads
    /// and dispatches incoming client responses.
    auto Run() noexcept -> void;
    common::TimeManager time_manager_;

    /// Callback when an incoming client response is read, we perform some
    /// checks and forward it to the lock free queue connected to the trade
    /// engine.
    auto RecvCallback() noexcept -> void {};
};
}  // namespace backtesting