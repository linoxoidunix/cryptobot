#include <iostream>

#include "simdjson.h"
#include "aot/Binance.h"
#include "aot/client_response.h"
#include "aot/Logger.h"
#include <set>
#include <string>
using namespace simdjson;

// int main(void) {
//     ondemand::parser parser;
//     padded_string json        = padded_string::load("twitter.json");
//     ondemand::document tweets = parser.iterate(json);
//     std::cout << std::string_view(tweets["k"]["o"]);
// }

/**
 * @brief example how parse bybit kline
 * 
 * @return int 
 */
// int main(void) {
//     ondemand::parser parser;
//     auto json = R"({"type":"snapshot","topic":"kline.1.BTCUSDT","data":[{"start":1714102140000,"end":1714102199999,"interval":"1","open":"50803.82","close":"50803.82","high":"50803.84","low":"50803.82","volume":"0.51374","turnover":"26099.95921086","confirm":false,"timestamp":1714102192562}],"ts":1714102192562})"_padded;
//     ondemand::document tweets = parser.iterate(json);
//     for(auto value : tweets["data"])
//     {
//         std::cout << value["open"] << std::endl;
//         std::cout << value["high"] << std::endl;
//         std::cout << value["low"] << std::endl;
//         std::cout << value["close"] << std::endl;
//         std::cout << value["volume"] << std::endl;
//     }
//     return 0;
// }

int main(void) {

            std::set<std::string> success_status{"NEW", "PARTIALLY_FILLED",
                                             "FILLED"};
                auto xxx = success_status.count(std::string("NEW"));
                                     
    fmtlog::setLogLevel(fmtlog::DBG);
    auto json = R"({"symbol":"BTCUSDT","orderId":14759058,"orderListId":-1,"clientOrderId":"5","transactTime":1717451079806,"price":"40000.00000000","origQty":"0.00100000","executedQty":"0.00000000","cummulativeQuoteQty":"0.00000000","status":"NEW","timeInForce":"GTC","type":"LIMIT","side":"BUY","workingTime":1717451079806,"fills":[],"selfTradePreventionMode":"EXPIRE_MAKER"})"_padded;
    Common::TradingPairReverseHashMap pairs_reverse;
    pairs_reverse["btcusdt"]=Common::TradingPair{2,1};
    binance::OrderNewLimit::ParserResponse parser(pairs_reverse);
    auto result = parser.Parse(json);
    logd("{}", result.ToString());
   
    return 0;
}
