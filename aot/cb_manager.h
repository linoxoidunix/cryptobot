#pragma once
#include <functional>
#include <atomic>
#include <memory>

template<class T>
class LockFreeCallbackManager {
public:
    using Callback = const std::function<void(T&)>;
    using CallbackID = uint64_t;

    LockFreeCallbackManager() : next_id_(0) {}

    // Register a callback and return its unique ID
    CallbackID RegisterCallback(Callback callback) {
        auto id = next_id_.fetch_add(1, std::memory_order_relaxed);
        auto node = std::make_shared<CallbackNode>(std::move(callback), id);
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

    // Invoke all active callbacks
    void InvokeAll(T& t) const {
        auto current = head_.load(std::memory_order_acquire);
        while (current) {
            if (current->active.load(std::memory_order_acquire)) {
                if (current->callback) {
                    current->callback(t);
                }
            }
            current = current->next;
        }
    }

private:
    struct CallbackNode {
        CallbackNode(Callback cb, CallbackID id) : callback(std::move(cb)), id(id) {}

        Callback callback;
        CallbackID id;
        std::atomic<bool> active{true};
        std::shared_ptr<CallbackNode> next;
    };

    std::atomic<CallbackID> next_id_;
    std::atomic<std::shared_ptr<CallbackNode>> head_{nullptr};
};

template<>
class LockFreeCallbackManager<void> {
public:
    using Callback = const std::function<void()>;
    using CallbackID = uint64_t;

    LockFreeCallbackManager() : next_id_(0) {}

    // Register a callback and return its unique ID
    CallbackID RegisterCallback(Callback callback) {
        auto id = next_id_.fetch_add(1, std::memory_order_relaxed);
        auto node = std::make_shared<CallbackNode>(std::move(callback), id);
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

    // Invoke all active callbacks
    void InvokeAll() const {
        auto current = head_.load(std::memory_order_acquire);
        while (current) {
            if (current->active.load(std::memory_order_acquire)) {
                if (current->callback) {
                    current->callback();
                }
            }
            current = current->next;
        }
    }

private:
    struct CallbackNode {
        CallbackNode(Callback cb, CallbackID id) : callback(std::move(cb)), id(id) {}

        Callback callback;
        CallbackID id;
        std::atomic<bool> active{true};
        std::shared_ptr<CallbackNode> next;
    };

    std::atomic<CallbackID> next_id_;
    std::atomic<std::shared_ptr<CallbackNode>> head_{nullptr};
};
