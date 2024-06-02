#pragma once

#include <functional>

#include "aot/common/macros.h"
#include "aot/common/thread_utils.h"
#include "aot/client_request.h"
#include "aot/client_response.h"


namespace inner {
class OrderNewI;
};

// namespace Exchange {
// class RequestNewLimitOrderLFQueue;
// class RequestCancelOrderLFQueue;
// class ClientResponseLFQueue;
// };  // namespace Exchange

namespace Trading {
class OrderGateway {
  public:
    OrderGateway(inner::OrderNewI *new_order,
                 Exchange::RequestNewLimitOrderLFQueue *requests_new_order,
                 Exchange::RequestCancelOrderLFQueue *requests_cancel_order,
                 Exchange::ClientResponseLFQueue *client_responses);

    ~OrderGateway() {
        stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }

    /// Start and stop the order gateway main thread.
    auto start() {
        run_ = true;
        ASSERT(common::createAndStartThread(-1, "Trading/OrderGateway",
                                            [this]() { Run(); }) != nullptr,
               "Failed to start OrderGateway thread.");
    }

    auto stop() -> void { run_ = false; }

    /// Deleted default, copy & move constructors and assignment-operators.
    OrderGateway()                                 = delete;

    OrderGateway(const OrderGateway &)             = delete;

    OrderGateway(const OrderGateway &&)            = delete;

    OrderGateway &operator=(const OrderGateway &)  = delete;

    OrderGateway &operator=(const OrderGateway &&) = delete;

  private:
    inner::OrderNewI *new_order_;

    /// Lock free queue on which we consume client requests from the trade
    /// engine and forward them to the exchange's order server.
    Exchange::RequestNewLimitOrderLFQueue *requests_new_order_  = nullptr;
    Exchange::RequestCancelOrderLFQueue *requests_cancel_order_ = nullptr;
    Exchange::ClientResponseLFQueue *incoming_responses_        = nullptr;

    volatile bool run_                                          = false;

  private:
    /// Main thread loop - sends out client requests to the exchange and reads
    /// and dispatches incoming client responses.
    auto Run() noexcept -> void;

    /// Callback when an incoming client response is read, we perform some
    /// checks and forward it to the lock free queue connected to the trade
    /// engine.
    auto RecvCallback() noexcept -> void {};
};
}  // namespace Trading