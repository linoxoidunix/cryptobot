#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "boost/asio.hpp"
#include "boost/asio/awaitable.hpp"
#include "boost/asio/strand.hpp"
#include "boost/thread.hpp"
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/transform.hpp>

#include "aot/Logger.h"
#include "aot/bus/bus_component.h"
#include "aot/bus/bus_event.h"


// namespace bus{
//     class Component;
//     //class Event;
//     //class Event2;
// };

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
            boost::range::for_each(
                it->second, [this, event](bus::Component* component) {
                    boost::asio::post(strand_, [component, event]() {
                        event->Accept(component);
                    });
                });
        } else {
            logd("No subscribers found for publisher {}",
                 static_cast<void*>(publisher));
        }
    }

    void Join() { pool_.join(); }
    virtual ~Bus() { logd("dtor Bus"); }
};

// class CoBus : public Bus {
//     public:
//     using Bus::Bus;

//     boost::asio::awaitable<void> CoSend(bus::Component* publisher,
//     bus::Event* event) {
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
    // template <class T>
    // void AsyncSend(bus::Component* publisher, T event) {
    //     if (!event) {
    //         loge("event is nullptr");
    //         return;
    //     }

    //     logd("Start sending event from {}", static_cast<void*>(publisher));

    //     auto it = subscribers_.find(publisher);
    //     if (it != subscribers_.end()) {
    //         logd("Found {} subscribers", it->second.size());

    //         //visit all subscribers
    //         boost::range::for_each(it->second, [this, event](bus::Component*
    //         component) {
    //             boost::asio::post(strand_, [component, event]() {
    //                 event->Accept(component);
    //             });
    //         });
    //     } else {
    //         logd("No subscribers found for publisher {}",
    //         static_cast<void*>(publisher));
    //     }
    // }
    template <class T>
    void AsyncSend(bus::Component* publisher, T event) {
        if (!event) {
            loge("event is nullptr");
            return;
        }

        // logd("Start sending event from name:{} with addr:{}",
        // publisher->GetName(), static_cast<void*>(publisher));

        auto it = subscribers_.find(publisher);
        if (it != subscribers_.end()) {
            // logd("Found {} subscribers", it->second.size());

            // Visit all subscribers and directly handle the event
            for (auto* component : it->second) {
                if (component) {
                    try {
                        event->Accept(component);
                    } catch (const std::exception& ex) {
                        loge("Exception while handling event: {}", ex.what());
                    } catch (...) {
                        loge("Unknown error occurred while handling event.");
                    }
                } else {
                    logw("Subscriber is nullptr, skipping.");
                }
            }
        } else {
            logd("No subscribers found for publisher {}",
                 static_cast<void*>(publisher));
        }
    }
    // template <class T>
    // void AsyncSend(bus::Component* publisher, T event) {
    //     if (!event) {
    //         loge("event is nullptr");
    //         return;
    //     }

    //     logd("Start sending event from name:{} with addr:{}",
    //          publisher->GetName(), static_cast<void*>(publisher));

    //     auto it = subscribers_.find(publisher);
    //     if (it != subscribers_.end()) {
    //         logd("Found {} subscribers", it->second.size());

    //         // Post to all subscribers simultaneously
    //         for (bus::Component* component : it->second) {
    //             if (component) {
    //                 boost::asio::post(pool_, [component, event]() {
    //                     try {
    //                         event->Accept(component);
    //                     } catch (const std::exception& ex) {
    //                         loge("Exception while handling event: {}",
    //                              ex.what());
    //                     } catch (...) {
    //                         loge(
    //                             "Unknown error occurred while handling
    //                             event.");
    //                     }
    //                 });
    //             } else {
    //                 logw("Subscriber is nullptr, skipping.");
    //             }
    //         }
    //     } else {
    //         logd("No subscribers found for publisher {}",
    //              static_cast<void*>(publisher));
    //     }
    // }

    template <class T>
    boost::asio::awaitable<void> CoSend(bus::Component* publisher, T event) {
        logi("suspend coroutine");
        co_await boost::asio::post(strand_, boost::asio::use_awaitable);
        logi("resume coroutine");
        AsyncSend(publisher, event);
        co_return;
    }

    void StopAllSubscribers() {
        std::unordered_set<bus::Component*> processed_subscribers;

        for (auto& [publisher, subscribers] : subscribers_) {
            for (auto* subscriber : subscribers) {
                if (subscriber && processed_subscribers.find(subscriber) ==
                                      processed_subscribers.end()) {
                    try {
                        subscriber->AsyncStop();
                        processed_subscribers.insert(subscriber);
                    } catch (const std::exception& ex) {
                        loge("Exception while stopping subscriber: {}",
                             ex.what());
                    } catch (...) {
                        loge(
                            "Unknown error occurred while stopping "
                            "subscriber.");
                    }
                } else if (!subscriber) {
                    logw("Subscriber is nullptr, skipping.");
                }
            }
        }
    }

    void StopSubscribersForPublisher(bus::Component* publisher) {
        if (!publisher) {
            logw("Publisher is nullptr, skipping.");
            return;
        }

        auto it = subscribers_.find(publisher);
        if (it != subscribers_.end()) {
            std::unordered_set<bus::Component*> processed_subscribers;

            for (auto* subscriber : it->second) {
                if (subscriber && processed_subscribers.find(subscriber) ==
                                      processed_subscribers.end()) {
                    try {
                        subscriber->AsyncStop();
                        processed_subscribers.insert(subscriber);
                    } catch (const std::exception& ex) {
                        loge("Exception while stopping subscriber: {}",
                             ex.what());
                    } catch (...) {
                        loge(
                            "Unknown error occurred while stopping "
                            "subscriber.");
                    }
                } else if (!subscriber) {
                    logw("Subscriber is nullptr, skipping.");
                }
            }
        } else {
            logd("No subscribers found for publisher {}",
                 static_cast<void*>(publisher));
        }
    }

    void Join() { pool_.join(); }
    virtual ~CoBus() { logd("dtor Bus"); }
};

};  // namespace aot