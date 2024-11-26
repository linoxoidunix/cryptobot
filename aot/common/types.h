#pragma once

#include <array>
#include <climits>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>

#include "magic_enum/magic_enum.hpp"
#include "aot/Logger.h"
#include "aot/common/macros.h"
#include "aot/third_party/emhash/hash_table7.hpp"

namespace common {
constexpr size_t ME_MAX_TICKERS        = 8;

constexpr size_t ME_MAX_CLIENT_UPDATES = 256 * 1024;
constexpr size_t ME_MAX_MARKET_UPDATES = 256 * 1024;

constexpr size_t ME_MAX_NUM_CLIENTS    = 256;
constexpr size_t ME_MAX_ORDER_IDS      = 1024 * 1024;
constexpr size_t ME_MAX_ORDERS_AT_PRICE =
    50000 * 2;  // for binance max depth for bid is 5000.//for binance max depth
                // for ask is 5000.

enum class ExchangeId{
  kBinance,
  kBybit,
  kMexc,
  kInvalid
};

constexpr auto kExchangeIdInvalid = ExchangeId::kInvalid;

using FrequencyMS = uint64_t;
constexpr auto kFrequencyMSInvalid   = std::numeric_limits<FrequencyMS>::max();


using OrderId                     = uint64_t;
constexpr auto kOrderIdInvalid    = std::numeric_limits<OrderId>::max();

/**
 * @brief PriceD = price double
 *
 */
// using PriceD                         = double;
/**
 * @brief QtyD = qty double
 *
 */
using QtyD                        = double;
/**
 * @brief TickerS = ticker string
 *
 */
using TickerS                     = std::string;
using TradingPairS                = std::string;

inline auto orderIdToString(OrderId order_id) -> std::string {
    if (UNLIKELY(order_id == common::kOrderIdInvalid)) {
        return "INVALID";
    }

    return std::to_string(order_id);
}

typedef uint32_t TickerId;
constexpr auto TickerId_INVALID = std::numeric_limits<TickerId>::max();
constexpr auto kTickerIdInvalid = std::numeric_limits<TickerId>::max();

inline auto tickerIdToString(TickerId ticker_id) -> std::string {
    if ((ticker_id == TickerId_INVALID)) {
        return "INVALID";
    }

    return std::to_string(ticker_id);
}

typedef std::string TickerString;

typedef uint32_t ClientId;
constexpr auto ClientId_INVALID = std::numeric_limits<ClientId>::max();

inline auto clientIdToString(ClientId client_id) -> std::string {
    if ((client_id == ClientId_INVALID)) [[unlikely]] {
        return "INVALID";
    }

    return std::to_string(client_id);
}

using Price                     = uint64_t;
using PriceD                    = double;
constexpr Price kPriceInvalid   = std::numeric_limits<Price>::max();
constexpr PriceD kPriceDInvalid = std::numeric_limits<PriceD>::max();

inline auto priceToString(Price price) -> std::string {
    if (price == kPriceInvalid) [[unlikely]] {
        return "INVALID";
    }

    return std::to_string(price);
}

using Qty                  = uint64_t;
constexpr auto kQtyInvalid = std::numeric_limits<Qty>::max();

inline auto qtyToString(Qty qty) -> std::string {
    if (qty == kQtyInvalid) [[unlikely]] {
        return "INVALID";
    }

    return std::to_string(qty);
}

using Priority                  = uint64_t;
constexpr auto Priority_INVALID = std::numeric_limits<Priority>::max();

inline auto priorityToString(Priority priority) -> std::string {
    if (priority == Priority_INVALID) [[unlikely]] {
        return "INVALID";
    }

    return std::to_string(priority);
}

/**
 * @brief side as taker
 * 
 */
enum class Side : int8_t { INVALID = 0, BUY = 1, SELL = -1, MAX = 2 };

enum class TradeAction {
    kEnterLong,
    kExitLong,
    kEnterShort,
    kExitShort,
    kNope
};

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

inline auto stringToAlgoType(const std::string& str) -> AlgoType {
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
    Qty clip         = 0;
    double threshold = 0;
    RiskCfg risk_cfg;

    auto toString() const {
        std::stringstream ss;
        ss << "TradeEngineCfg{" << "clip:" << qtyToString(clip) << " "
           << "thresh:" << threshold << " " << "risk:" << risk_cfg.toString()
           << "}";

        return ss.str();
    }
};

// using TradeEngineCfgHashMap = std::array<TradeEngineCfg, ME_MAX_TICKERS>;

struct StringHash {
    using is_transparent = void;
    std::size_t operator()(const char* key) const {
        return std::hash<std::string_view>()(
            std::string_view(key, strlen(key)));
    }
    std::size_t operator()(const std::string& key) const {
        return std::hash<std::string_view>()(key);
    }
    std::size_t operator()(const std::string_view& key) const {
        return std::hash<std::string_view>()(key);
    }
};

struct StringEqual {
    using is_transparent = int;

    bool operator()(const std::string_view& lhs, const std::string& rhs) const {
        return lhs == rhs;
    }

    bool operator()(const std::string& lhs, const std::string& rhs) const {
        return lhs == rhs;
    }

    bool operator()(const char* lhs, const std::string& rhs) const {
        return std::strcmp(lhs, rhs.data()) == 0;
    }
    bool operator()(const std::string& rhs, const char* lhs) const {
        return std::strcmp(lhs, rhs.data()) == 0;
    }
};

struct TradingPair {
    common::TickerId first;
    common::TickerId second;
    friend bool operator==(const TradingPair& left, const TradingPair& right) {
        if (left.first == right.first && left.second == right.second)
            return true;
        return false;
    }
    TradingPair(const common::TradingPair& pair)
        : first(pair.first), second(pair.second) {}
    TradingPair() = default;
    TradingPair(common::TickerId _first, common::TickerId _second): first(_first), second(_second){};
    auto ToString() const {
        return fmt::format("TradingPair[f:{} s:{}]", first, second);
    }
};

struct TradingPairInfo {
    //common::TradingPairS trading_pairs;
    uint8_t price_precission;
    uint8_t qty_precission;
    /**
     * @brief if you want pass trading pair to https request with query
     * 
     */
    common::TradingPairS https_json_request;
    /**
     * @brief if you want pass trading pair to https request with query
     * 
     */
    common::TradingPairS https_query_request;
    /**
     * @brief if you want pass trading pair to ws request with query
     * 
    */
    common::TradingPairS ws_query_request;
    /**
     * @brief if you want parse trading pair in https response in json
     * 
    */
    common::TradingPairS https_query_response;

};

struct TradingPairHash {
    using is_transparent = void;
    std::size_t operator()(const TradingPair key) const {
        std::size_t h1 = std::hash<common::TickerId>{}(key.first);
        std::size_t h2 = std::hash<common::TickerId>{}(key.second);
        return h1 ^ (h2 << 1);  // or use boost::hash_combine
    }
    std::size_t operator()(const TradingPair* key) const {
        std::size_t h1 = std::hash<common::TickerId>{}(key->first);
        std::size_t h2 = std::hash<common::TickerId>{}(key->second);
        return h1 ^ (h2 << 1);  // or use boost::hash_combine
    }
};

struct TradingPairEqual {
    using is_transparent = int;

    bool operator()(TradingPair lhs, TradingPair rhs) const {
        return lhs == rhs;
    }
};

using TradingPairHashMap = emhash7::HashMap<TradingPair, TradingPairInfo,
                                            TradingPairHash, TradingPairEqual>;

/**
 * @brief ExchangeTPs = Exchange Trading PairS
 * 
 */
using ExchangeTPs = std::unordered_map<common::ExchangeId, TradingPairHashMap>;

using TradingPairReverseHashMap =
    emhash7::HashMap<common::TickerS, common::TradingPair, StringHash,
                     StringEqual>;

common::TradingPairReverseHashMap InitTPsJR(const common::TradingPairHashMap &pairs);


/**
 * @brief ExchangeTPsR = Exchange Trading PairS Json Reverse used for finding common::TradingPair by std::string
 * 
 */
using ExchangeTPsJR = std::unordered_map<common::ExchangeId, TradingPairReverseHashMap>;

using TickerHashMap = emhash7::HashMap<common::TickerId, common::TickerS>;
using TradeEngineCfgHashMap =
    emhash7::HashMap<common::TradingPair, common::TradeEngineCfg,
                     common::TradingPairHash, common::TradingPairEqual>;

inline uint32_t Digits10(uint64_t v) {
    return 1 + (std::uint32_t)(v >= 10) + (std::uint32_t)(v >= 100) +
           (std::uint32_t)(v >= 1000) + (std::uint32_t)(v >= 10000) +
           (std::uint32_t)(v >= 100000) + (std::uint32_t)(v >= 1000000) +
           (std::uint32_t)(v >= 10000000) + (std::uint32_t)(v >= 100000000) +
           (std::uint32_t)(v >= 1000000000) +
           (std::uint32_t)(v >= 10000000000ULL) +
           (std::uint32_t)(v >= 100000000000ULL) +
           (std::uint32_t)(v >= 1000000000000ULL) +
           (std::uint32_t)(v >= 10000000000000ULL) +
           (std::uint32_t)(v >= 100000000000000ULL) +
           (std::uint32_t)(v >= 1000000000000000ULL) +
           (std::uint32_t)(v >= 10000000000000000ULL) +
           (std::uint32_t)(v >= 100000000000000000ULL) +
           (std::uint32_t)(v >= 1000000000000000000ULL) +
           (std::uint32_t)(v >= 10000000000000000000ULL);
};
}  // namespace common

template <>
class fmt::formatter<common::ExchangeId> {
  public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(const common::ExchangeId& foo,
                          Context& ctx) const {
        return fmt::format_to(ctx.out(), "Exchange:{}]", magic_enum::enum_name(foo));
    }
};