#pragma once
#include "aot/strategy/arbitrage/arbitrage_step.h"

namespace aot {
class StepManager {
    uint32_t max_transactions_;

  public:
    StepManager(uint32_t max_transactions)
        : max_transactions_(max_transactions) {}
};
};  // namespace aot
