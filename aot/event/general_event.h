#pragma once
#include <atomic>

namespace aot{
template <typename Pool>
class Event {
public:
    explicit Event(Pool* memory_pool = nullptr) : memory_pool_(memory_pool) {}

    virtual ~Event() = default;

    void AddReference() { ref_count_.fetch_add(1, std::memory_order_relaxed); }

    virtual void Release() {
    }

protected:
    Pool* memory_pool_ = nullptr;  // Pointer to the memory pool
    std::atomic<int> ref_count_{0};  // Atomic reference count
};
}