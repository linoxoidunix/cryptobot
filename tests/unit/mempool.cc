#include <gtest/gtest.h>
#include "aot/common/mem_pool.h"

namespace common {

class TestObject {
 public:
  explicit TestObject(int value) : value_(value) {}
  int GetValue() const { return value_; }

 private:
  int value_;
};

class MemoryPoolTest : public ::testing::Test {
 protected:
  void SetUp() override {
  }

  void TearDown() override {
  }

  MemoryPool<TestObject> pool_{10};
};


TEST(MemoryPoolTest, ShouldAllocateMemoryForSingleObjectAndReturnValidPointer) {
  common::MemoryPool<common::TestObject> pool(10);
  common::TestObject* obj = pool.Allocate(42);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->GetValue(), 42);
  pool.Deallocate(obj);
}

TEST(MemoryPoolTest, ShouldCorrectlyDeallocatePreviouslyAllocatedObject) {
  common::MemoryPool<common::TestObject> pool(10);
  common::TestObject* obj = pool.Allocate(42);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->GetValue(), 42);
  
  pool.Deallocate(obj);
  
  common::TestObject* new_obj = pool.Allocate(84);
  ASSERT_NE(new_obj, nullptr);
  EXPECT_EQ(new_obj->GetValue(), 84);
  pool.Deallocate(new_obj);
}

TEST(MemoryPoolTest, ShouldConstructObjectWithProvidedArgumentsDuringAllocation) {
  common::MemoryPool<common::TestObject> pool(10);
  common::TestObject* obj = pool.Allocate(42);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->GetValue(), 42);
  pool.Deallocate(obj);
}

TEST(MemoryPoolTest, ShouldResetThePoolAndAllowReallocationOfObjects) {
  common::MemoryPool<common::TestObject> pool(2);
  
  // Allocate two objects
  common::TestObject* obj1 = pool.Allocate(1);
  common::TestObject* obj2 = pool.Allocate(2);
  
  ASSERT_NE(obj1, nullptr);
  ASSERT_NE(obj2, nullptr);
  
  // Reset the pool
  pool.Reset();
  
  // Allocate two objects again after reset
  common::TestObject* obj3 = pool.Allocate(3);
  common::TestObject* obj4 = pool.Allocate(4);
  
  ASSERT_NE(obj3, nullptr);
  ASSERT_NE(obj4, nullptr);
  
  EXPECT_EQ(obj3->GetValue(), 3);
  EXPECT_EQ(obj4->GetValue(), 4);
  
  pool.Deallocate(obj3);
  pool.Deallocate(obj4);
}

TEST(MemoryPoolTest, ShouldCallDestructorDuringDeallocation) {
  common::MemoryPool<common::TestObject> pool(10);
  
  // Allocate an object
  common::TestObject* obj = pool.Allocate(42);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->GetValue(), 42);

  // Deallocate the object
  pool.Deallocate(obj);

  // Allocate another object, should reuse the deallocated space
  common::TestObject* new_obj = pool.Allocate(84);
  ASSERT_NE(new_obj, nullptr);
  EXPECT_EQ(new_obj->GetValue(), 84);
  EXPECT_EQ(new_obj, obj);


  pool.Deallocate(new_obj);
}

TEST(MemoryPoolTest, ShouldNotCrashWhenDeallocatingNullptr) {
  common::MemoryPool<common::TestObject> pool(10);
  common::TestObject* obj = nullptr;
  EXPECT_NO_THROW(pool.Deallocate(obj));
}

TEST(MemoryPoolTest, ShouldNotAllowAllocationIfPoolSizeIsZero) {
  common::MemoryPool<common::TestObject> pool(0);
  common::TestObject* obj = pool.Allocate(42);
  EXPECT_EQ(obj, nullptr);
}

TEST(MemoryPoolTest, ShouldHandleAllocationAndDeallocationOfMultipleObjectsCorrectly) {
  common::MemoryPool<common::TestObject> pool(5);

  // Allocate multiple objects
  common::TestObject* obj1 = pool.Allocate(1);
  common::TestObject* obj2 = pool.Allocate(2);
  common::TestObject* obj3 = pool.Allocate(3);

  ASSERT_NE(obj1, nullptr);
  ASSERT_NE(obj2, nullptr);
  ASSERT_NE(obj3, nullptr);

  EXPECT_EQ(obj1->GetValue(), 1);
  EXPECT_EQ(obj2->GetValue(), 2);
  EXPECT_EQ(obj3->GetValue(), 3);

  // Deallocate one object
  pool.Deallocate(obj2);

  // Allocate another object, should reuse the deallocated space
  common::TestObject* obj4 = pool.Allocate(4);
  ASSERT_NE(obj4, nullptr);
  EXPECT_EQ(obj4->GetValue(), 4);

  // Deallocate all objects
  pool.Deallocate(obj1);
  pool.Deallocate(obj3);
  pool.Deallocate(obj4);

  // Allocate all objects again
  common::TestObject* obj5 = pool.Allocate(5);
  common::TestObject* obj6 = pool.Allocate(6);
  common::TestObject* obj7 = pool.Allocate(7);

  ASSERT_NE(obj5, nullptr);
  ASSERT_NE(obj6, nullptr);
  ASSERT_NE(obj7, nullptr);

  EXPECT_EQ(obj5->GetValue(), 5);
  EXPECT_EQ(obj6->GetValue(), 6);
  EXPECT_EQ(obj7->GetValue(), 7);

  pool.Deallocate(obj5);
  pool.Deallocate(obj6);
  pool.Deallocate(obj7);
}

TEST(MemoryPoolTest, ResetReleasesMemory) {
    size_t poolSize = 10;
    MemoryPool<common::TestObject> pool(poolSize);

    // Allocate memory for several Dummy objects
    std::vector<common::TestObject*> allocatedObjects;
    for (size_t i = 0; i < poolSize; ++i) {
        common::TestObject* obj = pool.Allocate(i);
        ASSERT_NE(obj, nullptr) << "Allocation failed for object " << i;
        allocatedObjects.push_back(obj);
    }

    // Call Reset to release all the memory back to the pool
    pool.Reset();

    // After Reset, all objects should be deallocated, and we should be able to reuse the memory
    std::vector<common::TestObject*> reallocatedObjects;
    for (size_t i = 0; i < poolSize; ++i) {
        common::TestObject* obj = pool.Allocate(i);
        ASSERT_NE(obj, nullptr) << "Reallocation failed after reset for object " << i;
        reallocatedObjects.push_back(obj);
    }

    // Check that the reallocated pointers are different from the previous ones
    for (size_t i = 0; i < poolSize; ++i) {
        ASSERT_NE(allocatedObjects[i], reallocatedObjects[i]) << "Object " << i << " was not properly deallocated";
    }
}

TEST(MemoryPoolTest, ShouldCorrectlyHandleAllocationAndDeallocationOfObjectsOfDifferentSizes) {
  // Define a small object
  class SmallObject {
   public:
    SmallObject() : value_(1) {}
    int GetValue() const { return value_; }
   private:
    int value_;
  };

  // Define a large object
  class LargeObject {
   public:
    LargeObject() : values_(1000, 42) {}
    int GetValue(size_t index) const { return values_.at(index); }
   private:
    std::vector<int> values_;
  };

  // Create memory pools for small and large objects
  common::MemoryPool<SmallObject> small_pool(10);
  common::MemoryPool<LargeObject> large_pool(10);

  // Allocate and deallocate small objects
  SmallObject* small_obj1 = small_pool.Allocate();
  ASSERT_NE(small_obj1, nullptr);
  EXPECT_EQ(small_obj1->GetValue(), 1);
  small_pool.Deallocate(small_obj1);

  SmallObject* small_obj2 = small_pool.Allocate();
  ASSERT_NE(small_obj2, nullptr);
  EXPECT_EQ(small_obj2->GetValue(), 1);
  small_pool.Deallocate(small_obj2);

  // Allocate and deallocate large objects
  LargeObject* large_obj1 = large_pool.Allocate();
  ASSERT_NE(large_obj1, nullptr);
  EXPECT_EQ(large_obj1->GetValue(0), 42);
  large_pool.Deallocate(large_obj1);

  LargeObject* large_obj2 = large_pool.Allocate();
  ASSERT_NE(large_obj2, nullptr);
  EXPECT_EQ(large_obj2->GetValue(0), 42);
  large_pool.Deallocate(large_obj2);
}
}  // namespace common

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

