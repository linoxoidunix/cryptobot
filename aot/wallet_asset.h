#pragma once 

#include <iostream>
#include "aot/client_response.h"
#include "aot/common/types.h"
/**
 * @brief key - ticker asset
 * value - cuurent number this asset
 *
 */
using WalletAsset = ankerl::unordered_dense::map<Common::TradingPair, Common::QtyD, Common::TradingPairHash, Common::TradingPairEqual>;

class Wallet : public WalletAsset {
  public:
    explicit Wallet() = default;
    void Update(const Exchange::MEClientResponse* response) {
        if (response->type == Exchange::ClientResponseType::FILLED) {
            if (response->side == Common::Side::BUY) {
                InitTicker(response->trading_pair);
                if(count(response->trading_pair))[[likely]]
                    at(response->trading_pair) += response->exec_qty;
            }
            if (response->side == Common::Side::SELL) {
                InitTicker(response->trading_pair);
                if(count(response->trading_pair))[[likely]]
                    at(response->trading_pair) -= response->exec_qty;
            }
        }
    };
    Common::QtyD SafetyGetNumberAsset(const Common::TradingPair& trading_pair) {
        InitTicker(trading_pair);
        return at(trading_pair);
    };

  private:
    void InitTicker(const Common::TradingPair& trading_pair) {
        if (!count(trading_pair)) [[unlikely]]
            insert({trading_pair, 0});
    }
};