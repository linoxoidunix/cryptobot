import warnings
warnings.filterwarnings('ignore')

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import talib
from talib import RSI, BBANDS, MACD, ATR, NATR, PPO
from dateutil.parser import parse

MONTH = 21
YEAR = 12 * MONTH

START = '2020-01-01'
END = '2023-12-31'

sns.set_style('darkgrid')
idx = pd.IndexSlice

percentiles = [.001, .01, .02, .03, .04, .05]
percentiles += [1-p for p in percentiles[::-1]]

T = [1, 5, 10, 21, 42, 63]

prices = pd.read_csv('data/ohlcv.csv', sep='\t')
#prices2 = pd.read_csv('data/ohlcv2.csv', sep='\t')


frames = [prices]

prices = pd.concat(frames)

prices['dollar_vol'] = prices[['close', 'volume']].prod(1).div(1e3)
dollar_vol_ma = (prices
                 .dollar_vol
                 .rolling(window=21, min_periods=1) # 1 trading month
                 .mean())
#print(dollar_vol_ma.rank())
prices['dollar_vol_rank'] = (dollar_vol_ma
                            .rank(axis=0, ascending=False)
                            )
prices['rsi'] = RSI(prices['close'])

grid = sns.displot(prices.rsi.dropna())
sns.despine()
plt.tight_layout();
# plt.show()

prices['bb_high'] = BBANDS(prices['close'], timeperiod=20)[0]
prices['bb_low'] = BBANDS(prices['close'], timeperiod=20)[2]

prices['bb_high'] = prices.bb_high.sub(prices.close).div(prices.bb_high).apply(np.log1p)
prices['bb_low'] = prices.close.sub(prices.bb_low).div(prices.close).apply(np.log1p)

prices['NATR'] = NATR(prices.high, prices.low, prices.close)

def compute_atr(stock_data):
    df = ATR(stock_data.high, stock_data.low, 
             stock_data.close, timeperiod=14)
    return df.sub(df.mean()).div(df.std())

prices['ATR'] = compute_atr(prices)
prices['PPO'] = PPO(prices.close)

def compute_macd(close):
    macd = MACD(close)[0]
    return (macd - np.mean(macd))/np.std(macd)

prices['MACD'] = compute_macd(prices.close)

by_sym = prices.close
for t in T:
    prices[f'r{t:02}'] = by_sym.pct_change(t)


#print(prices[f'r{63:02}'].to_numpy)
for t in T:
    prices[f'r{t:02}dec'] = pd.qcut(prices[f'r{t:02}'], 
                                                    q=10, 
                                                    labels=False, 
                                                    duplicates='drop')
    
for t in [1, 5, 21]:
    prices[f'r{t:02}_fwd'] = prices[f'r{t:02}'].shift(-t)

prices['ts_parsed'] = [parse(x, fuzzy=True) for x in prices['timestamp']]
prices['year'] = prices['ts_parsed']
prices['month'] = prices['ts_parsed']
prices['day'] = prices['ts_parsed']
prices['weekday'] = prices['ts_parsed']


def compute_year(x):
    timestamp = parse(x, fuzzy=True)
    return timestamp.year

def compute_month(x):
    timestamp = parse(x, fuzzy=True)
    return timestamp.month

def compute_day(x):
    timestamp = parse(x, fuzzy=True)
    return timestamp.day

prices['year'] = prices['year'].apply(lambda x : x.year)
prices['month'] = prices['month'].apply(lambda x : x.month)
prices['day'] = prices['day'].apply(lambda x : x.day)
prices['weekday'] = prices['weekday'].apply(lambda x : x.weekday())
prices.drop(['open', 'close', 'low', 'high', 'volume'], axis=1).to_hdf('data/data.h5', 'model_data')
# print(prices.info())


# print(prices.dropna())