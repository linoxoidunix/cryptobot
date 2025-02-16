#include <list>
#include <string_view>
#include <unordered_set>

#include "nlohmann/json.hpp"
#include "simdjson.h"

#include "aot/Logger.h"
#include "aot/Bybit.h"
#include "aot/market_data/market_update.h"

std::string bybit::ArgsBody::Body() {
    nlohmann::json json_object;

    for (const auto& [key, value] : *this) {
        if (key == "args") {
            json_object[key] = {value};
            continue;
        }
        // Check if the value starts and ends with quotes
        if (value.front() == '"' && value.back() == '"') {
            // Remove the quotes and store the value as a string
            json_object[key] = value.substr(1, value.size() - 2);
        }
    }
    return json_object.dump();
};

Exchange::BookSnapshot bybit::detail::FamilyBookSnapshot::ParserResponse::Parse(
    std::string_view response) {
    // NEED ADD EXCHANGE ID FIELD
    Exchange::BookSnapshot book_snapshot;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    try {
        book_snapshot.lastUpdateId = doc["lastUpdateId"].get_uint64();
        auto price_prec            = std::pow(10, pair_info_.price_precission);
        auto qty_prec              = std::pow(10, pair_info_.qty_precission);
        for (auto all : doc["bids"]) {
            simdjson::ondemand::array arr = all.get_array();
            std::array<double, 2> pair;  // price + qty
            uint8_t i = 0;
            for (auto number : arr) {
                pair[i] = number.get_double_in_string();
                i++;
            }
            book_snapshot.bids.emplace_back(
                static_cast<int>(pair[0] * price_prec),
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
            book_snapshot.asks.emplace_back(
                static_cast<int>(pair[0] * price_prec),
                static_cast<int>(pair[1] * qty_prec));
        }
    } catch (simdjson::simdjson_error& error) {
        loge("JSON error in FamilyBookSnapshot response: {}", error.what());
    }
    book_snapshot.exchange_id = common::ExchangeId::kBybit;
    return book_snapshot;
}

namespace bybit{
    bool IsValidType(std::string_view type) {
        return type == "snapshot" || type == "delta";
    }
    bool IsSnapshot(std::string_view type) {
        return type == "snapshot";
    }
    bool IsDelta(std::string_view type) {
        return type == "delta";
    }
}

bybit::detail::FamilyBookEventGetter::ParserResponse::ResponseVariant
bybit::detail::FamilyBookEventGetter::ParserResponse::Parse(
    std::string_view response) {
    bybit::detail::FamilyBookEventGetter::ParserResponse::ResponseVariant out;
    Exchange::BookDiffSnapshot book_diff_snapshot;
    Exchange::BookSnapshot book_snapshot;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    std::string_view type;
    try {
        if (doc.find_field_unordered("data").error() != simdjson::SUCCESS) {
            logw("Field 'data' is missing in the JSON document");
            return {};
        }
        auto data                  = doc["data"];

        auto get_value_s = [&](const char* key, std::string_view& variable,
                               const char* error_msg, bool from_data = true) -> bool {
            if (from_data) {
                // Handling data, which is an object
                auto error = doc["data"][key].get_string().get(variable);
                if (error != simdjson::SUCCESS) [[unlikely]] {
                    loge("{}", error_msg);
                    return false;
                }
            } else {
                // Handling doc, which is a document
                auto error = doc[key].get_string().get(variable);
                if (error != simdjson::SUCCESS) [[unlikely]] {
                    loge("{}", error_msg);
                    return false;
                }
            }
            return true;
        };

        
        if (!get_value_s("type", type, "no trading_pair in response", false)) {
            return {};  // Early return if any value fetch fails
        }

        // Validate type and handle accordingly
        if (!IsValidType(type)) {
            loge("Unexpected type:{}", type);
            return {};  // Return early if type is not "snapshot" or "delta"
        }
        

        
        std::string_view trading_pair_string;
        if (!get_value_s("s", trading_pair_string, "no trading_pair in response")) {
            return {};  // Early return if any value fetch fails
        }

        // Resolve trading pair
        common::TradingPair trading_pair;
        if (pairs_reverse_.count(trading_pair_string)){
            trading_pair = pairs_reverse_.find(trading_pair_string)->second;
        } else {
            loge("pairs_reverse not contain {}", trading_pair.ToString());
            return {};
        }

        auto is_snapshot = IsSnapshot(type);
        auto is_diff = IsDelta(type);
        std::list<Exchange::BookSnapshotElem> bids;
        std::list<Exchange::BookSnapshotElem> asks;
//------------------------
        auto price_prec = std::pow(
            10, pairs_[trading_pair].price_precission);
        auto qty_prec = std::pow(
            10, pairs_[trading_pair].qty_precission);
        if (data["b"].error() == simdjson::SUCCESS) {

            for (auto all : data["b"]) {
                simdjson::ondemand::array arr = all.get_array();
                std::array<double, 2> pair;
                uint8_t i = 0;
                for (auto number : arr) {
                    if (i >= 2){
                        logw("Ensure no out-of-bounds access");
                        break;
                    }
                    auto num_result = number.get_double_in_string();
                    if (num_result.error() == simdjson::SUCCESS) {
                        pair[i] = num_result.value();
                        i++;
                    } else {
                        // Handle invalid number format (log or skip)
                        break;
                    }
                }
                if (i == 2) { // Ensure we have both price and quantity
                    bids.emplace_back(
                        static_cast<int>(pair[0] * price_prec),
                        static_cast<int>(pair[1] * qty_prec));
                } else {
                    logw("pair is incomplete");
                    // Handle case where pair is incomplete
                }
            }
        } else {
            logw("missing 'b' in responce");
            // Handle missing or invalid "a"
            // Log or handle accordingly
        }
        if (data["a"].error() == simdjson::SUCCESS) {
            for (auto all : data["a"]) {
                auto arr_result = all.get_array();
                if (arr_result.error() == simdjson::SUCCESS) {
                    simdjson::ondemand::array arr = arr_result.value();
                    std::array<double, 2> pair;
                    uint8_t i = 0;

                    for (auto number : arr) {
                        if (i >= 2) break; // Ensure no out-of-bounds access
                        auto num_result = number.get_double_in_string();
                        if (num_result.error() == simdjson::SUCCESS) {
                            pair[i] = num_result.value();
                            i++;
                        } else {
                            // Handle invalid number format (log or skip)
                            break;
                        }
                    }

                    if (i == 2) { // Ensure we have both price and quantity
                        asks.emplace_back(
                            static_cast<int>(pair[0] * price_prec),
                            static_cast<int>(pair[1] * qty_prec));
                    } else {
                        logw("pair is incomplete");
                        // Handle case where pair is incomplete
                        // Log or handle accordingly
                    }
                } else {
                    // Handle invalid array in "a"
                    // Log or handle accordingly
                }
            }
        } else {
            logw("missing 'a' in responce");
            // Handle missing or invalid "a"
            // Log or handle accordingly
        }
//-----------------------------------------------------------------
        if(is_snapshot){
            book_snapshot.trading_pair = trading_pair;
            book_snapshot.lastUpdateId = data["u"].get_uint64();
            book_snapshot.bids = std::move(bids);
            book_snapshot.asks = std::move(asks);
            book_snapshot.exchange_id = common::ExchangeId::kBybit;
            out = book_snapshot;
        } else if(is_diff){
            book_diff_snapshot.trading_pair = trading_pair;
            book_diff_snapshot.last_id = data["u"].get_uint64();
            book_diff_snapshot.bids = std::move(bids);
            book_diff_snapshot.asks = std::move(asks);
            book_diff_snapshot.exchange_id = common::ExchangeId::kBybit;
            out = book_diff_snapshot;
        }
    } catch (const simdjson::simdjson_error& error) {
        loge("JSON error: {}", error.what());
    }
    return out;
}

bybit::detail::FamilyBookEventGetter::ParserResponse::ResponseVariant
bybit::detail::FamilyBookEventGetter::ParserResponse::Parse(
    simdjson::ondemand::document& doc) {
    bybit::detail::FamilyBookEventGetter::ParserResponse::ResponseVariant out;
    Exchange::BookDiffSnapshot book_diff_snapshot;
    Exchange::BookSnapshot book_snapshot;
    std::string_view type;
    try {
        if (doc.find_field_unordered("data").error() != simdjson::SUCCESS) {
            logw("Field 'data' is missing in the JSON document");
            return {};
        }
        auto data                  = doc["data"];

        auto get_value_s = [&](const char* key, std::string_view& variable,
                               const char* error_msg, bool from_data = true) -> bool {
            if (from_data) {
                // Handling data, which is an object
                auto error = doc["data"][key].get_string().get(variable);
                if (error != simdjson::SUCCESS) [[unlikely]] {
                    loge("{}", error_msg);
                    return false;
                }
            } else {
                // Handling doc, which is a document
                auto error = doc[key].get_string().get(variable);
                if (error != simdjson::SUCCESS) [[unlikely]] {
                    loge("{}", error_msg);
                    return false;
                }
            }
            return true;
        };

        
        if (!get_value_s("type", type, "no trading_pair in response", false)) {
            return {};  // Early return if any value fetch fails
        }

        // Validate type and handle accordingly
        if (!IsValidType(type)) {
            loge("Unexpected type:{}", type);
            return {};  // Return early if type is not "snapshot" or "delta"
        }
        

        
        std::string_view trading_pair_string;
        if (!get_value_s("s", trading_pair_string, "no trading_pair in response")) {
            return {};  // Early return if any value fetch fails
        }

        // Resolve trading pair
        common::TradingPair trading_pair;
        if (pairs_reverse_.count(trading_pair_string)){
            trading_pair = pairs_reverse_.find(trading_pair_string)->second;
        } else {
            loge("pairs_reverse not contain {}", trading_pair.ToString());
            return {};
        }

        auto is_snapshot = IsSnapshot(type);
        auto is_diff = IsDelta(type);
        std::list<Exchange::BookSnapshotElem> bids;
        std::list<Exchange::BookSnapshotElem> asks;
//------------------------
        auto price_prec = std::pow(
            10, pairs_[trading_pair].price_precission);
        auto qty_prec = std::pow(
            10, pairs_[trading_pair].qty_precission);
        if (data["b"].error() == simdjson::SUCCESS) {

            for (auto all : data["b"]) {
                simdjson::ondemand::array arr = all.get_array();
                std::array<double, 2> pair;
                uint8_t i = 0;
                for (auto number : arr) {
                    if (i >= 2){
                        logw("Ensure no out-of-bounds access");
                        break;
                    }
                    auto num_result = number.get_double_in_string();
                    if (num_result.error() == simdjson::SUCCESS) {
                        pair[i] = num_result.value();
                        i++;
                    } else {
                        // Handle invalid number format (log or skip)
                        break;
                    }
                }
                if (i == 2) { // Ensure we have both price and quantity
                    bids.emplace_back(
                        static_cast<int>(pair[0] * price_prec),
                        static_cast<int>(pair[1] * qty_prec));
                } else {
                    logw("pair is incomplete");
                    // Handle case where pair is incomplete
                }
            }
        } else {
            logw("missing 'b' in responce");
            // Handle missing or invalid "a"
            // Log or handle accordingly
        }
        if (data["a"].error() == simdjson::SUCCESS) {
            for (auto all : data["a"]) {
                auto arr_result = all.get_array();
                if (arr_result.error() == simdjson::SUCCESS) {
                    simdjson::ondemand::array arr = arr_result.value();
                    std::array<double, 2> pair;
                    uint8_t i = 0;

                    for (auto number : arr) {
                        if (i >= 2) break; // Ensure no out-of-bounds access
                        auto num_result = number.get_double_in_string();
                        if (num_result.error() == simdjson::SUCCESS) {
                            pair[i] = num_result.value();
                            i++;
                        } else {
                            // Handle invalid number format (log or skip)
                            break;
                        }
                    }

                    if (i == 2) { // Ensure we have both price and quantity
                        asks.emplace_back(
                            static_cast<int>(pair[0] * price_prec),
                            static_cast<int>(pair[1] * qty_prec));
                    } else {
                        logw("pair is incomplete");
                        // Handle case where pair is incomplete
                        // Log or handle accordingly
                    }
                } else {
                    // Handle invalid array in "a"
                    // Log or handle accordingly
                }
            }
        } else {
            logw("missing 'a' in responce");
            // Handle missing or invalid "a"
            // Log or handle accordingly
        }
//-----------------------------------------------------------------
        if(is_snapshot){
            book_snapshot.trading_pair = trading_pair;
            book_snapshot.lastUpdateId = data["u"].get_uint64();
            book_snapshot.bids = std::move(bids);
            book_snapshot.asks = std::move(asks);
            book_snapshot.exchange_id = common::ExchangeId::kBybit;
            out = book_snapshot;
        } else if(is_diff){
            book_diff_snapshot.trading_pair = trading_pair;
            book_diff_snapshot.last_id = data["u"].get_uint64();
            book_diff_snapshot.bids = std::move(bids);
            book_diff_snapshot.asks = std::move(asks);
            book_diff_snapshot.exchange_id = common::ExchangeId::kBybit;
            out = book_diff_snapshot;
        }
    } catch (const simdjson::simdjson_error& error) {
        loge("JSON error: {}", error.what());
    }
    return out;
}

bybit::ParserManager bybit::InitParserManager(
    common::TradingPairHashMap& pairs,
    common::TradingPairReverseHashMap& pair_reverse,
    bybit::ApiResponseParser& api_response_parser,
    bybit::detail::FamilyBookEventGetter::ParserResponse& parser_ob_diff) {
    bybit::ParserManager parser_manager;

    parser_manager.RegisterHandler(ResponseType::kNonQueryResponse,
        [&api_response_parser](simdjson::ondemand::document& doc) {
            return api_response_parser.Parse(doc);
        });

    parser_manager.RegisterHandler(ResponseType::kDepthUpdate,
        [&parser_ob_diff](simdjson::ondemand::document& doc) {
            return std::get<Exchange::BookDiffSnapshot>(parser_ob_diff.Parse(doc));  // Ensure it returns BookDiffSnapshot
        });

    parser_manager.RegisterHandler(ResponseType::kSnapshot,
        [&parser_ob_diff](simdjson::ondemand::document& doc) {
            return std::get<Exchange::BookSnapshot>(parser_ob_diff.Parse(doc));  // Ensure it returns BookSnapshot
        });

    return parser_manager;
};