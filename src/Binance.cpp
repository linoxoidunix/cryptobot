#include "aot/Binance.h"

#include <string_view>
#include <unordered_set>

#include "aot/Logger.h"
#include "simdjson.h"

Exchange::BookSnapshot binance::BookSnapshot::ParserResponse::Parse(
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
        book_snapshot.bids.emplace_back(
            static_cast<int>(pair[0] *
                             price_prec),
            static_cast<int>(pair[1] *
                             qty_prec));
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
            static_cast<int>(pair[0] *
                             price_prec),
            static_cast<int>(pair[1] *
                             qty_prec));
    }
    return book_snapshot;
}

Exchange::BookDiffSnapshot binance::BookEventGetter::ParserResponse::Parse(
    std::string_view response) {
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
        if (need_snapshot) [[unlikely]] {
            snapshot_and_diff_was_synced = false;
            Exchange::MEMarketUpdate event_clear_queue;
            event_clear_queue.type = Exchange::MarketUpdateType::CLEAR;
            logd("clear order book. try make snapshot");
            event_lfqueue_->enqueue(event_clear_queue);

            binance::BookSnapshot::ArgsOrder args{
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
                    "need new diff event from excnage. "
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

Exchange::MEClientResponse binance::OrderNewLimit::ParserResponse::Parse(
    std::string_view response) {
    Exchange::MEClientResponse output;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    try {
        output.order_id =
            doc["clientOrderId"]
                .get_uint64_in_string();  // TODO check this is uint64
        std::string_view status;
        if (auto error_status = doc["status"].get_string().get(status);
            error_status != simdjson::SUCCESS) {
            loge("no key status in response");
            return output;
        }

        if (std::unordered_set<std::string_view> success_status{
                "NEW", "PARTIALLY_FILLED", "FILLED"};
            !success_status.contains(status))
            return output;
        if (std::unordered_set<std::string_view> accepted_status{"NEW"};
            accepted_status.contains(status)) {
            output.type = Exchange::ClientResponseType::ACCEPTED;
            auto error  = doc["price"].get_double_in_string().get(output.price);
            if (error != simdjson::SUCCESS) [[unlikely]] {
                loge("no key price in response");
            }
        }

        if (std::unordered_set<std::string_view> filled_status{
                "PARTIALLY_FILLED", "FILLED"};
            filled_status.contains(status)) {
            output.type = Exchange::ClientResponseType::FILLED;
            auto error  = doc["cummulativeQuoteQty"].get_double_in_string().get(
                output.price);
            if (error != simdjson::SUCCESS) [[unlikely]] {
                loge("no key cummulativeQuoteQty in response");
            }
        }
        std::string_view side;
        auto error = doc["side"].get_string().get(side);
        if (error != simdjson::SUCCESS) [[unlikely]] {
            loge("no key side in response");
        } else {
            std::string_view buy_side  = "BUY";
            std::string_view sell_side = "SELL";
            if (side == buy_side) output.side = common::Side::BUY;
            if (side == sell_side) output.side = common::Side::SELL;
        }
        error = doc["executedQty"].get_double_in_string().get(output.exec_qty);
        if (error != simdjson::SUCCESS) [[unlikely]] {
            loge("no key executedQty in response");
        } else {
            double orig_qty;
            error = doc["origQty"].get_double_in_string().get(orig_qty);
            if (error != simdjson::SUCCESS) [[unlikely]] {
                loge("no key origQty in response");
            } else {
                output.leaves_qty = orig_qty - output.exec_qty;
            }
        }
        std::string_view ticker;
        auto error_symbol = doc["symbol"].get_string().get(ticker);
        if (error_symbol != simdjson::SUCCESS) [[unlikely]] {
            loge("no key symbol in response");
            return output;
        }
        if (pairs_reverse_.count(ticker)) [[likely]]
            output.trading_pair = pairs_reverse_.find(ticker)->second;
        else
            loge("pairs_reverse not contain {}", ticker);
    } catch (simdjson::simdjson_error& error) {
        // std::cerr << "JSON error: " << error.what() <<
        loge("JSON error: {}", error.what());
    }
    return output;
}

Exchange::MEClientResponse binance::CancelOrder::ParserResponse::Parse(
    std::string_view response) {
    Exchange::MEClientResponse output;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    try {
        output.order_id =
            doc["origClientOrderId"]
                .get_uint64_in_string();  // TODO check this is uint64
        std::string_view status;
        auto error_status = doc["status"].get_string().get(status);
        if (error_status != simdjson::SUCCESS) {
            loge("no key status in response");
            return output;
        }
        std::unordered_set<std::string_view> success_status{"CANCELED"};
        if (!success_status.count(status)) return output;
        output.type = Exchange::ClientResponseType::CANCELED;
        std::string_view ticker;
        auto error_symbol = doc["symbol"].get_string().get(ticker);
        if (error_symbol != simdjson::SUCCESS) [[unlikely]] {
            loge("no key symbol in response");
            return output;
        }
        if (pairs_reverse_.count(ticker)) [[likely]]
            output.trading_pair = pairs_reverse_.find(ticker)->second;
        else
            loge("pairs_reverse not contain {}", ticker);
    } catch (simdjson::simdjson_error& error) {
        loge("JSON error: {}", error.what());
    }
    return output;
}

OHLCVExt binance::OHLCVI::ParserResponse::Parse(std::string_view response) {
    OHLCVExt output;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    double open;
    double high;
    double low;
    double close;
    double volume;
    auto price_prec = std::pow(10, map_[pair_].price_precission);
    auto qty_prec   = std::pow(10, map_[pair_].qty_precission);

    try {
        auto error = doc["k"]["o"].get_double_in_string().get(open);
        if (error != simdjson::SUCCESS) [[unlikely]] {
            loge("no key open in response");
            return output;
        }
        output.ohlcv.open = static_cast<int>(open * price_prec);
        error             = doc["k"]["c"].get_double_in_string().get(close);
        if (error != simdjson::SUCCESS) [[unlikely]] {
            loge("no key close in response");
            return output;
        }
        output.ohlcv.close = static_cast<int>(close * price_prec);
        error              = doc["k"]["h"].get_double_in_string().get(high);
        if (error != simdjson::SUCCESS) [[unlikely]] {
            loge("no key high in response");
            return output;
        }
        output.ohlcv.high = static_cast<int>(high * price_prec);
        error             = doc["k"]["l"].get_double_in_string().get(low);
        if (error != simdjson::SUCCESS) [[unlikely]] {
            loge("no key low in response");
            return output;
        }
        output.ohlcv.low = static_cast<int>(low * price_prec);
        error            = doc["k"]["v"].get_double_in_string().get(volume);
        if (error != simdjson::SUCCESS) [[unlikely]] {
            loge("no key volume in response");
            return output;
        }
        output.ohlcv.volume = static_cast<int>(volume * qty_prec);
        std::string_view ticker;
        error = doc["k"]["s"].get_string().get(ticker);
        if (error != simdjson::SUCCESS) [[unlikely]] {
            loge("no ticker in response");
            return output;
        }
        if (pairs_reverse_.count(ticker)) [[likely]]
            output.trading_pair = pairs_reverse_.find(ticker)->second;
        else
            loge("pairs_reverse not contain {}", ticker);
    } catch (simdjson::simdjson_error& error) {
        loge("JSON error: {}", error.what());
    }
    return output;
}
