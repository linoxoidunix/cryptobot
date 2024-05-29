#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <sstream>

#include "aot/Logger.h"
#include "aot/common/macros.h"

namespace Common {
constexpr size_t ME_MAX_TICKERS        = 8;

constexpr size_t ME_MAX_CLIENT_UPDATES = 256 * 1024;
constexpr size_t ME_MAX_MARKET_UPDATES = 256 * 1024;

constexpr size_t ME_MAX_NUM_CLIENTS    = 256;
constexpr size_t ME_MAX_ORDER_IDS      = 1024 * 1024;
constexpr size_t ME_MAX_ORDERS_AT_PRICE  = 5000 * 2;//for binance max depth for bid is 5000.//for binance max depth for ask is 5000.

typedef uint64_t OrderId;
constexpr auto OrderId_INVALID = std::numeric_limits<OrderId>::max();

inline auto orderIdToString(OrderId order_id) -> std::string {
    if (UNLIKELY(order_id == OrderId_INVALID)) {
        return "INVALID";
    }

    return std::to_string(order_id);
}

typedef uint32_t TickerId;
constexpr auto TickerId_INVALID = std::numeric_limits<TickerId>::max();

inline auto tickerIdToString(TickerId ticker_id) -> std::string {
    if (UNLIKELY(ticker_id == TickerId_INVALID)) {
        return "INVALID";
    }

    return std::to_string(ticker_id);
}

typedef uint32_t ClientId;
constexpr auto ClientId_INVALID = std::numeric_limits<ClientId>::max();

inline auto clientIdToString(ClientId client_id) -> std::string {
    if (UNLIKELY(client_id == ClientId_INVALID)) {
        return "INVALID";
    }

    return std::to_string(client_id);
}

using Price                  = uint64_t;
constexpr auto Price_INVALID = std::numeric_limits<Price>::max();

inline auto priceToString(Price price) -> std::string {
    if (UNLIKELY(price == Price_INVALID)) {
        return "INVALID";
    }

    return std::to_string(price);
}

using Qty                  = uint64_t;
constexpr auto Qty_INVALID = std::numeric_limits<Qty>::max();

inline auto qtyToString(Qty qty) -> std::string {
    if (UNLIKELY(qty == Qty_INVALID)) {
        return "INVALID";
    }

    return std::to_string(qty);
}

using Priority                  = uint64_t;
constexpr auto Priority_INVALID = std::numeric_limits<Priority>::max();

inline auto priorityToString(Priority priority) -> std::string {
    if (UNLIKELY(priority == Priority_INVALID)) {
        return "INVALID";
    }

    return std::to_string(priority);
}

enum class Side : int8_t { INVALID = 0, BUY = 1, SELL = -1, MAX = 2 };

inline auto sideToString(Side side) -> std::string {
    switch (side) {
        case Side::BUY:
            return "BUY";
        case Side::SELL:
            return "SELL";
        case Side::INVALID:
            return "INVALID";
        case Side::MAX:
            return "MAX";
    }

    return "UNKNOWN";
}

inline constexpr auto sideToIndex(Side side) noexcept {
    return static_cast<size_t>(side) + 1;
}

inline constexpr auto sideToValue(Side side) noexcept {
    return static_cast<int>(side);
}

enum class AlgoType : int8_t {
    INVALID = 0,
    RANDOM  = 1,
    MAKER   = 2,
    TAKER   = 3,
    MAX     = 4
};

inline auto algoTypeToString(AlgoType type) -> std::string {
    switch (type) {
        case AlgoType::RANDOM:
            return "RANDOM";
        case AlgoType::MAKER:
            return "MAKER";
        case AlgoType::TAKER:
            return "TAKER";
        case AlgoType::INVALID:
            return "INVALID";
        case AlgoType::MAX:
            return "MAX";
    }

    return "UNKNOWN";
}

inline auto stringToAlgoType(const std::string &str) -> AlgoType {
    for (auto i = static_cast<int>(AlgoType::INVALID);
         i <= static_cast<int>(AlgoType::MAX); ++i) {
        const auto algo_type = static_cast<AlgoType>(i);
        if (algoTypeToString(algo_type) == str) return algo_type;
    }

    return AlgoType::INVALID;
}

struct RiskCfg {
    Qty max_order_size_ = 0;
    Qty max_position_   = 0;
    double max_loss_    = 0;

    auto toString() const {
        std::stringstream ss;

        ss << "RiskCfg{" << "max-order-size:" << qtyToString(max_order_size_)
           << " " << "max-position:" << qtyToString(max_position_) << " "
           << "max-loss:" << max_loss_ << "}";

        return ss.str();
    }
};

struct TradeEngineCfg {
    Qty clip_         = 0;
    double threshold_ = 0;
    RiskCfg risk_cfg_;

    auto toString() const {
        std::stringstream ss;
        ss << "TradeEngineCfg{" << "clip:" << qtyToString(clip_) << " "
           << "thresh:" << threshold_ << " " << "risk:" << risk_cfg_.toString()
           << "}";

        return ss.str();
    }
};

using TradeEngineCfgHashMap = std::array<TradeEngineCfg, ME_MAX_TICKERS>;

inline uint32_t Digits10(uint64_t v) {
    return 1 + (std::uint32_t)(v >= 10) + (std::uint32_t)(v >= 100) +
           (std::uint32_t)(v >= 1000) + (std::uint32_t)(v >= 10000) +
           (std::uint32_t)(v >= 100000) + (std::uint32_t)(v >= 1000000) +
           (std::uint32_t)(v >= 10000000) + (std::uint32_t)(v >= 100000000) +
           (std::uint32_t)(v >= 1000000000) +
           (std::uint32_t)(v >= 10000000000ull) +
           (std::uint32_t)(v >= 100000000000ull) +
           (std::uint32_t)(v >= 1000000000000ull) +
           (std::uint32_t)(v >= 10000000000000ull) +
           (std::uint32_t)(v >= 100000000000000ull) +
           (std::uint32_t)(v >= 1000000000000000ull) +
           (std::uint32_t)(v >= 10000000000000000ull) +
           (std::uint32_t)(v >= 100000000000000000ull) +
           (std::uint32_t)(v >= 1000000000000000000ull) +
           (std::uint32_t)(v >= 10000000000000000000ull);
};

inline uint32_t LeengthFractionalPart(double v) {
    auto v0  = v * 1;
    auto v1  = v * 10;
    auto v2  = v * 100;
    auto v3  = v * 1000;
    auto v4  = v * 10000;
    auto v5  = v * 100000;
    auto v6  = v * 1000000;
    auto v7  = v * 10000000;
    auto v8  = v * 100000000;
    auto v9  = v * 1000000000;
    auto v10 = v * 10000000000;
    double part;
    auto xx0 = (std::modf(v, &part) == 0);
    if 
        (xx0) [[likely]] return 0;
    auto xx1 = (std::modf(v * 10, &part) == 0);
    if 
        (xx1) [[likely]] return 1;
    auto xx2 = (std::modf(v * 100, &part) == 0);
    if 
        (xx2) [[likely]] return 2;
    auto xx3 = (std::modf(v * 1000, &part) == 0);
    if (xx3) return 3;
    auto xx4 = (std::modf(v * 10000, &part) == 0);
    if (xx4) return 4;
    auto xx5 = (std::modf(v * 100000, &part) == 0);
    if (xx5) return 5;
    auto xx6 = (std::modf(v * 1000000, &part) == 0);
    if (xx6) return 6;
    auto xx7 = (std::modf(v * 10000000, &part) == 0);
    if (xx7) return 7;
    auto xx8 = (std::modf(v * 100000000, &part) == 0);
    if (xx8) return 8;
    auto xx9 = (std::modf(v * 1000000000, &part) == 0);
    if (xx9) return 9;
    auto xx10 = (std::modf(v * 10000000000, &part) == 0);
    if (xx10) return 10;
    auto xx11 = (std::modf(v * 100000000000, &part) == 0);
    if (xx11) return 11;
    auto xx12 = (std::modf(v * 1000000000000, &part) == 0);
    if (xx12) return 12;
    auto xx13 = (std::modf(v * 10000000000000, &part) == 0);
    if (xx13) return 13;
    auto xx14 = (std::modf(v * 100000000000000, &part) == 0);
    if (xx14) return 14;
    auto xx15 = (std::modf(v * 1000000000000000, &part) == 0);
    if (xx15) return 15;
    auto xx16 = (std::modf(v * 10000000000000000, &part) == 0);
    if (xx16) return 16;
    auto xx17 = (std::modf(v * 100000000000000000, &part) == 0);
    if (xx17) return 17;
    auto xx18 = (std::modf(v * 1000000000000000000, &part) == 0);
    if (xx18) return 18;
    ASSERT(false, "presiccion error");
    loge("presiccion error");
    return 0;
};
}  // namespace Common