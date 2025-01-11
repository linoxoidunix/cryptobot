#pragma once
#include <functional>
#include <atomic>
#include <memory>

#include "aot/common/types.h"

namespace aot{

using CallbackID = uint64_t;

template<typename T>
struct CallbackNode {
    using Type = T;
    CallbackNode(const std::function<void(T&)> cb) : callback(std::move(cb)){}
    const std::function<void(T&)> callback;
    CallbackID id;
    std::atomic<bool> active{true};
    std::shared_ptr<CallbackNode<T>> next;
};

template<>
struct CallbackNode<void> {
    using Type = void;
    CallbackNode(const std::function<void()> cb) : callback(std::move(cb)) {}
    const std::function<void()> callback;
    CallbackID id;
    std::atomic<bool> active{true};
    std::shared_ptr<CallbackNode> next;
};

template<typename... Args>
struct CallbackNodeTradingPair {
    //using Type = T;
    using CallbackType = std::function<void(Args...)>;
    CallbackNodeTradingPair(CallbackType cb, common::TradingPair tp) :
    callback(std::move(cb)),
    trading_pair(tp) {}
    common::TradingPair trading_pair;
    CallbackType callback;
    CallbackID id;
    std::atomic<bool> active{true};
    std::shared_ptr<CallbackNodeTradingPair<Args...>> next;
};

template<class Node>
class LockFreeCallbackManager {
public:
    using CallBackType = Node;
    LockFreeCallbackManager() : next_id_(0) {}

    // Register a callback and return its unique ID
    template<typename ... Args>
    CallbackID RegisterCallback(Args&& ...args) {
        auto id = next_id_.fetch_add(1, std::memory_order_relaxed);
        auto node = std::make_shared<Node>(std::forward<Args>(args)...);
        node->id = id;
        auto old_head = head_.load(std::memory_order_acquire);
        do {
            node->next = old_head;
        } while (!head_.compare_exchange_weak(old_head, node, std::memory_order_release, std::memory_order_acquire));
        return id;
    }

    // Unregister a callback by ID
    bool UnregisterCallback(CallbackID id) {
        auto current = head_.load(std::memory_order_acquire);
        while (current) {
            if (current->id == id) {
                current->active.store(false, std::memory_order_release);
                return true;
            }
            current = current->next;
        }
        return false; // ID not found
    }

    template<typename T>
    void InvokeAll(T& t) const {
        auto current = head_.load(std::memory_order_acquire);
        while (current) {
            if (current->active.load(std::memory_order_acquire)) {
            if constexpr (std::is_same_v<decltype(current->trading_pair), common::TradingPair>) {
                // Если поле trading_pair существует, вызываем callback с дополнительным параметром
                current->callback(t, current->trading_pair);
            } else {
                // Если поля trading_pair нет, вызываем callback как обычно
                current->callback(t);
            }
            }
            current = current->next;
        }
    }
    
    void InvokeAll() const {
        auto current = head_.load(std::memory_order_acquire);
        while (current) {
            if (current->active.load(std::memory_order_acquire)) {
                if (current->callback) {
                    current->callback();  // Correctly invoke callback with no parameters
                }
            }
            current = current->next;
        }
    }
private:
    std::atomic<CallbackID> next_id_;
    std::atomic<std::shared_ptr<Node>> head_{nullptr};
};
};

// Specialization for void
