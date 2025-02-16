#pragma once

#include <atomic>

#include "aot/Logger.h"
#include "aot/Types.h"
#include "aot/event/general_event.h"
#include "boost/asio/awaitable.hpp"
#include <boost/intrusive_ptr.hpp>

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

template < typename BusPool>
class Event2 {
  public:
    Event2(BusPool* memory_pool)
        : memory_pool_(memory_pool) {
    }

    virtual ~Event2() {
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
    BusPool* memory_pool_;             // Pointer to the memory pool
    std::atomic<int> ref_count_{0};  // Reference count for BusEvent
};
};  // namespace bus