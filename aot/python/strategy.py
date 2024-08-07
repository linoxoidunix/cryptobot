import prepare_model_for_bot as my_strategy

# sys.path.append(os.getcwd())


class Predictor:
    def __init__(self, path_where_models):
        """_summary_

        Args:
            path_where_models (string): path where folder "models" exist
        """
        self.__strategy__ = my_strategy.BaseStrategy(path_where_models)

    def predict(self, open, high, low, close, volume):
        return self.__strategy__.get_action(open, high, low, close, volume)

def main():
    predictor = Predictor("./")
    result = predictor.predict(50000.0,70000.0,40000.0,560000.0, 10000)
    print(result)

if __name__ == "__main__":
    main()