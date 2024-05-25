#pragma once

#include <cstdint>
#include <limits>
#include <sstream>
#include <array>

#include "aot/common/macros.h"

namespace Common {
  constexpr size_t ME_MAX_TICKERS = 8;

  constexpr size_t ME_MAX_CLIENT_UPDATES = 256 * 1024;
  constexpr size_t ME_MAX_MARKET_UPDATES = 256 * 1024;

  constexpr size_t ME_MAX_NUM_CLIENTS = 256;
  constexpr size_t ME_MAX_ORDER_IDS = 1024 * 1024;
  constexpr size_t ME_MAX_PRICE_LEVELS = 256;

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

  using Price = double;
  constexpr auto Price_INVALID = std::numeric_limits<Price>::max();

  inline auto priceToString(Price price) -> std::string {
    if (UNLIKELY(price == Price_INVALID)) {
      return "INVALID";
    }

    return std::to_string(price);
  }

  using Qty = double;
  constexpr auto Qty_INVALID = std::numeric_limits<Qty>::max();

  inline auto qtyToString(Qty qty) -> std::string {
    if (UNLIKELY(qty == Qty_INVALID)) {
      return "INVALID";
    }

    return std::to_string(qty);
  }

  using Priority = uint64_t;
  constexpr auto Priority_INVALID = std::numeric_limits<Priority>::max();

  inline auto priorityToString(Priority priority) -> std::string {
    if (UNLIKELY(priority == Priority_INVALID)) {
      return "INVALID";
    }

    return std::to_string(priority);
  }

  enum class Side : int8_t {
    INVALID = 0,
    BUY = 1,
    SELL = -1,
    MAX = 2
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
    RANDOM = 1,
    MAKER = 2,
    TAKER = 3,
    MAX = 4
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
    for (auto i = static_cast<int>(AlgoType::INVALID); i <= static_cast<int>(AlgoType::MAX); ++i) {
      const auto algo_type = static_cast<AlgoType>(i);
      if (algoTypeToString(algo_type) == str)
        return algo_type;
    }

    return AlgoType::INVALID;
  }

  struct RiskCfg {
    Qty max_order_size_ = 0;
    Qty max_position_ = 0;
    double max_loss_ = 0;

    auto toString() const {
      std::stringstream ss;

      ss << "RiskCfg{"
         << "max-order-size:" << qtyToString(max_order_size_) << " "
         << "max-position:" << qtyToString(max_position_) << " "
         << "max-loss:" << max_loss_
         << "}";

      return ss.str();
    }
  };

  struct TradeEngineCfg {
    Qty clip_ = 0;
    double threshold_ = 0;
    RiskCfg risk_cfg_;

    auto toString() const {
      std::stringstream ss;
      ss << "TradeEngineCfg{"
         << "clip:" << qtyToString(clip_) << " "
         << "thresh:" << threshold_ << " "
         << "risk:" << risk_cfg_.toString()
         << "}";

      return ss.str();
    }
  };

  using TradeEngineCfgHashMap = std::array<TradeEngineCfg, ME_MAX_TICKERS>;
}