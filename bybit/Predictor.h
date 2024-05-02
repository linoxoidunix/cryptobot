#pragma once
#include <bybit/Exchange.h>

#include <memory>

enum class Side { LONG, SHORT, NOPE };
class Action {
  public:
    using Pointer     = std::unique_ptr<Action>;
    virtual void Do() = 0;
};
using ActionPtr = std::unique_ptr<Action>;

class Buy : public Action {
    static Action::Pointer Ptr() {
        return std::unique_ptr<Buy>(new Buy(Side::NOPE));
    }

  public:
    explicit Buy(Side side) : side_(side){};

    void Do() override {};

  private:
    Side side_ = Side::NOPE;
};

class Sell : public Action {
    static Action::Pointer Ptr() {
        return std::unique_ptr<Sell>(new Sell(Side::NOPE));
    }

  public:
    explicit Sell(Side side) : side_(side){};
    void Do() override {};

  private:
    Side side_ = Side::NOPE;
};

class ActionEmpty : public Action {
  public:
    explicit ActionEmpty() = default;
    void Do() override {};
    static inline Action::Pointer Ptr() {
        return std::unique_ptr<ActionEmpty>(new ActionEmpty());
    };
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
    ~Predictor();

  private:
    std::unique_ptr<detail::PredictorImpl> impl;
};