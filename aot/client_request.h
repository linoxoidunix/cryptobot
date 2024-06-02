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
#include "moodycamel/concurrentqueue.h"

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
    std::string ticker;
    Common::OrderId order_id = Common::OrderId_INVALID;
    Common::Side side        = Common::Side::INVALID;
    double price             = Common::kPRICE_DOUBLE_INVALID;
    double qty               = Common::kQTY_DOUBLE_INVALID;
    uint8_t price_prec       = 0;
    uint8_t qty_prec         = 0;
    auto ToString() const {
        std::string price_string =
            (price != Common::kPRICE_DOUBLE_INVALID)
                ? fmt::format("{:.{}f}", price, price_prec)
                : "INVALID";
        std::string qty_string = (qty != Common::kQTY_DOUBLE_INVALID)
                                     ? fmt::format("{:.{}f}", qty, qty_prec)
                                     : "INVALID";
        return fmt::format(
            "RequestNewOrder[type:{} ticker:{} order_id:{} side:{} qty:{} price:{}]",
            ClientRequestTypeToString(type), ticker, order_id,
            Common::sideToString(side), qty_string, price_string);
    }
};

class RequestCancelOrder {
  public:
    ClientRequestType type = ClientRequestType::CANCEL;
    std::string ticker;
    Common::OrderId order_id = Common::OrderId_INVALID;
    
    auto ToString() const {
        return fmt::format(
            "RequestNewOrder[type:{} ticker:{}]",
            ClientRequestTypeToString(type), ticker);
    }
};

// struct MEClientRequest {
//     ClientRequestType type = ClientRequestType::INVALID;

//     std::string ticker;
//     Common::OrderId order_id = Common::OrderId_INVALID;
//     Common::Side side        = Common::Side::INVALID;
//     double price             = Common::kPRICE_DOUBLE_INVALID;
//     double qty               = Common::kQTY_DOUBLE_INVALID;
//     uint8_t price_prec       = 0;
//     uint8_t qty_prec         = 0;

//     auto toString() const {
//         std::string price_string =
//             (price != Common::kPRICE_DOUBLE_INVALID)
//                 ? fmt::format("{:.{}f}", price, price_prec)
//                 : "INVALID";
//         std::string qty_string = (qty != Common::kQTY_DOUBLE_INVALID)
//                                      ? fmt::format("{:.{}f}", qty, qty_prec)
//                                      : "INVALID";
//         return fmt::format(
//             "MEClientRequest[type:{} ticker:{} side:{} qty:{} price:{}]",
//             Common::clientRequestTypeToString(type), ticker,
//             Common::sideToString(side), qty_string, price_string);
//     }
// };

//#pragma pack(pop)  // Undo the packed binary structure directive moving forward.

/// Lock free queues of matching engine client order request messages.
using RequestNewLimitOrderLFQueue = moodycamel::ConcurrentQueue<RequestNewOrder>;
using RequestCancelOrderLFQueue = moodycamel::ConcurrentQueue<RequestCancelOrder>;

}  // namespace Exchange