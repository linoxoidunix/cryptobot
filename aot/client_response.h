/**
 * @file client_response.h
 * @author your name (you@domain.com)
 * @brief responce from exchange to for gateway module
 * @version 0.1
 * @date 2024-06-01
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <sstream>

#include "aot/bus/bus_event.h"
#include "aot/common/types.h"
#include "aot/common/mem_pool.h"
#include "concurrentqueue.h"

namespace bus{
    class Component;
};

namespace Exchange {
/// Type of the order response sent by the exchange to the trading client.
enum class ClientResponseType : uint8_t {
    INVALID         = 0,
    /**
     * @brief clear reserved money
     * 
     */
    ACCEPTED        = 1,
    CANCELED        = 2,
    FILLED          = 3,
    /**
     * @brief order can't be canceled
     * 
     */
    CANCEL_REJECTED = 4
};

inline std::string ClientResponseTypeToString(ClientResponseType type) {
    switch (type) {
        case ClientResponseType::ACCEPTED:
            return "ACCEPTED";
        case ClientResponseType::CANCELED:
            return "CANCELED";
        case ClientResponseType::FILLED:
            return "FILLED";
        case ClientResponseType::CANCEL_REJECTED:
            return "CANCEL_REJECTED";
        case ClientResponseType::INVALID:
            return "INVALID";
    }
    return "UNKNOWN";
}

/// These structures go over the wire / network, so the binary structures are
/// packed to remove system dependent extra padding.
#pragma pack(push, 1)

class IResponse {
  public:
    virtual ~IResponse()                               = default;

    virtual common::Side GetSide() const               = 0;
    virtual common::Qty GetExecQty() const             = 0;
    virtual common::Qty GetLeavesQty() const           = 0;
    virtual common::Price GetPrice() const             = 0;
    virtual std::string ToString() const               = 0;
    virtual common::TradingPair GetTradingPair() const = 0;
    virtual common::ExchangeId GetExchangeId() const   = 0;
    virtual common::OrderId GetOrderId() const = 0;
    virtual void Deallocate()                           = 0;
    virtual Exchange::ClientResponseType GetType() const = 0;

};

/// Client response structure used internally by the matching engine.
struct MEClientResponse;
using ResponsePool = common::MemPool<MEClientResponse>;

struct MEClientResponse : public IResponse {
    /**
     * @brief PriceQty first - price, second - qty
     *
     */
    using PriceQty                 = std::pair<double, double>;
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    ClientResponseType type        = ClientResponseType::INVALID;
    common::TradingPair trading_pair;
    common::OrderId order_id = common::kOrderIdInvalid;
    common::Side side        = common::Side::INVALID;
    common::Price price      = common::kPriceInvalid;
    common::Qty exec_qty     = common::kQtyInvalid;
    common::Qty leaves_qty   = common::kQtyInvalid;
    Exchange::ResponsePool* mem_pool = nullptr;
    std::string ToString() const override {
        auto PrintAsCancelled = [this]() {
            return fmt::format(
                "MEClientResponse[type:{} {} order_id:{}]",
                ClientResponseTypeToString(type), trading_pair.ToString(),
                common::orderIdToString(order_id));
        };
        if (type == ClientResponseType::CANCELED) return PrintAsCancelled();
        std::string price_string      = (price != common::kPriceInvalid)
                                            ? fmt::format("{}", price)
                                            : "INVALID";
        std::string exec_qty_string   = (exec_qty != common::kQtyInvalid)
                                            ? fmt::format("{}", exec_qty)
                                            : "INVALID";

        std::string leaves_qty_string = (leaves_qty != common::kQtyInvalid)
                                            ? fmt::format("{}", leaves_qty)
                                            : "INVALID";
        return fmt::format(
            "MEClientResponse[type:{} {} order_id:{} side:{} "
            "exec_qty:{} "
            "leaves_qty:{} price:{}]",
            ClientResponseTypeToString(type), trading_pair.ToString(),
            common::orderIdToString(order_id), sideToString(side),
            exec_qty_string, leaves_qty_string, price_string);
    }
    common::Side GetSide() const override { return side; };
    common::Qty GetExecQty() const override { return exec_qty; };
    common::Qty GetPrice() const override { return price; };
    common::TradingPair GetTradingPair() const override {
        return trading_pair;
    };
    common::ExchangeId GetExchangeId() const override { return exchange_id; };
    void Deallocate() override {
        if(!mem_pool)
        {
            logw("mem_pool = nullptr. can.t deallocate MEClientResponse");
                return;
        }
        mem_pool->deallocate(this);
    };
    common::OrderId GetOrderId() const override{
        return order_id;
    };
    Exchange::ClientResponseType GetType() const override {return type;};
    common::Qty GetLeavesQty() const override {return leaves_qty;};

};

/// Client response structure published over the network by the order server.
#pragma pack(pop)  // Undo the packed binary structure directive moving forward.

/// Lock free queues of matching engine client order response messages.
using ClientResponseLFQueue = moodycamel::ConcurrentQueue<MEClientResponse>;

struct BusEventResponse : public bus::Event{
    explicit BusEventResponse(Exchange::IResponse* _response) : response(_response){}
    ~BusEventResponse() override = default;
    Exchange::IResponse* response;
    void Accept(bus::Component* comp) override;
    protected:
    void Deallocate() override{
        logd("Deallocating resources");
        response->Deallocate();
        response = nullptr;
    }


                    //order->Deallocate();

};

}  // namespace Exchange