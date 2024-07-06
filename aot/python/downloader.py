import binance_ohlcv as bo
from datetime import date
from scipy import stats

df1 = bo.get_spot(symbol='BTCUSDT', timeframe='6h', start=date(2020, 1, 1), end=date(2023, 12, 31))
df1.to_csv('ohlcv.csv', sep='\t')
print(df1.tail())
