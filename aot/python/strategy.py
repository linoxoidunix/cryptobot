import my_types as types
import prepare_model_for_bot as strategy


class Predictor:
    def __init__(self):
        self.__strategy__ = strategy.BaseStrategy()

    def predict(self, open, high, low, close, volume):
        return self.__strategy__.get_action(open, high, low, close, volume)

