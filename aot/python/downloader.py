import binance_ohlcv as bo
from datetime import date
from scipy import stats

df1 = bo.get_spot(symbol='BTCUSDT', timeframe='5m', start=date(2017, 1, 1), end=date(2023, 12, 31))
df1.to_csv('ohlcv_new.csv', sep='\t')
print(df1.tail())
