#pragma once

#include <boost/pool/pool_alloc.hpp>  // Include Boost Pool header
#include <cassert>
#include <cstdint>
#include <list>
#include <new>  // For placement new
#include <string>
#include <vector>

#include "aot/common/macros.h"

namespace common {
template <typename T>
class MemPool final {
  public:
    using value_type = T;
    explicit MemPool(std::size_t num_elems)
        : store_(num_elems,
                 {T(), true}) /* pre-allocation of vector storage. */ {
        ASSERT(reinterpret_cast<const ObjectBlock *>(&(store_[0].object_)) ==
                   &(store_[0]),
               "T object should be first member of ObjectBlock.");
    }

    /// Allocate a new object of type T, use placement new to initialize the
    /// object, mark the block as in-use and return the object.
    template <typename... Args>
    T *allocate(Args... args) noexcept {
        auto obj_block = &(store_[next_free_index_]);
        ASSERT(obj_block->is_free_, "Expected free ObjectBlock at index:" +
                                        std::to_string(next_free_index_));
        T *ret              = &(obj_block->object_);
        ret                 = new (ret) T(args...);  // placement new.
        obj_block->is_free_ = false;

        updateNextFreeIndex();

        return ret;
    }

    /// Return the object back to the pool by marking the block as free again.
    /// Destructor is not called for the object.
    auto deallocate(const T *elem) noexcept {
        const auto elem_index =
            (reinterpret_cast<const ObjectBlock *>(elem) - &store_[0]);
        ASSERT(
            elem_index >= 0 && static_cast<size_t>(elem_index) < store_.size(),
            "Element being deallocated does not belong to this Memory pool.");
        ASSERT(!store_[elem_index].is_free_,
               "Expected in-use ObjectBlock at index:" +
                   std::to_string(elem_index));
        store_[elem_index].is_free_ = true;
    }

    void reset() noexcept {
        for (auto &block : store_) {
            block.is_free_ = true;  // Mark all blocks as free
        }
        next_free_index_ = 0;  // Reset the free index
    }

    // Deleted default, copy & move constructors and assignment-operators.
    MemPool()                            = delete;

    MemPool(const MemPool &)             = delete;

    MemPool(const MemPool &&)            = delete;

    MemPool &operator=(const MemPool &)  = delete;

    MemPool &operator=(const MemPool &&) = delete;

  private:
    /// Find the next available free block to be used for the next allocation.
    auto updateNextFreeIndex() noexcept {
        const auto initial_free_index = next_free_index_;
        while (!store_[next_free_index_].is_free_) {
            ++next_free_index_;
            if (next_free_index_ == store_.size())
                [[unlikely]] {  // hardware branch predictor should almost
                                // always predict this to be false any ways.
                next_free_index_ = 0;
            }
            if (initial_free_index == next_free_index_) [[unlikely]] {
                ASSERT(initial_free_index != next_free_index_,
                       "Memory Pool out of space.");
            }
        }
    }

    /// It is better to have one vector of structs with two objects than two
    /// vectors of one object. Consider how these are accessed and cache
    /// performance.
    struct ObjectBlock {
        T object_;
        bool is_free_ = true;
    };

    /// We could've chosen to use a std::array that would allocate the memory on
    /// the stack instead of the heap. We would have to measure to see which one
    /// yields better performance. It is good to have objects on the stack but
    /// performance starts getting worse as the size of the pool increases.
    std::vector<ObjectBlock> store_;

    size_t next_free_index_ = 0;
};

template <typename T>
class MemoryPool {
  public:
    // Constructor
    explicit MemoryPool(size_t poolSize)
        : pool_(sizeof(T), poolSize), pool_size_(poolSize) {}

    // Allocates memory for an object of type T
    template <typename... Args>
    T *Allocate(Args &&...args) {
        if (pool_size_ == 0) return nullptr;

        T *obj = static_cast<T *>(pool_.malloc());
        if (obj) {
            new (obj) T(std::forward<Args>(args)...);
        }
        return obj;
    }

    ~MemoryPool(){
      Clear();
    }

    // Deallocates memory for an object of type T
    void Deallocate(T *obj) {
        if (obj) {
            // Call the destructor of the object
            obj->~T();
            pool_.free(obj);
        }
    }

    // Reset the pool by marking all allocated memory as free (without actually
    // purging)
    void Reset() {
        // Mark all objects as free by calling their destructors and returning
        // memory to the pool
        pool_.release_memory();
    }

    void Clear() {
        // Mark all objects as free by calling their destructors and returning
        // memory to the pool
        pool_.purge_memory();
    }

  private:
    // Boost pool for memory management
    boost::pool<> pool_;
    const size_t pool_size_;
};
}  // namespace common