/**
 * @file client_request.h
 * @author your name (you@domain.com)
 * @brief define struct for sending and receiving gateway module
 * @version 0.1
 * @date 2024-06-01
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <sstream>

#include "aot/bus/bus_event.h"
#include "aot/bus/bus_component.h"

#include "aot/common/types.h"
#include "aot/Logger.h"
#include "aot/common/mem_pool.h"
// #include "moodycamel/concurrentqueue.h"//if link as 3rd party
#include "concurrentqueue.h"  //if link form source


namespace bus{
    class BusComponent;
}
namespace Exchange {
/// Type of the order request sent by the trading client to the exchange.
enum class ClientRequestType : uint8_t { INVALID = 0, NEW = 1, CANCEL = 2 };

inline std::string ClientRequestTypeToString(ClientRequestType type) {
    switch (type) {
        case ClientRequestType::NEW:
            return "NEW";
        case ClientRequestType::CANCEL:
            return "CANCEL";
        case ClientRequestType::INVALID:
            return "INVALID";
    }
    return "UNKNOWN";
}

class Request {
  public:
    ClientRequestType type = ClientRequestType::INVALID;
    auto ToString() const {
        return fmt::format("Request[type:{}]", ClientRequestTypeToString(type));
    }
};

class RequestNewOrder;
using RequestNewLimitOrderPool = common::MemPool<RequestNewOrder>;

class RequestNewOrder {
  public:
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    ClientRequestType type = ClientRequestType::NEW;
    common::TradingPair trading_pair;
    common::OrderId order_id = common::kOrderIdInvalid;
    common::Side side        = common::Side::kInvalid;
    common::Price price      = common::kPriceInvalid;
    common::Qty qty          = common::kQtyInvalid;
    RequestNewLimitOrderPool* mem_pool = nullptr;
    auto ToString() const {
        std::string price_string = (price != common::kPriceInvalid)
                                       ? fmt::format("{}", price)
                                       : "INVALID";
        std::string qty_string   = (qty != common::kQtyInvalid)
                                       ? fmt::format("{}", qty)
                                       : "INVALID";
        return fmt::format(
            "RequestNewOrder[type:{} {} order_id:{} side:{} qty:{} price:{}]",
            ClientRequestTypeToString(type), trading_pair.ToString(), order_id,
            common::sideToString(side), qty_string, price_string);
    }
    void Deallocate() {
        if(!mem_pool)
        {
            logw("mem_pool = nullptr. can.t deallocate RequestNewOrder");
                return;
        }
        mem_pool->deallocate(this);
    };
    explicit RequestNewOrder(common::ExchangeId _exchange_id,
     ClientRequestType _type,
      common::TradingPair _trading_pair,
      common::OrderId _order_id,
      common::Side _side,
      common::Price _price,
      common::Qty _qty) : 
      exchange_id(_exchange_id),
       type(_type),
        trading_pair(_trading_pair),
         order_id(_order_id), 
         side(_side),
         price(_price),
         qty(_qty){};
    RequestNewOrder() = default;
    virtual ~RequestNewOrder() = default;
};

struct BusEventRequestNewLimitOrder : public bus::Event, public RequestNewOrder{
    explicit BusEventRequestNewLimitOrder(RequestNewOrder* new_request) : request(new_request){}
    ~BusEventRequestNewLimitOrder() override = default;
    RequestNewOrder* request;
    void Accept(bus::Component* comp, const OnHttpsResponce& cb) override{
        comp->AsyncHandleEvent(this, cb);
    };
};

class RequestCancelOrder {
  public:
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    ClientRequestType type = ClientRequestType::CANCEL;
    common::TradingPair trading_pair;
    common::OrderId order_id = common::kOrderIdInvalid;
    virtual ~RequestCancelOrder() = default;
    auto ToString() const {
        return fmt::format("RequestCancelOrder[type:{} {} id:{}]",
                           ClientRequestTypeToString(type), trading_pair.ToString(), order_id);
    }
    RequestCancelOrder() = default;
    RequestCancelOrder(common::ExchangeId _exchange_id,
                          ClientRequestType _type,
                          common::TradingPair _trading_pair,
                          common::OrderId _order_id) : exchange_id(_exchange_id), 
                          type(_type),
                          trading_pair(_trading_pair),
                          order_id(_order_id){}

};

struct BusEventRequestCancelOrder : public bus::Event, public RequestCancelOrder{
    explicit BusEventRequestCancelOrder(RequestCancelOrder* new_request) : request(new_request){}
    ~BusEventRequestCancelOrder() override = default;
    RequestCancelOrder* request;
    void Accept(bus::Component* comp, const OnHttpsResponce& cb) override{
        comp->AsyncHandleEvent(this, cb);
    };
};

/// Lock free queues of matching engine client order request messages.
using RequestNewLimitOrderLFQueue =
    moodycamel::ConcurrentQueue<RequestNewOrder>;
using RequestCancelOrderLFQueue =
    moodycamel::ConcurrentQueue<RequestCancelOrder>;

}  // namespace Exchange

