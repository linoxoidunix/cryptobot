from pathlib import Path
import pandas as pd
import numpy as np
from talib import RSI, BBANDS, MACD, ATR, NATR, PPO
import lightgbm as lgb
import statistics
import my_types as ksignals

class BaseStrategy:
    """_summary_
    """
    def __init__(self, path_where_models):
        """__window__ - i suppose we calculate forward return 21
        __min_period__ -  need for calculate dollar rank. mesure in month
        __T__ - list timeperiods
        Args:
            path_where_models (string): path where folder "models" exist
        """
        self.__path_where_models__ = path_where_models
        self.__window__ = 21
        self.__min_period__ = 1
        self.__T__ = [1, 5, 10, 21, 42, 63]
        self.__data__ = pd.DataFrame()
        self.__load_trained_subset__(path_where_models)
        self.__load_prediction_models__(path_where_models)

    def get_action(self, open, high, low, close, volume):
        """function that transformate raw data to action 

        Args:
            open (float): open price during tick
            high (float): highest price during tick
            low (float): lowest price during tick
            close (float): close price during tick
            volume (float): volume during tick

        Returns:
            tuple: return action: enter_long, enter_short, exit_long, exit_short or nothing with confidence?? 0 or 1
        """
        date = pd.Timestamp.utcnow()
        dict_with_new_data = {'date':date, 'open':open, 'high':high, 'low':low, 'close':close, 'volume':volume}
        new_row = pd.DataFrame([dict_with_new_data])
        new_row.set_index('date', inplace=True)
        data_with_new_row = pd.concat([self.__data__, new_row], ignore_index=False)
        self.__calculate_features__(data_with_new_row, date)
        print('calculate features is SUCCESS')
        predicted_value = self.__predict__(data_with_new_row.loc[date, self.__name_features_for_model__])
        print('calculate predicted values is SUCCESS')
        action = self.__generate_action__(predicted_value)
        print('calculate action is SUCCESS')
        return {action:1}
    
    def __calculate_features__(self, data, date):
        self.__calculate_dol_vol__(data, date)
        self.__calculate_dollar_vol_rank__(data, date)
        self.__calculate_rsi__(data, date)
        self.__calculate_bb_high_bb_low__(data, date)
        self.__calculate_natr__(data, date)
        self.__calculate_atr__(data, date)
        self.__calculate_ppo__(data, date)
        self.__calculate_macd__(data, date)
        self.__calculate_return__(data, date)
        self.__calculate_quantile_return__(data, date)
        self.__calculate_ymdw__(data, date)

    def __calculate_dol_vol__(self, data, date):
        data.loc[date:,'dollar_vol'] = data.loc[date:][['close', 'volume']].prod(1).div(1e3)

    def __calculate_dollar_vol_rank__(self, data, date):
        dollar_vol_ma = (data
                        .dollar_vol
                        .rolling(window=self.__window__, min_periods=self.__min_period__) # 1 trading month
                        .mean())
        data['dollar_vol_rank'] = (dollar_vol_ma.rank(axis=0, ascending=False))

    def __calculate_rsi__(self, data, date):
       data['rsi'] = RSI(data['close'])

    def __calculate_bb_high_bb_low__(self, data, date):
        data['bb_high'] = BBANDS(data['close'], timeperiod=20)[0]
        data['bb_low'] = BBANDS(data['close'], timeperiod=20)[2]
        data['bb_high'] = data.bb_high.sub(data.close).div(data.bb_high).apply(np.log1p)
        data['bb_low'] = data.close.sub(data.bb_low).div(data.close).apply(np.log1p)  

    def __calculate_natr__(self, data, date):
        data['NATR'] = NATR(data.high, data.low, data.close)

    def __calculate_atr__(self, data, date):
        def compute_atr(stock_data):
            df = ATR(stock_data.high, stock_data.low, 
                    stock_data.close, timeperiod=14)
            return df.sub(df.mean()).div(df.std())

        data['ATR'] = compute_atr(data)

    def __calculate_ppo__(self, data, date):
        data['PPO'] = PPO(data.close)

    def __calculate_macd__(self, data, date):
        def compute_macd(close):
            macd = MACD(close)[0]
            return (macd - np.mean(macd))/np.std(macd)
        data['MACD'] = compute_macd(data.close)

    def __calculate_return__(self, data, date):
        by_sym = data.close
        for t in self.__T__:
            data[f'r{t:02}'] = by_sym.pct_change(t)

    def __calculate_quantile_return__(self, data, date):
        for t in self.__T__:
            data[f'r{t:02}dec'] = pd.qcut(data[f'r{t:02}'], 
                                                        q=10, 
                                                        labels=False, 
                                                        duplicates='drop')
            
    def __calculate_ymdw__(self, data, date):
        """calculate year(y) month(m) day(d) weekday(w)

        Args:
            data (pandas.DataFrame): _description_
            date (Timestamp): _description_
        """
        data['year'] = data.index
        data['month'] = data.index
        data['day'] = data.index
        data['weekday'] = data.index

        data['year'] = data['year'].apply(lambda x : x.year)
        data['month'] = data['month'].apply(lambda x : x.month)
        data['day'] = data['day'].apply(lambda x : x.day)
        data['weekday'] = data['weekday'].apply(lambda x : x.weekday())

    def __load_prediction_models__(self, path_where_models):
        """suppose we have 3 models in models folder
        """
        self.__list_model__ = [lgb.Booster(model_file=f'{path_where_models}/models/model_{current}.txt') for current in range(3)]
        self.__name_features_for_model__ = self.__list_model__[0].feature_name()

    def __load_trained_subset__(self, path_where_models):
        """load X trained subset
        """
        results_path = Path(f'{path_where_models}/models')
        with pd.HDFStore(results_path / 'features_df.h5') as store:
            for i, key in enumerate(
                [k[1:] for k in store.keys() if k[1:].startswith('test_idx')]):
                self.__data__ = store[key]

    def __predict__(self, new_features):
        predicted_value = [ model.predict(new_features)[0] for model in self.__list_model__]
        return statistics.mean(predicted_value)

    def __generate_action__(self, predicted_value):
        print(f'start calculate action for predicted_value={predicted_value}')
        if(predicted_value > 0.01):
            return ksignals.enter_long
        if(predicted_value < -0.01):
            print(ksignals.enter_short)
            return ksignals.enter_short
        if(predicted_value < 0):
            return ksignals.exit_long
        if(predicted_value > 0):
            return ksignals.exit_short
        return ksignals.nothing