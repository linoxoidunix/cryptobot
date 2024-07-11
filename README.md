# cryptobot
bot for algo trading

# Features
1. Post order for Binance and Bybit
2. Capture __raw__ exchange market order book update events for Binance
3. Capture __raw__ Klines for Binance and Bybit  
4. Capture __raw__ order book snapshot for Binance
5. Order book sync snap and diff for binance
6. Get bbo using order book for binance 
7. Order gateway [PARTITIAL DEBUG for BINANCE] 

thanks to https://martin.ankerl.com/2022/08/27/hashmap-bench-01/

# TODO
1. make order manager move_order function
2. for order manager need consider riskmanager 

# Restrictions
1. OrderManager impl supports only 1 active order for each side!!!!
