#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

#include "boost/asio.hpp"
#include "boost/asio/strand.hpp"
#include "boost/asio/awaitable.hpp"
#include "boost/thread.hpp"

#include "aot/Logger.h"

namespace bus{
    class Component;
    class Event;
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

    void AsyncSend(bus::Component* publisher, bus::Event* event);

    void Join() { pool_.join(); }
    virtual ~Bus(){logd("dtor Bus");}
  protected:
    void SetNumberCopyEvent(bus::Event* event, size_t number) const;
};

class CoBus : public Bus {
    public:
    using Bus::Bus;
    
    boost::asio::awaitable<void> CoSend(bus::Component* publisher, bus::Event* event) {
        logi("suspend coroutine");
        co_await boost::asio::post(strand_, boost::asio::use_awaitable);
        logi("resume coroutine");
        AsyncSend(publisher, event);
        co_return;
    }
    
    ~CoBus() override{logd("dtor CoBus");}
};
};  // namespace aot