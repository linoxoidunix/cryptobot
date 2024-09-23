#include "magic_enum.hpp"
#include "aot/wallet_asset.h"

void Wallet::Update(const Exchange::IResponse* response){
    auto type = response->GetType();
    auto executed_qty = response->GetExecQty();

    if (type == Exchange::ClientResponseType::ACCEPTED) {
        reserves_.erase(response->GetOrderId());
        return;
    }
    if (type == Exchange::ClientResponseType::CANCEL_REJECTED) {
        reserves_.erase(response->GetOrderId());
        return;
    }
    if (type == Exchange::ClientResponseType::CANCELED) {
        auto order_id = response->GetOrderId();
        auto local_reserved = reserves_[order_id];
        at(local_reserved.reserved_ticker) += local_reserved.reserved_qty;
        reserves_.erase(order_id);
        return;
    }
    if (type == Exchange::ClientResponseType::FILLED) {
        reserves_.erase(response->GetOrderId());
        auto side = response->GetSide();
        if (side == common::Side::BUY) {
            auto ticker = response->GetTradingPair().first;
            InitTicker(ticker);
            if(count(ticker))
                at(ticker) += executed_qty;
            return;
        }
        if (side == common::Side::SELL) {
            auto ticker = response->GetTradingPair().second;
            InitTicker(ticker);
            if(count(ticker))
                at(ticker) += executed_qty;
            return;
        }
        return;
    }
    if (type == Exchange::ClientResponseType::INVALID) {
        auto order_id = response->GetOrderId();
        if(!reserves_.contains(order_id))
        {
            loge("reserves_ not contain order id:{} for type:{}", order_id, magic_enum::enum_name(type));
            return;
        }
        auto local_reserved = reserves_[order_id];
        at(local_reserved.reserved_ticker) += local_reserved.reserved_qty;
        reserves_.erase(order_id);
        return;
    }
    loge("unknown type of response:{} with name:{}", static_cast<uint8_t>(type), magic_enum::enum_name(type));
};