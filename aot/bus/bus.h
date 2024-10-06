#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

#include "boost/asio.hpp"
#include "boost/asio/strand.hpp"
#include "boost/thread.hpp"

namespace bus{
    class Component;
    class Event;
};

namespace aot {

class Bus {
    using Subscribers   = std::vector<bus::Component*>;
    using ComponentsMap = std::unordered_map<bus::Component*, Subscribers>;

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

  private:
    void SetNumberCopyEvent(bus::Event* event, size_t number);
};
};  // namespace aot