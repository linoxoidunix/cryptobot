#pragma once

#include <iostream>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "boost/asio/thread_pool.hpp"
#include "boost/asio/strand.hpp"
#include "ankerl/unordered_dense.h"

#include "aot/bus/bus_event.h"
#include "aot/bus/bus_component.h"
#include "aot/common/thread_utils.h"
#include "aot/common/types.h"
#include "aot/client_response.h"
#include "aot/common/types.h"
#include "aot/Logger.h"
#include "boost/noncopyable.hpp"
/**
 * @brief key - ticker asset
 * value - cuurent number this asset
 *
 */

namespace wallet{
    struct BusEventReserveQty : public bus::Event{
        ~BusEventReserveQty() override = default;
        void Accept(bus::Component* comp) override{
            comp->AsyncHandleEvent(this);
        }
        common::ExchangeId exchange_id = common::kExchangeIdInvalid;
        common::OrderId order_id = common::kOrderIdInvalid;
        common::TickerId ticker_id = common::kTickerIdInvalid;
        common::Qty qty = common::kQtyInvalid;
    };
}
using WalletAsset =
    ankerl::unordered_dense::map<common::TickerId, common::QtyD>;

class Wallet : public WalletAsset {
    // Wallet &map_ = *this;
    struct ReservedForTicker {
        common::TickerId reserved_ticker = common::kTickerIdInvalid;
        common::Qty reserved_qty         = common::kQtyInvalid;
    };

  protected:
    using ReservedForOrder =
        std::unordered_map<common::OrderId, ReservedForTicker>;
    ReservedForOrder reserves_;

  public:
    explicit Wallet() = default;
    virtual ~Wallet() = default;
    void Update(Exchange::IResponse* response);
    bool CanReserve(common::TickerId ticker, common::Qty qty) {
        InitTicker(ticker);
        if (at(ticker) >= qty) return true;
        return false;
    }
    bool Reserve(common::OrderId order_id, common::TickerId ticker,
                 common::Qty qty) {
        if (!CanReserve(ticker, qty)) return false;
        reserves_[order_id]  = {.reserved_ticker = ticker, .reserved_qty = qty};
        at(ticker)          -= qty;
        return true;
    }

  private:
    void InitTicker(const common::TickerId& ticker) {
        if (!count(ticker)) [[unlikely]]
            insert({ticker, 0});
    }
};

namespace testing {
class Wallet : public ::Wallet {
  public:
    Wallet::ReservedForOrder GetReserves() const { return reserves_; }
    ~Wallet() override = default;
};
};

namespace exchange {
class Wallet : public boost::noncopyable {  // Forbid copying
    protected:
    std::unordered_map<common::ExchangeId, ::Wallet> wallets_;

  public:
    virtual ~Wallet() = default;
    /**
     * @brief Retrieves the wallet associated with the given exchange ID.
     *
     * This function searches for a wallet corresponding to the provided exchange ID.
     * If found, it returns a pointer to the wallet. If not found, it returns nullptr.
     *
     * @param exchange_id The ID of the exchange for which the wallet is being retrieved.
     * @return ::Wallet* A pointer to the wallet associated with the given exchange ID,
     *                   or nullptr if no such wallet exists.
     */
    ::Wallet* At(common::ExchangeId exchange_id){
        if(!wallets_.count(exchange_id)){
            return nullptr;
        }
        return &wallets_[exchange_id];  
    }

    void InitWallets(
        const std::unordered_set<common::ExchangeId>& exchange_ids) {
        for (const auto& id : exchange_ids) {
            wallets_.try_emplace(id, ::Wallet());
        }
    }
    void Update(Exchange::IResponse* response) {
        auto exchange_id = response->GetExchangeId();
        if (exchange_id == common::kExchangeIdInvalid) {
            logw("response->exchange_id = invalid id");
            return;
        }
        if (!wallets_.contains(exchange_id)) {
            logw("don't found wallet for exchange_id:{}", exchange_id);
            return;
        }
        wallets_[exchange_id].Update(response);
    }
    bool CanReserve(const common::ExchangeId exchange_id,
                    common::TickerId ticker, common::Qty qty) {
        auto it = wallets_.find(exchange_id);
        if (it == wallets_.end()) {
            loge("can't find exchange id:{}", exchange_id);
            return false;
        }
        return it->second.CanReserve(ticker, qty);
    }
    bool Reserve(const common::ExchangeId exchange_id, common::OrderId order_id,
                 common::TickerId ticker, common::Qty qty) {
        auto it = wallets_.find(exchange_id);
        if (it == wallets_.end()) {
            loge("can't find exchange id:{}", exchange_id);
            return false;
        }
        return it->second.Reserve(order_id, ticker, qty);
    }
    bool OnNewSignal(wallet::BusEventReserveQty* event) {
        auto exchange_id = event->exchange_id;
        if (exchange_id== common::kExchangeIdInvalid) {
            logw("event->exchange_id = invalid id");
            return false;
        }
        return Reserve(exchange_id, event->order_id, event->ticker_id, event->qty);
    }
};
};

template<typename Executor>
class WalletComponent : public bus::Component{
    Executor executor_;

    exchange::Wallet *wallet_         = nullptr;
  public:
    explicit WalletComponent(Executor&& executor, exchange::Wallet *wallet)
        : executor_(std::move(executor)),wallet_(wallet) {}
    
    ~WalletComponent() override = default;
    
    void AsyncHandleEvent(wallet::BusEventReserveQty* event) override{
         boost::asio::post(executor_, [this, event]() {
            wallet_->OnNewSignal(event);
            event->Release();
        });
    }

    void AsyncHandleEvent(Exchange::BusEventResponse* event) override{
         boost::asio::post(executor_, [this, event]() {
            wallet_->Update(event->response);
            event->Release();
        });
    }
};



namespace testing {
    namespace exchange{
        class Wallet : public ::exchange::Wallet {
            public:
                std::unordered_map<common::ExchangeId, ::Wallet> GetWallets() const {
                    return wallets_;
                }
        };
    };
};