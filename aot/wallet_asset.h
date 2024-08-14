#pragma once 

#include <iostream>
#include "aot/client_response.h"
#include "aot/common/types.h"
/**
 * @brief key - ticker asset
 * value - cuurent number this asset
 *
 */
using WalletAsset = ankerl::unordered_dense::map<Common::TickerS, Common::QtyD>;

class Wallet : public WalletAsset {
  public:
    explicit Wallet() = default;
    void Update(const Exchange::MEClientResponse* response) {
        if (response->type == Exchange::ClientResponseType::ACCEPTED) {
            if (response->side == Common::Side::BUY) {
                InitTicker(response->ticker);
                if(count(response->ticker))[[likely]]
                    at(response->ticker) += response->exec_qty;
            }
            if (response->side == Common::Side::SELL) {
                InitTicker(response->ticker);
                if(count(response->ticker))[[likely]]
                    at(response->ticker) -= response->exec_qty;
            }
        }
    };
    Common::QtyD SafetyGetNumberAsset(const Common::TickerS& ticker) {
        InitTicker(ticker);
        return at(ticker);
    };

  private:
    void InitTicker(const Common::TickerS& ticker_id) {
        if (!count(ticker_id)) [[unlikely]]
            insert({ticker_id, 0});
    }
};