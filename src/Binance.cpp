
#include <string_view>
#include <unordered_set>

#include "nlohmann/json.hpp"


#include "aot/Logger.h"
#include "aot/Binance.h"

using namespace std::literals;

/**
 * @brief Serializes the arguments into a JSON body string.
 *
 * This method converts the key-value pairs of the ArgsBody object into a JSON object.
 * Special handling is applied for specific keys (e.g., "params") and values (e.g., strings
 * with quotes). The resulting JSON object is returned as a string.
 *
 * @return std::string The serialized JSON string representing the arguments.
 */
std::string binance::ArgsBody::Body() {
    nlohmann::json json_object;

    for (const auto& [key, value] : *this) {
        if (key == "params") {
            // Wrap "params" value in an array
            json_object[key] = {value};
        } else if (!value.empty() && value.front() == '"' && value.back() == '"') {
            // Remove surrounding quotes for quoted strings
            json_object[key] = value.substr(1, value.size() - 2);
        } else {
            // Direct assignment for other key-value pairs
            json_object[key] = value;
        }
    }

    return json_object.dump();
}

Exchange::BookSnapshot binance::detail::FamilyBookSnapshot::ParserResponse::Parse(
    std::string_view response) {
    // NEED ADD EXCHANGE ID FIELD
    Exchange::BookSnapshot book_snapshot;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    try{
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
    } catch (simdjson::simdjson_error& error) {
        loge("JSON error in FamilyBookSnapshot response: {}", error.what());
    }
    book_snapshot.exchange_id = common::ExchangeId::kBinance;
    return book_snapshot;
}

Exchange::BookDiffSnapshot binance::BookEventGetter::ParserResponse::Parse(
    std::string_view response) {
    // NEED ADD EXCHANGE ID FIELD
    assert(false);
    Exchange::BookDiffSnapshot book_diff_snapshot;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    book_diff_snapshot.first_id      = doc["U"].get_uint64();
    book_diff_snapshot.last_id       = doc["u"].get_uint64();
    auto price_prec = std::pow(10, pair_info_.price_precission);
    auto qty_prec   = std::pow(10, pair_info_.qty_precission);
    for (auto all : doc["b"]) {
        simdjson::ondemand::array arr = all.get_array();
        std::array<double, 2> pair;
        uint8_t i = 0;
        for (auto number : arr) {
            pair[i] = number.get_double_in_string();
            i++;
        }
        book_diff_snapshot.bids.emplace_back(
            static_cast<int>(pair[0] * price_prec),
            static_cast<int>(pair[1] * qty_prec));
    }
    for (auto all : doc["a"]) {
        simdjson::ondemand::array arr = all.get_array();
        std::array<double, 2> pair;
        uint8_t i = 0;
        for (auto number : arr) {
            pair[i] = number.get_double_in_string();
            i++;
        }
        book_diff_snapshot.asks.emplace_back(
            static_cast<int>(pair[0] * price_prec),
            static_cast<int>(pair[1] * qty_prec));
    }
    return book_diff_snapshot;
}

auto binance::GeneratorBidAskService::Run() noexcept -> void {
    logd("GeneratorBidAskService start");
    book_event_getter_->Init(book_diff_lfqueue_);
    logd("start subscribe book diff binance");
    bool is_first_run                 = true;
    bool diff_packet_lost             = true;
    bool snapshot_and_diff_was_synced = false;
    while (run_) {
        book_event_getter_->LaunchOne();
        Exchange::BookDiffSnapshot item;

        if (bool found = book_diff_lfqueue_.try_dequeue(item); !found)
            [[unlikely]]
            continue;
        logd("fetch diff book event from exchange {}", item.ToString());
        diff_packet_lost =
            !is_first_run && (item.first_id != last_id_diff_book_event + 1);
        auto need_snapshot      = (is_first_run || diff_packet_lost);
        last_id_diff_book_event = item.last_id;
        auto snapshot_and_diff_now_sync =
            (item.first_id <= snapshot_.lastUpdateId + 1) &&
            (item.last_id >= snapshot_.lastUpdateId + 1);
        need_snapshot += !snapshot_and_diff_now_sync;
        if (need_snapshot) [[unlikely]] {
            snapshot_and_diff_was_synced = false;
            Exchange::MEMarketUpdate event_clear_queue;
            event_clear_queue.type = Exchange::MarketUpdateType::CLEAR;
            logd("clear order book. try make snapshot");
            event_lfqueue_->enqueue(event_clear_queue);

            binance::detail::FamilyBookSnapshot::ArgsOrder args{
                ticker_hash_map_[trading_pair_.first],
                ticker_hash_map_[trading_pair_.second],
                1000};  // TODO parametrize 1000
            binance::BookSnapshot book_snapshoter(
                std::move(args), type_exchange_, &snapshot_,
                pair_info_);  // TODO parametrize 1000
            book_snapshoter.Exec();
            if (item.last_id <= snapshot_.lastUpdateId) {
                is_first_run = false;
                logd(
                    "need new diff event from exchange. "
                    "snapshot_.lastUpdateId = {}",
                    snapshot_.lastUpdateId);
                continue;
            } else if (snapshot_and_diff_now_sync) {
                is_first_run = false;
                logd(
                    "add snapshot and diff {} to order book. "
                    "snapshot_.lastUpdateId = {}",
                    item.ToString(), snapshot_.lastUpdateId);
            } else {
                /**
                 * @brief TODO may be there is a problem
                 * getting snapshot if snapshot last update id < diff last update id
                 * need test it
                 */
                logd(
                    "snapshot too old snapshot_.lastUpdateId = {}. Need "
                    "new snapshot",
                    snapshot_.lastUpdateId);
                is_first_run = true;
                continue;
            }
        }
        if (!snapshot_and_diff_was_synced) [[unlikely]] {
            if (snapshot_and_diff_now_sync) {
                snapshot_and_diff_was_synced = true;
                logd("add {} to order book", snapshot_.ToString());
                snapshot_.AddToQueue(*event_lfqueue_);
            }
        }
        if (!diff_packet_lost && snapshot_and_diff_was_synced) [[likely]] {
            logd("add {} to order book. snapshot_.lastUpdateId = {}",
                 item.ToString(), snapshot_.lastUpdateId);
            item.AddToQueue(*event_lfqueue_);
        }

        time_manager_.Update();
    }
}

binance::GeneratorBidAskService::GeneratorBidAskService(
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
            current_exchange_ = &binance_main_net_;
            break;
        default:
            current_exchange_ = &binance_test_net_;
            break;
    }
    book_event_getter_ =
        std::make_unique<BookEventGetter>(pair_info_, interval, type_exchange_);
}

Exchange::MEClientResponse
binance::detail::FamilyLimitOrder::ParserResponse::Parse(
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
        output.order_id     = doc["clientOrderId"].get_uint64_in_string();

        // Getting status
        std::string_view status;
        if (!getStringView("status", status)) return {};

        // Checking for success status
        const std::unordered_set<std::string_view> success_status{
            "NEW", "PARTIALLY_FILLED", "FILLED"};
        if (!success_status.contains(status)) return {};

        // Handling price based on status
        double price = 0;

        if (status == "NEW"sv) {
            output.type = Exchange::ClientResponseType::ACCEPTED;
            if (doc["price"].get_double_in_string().get(price) !=
                simdjson::SUCCESS)
                return {};
            output.price = static_cast<common::Price>(
                price *
                std::pow(10, pairs_[output.trading_pair].price_precission));

        } else if (status == "PARTIALLY_FILLED"sv || status == "FILLED"sv) {
            output.type = Exchange::ClientResponseType::FILLED;
            if (doc["cummulativeQuoteQty"].get_double_in_string().get(price) !=
                simdjson::SUCCESS)
                return {};
            output.price = static_cast<common::Price>(
                price *
                std::pow(10, pairs_[output.trading_pair].price_precission));
        }

        // Handling side
        std::string_view side;
        if (getStringView("side", side)) {
            output.side = (side == "BUY"sv)    ? common::Side::BUY
                          : (side == "SELL"sv) ? common::Side::SELL
                                               : output.side;
        }

        // Getting executed quantity
        double executed_qty = 0;
        if (doc["executedQty"].get_double_in_string().get(executed_qty) !=
            simdjson::SUCCESS)
            return {};
        output.exec_qty = static_cast<common::Qty>(
            executed_qty *
            std::pow(10, pairs_[output.trading_pair].qty_precission));

        // Getting original quantity and calculating leaves quantity
        double orig_qty;
        if (doc["origQty"].get_double_in_string().get(orig_qty) ==
            simdjson::SUCCESS) {
            output.leaves_qty = static_cast<common::Qty>(
                (orig_qty - executed_qty) *
                std::pow(10, pairs_[output.trading_pair].qty_precission));
        } else {
            loge("no key origQty in response");
        }

    } catch (simdjson::simdjson_error& error) {
        loge("JSON error in FamilyLimitOrder response: {}", error.what());
    }
    output.exchange_id = common::ExchangeId::kBinance;
    return output;
}

Exchange::MEClientResponse
binance::detail::FamilyCancelOrder::ParserResponse::Parse(
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
    output.exchange_id = common::ExchangeId::kBinance;
    return output;
}

OHLCVExt binance::OHLCVI::ParserResponse::Parse(std::string_view response) {
    OHLCVExt output;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    std::string_view ticker;
    double open, high, low, close, volume;
    auto price_prec  = std::pow(10, map_[pair_].price_precission);
    auto qty_prec    = std::pow(10, map_[pair_].qty_precission);

    // Helper function to fetch double values from the document
    auto get_value_d = [&](const char* key, double& variable,
                           const std::string& error_msg) -> bool {
        if (auto error = doc["k"][key].get_double_in_string().get(variable);
            error != simdjson::SUCCESS) [[unlikely]] {
            loge("{}", error_msg);
            return false;
        }
        return true;
    };

    auto get_value_s = [&](const char* key, std::string_view& variable,
                           const std::string& error_msg) -> bool {
        if (auto error = doc["k"][key].get_string().get(variable);
            error != simdjson::SUCCESS) [[unlikely]] {
            loge("{}", error_msg);
            return false;
        }
        return true;
    };

    if (!get_value_d("o", open, "no key open in response") ||
        !get_value_d("c", close, "no key close in response") ||
        !get_value_d("h", high, "no key high in response") ||
        !get_value_d("l", low, "no key low in response") ||
        !get_value_d("v", volume, "no key volume in response") ||
        !get_value_s("s", ticker, "no ticker in response")) {
        return {};  // Early return if any value fetch fails
    }

    output.ohlcv.open   = static_cast<int>(open * price_prec);
    output.ohlcv.close  = static_cast<int>(close * price_prec);
    output.ohlcv.high   = static_cast<int>(high * price_prec);
    output.ohlcv.low    = static_cast<int>(low * price_prec);
    output.ohlcv.volume = static_cast<int>(volume * qty_prec);

    if (pairs_reverse_.count(ticker)) [[likely]] {
        output.trading_pair = pairs_reverse_.find(ticker)->second;
    } else {
        loge("pairs_reverse not contain {}", ticker);
        return {};
    }
    return output;
}

Exchange::BookDiffSnapshot
binance::detail::FamilyBookEventGetter::ParserResponse::Parse(
    std::string_view response) {
    Exchange::BookDiffSnapshot book_diff_snapshot;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    try {
        book_diff_snapshot.first_id = doc["U"].get_uint64();
        book_diff_snapshot.last_id  = doc["u"].get_uint64();

        auto get_value_s = [&](const char* key, std::string_view& variable,
                               const char* error_msg) -> bool {
            if (auto error = doc[key].get_string().get(variable);
                error != simdjson::SUCCESS) [[unlikely]] {
                loge("{}", error_msg);
                return false;
            }
            return true;
        };
        std::string_view trading_pair;
        if (!get_value_s("s", trading_pair, "no trading_pair in response")) {
            return {};  // Early return if any value fetch fails
        }

        if (pairs_reverse_.count(trading_pair)) [[likely]] {
            book_diff_snapshot.trading_pair =
                pairs_reverse_.find(trading_pair)->second;
        } else {
            loge("pairs_reverse not contain {}", trading_pair);
            return {};
        }
        auto price_prec = std::pow(
            10, pairs_[book_diff_snapshot.trading_pair].price_precission);
        auto qty_prec = std::pow(
            10, pairs_[book_diff_snapshot.trading_pair].qty_precission);
        for (auto all : doc["b"]) {
            simdjson::ondemand::array arr = all.get_array();
            std::array<double, 2> pair;
            uint8_t i = 0;
            for (auto number : arr) {
                pair[i] = number.get_double_in_string();
                i++;
            }
            book_diff_snapshot.bids.emplace_back(
                static_cast<int>(pair[0] * price_prec),
                static_cast<int>(pair[1] * qty_prec));
        }
        for (auto all : doc["a"]) {
            simdjson::ondemand::array arr = all.get_array();
            std::array<double, 2> pair;
            uint8_t i = 0;
            for (auto number : arr) {
                pair[i] = number.get_double_in_string();
                i++;
            }
            book_diff_snapshot.asks.emplace_back(
                static_cast<int>(pair[0] * price_prec),
                static_cast<int>(pair[1] * qty_prec));
        }
    } catch (const simdjson::simdjson_error& error) {
        loge("JSON error: {}", error.what());
    }
    book_diff_snapshot.exchange_id = common::ExchangeId::kBinance;
    return book_diff_snapshot;
}

/**
 * @brief Parses the response from the Binance API to extract book diff snapshot data.
 * 
 * This function processes a `simdjson::ondemand::document` containing a Binance 
 * API response and extracts information such as the first and last IDs, 
 * trading pair, bids, and asks. It converts the relevant data into an 
 * `Exchange::BookDiffSnapshot` object.
 * 
 * @param doc The parsed JSON document containing the API response.
 * @return An `Exchange::BookDiffSnapshot` containing the parsed data.
 */
Exchange::BookDiffSnapshot
binance::detail::FamilyBookEventGetter::ParserResponse::Parse(
    simdjson::ondemand::document& doc) {
    
    Exchange::BookDiffSnapshot book_diff_snapshot;

    try {
        // Extract the first and last IDs
        book_diff_snapshot.first_id = doc["U"].get_uint64();
        book_diff_snapshot.last_id = doc["u"].get_uint64();

        // Lambda function to safely extract a string value from the document
        auto get_value_s = [&](const char* key, std::string_view& variable,
                               const char* error_msg) -> bool {
            if (auto error = doc[key].get_string().get(variable);
                error != simdjson::SUCCESS) [[unlikely]] {
                loge("{}", error_msg);
                return false;
            }
            return true;
        };

        // Extract trading pair from the response
        std::string_view trading_pair;
        if (!get_value_s("s", trading_pair, "no trading_pair in response")) {
            return {};  // Early return if trading pair is missing
        }

        // Resolve trading pair
        if (pairs_reverse_.count(trading_pair)) [[likely]] {
            book_diff_snapshot.trading_pair =
                pairs_reverse_.find(trading_pair)->second;
        } else {
            loge("pairs_reverse does not contain {}", trading_pair);
            return {};  // Early return if trading pair is not found
        }

        // Calculate price and quantity precision
        auto price_prec = std::pow(10, pairs_[book_diff_snapshot.trading_pair].price_precission);
        auto qty_prec = std::pow(10, pairs_[book_diff_snapshot.trading_pair].qty_precission);

        // Parse bids from the document and store them in the snapshot
        for (auto all : doc["b"]) {
            simdjson::ondemand::array arr = all.get_array();
            std::array<double, 2> pair;
            uint8_t i = 0;
            for (auto number : arr) {
                pair[i] = number.get_double_in_string();
                i++;
            }
            book_diff_snapshot.bids.emplace_back(
                static_cast<int>(pair[0] * price_prec),
                static_cast<int>(pair[1] * qty_prec));
        }

        // Parse asks from the document and store them in the snapshot
        for (auto all : doc["a"]) {
            simdjson::ondemand::array arr = all.get_array();
            std::array<double, 2> pair;
            uint8_t i = 0;
            for (auto number : arr) {
                pair[i] = number.get_double_in_string();
                i++;
            }
            book_diff_snapshot.asks.emplace_back(
                static_cast<int>(pair[0] * price_prec),
                static_cast<int>(pair[1] * qty_prec));
        }

    } catch (const simdjson::simdjson_error& error) {
        // Log error if JSON parsing fails
        loge("JSON error: {}", error.what());
    }

    // Set the exchange ID
    book_diff_snapshot.exchange_id = common::ExchangeId::kBinance;

    return book_diff_snapshot;
}
