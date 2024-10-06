#pragma once

namespace position_keeper{
  struct BusEventAddFill;
  struct BusEventUpdateBBO;
}

namespace order_manager{
  struct BusEventRequestNewLimitOrder;
  struct BusEventRequestCancelOrder;
  struct BusEventResponse;
}

namespace bus{
class Component {
  public:
    virtual ~Component() = default;
    /**
     * @brief update position keeper when new exchange response incoming
     * 
     */
    virtual void AsyncHandleEvent(position_keeper::BusEventAddFill*) {
      //it is empty class
    };
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
    virtual void AsyncHandleEvent(order_manager::BusEventRequestNewLimitOrder*){
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
     * @brief order manager process response from exchange or other
     * 
     */
    virtual void AsyncHandleEvent(order_manager::BusEventResponse*){
        //it is empty class
    };
};
};