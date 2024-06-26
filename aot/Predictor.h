#pragma once
#include <memory>

#include "aot/Exchange.h"
#include "aot/Logger.h"

enum class Direction { LONG, SHORT, NOPE };
class Action {
  public:
    using Pointer     = std::unique_ptr<Action>;
    virtual void Do() = 0;
};
using ActionPtr = std::unique_ptr<Action>;

class Buy : public Action {
  public:
    explicit Buy(Direction side) : side_(side){};
    void Do() override { fmt::print("buy {}\n", (int)side_); };
    static Action::Pointer Ptr(Direction side = Direction::NOPE) {
        return std::unique_ptr<Buy>(new Buy(side));
    };

  private:
    Direction side_ = Direction::NOPE;
};

class Sell : public Action {
  public:
    explicit Sell(Direction side) : side_(side){};
    void Do() override { fmt::print("sell {}\n", (int)side_); };
    static Action::Pointer Ptr(Direction side = Direction::NOPE) {
        return std::unique_ptr<Sell>(new Sell(side));
    };

  private:
    Direction side_ = Direction::NOPE;
};

class ActionEmpty : public Action {
  public:
    explicit ActionEmpty() = default;
    void Do() override { fmt::print("action nope\n"); };
    static inline Action::Pointer Ptr() {
        return std::unique_ptr<ActionEmpty>(new ActionEmpty());
    };
};

class ActionFactory {
  public:
    explicit ActionFactory() = default;
    Action::Pointer Produce(std::pair<std::string, long>);
};

namespace detail {
class PredictorImpl;
};
/**
 * @brief cpp wrapper about python class predictor for predict signall buy, sell
 * for long and short
 *
 */
class Predictor {
  public:
    /**
     * @brief Construct a new Predictor object
     *
     * @param maximum_window_size start predict after maximum_window_size
     * candles
     */
    explicit Predictor(uint maximum_window);
    Action::Pointer Predict(OHLCV data);
    Predictor(const Predictor&)            = delete;
    Predictor& operator=(const Predictor&) = delete;
    ~Predictor() = default;

  private:
    std::unique_ptr<detail::PredictorImpl> impl;
};
