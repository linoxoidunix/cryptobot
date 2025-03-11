#pragma once

#include <vector>

#include "aot/Logger.h"
#include "aot/common/time_utils.h"
#include "aot/common/types.h"
#include "aot/strategy/arbitrage/arbitrage_step.h"
#include "nlohmann/json.hpp"

namespace aot {
struct ArbitrageCycleHash;
class ArbitrageCycle : public std::vector<Step> {
  public:
    // Constructor to assign a unique ID
    ArbitrageCycle();
    virtual ~ArbitrageCycle() = default;
    common::Nanos time_open_;
    common::Nanos time_close_;

    using std::vector<Step>::vector;  // Inherit all constructors
    void push_back(const Step step) {
        // Добавляем шаг в вектор
        std::vector<Step>::push_back(step);
    }

    void SerializeToJson(nlohmann::json& j) const;
};
struct ArbitrageCycleHash {
    std::size_t operator()(const ArbitrageCycle& cycle) const;
};

};  // namespace aot

bool operator==(const aot::ArbitrageCycle& lhs, const aot::ArbitrageCycle& rhs);
