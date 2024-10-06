#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <memory>
#include <string_view>
#include <thread>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace sp {
    namespace bus{
    class Component;
    };


class Event {
  public:
    Event() : ref_count_(0) {}  // Initialize reference count

    virtual ~Event() = default;

    // Increment the reference count
    void AddReference() { ref_count_.fetch_add(1, std::memory_order_relaxed); }

    // Decrement the reference count and deallocate if it reaches zero
    void Release() {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            Deallocate();
        }
    }

    virtual void Accept(bus::Component*) = 0;


  protected:
    // Method to deallocate resources
    virtual void Deallocate() {
        std::cout << "Deallocating resources\n";
        delete this;  // Deletes the event object
    }

  private:
    std::atomic<int> ref_count_;  // Atomic counter for reference counting
};

class ImplementationEventA : public Event {
  public:
    ImplementationEventA() = default;
    std::string GetName() const { return "ImplementationEventA"; }

    void Accept(sp::bus::Component* comp) override;
    ~ImplementationEventA() override = default; 
};

class ImplementationEventB : public Event {
  public:
    ImplementationEventB() = default;

    std::string GetName() const { return "ImplementationEventB"; }

    void Accept(sp::bus::Component* comp) override;
    ~ImplementationEventB() override = default; 

};

class ImplementationEventC : public Event {
  public:
    ImplementationEventC() = default;
    std::string GetName() const { return "ImplementationEventC"; }

    void Accept(sp::bus::Component* comp) override;
    ~ImplementationEventC() override = default; 

};



namespace bus {

enum class Type { A, B, C, UNKNOWN };

class Component {
  public:
    virtual ~Component() = default;

    virtual void HandleEvent(ImplementationEventA* event) = 0;
    virtual void HandleEvent(ImplementationEventB* event) = 0;
    virtual void HandleEvent(ImplementationEventC* event) = 0;
};

class MyComponentA : public Component {
    boost::asio::thread_pool& pool_;
    boost::asio::strand<boost::asio::thread_pool::executor_type> strand_;

  public:
    explicit MyComponentA(boost::asio::thread_pool& pool)
        : pool_(pool), strand_(boost::asio::make_strand(pool_)) {}

   void HandleEvent(ImplementationEventA* event) override {
        std::cout << "MyComponentA handling ImplementationEventA\n";
        event->Release();
    }

    void HandleEvent(ImplementationEventB* event) override {
        std::cout << "MyComponentA handling ImplementationEventB\n";
        event->Release();
    }

    void HandleEvent(ImplementationEventC* event) override {
        std::cout << "MyComponentA handling ImplementationEventC\n";
        event->Release();
    }
    ~MyComponentA() override = default;
};

class MyComponentB : public Component {
    boost::asio::thread_pool& pool_;
    boost::asio::strand<boost::asio::thread_pool::executor_type> strand_;

  public:
    MyComponentB(boost::asio::thread_pool& pool)
        : pool_(pool), strand_(boost::asio::make_strand(pool_)) {}

    void HandleEvent(ImplementationEventA* event) override {
        std::cout << "MyComponentB handling ImplementationEventA\n";
        event->Release();
    }

    void HandleEvent(ImplementationEventB* event) override {
        std::cout << "MyComponentB handling ImplementationEventB\n";
        event->Release();
    }

    void HandleEvent(ImplementationEventC* event) override {
        std::cout << "MyComponentB handling ImplementationEventC\n";
        event->Release();
    }
        ~MyComponentB() override = default;

};

class MyComponentC : public Component {
    boost::asio::thread_pool& pool_;
    boost::asio::strand<boost::asio::thread_pool::executor_type> strand_;

  public:
    MyComponentC(boost::asio::thread_pool& pool)
        : pool_(pool), strand_(boost::asio::make_strand(pool_)) {}

    void HandleEvent(ImplementationEventA* event) override {
        std::cout << "MyComponentC handling ImplementationEventA\n";
        event->Release();
    }

    void HandleEvent(ImplementationEventB* event) override {
        std::cout << "MyComponentC handling ImplementationEventB\n";
        event->Release();
    }

    void HandleEvent(ImplementationEventC* event) override {
        std::cout << "MyComponentC handling ImplementationEventC\n";
        event->Release();
    }
        ~MyComponentC() override = default;
};

class Bus {
    using Subscribers = std::vector<Component*>;
    using ComponentsMap = std::unordered_map<Component*, Subscribers>;

    ComponentsMap subscribers_;
    boost::asio::thread_pool& pool_;
    boost::asio::strand<boost::asio::thread_pool::executor_type> strand_;

  public:
    explicit Bus(boost::asio::thread_pool& pool)
        : pool_(pool), strand_(boost::asio::make_strand(pool_)) {}

    void Subscribe(Component* publisher, Component* subscriber) {
        subscribers_[publisher].push_back(subscriber);
    }

    void Send(Component* publisher, Event* event) {
        auto it = subscribers_.find(publisher);
        if (it != subscribers_.end()) {
            SetNumberCopyEvent(event, it->second.size());
            for (auto component : it->second) {
                boost::asio::post(strand_, [component, event, ]() {
                    event->Accept(component);
                });
            }
            }
    }

    void Join() {
        pool_.join();
    }
    private:
        void SetNumberCopyEvent(Event* event, unsigned int number){
            for(int i = 0; i < number; i++){
                event->AddReference();
            }
        }
};

};  // namespace bus
};  // namespace sp