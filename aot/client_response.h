/**
 * @file client_responce.h
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

#include "aot/common/types.h"
#include "moodycamel/concurrentqueue.h"

namespace Exchange {
/// Type of the order response sent by the exchange to the trading client.
enum class ClientResponseType : uint8_t {
    INVALID         = 0,
    ACCEPTED        = 1,
    CANCELED        = 2,
    FILLED          = 3,
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

/// Client response structure used internally by the matching engine.
struct MEClientResponse {
    ClientResponseType type = ClientResponseType::INVALID;
    std::string ticker;
    // OrderId client_order_id_ = OrderId_INVALID;
    // OrderId market_order_id_ = OrderId_INVALID;
    Common::Side side = Common::Side::INVALID;
    double price      = Common::kPRICE_DOUBLE_INVALID;
    double exec_qty   = Common::kQTY_DOUBLE_INVALID;
    double leaves_qty = Common::kQTY_DOUBLE_INVALID;

    auto toString() const {
        return fmt::format(
            "MEClientResponse[type:{} ticker:{} side:{} exec_qty:{} "
            "leaves_qty:{} price:{}]",
            ClientResponseTypeToString(type), ticker, sideToString(side),
            exec_qty, leaves_qty, price);
    }
};

/// Client response structure published over the network by the order server.
#pragma pack(pop)  // Undo the packed binary structure directive moving forward.

/// Lock free queues of matching engine client order response messages.
using ClientResponseLFQueue = moodycamel::ConcurrentQueue<MEClientResponse>;
}  // namespace Exchange