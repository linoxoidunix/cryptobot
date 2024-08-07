#include "aot/order_gw/order_gw.h"

#include "aot/Exchange.h"
#include "aot/client_request.h"

auto Trading::OrderGateway::Run() noexcept -> void {
    Exchange::RequestNewOrder
            results_new_orders[50];  // Could also be any iterator
    Exchange::RequestCancelOrder
            requests_cancel_orders[50];  // Could also be any iterator

    while (run_) {
        

        size_t count_new_order =
            requests_new_order_->try_dequeue_bulk(results_new_orders, 50);
        for (int i = 0; i < count_new_order; i++) {
            logd("order gw start exec {}", results_new_orders[i].ToString());
            executor_new_orders_->Exec(&results_new_orders[i],
                                       incoming_responses_);
            time_manager_.Update();
        }
        
        size_t count_cancel_order =
            requests_cancel_order_->try_dequeue_bulk(requests_cancel_orders, 50);
        for (int i = 0; i < count_cancel_order; i++) {
            logd("order gw start exec {}",
                 requests_cancel_orders[i].ToString());
            executor_canceled_orders_->Exec(&requests_cancel_orders[i],
                                            incoming_responses_);
            time_manager_.Update();
        }
    }
}
Trading::OrderGateway::OrderGateway(
    inner::OrderNewI *executor_new_orders,
    inner::CancelOrderI *executor_cancel_orders,
    Exchange::RequestNewLimitOrderLFQueue *requests_new_order,
    Exchange::RequestCancelOrderLFQueue *requests_cancel_order,
    Exchange::ClientResponseLFQueue *client_responses)
    : executor_new_orders_(executor_new_orders),
      executor_canceled_orders_(executor_cancel_orders),
      requests_new_order_(requests_new_order),
      requests_cancel_order_(requests_cancel_order),
      incoming_responses_(client_responses) {}
