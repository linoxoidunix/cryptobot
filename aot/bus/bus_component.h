#pragma once

#include <boost/intrusive_ptr.hpp>

#include "aot/Types.h"

//universal response event
struct BusEventResponse;
struct BusEventRequestBBOPrice;

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
  struct BusEventRequestNewSnapshot;
  struct BusEventResponseNewSnapshot;
  struct BusEventRequestDiffOrderBook;
  struct BusEventBookDiffSnapshot;
  struct BusEventMEMarketUpdate2;
}

namespace Trading{
    struct NewBBO;
    struct BusEventNewBBO;
}

namespace bus{
class Component {
  public:
    virtual ~Component() = default;
    virtual void AsyncStop(){
        //it is empty class
    };
    virtual std::string_view GetName() const { return {};
    }
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
    /**
     * @brief
     * 
     */
    virtual void AsyncHandleEvent(Exchange::BusEventRequestNewSnapshot*){
        //it is empty class
    };
    /**
     * @brief 
     * 
     */
    // virtual void AsyncHandleEvent(Exchange::BusEventResponseNewSnapshot*){
    //     logi("invoke AsyncHandleEvent with BusEventResponseNewSnapshot arg");
    //     //it is empty class
    // };
    virtual void AsyncHandleEvent(boost::intrusive_ptr<Exchange::BusEventResponseNewSnapshot>){
        logi("invoke AsyncHandleEvent with boost::intrusive_ptr<Exchange::BusEventResponseNewSnapshot> arg");
        //it is empty class
    };
    /**
     * @brief
     * 
     */
    virtual void AsyncHandleEvent(boost::intrusive_ptr<Exchange::BusEventRequestDiffOrderBook>){
        //it is empty class
    };
    virtual void AsyncHandleEvent(boost::intrusive_ptr<Exchange::BusEventBookDiffSnapshot>){
        //it is empty class
    };
    /**
     * @brief
     * 
     */
    virtual void AsyncHandleEvent(boost::intrusive_ptr<BusEventRequestBBOPrice>){
        //it is empty class
    };
    /**
     * @brief this event when order book need process diff from exchange order book 
     * 
     */
    virtual void AsyncHandleEvent(boost::intrusive_ptr<Exchange::BusEventMEMarketUpdate2>){
        //it is empty class
    };
    /**
     * @brief this event when new bbo occure 
     * 
     */
    virtual void AsyncHandleEvent(boost::intrusive_ptr<Trading::BusEventNewBBO>){
        //it is empty class
    };
    
};
};