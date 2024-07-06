from pathlib import Path
import pandas as pd
import numpy as np
from talib import RSI, BBANDS, MACD, ATR, NATR, PPO
import lightgbm as lgb
import statistics


T = [1, 5, 10, 21, 42, 63]

results_path = Path('models')
lgb_store = Path(results_path /'features_df.h5')

test_idx = np.load(results_path / 'test_idx.npy')

data = pd.DataFrame()

with pd.HDFStore(results_path / 'features_df.h5') as store:
    for i, key in enumerate(
        [k[1:] for k in store.keys() if k[1:].startswith('test_idx')]):
        data = store[key]

open = 55555.5
high = 66666.6
low = 44444.4
close = 55577.9
volume = 20000.9
date = pd.Timestamp.utcnow()
dict = {'date':date, 'open':open, 'high':high, 'low':low, 'close':close, 'volume':volume}
new_row = pd.DataFrame([dict])
new_row.set_index('date', inplace=True)

print(new_row)

data_with_new_row = pd.concat([data, new_row], ignore_index=False)

#asign dollar_vol
data_with_new_row.loc[date:,'dollar_vol'] = data_with_new_row.loc[date:][['close', 'volume']].prod(1).div(1e3)

#asign dollar_vol_rank
window = 21
min_period = 1

dollar_vol_ma = (data_with_new_row
                 .dollar_vol
                 .rolling(window=window, min_periods=min_period) # 1 trading month
                 .mean())
data_with_new_row['dollar_vol_rank'] = (dollar_vol_ma.rank(axis=0, ascending=False))

#asign rsi
data_with_new_row['rsi'] = RSI(data_with_new_row['close'])

#asign bb_high bb_low
data_with_new_row['bb_high'] = BBANDS(data_with_new_row['close'], timeperiod=20)[0]
data_with_new_row['bb_low'] = BBANDS(data_with_new_row['close'], timeperiod=20)[2]
data_with_new_row['bb_high'] = data_with_new_row.bb_high.sub(data_with_new_row.close).div(data_with_new_row.bb_high).apply(np.log1p)
data_with_new_row['bb_low'] = data_with_new_row.close.sub(data_with_new_row.bb_low).div(data_with_new_row.close).apply(np.log1p)

#asign NATR
data_with_new_row['NATR'] = NATR(data_with_new_row.high, data_with_new_row.low, data_with_new_row.close)

def compute_atr(stock_data):
    df = ATR(stock_data.high, stock_data.low, 
             stock_data.close, timeperiod=14)
    return df.sub(df.mean()).div(df.std())

#asign ATR
data_with_new_row['ATR'] = compute_atr(data_with_new_row)

#asign PPO
data_with_new_row['PPO'] = PPO(data_with_new_row.close)

#asign MACD
def compute_macd(close):
    macd = MACD(close)[0]
    return (macd - np.mean(macd))/np.std(macd)
data_with_new_row['MACD'] = compute_macd(data_with_new_row.close)

#asign return
by_sym = data_with_new_row.close
for t in T:
    data_with_new_row[f'r{t:02}'] = by_sym.pct_change(t)

#asign return per quantile
for t in T:
    data_with_new_row[f'r{t:02}dec'] = pd.qcut(data_with_new_row[f'r{t:02}'], 
                                                    q=10, 
                                                    labels=False, 
                                                    duplicates='drop')
#asign year moth weekday
data_with_new_row['year'] = data_with_new_row.index
data_with_new_row['month'] = data_with_new_row.index
data_with_new_row['day'] = data_with_new_row.index
data_with_new_row['weekday'] = data_with_new_row.index

def compute_year(x):
    return x.year

def compute_month(x):
    return x.month

def compute_day(x):
    return x.day

data_with_new_row['year'] = data_with_new_row['year'].apply(lambda x : x.year)
data_with_new_row['month'] = data_with_new_row['month'].apply(lambda x : x.month)
data_with_new_row['day'] = data_with_new_row['day'].apply(lambda x : x.day)
data_with_new_row['weekday'] = data_with_new_row['weekday'].apply(lambda x : x.weekday())

#load lgb model
#i know we use 3 models
list_model = [lgb.Booster(model_file=f'models/model_{current}.txt') for current in range(3)]

#get list all features from model
features = list_model[0].feature_name()

#get df last elem
df_for_predict = data_with_new_row.tail(1).loc[:, features]

predicted_label = [ model.predict(df_for_predict) for model in list_model]

# print (df_for_predict)
print ( predicted_label)
