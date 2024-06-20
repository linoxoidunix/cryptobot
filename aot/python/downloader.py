import binance_ohlcv as bo
from datetime import date

df1 = bo.get_spot(symbol='BTCUSDT', timeframe='1s', start=date(2023, 1, 1), end=date(2023, 1, 2))
df1.to_csv('ohlcv.csv', sep='\t')
print(df1.tail())
