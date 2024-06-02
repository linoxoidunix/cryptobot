#include "aot/Binance.h"
#include <set>
#include "aot/Logger.h"
#include "fast_double_parser.h"
#include "simdjson.h"

Exchange::BookSnapshot binance::BookSnapshot::ParserResponse::Parse(
    std::string_view response) {
    Exchange::BookSnapshot book_snapshot;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    book_snapshot.lastUpdateId       = doc["lastUpdateId"].get_uint64();

    for (auto all : doc["bids"]) {
        simdjson::ondemand::array arr = all.get_array();
        std::array<double, 2> pair;
        uint8_t i = 0;
        for (auto number : arr) {
            pair[i] = number.get_double_in_string();
            i++;
        }
        book_snapshot.bids.emplace_back(pair[0], pair[1]);
    }
    for (auto all : doc["asks"]) {
        simdjson::ondemand::array arr = all.get_array();
        std::array<double, 2> pair;
        uint8_t i = 0;
        for (auto number : arr) {
            pair[i] = number.get_double_in_string();
            i++;
        }
        book_snapshot.asks.emplace_back(pair[0], pair[1]);
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

    for (auto all : doc["b"]) {
        simdjson::ondemand::array arr = all.get_array();
        std::array<double, 2> pair;
        uint8_t i = 0;
        for (auto number : arr) {
            pair[i] = number.get_double_in_string();
            i++;
        }
        book_diff_snapshot.bids.emplace_back(pair[0], pair[1]);
    }
    for (auto all : doc["a"]) {
        simdjson::ondemand::array arr = all.get_array();
        std::array<double, 2> pair;
        uint8_t i = 0;
        for (auto number : arr) {
            pair[i] = number.get_double_in_string();
            i++;
        }
        book_diff_snapshot.asks.emplace_back(pair[0], pair[1]);
    }
    return book_diff_snapshot;
}

auto binance::GeneratorBidAskService::Run() noexcept -> void {
    logd("GeneratorBidAskService start");
    book_event_getter_->Init(book_diff_lfqueue_);
    logd("start subscribe book diff binance");
    fmtlog::poll();
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
            Exchange::MEMarketUpdateDouble event_clear_queue;
            event_clear_queue.type = Exchange::MarketUpdateType::CLEAR;
            logd("clear order book. try make snapshot");
            event_lfqueue_->enqueue(event_clear_queue);

            binance::BookSnapshot::ArgsOrder args{
                ticker_.symbol->ToString(), 1000};  // TODO parametrize 1000
            binance::BookSnapshot book_snapshoter(
                std::move(args), type_exchange_,
                &snapshot_);  // TODO parametrize 1000
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

        fmtlog::poll();
        time_manager_.Update();
    }
}

binance::GeneratorBidAskService::GeneratorBidAskService(
    Exchange::EventLFQueue* event_lfqueue, const Ticker& ticker,
    const DiffDepthStream::StreamIntervalI* interval,
    TypeExchange type_exchange)
    : event_lfqueue_(event_lfqueue),
      ticker_(ticker),
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
    book_event_getter_ = std::make_unique<BookEventGetter>(
        ticker.symbol, interval, type_exchange_);
}

Exchange::MEClientResponse binance::OrderNewLimit::ParserResponse::Parse(
    std::string_view response) {
    Exchange::MEClientResponse output;
    simdjson::ondemand::parser parser;
    simdjson::padded_string my_padded_data(response.data(), response.size());
    simdjson::ondemand::document doc = parser.iterate(my_padded_data);
    output.order_id =
        doc["clientOrderId"].get_uint64();  // TODO check this is uint64
    std::string_view status;
    auto error_status = doc["status"].get_string().get(status);
    if (error_status) {
        loge("no key status in response");
        return output;
    }
    std::set<std::string> success_status{"NEW", "PARTIALLY_FILLED", "FILLED"};
    if (!success_status.count(status.data())) return output;
    std::set<std::string> accepted_status{"NEW"};
    if (success_status.count(status.data()))
        output.type = Exchange::ClientResponseType::ACCEPTED;
    std::set<std::string> filled_status{"PARTIALLY_FILLED", "FILLED"};
    if (success_status.count(status.data()))
        output.type = Exchange::ClientResponseType::FILLED;
    auto error = doc["cummulativeQuoteQty"].get_double().get(output.price);
    if (error) [[unlikely]] {
        loge("no key cummulativeQuoteQty in response");
    }
    error = doc["executedQty"].get_double().get(output.exec_qty);
    if (error) [[unlikely]] {
        loge("no key executedQty in response");
    } else {
        double orig_qty;
        error = doc["origQty"].get_double().get(orig_qty);
        if (error) [[unlikely]] {
            loge("no key origQty in response");
        } else {
            output.leaves_qty = orig_qty - output.exec_qty;
        }
    }
    std::string_view ticker;
    auto error_symbol = doc["symbol"].get_string().get(ticker);
    if (error_symbol) [[unlikely]] {
        loge("no key symbol in response");
        return output;
    }
    output.ticker = ticker;
    return output;
}
