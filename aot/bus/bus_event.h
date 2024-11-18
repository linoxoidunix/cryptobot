#pragma once

#include <atomic>

#include "aot/Logger.h"
#include "aot/Types.h"
#include "aot/event/general_event.h"
#include "boost/asio/awaitable.hpp"

namespace bus {
class Component;
}

namespace bus {
class Event {
  public:
    Event()          = default;  // Initialize reference count

    virtual ~Event() = default;

    // Increment the reference count
    void AddReference() { ref_count_.fetch_add(1, std::memory_order_relaxed); }

    // Decrement the reference count and deallocate if it reaches zero
    void Release() {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            Deallocate();
        }
    }

    virtual void Accept(bus::Component*) {};
    virtual void Accept(bus::Component*, const OnHttpsResponce& cb) {};
    virtual void Accept(bus::Component*, const OnHttpsResponce* cb) {};
    virtual void Accept(bus::Component*, const OnWssResponse* cb) {};

    virtual boost::asio::awaitable<void> CoAccept(bus::Component* component) {
        logd("Accept component");
        co_await boost::asio::this_coro::executor;
        Accept(component);
        co_return;
    };

  protected:
    // Method to deallocate resources
    virtual void Deallocate() {
        logd("Deallocating resources");
        // delete this;  // Deletes the event object
    }

  private:
    std::atomic<int> ref_count_{0};  // Atomic counter for reference counting
};

template < typename BusPool, typename Pool>
class Event2 {
  public:
    Event2(BusPool* memory_pool, aot::Event<Pool>* event)
        : wrapped_event_(event), memory_pool_(memory_pool) {
        // if (wrapped_event_) {
        //     wrapped_event_
        //         ->AddReference();  // Increment reference count for Event
        // }
    }

    virtual ~Event2() {
        if (wrapped_event_) {
            wrapped_event_->Release();  // Decrement reference count for Event
        }
    }

    void AddReference() {
        ref_count_.fetch_add(1, std::memory_order_relaxed);
        if (wrapped_event_) {
            wrapped_event_
                ->AddReference();  // Increment reference count for Event
        }
    }

    virtual void Release() {
    }

    virtual void Accept(bus::Component*) {};
    virtual void Accept(bus::Component*, const OnHttpsResponce& cb) {};
    virtual void Accept(bus::Component*, const OnHttpsResponce* cb) {};
    virtual void Accept(bus::Component*, const OnWssResponse* cb) {};

    virtual boost::asio::awaitable<void> CoAccept(bus::Component* component) {
        logd("Accept component");
        co_await boost::asio::this_coro::executor;
        Accept(component);
        co_return;
    };

  protected:
    aot::Event<Pool>* wrapped_event_;  // Pointer to wrapped Event
    BusPool* memory_pool_;             // Pointer to the memory pool
    std::atomic<int> ref_count_{0};  // Reference count for BusEvent
};
};  // namespace bus