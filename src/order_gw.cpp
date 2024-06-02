#include "aot/order_gw/order_gw.h"

#include "aot/Exchange.h"
#include "aot/client_request.h"


auto Trading::OrderGateway::Run() noexcept -> void {
    while (run_) {
        Exchange::RequestNewOrder results_new_orders[50];  // Could also be any iterator

        size_t count_new_order = requests_new_order_->try_dequeue_bulk(results_new_orders, 50);
        for (int i = 0; i < count_new_order; i++) {
            new_order_->Exec(&results_new_orders[i], incoming_responses_);
        }
        Exchange::RequestCancelOrder results_cancel_orders[50];  // Could also be any iterator

        size_t count_cancel_order = requests_cancel_order_->try_dequeue_bulk(results_cancel_orders, 50);
        for (int i = 0; i < count_cancel_order; i++) {
        }
    }
}
Trading::OrderGateway::OrderGateway(
    inner::OrderNewI *new_order,
    Exchange::RequestNewLimitOrderLFQueue *requests_new_order,
    Exchange::RequestCancelOrderLFQueue *requests_cancel_order,
    Exchange::ClientResponseLFQueue *client_responses)
    : new_order_(new_order),
      requests_new_order_(requests_new_order),
      requests_cancel_order_(requests_cancel_order),
      incoming_responses_(client_responses) {}
