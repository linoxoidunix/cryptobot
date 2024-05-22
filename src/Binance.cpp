#include "aot/Binance.h"

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

    while (run_) {
        book_event_getter_->LaunchOne();
        Exchange::BookDiffSnapshot item;
        if (bool found = book_diff_lfqueue_.try_dequeue(item); found) {
            logd("fetch diff book event from exchange {}", item.ToString());
            fmtlog::poll();
        } 

        //using namespace std::literals::chrono_literals;

        //std::this_thread::sleep_for(100ms);
        time_manager_.Update();
    }
}

binance::GeneratorBidAskService::GeneratorBidAskService(
    Exchange::EventLFQueue* event_lfqueue, const Symbol* s,
    const DiffDepthStream::StreamIntervalI* interval)
    : event_lfqueue_(event_lfqueue), symbol_(s), interval_(interval) {
    book_event_getter_ = std::unique_ptr<BookEventGetter>(
        new BookEventGetter(symbol_, interval_));
}
