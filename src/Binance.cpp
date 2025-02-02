
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
    std::string_view response, common::TradingPair trading_pair) {
    // NEED ADD EXCHANGE ID FIELD
    logi("{}", response);
    Exchange::BookSnapshot book_snapshot;
    simdjson::ondemand::parser parser;
    if(!trading_pairs_.count(trading_pair)){
        logw("no existing registered {} trading pair info", trading_pair.ToString());
        return book_snapshot;
    }
    auto& pair_info = trading_pairs_[trading_pair];
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    try{
        book_snapshot.lastUpdateId       = doc["lastUpdateId"].get_uint64();
        auto price_prec = std::pow(10, pair_info.price_precission);
        auto qty_prec   = std::pow(10, pair_info.qty_precission);
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
    book_snapshot.trading_pair = trading_pair;
    return book_snapshot;
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
            output.side = (side == "BUY"sv)    ? common::Side::kAsk
                          : (side == "SELL"sv) ? common::Side::kBid
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

binance::ParserManager binance::InitParserManager(
    common::TradingPairHashMap& pairs,
    common::TradingPairReverseHashMap& pair_reverse,
    binance::ApiResponseParser& api_response_parser,
    binance::detail::FamilyBookEventGetter::ParserResponse& parser_ob_diff){
    binance::ParserManager parser_manager;

    parser_manager.RegisterHandler(ResponseType::kNonQueryResponse,
        [&api_response_parser](simdjson::ondemand::document& doc) {
            return api_response_parser.Parse(doc);
        });

    parser_manager.RegisterHandler(ResponseType::kDepthUpdate,
        [&parser_ob_diff](simdjson::ondemand::document& doc) {
            return parser_ob_diff.Parse(doc);
        });

    return parser_manager;
};
