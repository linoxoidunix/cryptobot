#pragma once 

#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

#include "boost/asio.hpp"
#include "boost/asio/strand.hpp"
#include "boost/asio/awaitable.hpp"
#include "boost/thread.hpp"

#include "aot/Logger.h"
#include "aot/bus/bus_event.h"

namespace bus{
    class Component;
    //class Event;
    //class Event2;
};

namespace aot {

class Bus {
    using Subscribers   = std::vector<bus::Component*>;
    using ComponentsMap = std::unordered_map<bus::Component*, Subscribers>;


protected:
    ComponentsMap subscribers_;
    boost::asio::thread_pool& pool_;
    boost::asio::strand<boost::asio::thread_pool::executor_type> strand_;

  public:
    explicit Bus(boost::asio::thread_pool& pool)
        : pool_(pool), strand_(boost::asio::make_strand(pool_)) {}

    void Subscribe(bus::Component* publisher, bus::Component* subscriber) {
        subscribers_[publisher].push_back(subscriber);
    }

    template <class T>
    void AsyncSend(bus::Component* publisher, T* event){
        logd("start send order from {}", (void*)publisher);
        auto it = subscribers_.find(publisher);
        if (it != subscribers_.end()) {
            logd("found {} subscribers", it->second.size());
            SetNumberCopyEvent(event, it->second.size());
            for (auto component : it->second) {
                boost::asio::post(
                    strand_, [component, event]() { 
                        event->Accept(component); 
                        });
            }
        }
    }

    void Join() { pool_.join(); }
    virtual ~Bus(){logd("dtor Bus");}
  protected:
    template <class T>
    void SetNumberCopyEvent(T* event, size_t number) const{
    for (auto i = 0; i < number; i++) {
            event->AddReference();
        }
    }
};

// class CoBus : public Bus {
//     public:
//     using Bus::Bus;
    
//     boost::asio::awaitable<void> CoSend(bus::Component* publisher, bus::Event* event) {
//         logi("suspend coroutine");
//         co_await boost::asio::post(strand_, boost::asio::use_awaitable);
//         logi("resume coroutine");
//         AsyncSend(publisher, event);
//         co_return;
//     }   
//     ~CoBus() override{logd("dtor CoBus");}
// };

class CoBus {
    using Subscribers   = std::vector<bus::Component*>;
    using ComponentsMap = std::unordered_map<bus::Component*, Subscribers>;


protected:
    ComponentsMap subscribers_;
    boost::asio::thread_pool& pool_;
    boost::asio::strand<boost::asio::thread_pool::executor_type> strand_;

  public:
    explicit CoBus(boost::asio::thread_pool& pool)
        : pool_(pool), strand_(boost::asio::make_strand(pool_)) {}

    void Subscribe(bus::Component* publisher, bus::Component* subscriber) {
        subscribers_[publisher].push_back(subscriber);
    }

    template <class T>
    void AsyncSend(bus::Component* publisher, T* event){
        logd("start send order from {}", (void*)publisher);
        auto it = subscribers_.find(publisher);
        if (it != subscribers_.end()) {
            logd("found {} subscribers", it->second.size());
            SetNumberCopyEvent(event, it->second.size());
            for (auto component : it->second) {
                boost::asio::post(
                    strand_, [component, event]() { 
                        event->Accept(component); 
                        });
            }
        }
    }
    boost::asio::awaitable<void> CoSend(bus::Component* publisher, bus::Event* event) {
            logi("suspend coroutine");
            co_await boost::asio::post(strand_, boost::asio::use_awaitable);
            logi("resume coroutine");
            AsyncSend(publisher, event);
            co_return;
        }   
    void Join() { pool_.join(); }
    virtual ~CoBus(){logd("dtor Bus");}
  protected:
    template <class T>
    void SetNumberCopyEvent(T* event, size_t number) const{
    for (auto i = 0; i < number; i++) {
            event->AddReference();
        }
    }
};


};  // namespace aot