#pragma once

#include <unordered_map>
#include "aot/common/types.h" // Ensure TradingPairHashMap is included

namespace aot{
class ExchangeTradingPairs {
public:
    using MapType = std::unordered_map<common::ExchangeId, common::TradingPairHashMap>;

private:
    MapType exchange_pairs_;

public:
    // Add a new exchange with its TradingPairHashMap
    void AddExchange(common::ExchangeId exchange_id, const common::TradingPairHashMap& pairs) {
        exchange_pairs_.emplace(exchange_id, pairs);
    }

    // Add or update a specific trading pair for an exchange
    void AddOrUpdatePair(common::ExchangeId exchange_id,
                         const common::TradingPair& trading_pair,
                         const common::TradingPairInfo& pair_info) {
        auto& pairs = exchange_pairs_[exchange_id];
        pairs[trading_pair] = pair_info;
    }

    // Get the TradingPairHashMap for a specific exchange
    const common::TradingPairHashMap* GetPairsForExchange(common::ExchangeId exchange_id) const {
        auto it = exchange_pairs_.find(exchange_id);
        return it != exchange_pairs_.end() ? &it->second : nullptr;
    }

    // Get information for a specific trading pair
    const common::TradingPairInfo* GetPairInfo(common::ExchangeId exchange_id, const common::TradingPair& trading_pair) const {
        auto it = exchange_pairs_.find(exchange_id);
        if (it != exchange_pairs_.end()) {
            const auto& pairs = it->second;
            auto pair_it = pairs.find(trading_pair);
            return pair_it != pairs.end() ? &pair_it->second : nullptr;
        }
        return nullptr;
    }

    // Remove an exchange and its trading pairs
    void RemoveExchange(common::ExchangeId exchange_id) {
        exchange_pairs_.erase(exchange_id);
    }

    // Remove a specific trading pair for an exchange
    void RemovePair(common::ExchangeId exchange_id, const common::TradingPair& trading_pair) {
        auto it = exchange_pairs_.find(exchange_id);
        if (it != exchange_pairs_.end()) {
            it->second.erase(trading_pair);
        }
    }

    // Check if an exchange exists
    bool ExchangeExists(common::ExchangeId exchange_id) const {
        return exchange_pairs_.find(exchange_id) != exchange_pairs_.end();
    }

    // Check if a specific trading pair exists for an exchange
    bool PairExists(common::ExchangeId exchange_id, const common::TradingPair& trading_pair) const {
        auto it = exchange_pairs_.find(exchange_id);
        if (it != exchange_pairs_.end()) {
            return it->second.find(trading_pair) != it->second.end();
        }
        return false;
    }

    // Get all exchanges
    std::vector<common::ExchangeId> GetAllExchanges() const {
        std::vector<common::ExchangeId> exchanges;
        for (const auto& [exchange_id, _] : exchange_pairs_) {
            exchanges.push_back(exchange_id);
        }
        return exchanges;
    }
};
};
