#pragma once
#include <string>
#include <unordered_map>

#include "aot/Logger.h"
#include "aot/client_response.h"
#include "aot/common/macros.h"
#include "aot/common/types.h"
#include "aot/strategy/market_order.h"
// #include "market_order_book.h"

using namespace common;

namespace Trading {
/// PositionInfo tracks the position, pnl (realized and unrealized) and volume
/// for a single trading instrument.
struct PositionInfo {
    double position   = 0;
    double real_pnl   = 0;
    double unreal_pnl = 0;
    double total_pnl  = 0;
    std::array<double, sideToIndex(Side::MAX) + 1> open_vwap;
    double volume                 = 0;
    const Trading::BBO *bbo = nullptr;

    auto ToString() const {
        std::stringstream ss;
        ss << "Position{" << "pos:" << position << " u-pnl:" << unreal_pnl
           << " r-pnl:" << real_pnl << " t-pnl:" << total_pnl
           << " vol:" << volume << " ovwaps:["
           << (position
                   ? open_vwap.at(common::sideToIndex(common::Side::BUY)) / std::abs(position)
                   : 0)
           << "X"
           << (position ? open_vwap.at(common::sideToIndex(common::Side::SELL)) /
                               std::abs(position)
                         : 0)
           << "] " << (bbo ? bbo->ToString() : "") << "}";

        return ss.str();
    }

    /**
     * @brief add only filled order. it is regulate from trade_engine.cpp
     * Process an execution and update the position, pnl and volume.
     * @param client_response 
     * @return auto 
     */
    auto addFill(const Exchange::MEClientResponse *client_response) noexcept {
        const auto old_position   = position;
        const auto side_index     = common::sideToIndex(client_response->side);
        const auto opp_side_index = common::sideToIndex(
            client_response->side == Side::BUY ? Side::SELL : Side::BUY);
        const auto side_value  = common::sideToValue(client_response->side);
        assert(client_response->exec_qty >= 0);
        assert(client_response->price >= 0);
        position              += client_response->exec_qty * side_value;
        volume                += client_response->exec_qty;

        if (old_position * sideToValue(client_response->side) >=
            0) {  // opened / increased position.
            open_vwap[side_index] +=
                (client_response->price * client_response->exec_qty);
        } else {  // decreased position.
            const auto opp_side_vwap =
                open_vwap[opp_side_index] / std::abs(old_position);
            open_vwap[opp_side_index] = opp_side_vwap * std::abs(position);
            real_pnl +=
                std::min(client_response->exec_qty,
                         std::abs(old_position)) *
                (opp_side_vwap - client_response->price) *
                sideToValue(client_response->side);
            if (position * old_position <
                0) {  // flipped position to opposite sign.
                open_vwap[side_index] =
                    (client_response->price * std::abs(position));
                open_vwap[opp_side_index] = 0;
            }
        }

        if (!position) {  // flat
            open_vwap[sideToIndex(Side::BUY)] =
                open_vwap[sideToIndex(common::Side::SELL)] = 0;
            unreal_pnl                                     = 0;
        } else {
            if (position > 0)
                unreal_pnl = (client_response->price -
                              open_vwap[sideToIndex(common::Side::BUY)] /
                                  std::abs(position)) *
                             std::abs(position);
            else
                unreal_pnl =
                    (open_vwap[sideToIndex(Side::SELL)] / std::abs(position) -
                     client_response->price) *
                    std::abs(position);
        }

        total_pnl = unreal_pnl + real_pnl;

        logi("{} {}", ToString(), client_response->ToString());
    }

    /// Process a change in top-of-book prices (BBO), and update unrealized pnl
    /// if there is an open position.
    auto updateBBO(const Trading::BBO *_bbo) noexcept {
        bbo = _bbo;

        if (position && bbo->bid_price != common::kPRICE_DOUBLE_INVALID &&
            bbo->ask_price != common::kPRICE_DOUBLE_INVALID) {
            const auto mid_price = (bbo->bid_price + bbo->ask_price) * 0.5;
            if (position > 0)
                unreal_pnl =
                    (mid_price -
                     open_vwap[common::sideToIndex(common::Side::BUY)] /
                         std::abs(position)) *
                    std::abs(position);
            else
                unreal_pnl =
                    (open_vwap[common::sideToIndex(common::Side::SELL)] /
                         std::abs(position) -
                     mid_price) *
                    std::abs(position);

            const auto old_total_pnl = total_pnl;
            total_pnl                = unreal_pnl + real_pnl;

            if (total_pnl != old_total_pnl)
                logi("{} {}", ToString(), bbo->ToString());
        }
    }
};

/// Top level position keeper class to compute position, pnl and volume for all
/// trading instruments.
class PositionKeeper {
  public:
    explicit PositionKeeper() = default;

    /// Deleted default, copy & move constructors and assignment-operators.

    PositionKeeper(const PositionKeeper &)             = delete;

    PositionKeeper(const PositionKeeper &&)            = delete;

    PositionKeeper &operator=(const PositionKeeper &)  = delete;

    PositionKeeper &operator=(const PositionKeeper &&) = delete;

  private:

    /// Hash map container from TickerId -> PositionInfo.
    //std::array<PositionInfo, ME_MAX_TICKERS> ticker_position_;
    std::unordered_map<common::TradingPair, PositionInfo, common::TradingPairHash, common::TradingPairEqual> ticker_position;
  public:
    auto AddFill(const Exchange::MEClientResponse *client_response) noexcept {
        ticker_position[client_response->trading_pair].addFill(client_response);
    };

    auto UpdateBBO(const common::TradingPair trading_pair, const Trading::BBO *bbo) noexcept {
        ticker_position[trading_pair].updateBBO(bbo);
    };

    auto getPositionInfo(const common::TradingPair trading_pair) noexcept {
        return &(ticker_position[trading_pair]);
    };

    auto ToString() const {
        double total_pnl = 0;
        double total_vol    = 0;

        std::stringstream ss;
        for(auto& it : ticker_position)
        {
            ss << "TickerId:" << it.first.ToString() << " "
               << it.second.ToString() << "\n";
                total_pnl += it.second.total_pnl;
                total_vol += it.second.volume;
        }
        ss << "Total PnL:" << total_pnl << " Vol:" << total_vol << "\n";
        return ss.str();
    };
};
}  // namespace Trading