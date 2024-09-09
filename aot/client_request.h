/**
 * @file client_request.h
 * @author your name (you@domain.com)
 * @brief define struct for sending and receiving gateway module
 * @version 0.1
 * @date 2024-06-01
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <sstream>

#include "aot/common/types.h"
//#include "moodycamel/concurrentqueue.h"//if link as 3rd party
#include "concurrentqueue.h"//if link form source

namespace Exchange {
/// Type of the order request sent by the trading client to the exchange.
enum class ClientRequestType : uint8_t { INVALID = 0, NEW = 1, CANCEL = 2 };

inline std::string ClientRequestTypeToString(ClientRequestType type) {
    switch (type) {
        case ClientRequestType::NEW:
            return "NEW";
        case ClientRequestType::CANCEL:
            return "CANCEL";
        case ClientRequestType::INVALID:
            return "INVALID";
    }
    return "UNKNOWN";
}

/// These structures go over the wire / network, so the binary structures are
/// packed to remove system dependent extra padding.
//#pragma pack(push, 1)

/// Client request structure used internally by the matching engine.
class Request {
  public:
    ClientRequestType type = ClientRequestType::INVALID;
    auto ToString() const {
         return fmt::format(
            "Request[type:{}]",
            ClientRequestTypeToString(type));
    }
};

class RequestNewOrder{
  public:
    ClientRequestType type = ClientRequestType::NEW;
    common::TradingPair trading_pair;
    common::OrderId order_id = common::OrderId_INVALID;
    common::Side side        = common::Side::INVALID;
    double price             = common::kPRICE_DOUBLE_INVALID;
    double qty               = common::kQTY_DOUBLE_INVALID;
    auto ToString() const {
        std::string price_string =
            (price != common::kPRICE_DOUBLE_INVALID)
                ? fmt::format("{:.{}f}", price, 5)
                : "INVALID";
        std::string qty_string = (qty != common::kQTY_DOUBLE_INVALID)
                                     ? fmt::format("{:.{}f}", qty, 5)
                                     : "INVALID";
        return fmt::format(
            "RequestNewOrder[type:{} {} order_id:{} side:{} qty:{} price:{}]",
            ClientRequestTypeToString(type), trading_pair.ToString(), order_id,
            common::sideToString(side), qty_string, price_string);
    }
};

class RequestCancelOrder {
  public:
    ClientRequestType type = ClientRequestType::CANCEL;
    common::TradingPair trading_pair;
    common::OrderId order_id = common::OrderId_INVALID;
    
    auto ToString() const {
        assert(false);
        return fmt::format(
            "RequestNewOrder[type:{} ticker:{} id:{}]",
            ClientRequestTypeToString(type), "", order_id);
    }
};

//#pragma pack(pop)  // Undo the packed binary structure directive moving forward.

/// Lock free queues of matching engine client order request messages.
using RequestNewLimitOrderLFQueue = moodycamel::ConcurrentQueue<RequestNewOrder>;
using RequestCancelOrderLFQueue = moodycamel::ConcurrentQueue<RequestCancelOrder>;

}  // namespace Exchange