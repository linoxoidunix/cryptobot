#include "bybit/Predictor.h"

#include "bybit/impl/PredictorImpl.h"

Predictor::Predictor(uint maximum_window)
    : impl(std::unique_ptr<detail::PredictorImpl>(
          new detail::PredictorImpl(maximum_window))) {
    impl->Init();
}

Predictor::~Predictor() {}

Action::Pointer Predictor::Predict(OHLCV data) { return ActionEmpty::Ptr(); }
