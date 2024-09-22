#pragma once 

#include <iostream>
#include <unordered_map>

#include <boost/noncopyable.hpp>

#include "aot/client_response.h"
#include "aot/common/types.h"
/**
 * @brief key - ticker asset
 * value - cuurent number this asset
 *
 */
using WalletAsset = ankerl::unordered_dense::map<common::TickerId, common::QtyD>;

class Wallet : public WalletAsset {
    Wallet &map_ = *this;
    struct ReservedForTicker{
        common::TickerId reserved_ticker = common::kTickerIdInvalid;
        common::Qty reserved_qty = common::kQtyInvalid;
    };
    using ReservedForOrder = std::unordered_map<common::OrderId, ReservedForTicker>;
    ReservedForOrder reserves_;
  public:
    explicit Wallet() = default;
    void Update(const Exchange::IResponse* response) {
        auto type = response->GetType();
        if (type == Exchange::ClientResponseType::ACCEPTED) {
            reserves_.erase(response->GetOrderId());
            return;
        }
        if (type == Exchange::ClientResponseType::CANCEL_REJECTED) {
            reserves_.erase(response->GetOrderId());
            return;
        }
        if (type == Exchange::ClientResponseType::CANCELED) {
            auto local_reserved = reserves_[response->GetOrderId()];
            map_[local_reserved.reserved_ticker] += local_reserved.reserved_qty;
            reserves_.erase(response->GetOrderId());
            return;
        }
        if (type == Exchange::ClientResponseType::FILLED) {
            reserves_.erase(response->GetOrderId());
            if (response->GetSide() == common::Side::BUY) {
                auto ticker = response->GetTradingPair().first;
                InitTicker(ticker);
                if(count(ticker))
                    map_[ticker] += response->GetExecQty();
                return;
            }
            if (response->GetSide() == common::Side::SELL) {
                auto ticker = response->GetTradingPair().second;
                InitTicker(ticker);
                if(count(ticker))
                    map_[ticker] += response->GetExecQty();
                return;
            }
            return;
        }
        if (type == Exchange::ClientResponseType::INVALID) {
            auto local_reserved = reserves_[response->GetOrderId()];
            map_[local_reserved.reserved_ticker] += local_reserved.reserved_qty;
            reserves_.erase(response->GetOrderId());
            return;
        }
        loge("unknown type of response: {}", type);
    };
    bool CanReserve(common::TickerId ticker, common::Qty qty){
        InitTicker(ticker);
        if(at(ticker) >= qty) return true;
        return false;
    }
    bool Reserve(common::OrderId order_id, common::TickerId ticker, common::Qty qty){
        if(!CanReserve(ticker, qty))
            return false;
        reserves_[order_id] = {.reserved_ticker=ticker, .reserved_qty=qty};
        map_[ticker] -= qty; 
        return true;
    }
  private:
    void InitTicker(const common::TickerId& ticker) {
        if (!count(ticker)) [[unlikely]]
            insert({ticker, 0});
    }
};

// namespace exchange{
//     class Wallet : public boost::noncopyable  // Forbid copying{
//         std::unordered_map<common::ExchangeId,  Wallet> wallets_;
//         public:
//             explicit Wallet(std::unordered_map<common::ExchangeId,  Wallet>& wallets):wallets_(wallets){}
//             void Update(const Exchange::MEClientResponse* response) {
//                 auto exchange_id = esponse->exchange_id
//                 if(exchange_id == common::kExchangeIdInvalid){
//                     logw("response->exchange_id = invalid id")
//                     return;
//                 }
//                 if(!wallets_.count(exchange_id)){
//                     logw("don't found wallet for exchange_id:{}", exchange_id)
//                     return;
//                 }
//                 wallets_[exchange_id].Update(response);
//             }
//             bool CheckBalance(common::ExchangeId exchange_id, common::TradingPair& pair, common::Side side, common::Price price, common::Qty qty){
//                 if(exchange_id == common::kExchangeIdInvalid){
//                     logw("pair.first = invalid id")
//                     return;
//                 }
//                 if(!wallets_.count(exchange_id)){
//                     logw("don't found wallet for exchange_id:{}", exchange_id)
//                     return;
//                 }
//                 auto& wallet = wallets_[exchange_id];
//                 if(side == common::Side::BUY){
//                     if(wallet.SafetyGetNumberAsset(pair.second) < qty * price){
//                         logw("balance not enough for buy order")
//                         return false;
//                     }
//                     return true;
//                 }
//                 if(side == common::Side::SELL){
//                     if(wallet.SafetyGetNumberAsset(pair.first) < qty * price){
//                         logw("balance not enough for sell order")
//                         return false;
//                     }
//                     return true;
//                 }               
//                 return false;
//             }
// }