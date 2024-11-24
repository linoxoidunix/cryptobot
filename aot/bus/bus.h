#pragma once 

#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

#include "boost/asio.hpp"
#include "boost/asio/strand.hpp"
#include "boost/asio/awaitable.hpp"
#include "boost/thread.hpp"
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/adaptor/map.hpp>

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
    /**
     * @brief 
     * 
     * @tparam T must be boost::intrusive_ptr
     * @param publisher 
     * @param event 
     */
    template <class T>
    void AsyncSend(bus::Component* publisher, T event) {
        if (!event) {
            loge("event is nullptr");
            return;
        }

        logd("Start sending event from {}", static_cast<void*>(publisher));

        auto it = subscribers_.find(publisher);
        if (it != subscribers_.end()) {
            logd("Found {} subscribers", it->second.size());

            // Использование boost::range для обхода подписчиков
            boost::range::for_each(it->second, [this, event](bus::Component* component) {
                boost::asio::post(strand_, [component, event]() {
                    event->Accept(component);
                });
            });
        } else {
            logd("No subscribers found for publisher {}", static_cast<void*>(publisher));
        }
    }

    void Join() { pool_.join(); }
    virtual ~Bus(){logd("dtor Bus");}
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

    /**
     * @brief 
     * 
     * @tparam T must be boost::intrusive_ptr
     * @param publisher 
     * @param event 
     */
    template <class T>
    void AsyncSend(bus::Component* publisher, T event) {
        if (!event) {
            loge("event is nullptr");
            return;
        }

        logd("Start sending event from {}", static_cast<void*>(publisher));

        auto it = subscribers_.find(publisher);
        if (it != subscribers_.end()) {
            logd("Found {} subscribers", it->second.size());

            //visit all subscribers
            boost::range::for_each(it->second, [this, event](bus::Component* component) {
                boost::asio::post(strand_, [component, event]() {
                    event->Accept(component);
                });
            });
        } else {
            logd("No subscribers found for publisher {}", static_cast<void*>(publisher));
        }
    }

    template <class T>
    boost::asio::awaitable<void> CoSend(bus::Component* publisher, T event) {
        logi("suspend coroutine");
        co_await boost::asio::post(strand_, boost::asio::use_awaitable);
        logi("resume coroutine");
        AsyncSend(publisher, event);
        co_return;
    }   
    void Join() { pool_.join(); }
    virtual ~CoBus(){logd("dtor Bus");}
};


};  // namespace aot