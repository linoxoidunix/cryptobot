#include "aot/common/types.h"

common::TradingPairReverseHashMap common::InitTPsJR(
    const common::TradingPairHashMap &pairs) {
    common::TradingPairReverseHashMap map;
    for (auto &it : pairs) {
        map[it.second.https_json_request] = it.first;
    }
    return map;
}