#pragma once
#include <atomic>

namespace aot {
template <typename Pool>
class Event {
  public:
    explicit Event(Pool* memory_pool = nullptr) : memory_pool_(memory_pool) {}

    virtual ~Event() = default;

  protected:
    Pool* memory_pool_ = nullptr;    // Pointer to the memory pool
    std::atomic<int> ref_count_{0};  // Atomic reference count
};
}  // namespace aot