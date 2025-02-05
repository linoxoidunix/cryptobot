#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <boost/intrusive/unordered_set.hpp>
#include <boost/functional/hash.hpp>

#include "aot/common/types.h"
#include "aot/strategy/arbitrage/arbitrage_step.h"
#include "aot/common/time_utils.h"

namespace aot{
class ArbitrageCycle : public std::vector<Step>
{
public:
    // Constructor to assign a unique ID
    ArbitrageCycle() {}
    virtual ~ArbitrageCycle() = default;
    common::Nanos time_open_;
    common::Nanos time_close_;
    bool open_ = false;
    common::Qty position_open_ = common::kQtyInvalid;
    common::Price delta_ = common::kPriceInvalid;
    common::Price buy_input_ = common::kPriceInvalid;
    common::Price sell_input_ = common::kPriceInvalid;
    common::Price buy_exit_ = common::kPriceInvalid;
    common::Price sell_exit_ = common::kPriceInvalid;

    using std::vector<Step>::vector; // Inherit all constructors
    void push_back(const Step step) {
        // Добавляем шаг в вектор
        std::vector<Step>::push_back(step);
    }
    void StartTransaction(){
        open_ = true;
        time_open_ = common::getCurrentNanoS();
    }
    void CloseTransaction(){
        if (!open_) {
            logi("No transaction to close");
            return;
        }
        time_close_ = common::getCurrentNanoS();
        open_ = false;
    }
    bool IsTransactionOpen() const {
        return open_;
    }
    common::Nanos GetDelta() const {
        return time_close_ - time_open_;
    }
    void SetDeltaPrice(common::Price delta){
        delta_ = delta;
    }
    void SetBuyInput(common::Price buy_input){
        buy_input_ = buy_input;
    }    
    void SetSellInput(common::Price sell_input){
        sell_input_ = sell_input;
    }    
    void SetBuyExit(common::Price buy_exit){
        buy_exit_ = buy_exit;
    }    
    void SetSellExit(common::Price sell_exit){
        sell_exit_ = sell_exit;
    }
    void SetPositionOpen(common::Qty position){
        position_open_ = position;
    }
};
struct ArbitrageCycleHash {
    std::size_t operator()(const ArbitrageCycle& cycle) const {
        std::size_t seed = 0;

        // Хэшируем содержимое вектора Step
        for (const auto& step : cycle) {
            boost::hash_combine(seed, step);  // Предполагается, что Step уже имеет свой хэш
        }
        return seed;
    }
};

bool operator==(const ArbitrageCycle& lhs, const ArbitrageCycle& rhs) {
    if (lhs.size() != rhs.size()) return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
};