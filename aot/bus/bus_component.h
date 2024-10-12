#pragma once

#include "aot/Types.h"

//universal response event
struct BusEventResponse;

namespace position_keeper{
  //struct BusEventAddFill;
  struct BusEventUpdateBBO;
}

namespace order_manager{
  /**need define SIDE variable. default RequestCancelOrder not suited for this */
  struct BusEventRequestCancelOrder;
}

namespace wallet{
  struct BusEventReserveQty;
}

namespace Exchange{
  struct BusEventRequestNewLimitOrder;
  struct BusEventRequestCancelOrder;
  struct BusEventResponse;
}

namespace bus{
class Component {
  public:
    virtual ~Component() = default;
    /**
     * @brief update position keeper when OrderBook emit new signal updateBBO
     * 
     */
    virtual void AsyncHandleEvent(position_keeper::BusEventUpdateBBO*){
        //it is empty class
    };
    /**
     * @brief order manager process ability to create new limit order
     * 
     */
    virtual void AsyncHandleEvent(Exchange::BusEventRequestNewLimitOrder*){
        //it is empty class
    };
    /**
     * @brief order manager process ability to cancel order
     * 
     */
    virtual void AsyncHandleEvent(order_manager::BusEventRequestCancelOrder*){
        //it is empty class
    };
    /**
     * @brief wallet reserve qty of ticker
     * 
     */
    virtual void AsyncHandleEvent(wallet::BusEventReserveQty*){
        //it is empty class
    };
    /**
     * @brief wallet reserve qty of ticker
     * 
     */
    virtual void AsyncHandleEvent(Exchange::BusEventResponse*){
        //it is empty class
    };
    /**
     * @brief if order executor need send new limit order
     * 
     */
    virtual void AsyncHandleEvent(Exchange::BusEventRequestNewLimitOrder*, const OnHttpsResponce& cb){
        //it is empty class
    };
      /**
     * @brief if order executor need send cancel order
     * 
     */
    virtual void AsyncHandleEvent(Exchange::BusEventRequestCancelOrder*, const OnHttpsResponce& cb){
        //it is empty class
    };
};
};