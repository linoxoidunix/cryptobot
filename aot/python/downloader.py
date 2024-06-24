import binance_ohlcv as bo
from datetime import date

df1 = bo.get_spot(symbol='BTCUSDT', timeframe='1d', start=date(2022, 1, 1), end=date(2023, 1, 1))
df1.to_csv('ohlcv1.csv', sep='\t')
print(df1.tail())
