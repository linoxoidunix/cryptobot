#include "aot/mexc.h"

#include <string_view>
#include <unordered_set>

#include "aot/Logger.h"
#include "simdjson.h"

using namespace std::literals;

Exchange::BookSnapshot mexc::BookSnapshot::ParserResponse::Parse(
    std::string_view response) {
    Exchange::BookSnapshot book_snapshot;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    book_snapshot.lastUpdateId       = doc["lastUpdateId"].get_uint64();
    auto price_prec = std::pow(10, pair_info_.price_precission);
    auto qty_prec   = std::pow(10, pair_info_.qty_precission);
    for (auto all : doc["bids"]) {
        simdjson::ondemand::array arr = all.get_array();
        std::array<double, 2> pair;  // price + qty
        uint8_t i = 0;
        for (auto number : arr) {
            pair[i] = number.get_double_in_string();
            i++;
        }
        book_snapshot.bids.emplace_back(static_cast<int>(pair[0] * price_prec),
                                        static_cast<int>(pair[1] * qty_prec));
    }
    for (auto all : doc["asks"]) {
        simdjson::ondemand::array arr = all.get_array();
        std::array<double, 2> pair;  // price + qty
        uint8_t i = 0;
        for (auto number : arr) {
            pair[i] = number.get_double_in_string();
            i++;
        }
        book_snapshot.asks.emplace_back(static_cast<int>(pair[0] * price_prec),
                                        static_cast<int>(pair[1] * qty_prec));
    }
    return book_snapshot;
}

Exchange::BookDiffSnapshot mexc::BookEventGetter::ParserResponse::Parse(
    std::string_view response) {
    Exchange::BookDiffSnapshot book_diff_snapshot;
    // simdjson::ondemand::parser parser;
    // simdjson::padded_string my_padded_data(response.data(), response.size());
    // simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    // book_diff_snapshot.first_id      = doc["U"].get_uint64();
    // book_diff_snapshot.last_id       = doc["u"].get_uint64();
    // auto price_prec = std::pow(10, pair_info_.price_precission);
    // auto qty_prec   = std::pow(10, pair_info_.qty_precission);
    // for (auto all : doc["b"]) {
    //     simdjson::ondemand::array arr = all.get_array();
    //     std::array<double, 2> pair;
    //     uint8_t i = 0;
    //     for (auto number : arr) {
    //         pair[i] = number.get_double_in_string();
    //         i++;
    //     }
    //     book_diff_snapshot.bids.emplace_back(
    //         static_cast<int>(pair[0] * price_prec),
    //         static_cast<int>(pair[1] * qty_prec));
    // }
    // for (auto all : doc["a"]) {
    //     simdjson::ondemand::array arr = all.get_array();
    //     std::array<double, 2> pair;
    //     uint8_t i = 0;
    //     for (auto number : arr) {
    //         pair[i] = number.get_double_in_string();
    //         i++;
    //     }
    //     book_diff_snapshot.asks.emplace_back(
    //         static_cast<int>(pair[0] * price_prec),
    //         static_cast<int>(pair[1] * qty_prec));
    // }
    return book_diff_snapshot;
}

auto mexc::GeneratorBidAskService::Run() noexcept -> void {
    // logd("GeneratorBidAskService start");
    // book_event_getter_->Init(book_diff_lfqueue_);
    // logd("start subscribe book diff binance");
    // bool is_first_run                 = true;
    // bool diff_packet_lost             = true;
    // bool snapshot_and_diff_was_synced = false;
    // while (run_) {
    //     book_event_getter_->LaunchOne();
    //     Exchange::BookDiffSnapshot item;

    //     if (bool found = book_diff_lfqueue_.try_dequeue(item); !found)
    //         [[unlikely]]
    //         continue;
    //     logd("fetch diff book event from exchange {}", item.ToString());
    //     diff_packet_lost =
    //         !is_first_run && (item.first_id != last_id_diff_book_event + 1);
    //     auto need_snapshot      = (is_first_run || diff_packet_lost);
    //     last_id_diff_book_event = item.last_id;
    //     auto snapshot_and_diff_now_sync =
    //         (item.first_id <= snapshot_.lastUpdateId + 1) &&
    //         (item.last_id >= snapshot_.lastUpdateId + 1);
    //     if (need_snapshot) [[unlikely]] {
    //         snapshot_and_diff_was_synced = false;
    //         Exchange::MEMarketUpdate event_clear_queue;
    //         event_clear_queue.type = Exchange::MarketUpdateType::CLEAR;
    //         logd("clear order book. try make snapshot");
    //         event_lfqueue_->enqueue(event_clear_queue);

    //         binance::BookSnapshot::ArgsOrder args{
    //             ticker_hash_map_[trading_pair_.first],
    //             ticker_hash_map_[trading_pair_.second],
    //             1000};  // TODO parametrize 1000
    //         binance::BookSnapshot book_snapshoter(
    //             std::move(args), type_exchange_, &snapshot_,
    //             pair_info_);  // TODO parametrize 1000
    //         book_snapshoter.Exec();
    //         if (item.last_id <= snapshot_.lastUpdateId) {
    //             is_first_run = false;
    //             logd(
    //                 "need new diff event from exchange. "
    //                 "snapshot_.lastUpdateId = {}",
    //                 snapshot_.lastUpdateId);
    //             continue;
    //         } else if (snapshot_and_diff_now_sync) {
    //             is_first_run = false;
    //             logd(
    //                 "add snapshot and diff {} to order book. "
    //                 "snapshot_.lastUpdateId = {}",
    //                 item.ToString(), snapshot_.lastUpdateId);
    //         } else {
    //             logd(
    //                 "snapshot too old snapshot_.lastUpdateId = {}. Need "
    //                 "new snapshot",
    //                 snapshot_.lastUpdateId);
    //             is_first_run = true;
    //             continue;
    //         }
    //     }
    //     if (!snapshot_and_diff_was_synced) [[unlikely]] {
    //         if (snapshot_and_diff_now_sync) {
    //             snapshot_and_diff_was_synced = true;
    //             logd("add {} to order book", snapshot_.ToString());
    //             snapshot_.AddToQueue(*event_lfqueue_);
    //         }
    //     }
    //     if (!diff_packet_lost && snapshot_and_diff_was_synced) [[likely]] {
    //         logd("add {} to order book. snapshot_.lastUpdateId = {}",
    //              item.ToString(), snapshot_.lastUpdateId);
    //         item.AddToQueue(*event_lfqueue_);
    //     }

    //     time_manager_.Update();
    // }
}

mexc::GeneratorBidAskService::GeneratorBidAskService(
    Exchange::EventLFQueue* event_lfqueue,
    prometheus::EventLFQueue* prometheus_event_lfqueue,
    const common::TradingPairInfo& trading_pair_info,
    common::TickerHashMap& ticker_hash_map, common::TradingPair trading_pair,
    const DiffDepthStream::StreamIntervalI* interval,
    TypeExchange type_exchange)
    : event_lfqueue_(event_lfqueue),
      prometheus_event_lfqueue_(prometheus_event_lfqueue),
      pair_info_(trading_pair_info),
      ticker_hash_map_(ticker_hash_map),
      trading_pair_(trading_pair),
      interval_(interval),
      type_exchange_(type_exchange) {
    switch (type_exchange_) {
        case TypeExchange::MAINNET:
            current_exchange_ = &mecx_main_net_;
            break;
        default:
            current_exchange_ = &mecx_main_net_;
            break;
    }
    book_event_getter_ =
        std::make_unique<BookEventGetter>(pair_info_, interval, type_exchange_);
}

Exchange::MEClientResponse mexc::detail::FamilyLimitOrder::ParserResponse::Parse(
    std::string_view response) {
        Exchange::MEClientResponse output;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    try {
        // Helper function to get a string view from the document
        auto getStringView = [&](const char* key, std::string_view& value) {
            auto error = doc[key].get_string().get(value);
            if (error != simdjson::SUCCESS) {
                loge("no key {} in response", key);
                return false;
            }
            return true;
        };
        
        // Getting ticker
        std::string_view ticker;
        if (!getStringView("symbol", ticker) || !pairs_reverse_.count(ticker)) 
            return {};

        output.trading_pair = pairs_reverse_.find(ticker)->second;

        // Getting order_id
        output.order_id = doc["clientOrderId"].get_uint64_in_string();
        
        // Getting status
        std::string_view status;
        if (!getStringView("status", status)) return {};

        // Checking for success status
        const std::unordered_set<std::string_view> success_status{"NEW", "PARTIALLY_FILLED", "FILLED"};
        if (!success_status.contains(status)) return {};

        // Handling price based on status
        double price = 0;

        if (status == "NEW"sv) {
            output.type = Exchange::ClientResponseType::ACCEPTED;
            if (doc["price"].get_double_in_string().get(price) != simdjson::SUCCESS) return {};
            output.price = static_cast<common::Price>(price * std::pow(10, pairs_[output.trading_pair].price_precission));

        } else if (status == "PARTIALLY_FILLED"sv || status == "FILLED"sv) {
            output.type = Exchange::ClientResponseType::FILLED;
            if (doc["cummulativeQuoteQty"].get_double_in_string().get(price) != simdjson::SUCCESS) return {};
            output.price = static_cast<common::Price>(price * std::pow(10, pairs_[output.trading_pair].price_precission));
        }

        // Handling side
        std::string_view side;
        if (getStringView("side", side)) {
            output.side = (side == "BUY"sv) ? common::Side::BUY : 
                          (side == "SELL"sv) ? common::Side::SELL : output.side;
        }

        // Getting executed quantity
        double executed_qty = 0;
        if (doc["executedQty"].get_double_in_string().get(executed_qty) != simdjson::SUCCESS) return {};
        output.exec_qty = static_cast<common::Qty>(executed_qty * std::pow(10, pairs_[output.trading_pair].qty_precission));

        // Getting original quantity and calculating leaves quantity
        double orig_qty;
        if (doc["origQty"].get_double_in_string().get(orig_qty) == simdjson::SUCCESS) {
            output.leaves_qty = static_cast<common::Qty>((orig_qty - executed_qty) * std::pow(10, pairs_[output.trading_pair].qty_precission));
        } else {
            loge("no key origQty in response");
        }

    } catch (simdjson::simdjson_error& error) {
        loge("JSON error: {}", error.what());
    }

    return output;
}

Exchange::MEClientResponse mexc::detail::FamilyCancelOrder::ParserResponse::Parse(
    std::string_view response) {
    Exchange::MEClientResponse output;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);

    try {
        // Retrieve order ID
        output.order_id = doc["origClientOrderId"].get_uint64_in_string();

        // Check and retrieve status
        std::string_view status;
        if (auto error_status = doc["status"].get_string().get(status);
            error_status != simdjson::SUCCESS) {
            loge("no key status in response");
            return {};
        }

        // Check if order is canceled
        if (status != "CANCELED"sv) return {};
        output.type = Exchange::ClientResponseType::CANCELED;

        // Check and retrieve ticker
        std::string_view ticker;
        if (auto error_symbol = doc["symbol"].get_string().get(ticker);
            error_symbol != simdjson::SUCCESS) {
            loge("no key symbol in response");
            return {};
        }

        // Find trading pair
        if (auto it = pairs_reverse_.find(ticker); it != pairs_reverse_.end()) {
            output.trading_pair = it->second;
        } else {
            loge("pairs_reverse not contain {}", ticker);
            return {};
        }

    } catch (const simdjson::simdjson_error& error) {
        loge("JSON error: {}", error.what());
    }
    return output;
}

// OHLCVExt binance::OHLCVI::ParserResponse::Parse(std::string_view response) {
//     OHLCVExt output;
//     simdjson::ondemand::parser parser;
//     simdjson::padded_string my_padded_data(response.data(), response.size());
//     simdjson::ondemand::document doc = parser.iterate(my_padded_data);
//     std::string_view ticker;
//     double open, high, low, close, volume;
//     auto price_prec  = std::pow(10, map_[pair_].price_precission);
//     auto qty_prec    = std::pow(10, map_[pair_].qty_precission);

//     // Helper function to fetch double values from the document
//     auto get_value_d = [&](const char* key, double& variable,
//                            const std::string& error_msg) -> bool {
//         if (auto error = doc["k"][key].get_double_in_string().get(variable);
//             error != simdjson::SUCCESS) [[unlikely]] {
//             loge("{}", error_msg);
//             return false;
//         }
//         return true;
//     };

//     auto get_value_s = [&](const char* key, std::string_view& variable,
//                            const std::string& error_msg) -> bool {
//         if (auto error = doc["k"][key].get_string().get(variable);
//             error != simdjson::SUCCESS) [[unlikely]] {
//             loge("{}", error_msg);
//             return false;
//         }
//         return true;
//     };

//     if (!get_value_d("o", open, "no key open in response") ||
//         !get_value_d("c", close, "no key close in response") ||
//         !get_value_d("h", high, "no key high in response") ||
//         !get_value_d("l", low, "no key low in response") ||
//         !get_value_d("v", volume, "no key volume in response") ||
//         !get_value_s("s", ticker, "no ticker in response")) {
//         return {};  // Early return if any value fetch fails
//     }

//     output.ohlcv.open   = static_cast<int>(open * price_prec);
//     output.ohlcv.close  = static_cast<int>(close * price_prec);
//     output.ohlcv.high   = static_cast<int>(high * price_prec);
//     output.ohlcv.low    = static_cast<int>(low * price_prec);
//     output.ohlcv.volume = static_cast<int>(volume * qty_prec);

//     if (pairs_reverse_.count(ticker)) [[likely]] {
//         output.trading_pair = pairs_reverse_.find(ticker)->second;
//     } else {
//         loge("pairs_reverse not contain {}", ticker);
//     }

//     return output;
// }
