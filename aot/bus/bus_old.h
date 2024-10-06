//-------------------------------------------
// #include <iostream>
// #include <memory>
// #include <string_view>
// #include <type_traits>
// #include <typeindex>
// #include <unordered_map>
// #include <vector>
// #include <atomic>
// #include <thread>
// #include <boost/asio.hpp>
// #include <boost/asio/strand.hpp>
// #include <boost/thread.hpp>

// namespace sp {
//     class Event {
//     public:
//         Event() : ref_count_(0) {}  // Initialize reference count

//         virtual ~Event() = default;

//         virtual std::type_index GetType() const = 0;  // Get the type of the event

//         // Increment the reference count
//         void AddReference() {
//             ref_count_.fetch_add(1, std::memory_order_relaxed);
//         }

//         // Decrement the reference count and deallocate if it reaches zero
//         void Release() {
//             if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
//                 Deallocate();
//             }
//         }

//     protected:
//         // Method to deallocate resources
//         virtual void Deallocate() {
//             std::cout << "Deallocating resources\n";
//             delete this;  // Deletes the event object
//         }

//     private:
//         std::atomic<int> ref_count_;  // Atomic counter for reference counting
//     };

//     class ImplementationEventA : public Event {
//     public:
//         std::type_index GetType() const override {
//             return typeid(ImplementationEventA);
//         }
//         std::string GetName() const {
//             return "ImplementationEventA";
//         }
//         // Additional event data and methods can be added here
//     };

//     class ImplementationEventB : public Event {
//     public:
//         std::type_index GetType() const override {
//             return typeid(ImplementationEventB);
//         }
//         std::string GetName() const {
//             return "ImplementationEventB";
//         }
//         // Additional event data and methods can be added here
//     };

//     class ImplementationEventC : public Event {
//     public:
//         std::type_index GetType() const override {
//             return typeid(ImplementationEventC);
//         }
//         std::string GetName() const {
//             return "ImplementationEventC";
//         }
//         // Additional event data and methods can be added here
//     };

//     namespace bus {

//         enum class Type {
//             A,
//             B,
//             C,
//             UNKNOWN
//         };

//         // Base class for components
//         template <typename Derived>
//         class Component {
//         public:
//             virtual ~Component() = default;

//             // The AsyncUpdate function will be implemented in the derived class
//             template <typename EventType>
//             void AsyncUpdate(EventType& event) {
//                 static_cast<Derived*>(this)->HandleEvent(event);
//                 event.Release();  // Release the event after processing
//             }
//         };

//         class MyComponentA : public Component<MyComponentA> {
//             boost::asio::thread_pool& pool_;
//             boost::asio::strand<boost::asio::thread_pool::executor_type> strand_;
//         public:
//             Type type_{Type::A};
//             MyComponentA(boost::asio::thread_pool& pool) : pool_(pool), strand_(boost::asio::make_strand(pool_)) {}

//             void HandleEvent(ImplementationEventA& event) {
//                 boost::asio::post(strand_, [&event]() {
//                     std::cout << "Handling " << event.GetName() << " in MyComponentA\n";
//                     event.Release();
//                 });
//             }
//             void HandleEvent(ImplementationEventC& event) {
//                 boost::asio::post(strand_, [&event]() {
//                     std::cout << "Handling " << event.GetName() << " in MyComponentC\n";
//                     event.Release();
//                 });
//             }
//             void HandleEvent(ImplementationEventB& event) {
//                 boost::asio::post(strand_, [&event]() {
//                     std::cout << "Handling " << event.GetName() << " in MyComponentB\n";
//                     event.Release();
//                 });
//             }
//         };

//         class MyComponentB : public Component<MyComponentB> {
//             boost::asio::thread_pool& pool_;
//             boost::asio::strand<boost::asio::thread_pool::executor_type> strand_;
//         public:
//             Type type_{Type::B};
//             MyComponentB(boost::asio::thread_pool& pool) : pool_(pool), strand_(boost::asio::make_strand(pool_)) {}

//              void HandleEvent(ImplementationEventA& event) {
//                 boost::asio::post(strand_, [&event]() {
//                     std::cout << "Handling " << event.GetName() << " in MyComponentA\n";
//                     event.Release();
//                 });
//             }
//             void HandleEvent(ImplementationEventC& event) {
//                 boost::asio::post(strand_, [&event]() {
//                     std::cout << "Handling " << event.GetName() << " in MyComponentC\n";
//                     event.Release();
//                 });
//             }
//             void HandleEvent(ImplementationEventB& event) {
//                 boost::asio::post(strand_, [&event]() {
//                     std::cout << "Handling " << event.GetName() << " in MyComponentB\n";
//                     event.Release();
//                 });
//             }
//         };

//         class MyComponentC : public Component<MyComponentC> {
//             boost::asio::thread_pool& pool_;
//             boost::asio::strand<boost::asio::thread_pool::executor_type> strand_;
//         public:
//             Type type_{Type::C};
//             MyComponentC(boost::asio::thread_pool& pool) : pool_(pool), strand_(boost::asio::make_strand(pool_)) {}

//              void HandleEvent(ImplementationEventA& event) {
//                 boost::asio::post(strand_, [&event]() {
//                     std::cout << "Handling " << event.GetName() << " in MyComponentA\n";
//                     event.Release();
//                 });
//             }
//             void HandleEvent(ImplementationEventC& event) {
//                 boost::asio::post(strand_, [&event]() {
//                     std::cout << "Handling " << event.GetName() << " in MyComponentC\n";
//                     event.Release();
//                 });
//             }
//             void HandleEvent(ImplementationEventB& event) {
//                 boost::asio::post(strand_, [&event]() {
//                     std::cout << "Handling " << event.GetName() << " in MyComponentB\n";
//                     event.Release();
//                 });
//             }
//         };

//         class TypeRegistry {
//         public:
//             // Register a pointer with its type ID
//             void registerPointer(void* ptr, Type typeId) {
//                 registry[ptr] = typeId;
//             }

//             // Get the type ID of a registered pointer
//             constexpr Type getType(void* ptr) const{
//                 if (registry.find(ptr) != registry.end()) {
//                     return registry.at(ptr);
//                 }
//                 return Type::UNKNOWN;
//             }

//         private:
//             std::unordered_map<void*, Type> registry; // Map of pointers to type IDs
//         };

//         // Bus class (no Component<void> anymore)
//         class Bus {
//             using ChannelMap = std::unordered_map<std::type_index, std::vector<void*>>;
//             using ComponentsMap = std::unordered_map<void*, ChannelMap>;
//             TypeRegistry type_registry_;
//             ComponentsMap subscribers_;  // Store subscriptions by publisher component pointer
//             boost::asio::thread_pool& pool_;  // Reference to an external Boost thread pool
//             boost::asio::strand<boost::asio::thread_pool::executor_type> strand_;

//         public:
//             explicit Bus(boost::asio::thread_pool& pool)
//                 : pool_(pool), strand_(boost::asio::make_strand(pool_)) {}

//             // Subscribe a channel to a specific publisher for a specific message type
//             template <typename EventType, typename PublisherType, typename SubscriberType>
//             void Subscribe(PublisherType* publisher, SubscriberType* subscriber) {
//                 static_assert(std::is_base_of<Component<PublisherType>, PublisherType>::value, "PublisherType must be a Component");
//                 static_assert(std::is_base_of<Component<SubscriberType>, SubscriberType>::value, "SubscriberType must be a Component");
//                 auto subscriber_ptr = static_cast<void*>(subscriber);
//                 type_registry_.registerPointer(subscriber_ptr, subscriber->type_);
//                 subscribers_[static_cast<void*>(publisher)][typeid(EventType)].push_back(static_cast<void*>(subscriber));
//             }

//             // Send an event from a component to its subscribers
//             template <typename EventType, typename ComponentType>
//             void Send(ComponentType& publisher, EventType& event) {
//                 auto it = subscribers_.find(static_cast<void*>(&publisher));
//                 if (it != subscribers_.end()) {
//                     auto typeIt = it->second.find(typeid(EventType));
//                     if (typeIt != it->second.end()) {
//                         auto size = typeIt->second.size();
//                         for (int j = 0; j < size; j++) {
//                             event.AddReference();
//                         }
//                         std::cout << "Sending event of type " << typeid(EventType).name() << " from " << typeid(ComponentType).name() << std::endl;
//                         for (void* component : typeIt->second) {
//                             // Post to the strand for ordered and safe delivery
//                             boost::asio::post(strand_, [this, component, &event]() {
//                                 auto type = type_registry_.getType(component);

//                                 if (type == Type::A) {
//                                     static_cast<MyComponentA*>(component)->AsyncUpdate(event);
//                                     return;
//                                 }
//                                 if (type == Type::B) {
//                                     static_cast<MyComponentB*>(component)->AsyncUpdate(event);
//                                     return;
//                                 }
//                                 if (type == Type::C) {
//                                     static_cast<MyComponentC*>(component)->AsyncUpdate(event);
//                                     return;
//                                 }
//                             });
//                         }
//                     }
//                 }
//             }

//             // Wait for all tasks to complete (optional)
//             void Join() {
//                 pool_.join();  // Wait for all threads to finish processing
//             }
//         };

//         // Example component classes
        
//     };
// };



