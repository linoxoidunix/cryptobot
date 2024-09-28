#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "aot/common/thread_utils.h"

namespace common {

class MockDerived : public Servisable<MockDerived> {
public:
    MOCK_METHOD(void, StartImpl, (), ());
    MOCK_METHOD(void, StopImmediatelyImpl, (), ());
    MOCK_METHOD(void, StopWaitAllQueueImpl, (), ());
};

class ServisableTest : public ::testing::Test {
protected:
    MockDerived mockDerived;
};

}  // namespace common
