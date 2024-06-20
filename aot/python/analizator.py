import warnings
warnings.filterwarnings('ignore')

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import talib
from talib import RSI, BBANDS, MACD, ATR

MONTH = 21
YEAR = 12 * MONTH

START = '2020-01-01'
END = '2023-01-01'

sns.set_style('darkgrid')
idx = pd.IndexSlice

percentiles = [.001, .01, .02, .03, .04, .05]
percentiles += [1-p for p in percentiles[::-1]]

T = [1, 5, 10, 21, 42, 63]

df = pd.read_csv('data/ohlcv.csv', sep='\t')

print(df.tail())