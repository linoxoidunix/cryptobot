#include "bybit/Predictor.h"

#include "bybit/impl/PredictorImpl.h"

Predictor::Predictor(uint maximum_window)
    : impl(std::unique_ptr<detail::PredictorImpl>(
          new detail::PredictorImpl(maximum_window))) {
    impl->Init();
}

Action::Pointer Predictor::Predict(OHLCV data) {
    auto report =
        impl->Predict(data.open, data.high, data.low, data.close, data.volume);
    ActionFactory factory;
    auto action = factory.Produce(report);
    action->Do();
    return action;
}

Predictor::~Predictor() {}

Action::Pointer ActionFactory::Produce(
    std::pair<std::string, long> python_module_answer) {
    if (python_module_answer.first == "enter_long" &&
        python_module_answer.second == 1)
        return Buy::Ptr(Side::LONG);
    if (python_module_answer.first == "enter_short" &&
        python_module_answer.second == 1)
        return Buy::Ptr(Side::SHORT);
    if (python_module_answer.first == "exit_long" &&
        python_module_answer.second == 1)
        return Sell::Ptr(Side::LONG);
    if (python_module_answer.first == "exit_short" &&
        python_module_answer.second == 1)
        return Sell::Ptr(Side::SHORT);
    return ActionEmpty::Ptr();
}