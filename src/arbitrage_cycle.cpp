#include "aot/strategy/arbitrage/arbitrage_cycle.h"

aot::ArbitrageCycle::ArbitrageCycle() {}

void aot::ArbitrageCycle::SerializeToJson(nlohmann::json& j) const {
    aot::ArbitrageCycleHash hasher;
    j = {
        {"id", hasher(*this)},
        {"steps", nlohmann::json::array()},
    };

    for (const auto& step : *this) {
        nlohmann::json step_json;
        step.SerializeToJson(step_json);
        j["steps"].push_back(step_json);
    };
}

std::size_t aot::ArbitrageCycleHash::operator()(
    const aot::ArbitrageCycle& cycle) const {
    std::size_t seed = 0;

    // Хэшируем содержимое вектора Step
    for (const auto& step : cycle) {
        boost::hash_combine(
            seed, step);  // Предполагается, что Step уже имеет свой хэш
    }
    return seed;
};

bool operator==(const aot::ArbitrageCycle& lhs,
                const aot::ArbitrageCycle& rhs) {
    if (lhs.size() != rhs.size()) return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
};