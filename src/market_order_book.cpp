#include <algorithm>

#include "aot/strategy/market_order_book.h"
#include "aot/Logger.h"

// #include "trade_engine.h"

namespace Trading {
MarketOrderBook::MarketOrderBook() 
 : orders_at_price_pool_(Common::ME_MAX_PRICE_LEVELS),
 order_pool_(Common::ME_MAX_ORDER_IDS)
{
    oid_to_order_.resize(Common::ME_MAX_ORDER_IDS);
    price_orders_at_price_.resize(Common::ME_MAX_PRICE_LEVELS);
}

MarketOrderBook::~MarketOrderBook() {
    logi("call ~MarketOrderBook() OrderBook\n{}", toString(false, true));
    bids_by_price_ = asks_by_price_ = nullptr;
    
    std::fill(oid_to_order_.begin(), oid_to_order_.end(), nullptr);
}

/// Process market data update and update the limit order book.
auto MarketOrderBook::onMarketUpdate(const Exchange::MEMarketUpdate *market_update) noexcept -> void {
    const auto bid_updated =
        (bids_by_price_ && market_update->side == Common::Side::BUY &&
         market_update->price >= bids_by_price_->price_);
    const auto ask_updated =
        (asks_by_price_ && market_update->side == Common::Side::SELL &&
         market_update->price <= asks_by_price_->price_);

    if (market_update->qty != 0) {
        auto order = order_pool_.allocate(
            Common::OrderId_INVALID,
            market_update->side,
            market_update->price, market_update->qty, nullptr, nullptr);
        addOrder(order);
    } else {
        removeOrdersAtPrice(market_update->side, market_update->price);
    }

    updateBBO(bid_updated, ask_updated);
    logi("{} {}", market_update->ToString(), bbo_.toString());

    // trade_engine_->onOrderBookUpdate(market_update->ticker_id_,
    // market_update->price_, market_update->side_, this);
}

auto MarketOrderBook::toString(bool detailed,
                               bool validity_check) const -> std::string {
    std::stringstream ss;
    std::string time_str;

    auto printer = [&](std::stringstream &ss, MarketOrdersAtPrice *itr,
                       Common::Side side, Common::Price &last_price, bool sanity_check) {
        char buf[4096];
        Common::Qty qty           = 0;
        size_t num_orders = 0;

        for (auto o_itr = itr->first_mkt_order_;; o_itr = o_itr->next_order_) {
            qty += o_itr->qty_;
            ++num_orders;
            if (o_itr->next_order_ == itr->first_mkt_order_) break;
        }
        sprintf(buf, " <px:%3s p:%3s n:%3s> %-3s @ %-5s(%-4s)",
                Common::priceToString(itr->price_).c_str(),
                Common::priceToString(itr->prev_entry_->price_).c_str(),
                Common::priceToString(itr->next_entry_->price_).c_str(),
                Common::priceToString(itr->price_).c_str(), Common::qtyToString(qty).c_str(),
                std::to_string(num_orders).c_str());
        ss << buf;
        for (auto o_itr = itr->first_mkt_order_;; o_itr = o_itr->next_order_) {
            if (detailed) {
                sprintf(buf, "[oid:%s q:%s p:%s n:%s] ",
                        Common::orderIdToString(o_itr->order_id_).c_str(),
                        Common::qtyToString(o_itr->qty_).c_str(),
                        Common::orderIdToString(o_itr->prev_order_
                                            ? o_itr->prev_order_->order_id_
                                            : Common::OrderId_INVALID)
                            .c_str(),
                        Common::orderIdToString(o_itr->next_order_
                                            ? o_itr->next_order_->order_id_
                                            : Common::OrderId_INVALID)
                            .c_str());
                ss << buf;
            }
            if (o_itr->next_order_ == itr->first_mkt_order_) break;
        }

        ss << std::endl;

        if (sanity_check) {
            if ((side == Common::Side::SELL && last_price >= itr->price_) ||
                (side == Common::Side::BUY && last_price <= itr->price_)) {
                FATAL(
                    "Bids/Asks not sorted by ascending/descending prices "
                    "last:" +
                    Common::priceToString(last_price) + " itr:" + itr->toString());
            }
            last_price = itr->price_;
        }
    };

    ss << "Ticker:" << /*Common::tickerIdToString(ticker_id_)*/ "no ticker" << std::endl;
    {
        auto ask_itr        = asks_by_price_;
        auto last_ask_price = std::numeric_limits<Common::Price>::min();
        for (size_t count = 0; ask_itr; ++count) {
            ss << "ASKS L:" << count << " => ";
            auto next_ask_itr =
                (ask_itr->next_entry_ == asks_by_price_ ? nullptr
                                                        : ask_itr->next_entry_);
            printer(ss, ask_itr, Common::Side::SELL, last_ask_price, validity_check);
            ask_itr = next_ask_itr;
        }
    }

    ss << std::endl << "                          X" << std::endl << std::endl;

    {
        auto bid_itr        = bids_by_price_;
        auto last_bid_price = std::numeric_limits<Common::Price>::max();
        for (size_t count = 0; bid_itr; ++count) {
            ss << "BIDS L:" << count << " => ";
            auto next_bid_itr =
                (bid_itr->next_entry_ == bids_by_price_ ? nullptr
                                                        : bid_itr->next_entry_);
            printer(ss, bid_itr, Common::Side::BUY, last_bid_price, validity_check);
            bid_itr = next_bid_itr;
        }
    }

    return ss.str();
};
};  // namespace Trading