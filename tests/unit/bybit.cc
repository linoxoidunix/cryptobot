#include "aot/Bybit.h"

#include <gtest/gtest.h>

#include "aot/Exchange.h"
#include "aot/WS.h"
#include "aot/common/types.h"
#include "boost/asio.hpp"
#include "boost/asio/io_context.hpp"

struct LogPolling {
    std::shared_ptr<boost::asio::steady_timer> timer;
    std::function<void(const boost::system::error_code&)> poll;

    LogPolling(boost::asio::thread_pool& pool,
               std::chrono::milliseconds interval)
        : timer(std::make_shared<boost::asio::steady_timer>(pool)) {
        StartPolling(interval);
    }

    void Stop() {
        if (timer) {
            timer->cancel();  // Stop polling by canceling the timer
        }
    }

  private:
    void StartPolling(std::chrono::milliseconds interval) {
        poll = [this, self = timer,
                interval](const boost::system::error_code& ec) mutable {
            if (!ec) {
                fmtlog::poll();  // Perform log polling
                self->expires_after(interval);
                self->async_wait(poll);  // Schedule the next poll
            }
        };

        timer->expires_after(interval);
        timer->async_wait(poll);
    }
};

// Handler function to be called when the timer expires
void StopIoContext(boost::asio::io_context& io,
                   const boost::system::error_code& ec) {
    if (!ec) {
        io.stop();
    } else {
        std::cerr << "Error: " << ec.message() << std::endl;
    }
};

// Google Test Fixture for the BookEventGetterComponent test
class BookEventGetterComponentTest : public ::testing::Test {
  protected:
    boost::asio::io_context io_context;
    ::V2::ConnectionPool<WSSesionType3, const std::string_view&> session_pool{
        io_context, WSSesionType3::Timeout{30}, 1, "stream.bybit.com",
        "443",      "/v5/public/spot"};
    size_t number_responses = 100;
    common::TradingPairHashMap pairs;
    boost::asio::thread_pool thread_pool;

    // The component we are going to test

    void SetUp() override {
        common::TradingPairInfo pair_info{.price_precission     = 2,
                                          .qty_precission       = 6,
                                          .https_json_request   = "BTCUSDT",
                                          .https_query_request  = "BTCUSDT",
                                          .ws_query_request     = "btcusdt",
                                          .https_query_response = "BTCUSDT"};
        pairs[{2, 1}] = pair_info;
    }

    void TearDown() override {
        // Any cleanup needed after each test
    }
};

// Test case for AsyncHandleEvent
TEST_F(BookEventGetterComponentTest, TestAsyncHandleEventBybit) {
    // boost::asio::steady_timer timer(io_context, std::chrono::seconds(20));

    fmtlog::setLogLevel(fmtlog::DBG);
    LogPolling log_polling(thread_pool, std::chrono::milliseconds(100));
    aot::CoBus bus(thread_pool);

    common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs);

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard(io_context.get_executor());
    std::thread t([this] { io_context.run(); });
    // timer.async_wait(std::bind(&StopIoContext, std::ref(io_context),
    // std::placeholders::_1));
    boost::asio::cancellation_signal cancel_signal;

    bybit::BookEventGetterComponent component(
        boost::asio::make_strand(thread_pool), number_responses,
        TypeExchange::TESTNET, pairs, &session_pool, cancel_signal);

    Exchange::RequestDiffOrderBook request;
    request.exchange_id  = common::ExchangeId::kBybit;
    request.trading_pair = {2, 1};
    /**
     * @brief
     * https://bybit-exchange.github.io/docs/v5/websocket/public/orderbook for
     * bybit depth = freq
     */
    request.frequency    = 50;

    Exchange::BusEventRequestDiffOrderBook bus_event_request(nullptr, &request);

    uint64_t counter_successfull   = 0;
    uint64_t counter_unsuccessfull = 0;

    OnWssResponse wss_cb               = [&component, &counter_successfull,
                        &counter_unsuccessfull, this,
                        &pair_reverse, &bus](boost::beast::flat_buffer& fb) {
        auto data     = fb.data();  // returns a const_buffer
        auto response = std::string_view(static_cast<const char*>(data.data()),
                                                       data.size());
        bybit::detail::FamilyBookEventGetter::ParserResponse parser(
            pairs, pair_reverse);
        auto result = parser.Parse(response);
        if (std::holds_alternative<Exchange::BookSnapshot>(result) ||
            std::holds_alternative<Exchange::BookDiffSnapshot>(result)) {
            std::visit(
                [&](auto&& snapshot) {
                    using T = std::decay_t<decltype(snapshot)>;
                    if constexpr (std::is_same_v<T,
                                                               Exchange::BookDiffSnapshot>) {
                        std::cout << "Received a BookDiffSnapshot!"
                                  << std::endl;
                        Exchange::BookDiffSnapshot& diffSnapshot = snapshot;
                        auto request = component.book_diff_mem_pool_.Allocate(
                            &component.book_diff_mem_pool_,
                            diffSnapshot.exchange_id, diffSnapshot.trading_pair,
                            std::move(diffSnapshot.bids),
                            std::move(diffSnapshot.asks), diffSnapshot.first_id,
                            diffSnapshot.last_id);
                        auto intr_ptr_request =
                            boost::intrusive_ptr<Exchange::BookDiffSnapshot2>(
                                request);

                        auto bus_event =
                            component.bus_event_book_diff_snapshot_mem_pool_
                                .Allocate(
                                    &component
                                         .bus_event_book_diff_snapshot_mem_pool_,
                                    intr_ptr_request);
                        auto intr_ptr_bus_request = boost::intrusive_ptr<
                                          Exchange::BusEventBookDiffSnapshot>(bus_event);

                        bus.AsyncSend(&component, intr_ptr_bus_request);

                        // Use diffSnapshot
                    } else if constexpr (std::is_same_v<
                                                           T, Exchange::BookSnapshot>) {
                        std::cout << "Received a BookSnapshot!" << std::endl;
                        Exchange::BookSnapshot& book_snapshot = snapshot;
                        // Use bookSnapshot
                        book_snapshot.trading_pair = {2, 1};
                        auto ptr              = component.snapshot_mem_pool_.Allocate(
                            &component.snapshot_mem_pool_, book_snapshot.exchange_id,
                            book_snapshot.trading_pair, std::move(book_snapshot.bids),
                            std::move(book_snapshot.asks), book_snapshot.lastUpdateId);
                        auto intr_ptr_snapsot =
                            boost::intrusive_ptr<Exchange::BookSnapshot2>(ptr);

                        auto bus_event =
                            component.bus_event_response_snapshot_mem_pool_.Allocate(
                                &component.bus_event_response_snapshot_mem_pool_,
                                intr_ptr_snapsot);
                        auto intr_ptr_bus_snapshot =
                            boost::intrusive_ptr<Exchange::BusEventResponseNewSnapshot>(
                                bus_event);

                        bus.AsyncSend(&component, intr_ptr_bus_snapshot);
                    }
                },
                result);
        } else {
            std::cout << "Invalid or unknown response." << std::endl;
        }

        // if ((result.exchange_id ==
        //         common::ExchangeId::kBybit) &&
        //     (result.trading_pair ==
        //         common::TradingPair(2, 1)))
        //     counter_successfull++;
        // else
        //     counter_unsuccessfull++;
        // if (counter_successfull == 5) {
        //     component.AsyncStop();
        // }
    };
    auto cancelation_slot = cancel_signal.slot();
    component.RegisterCallback(request.trading_pair, &wss_cb);
    component.AsyncHandleEvent(&bus_event_request);
    thread_pool.join();
    session_pool.CloseAllSessions();
    work_guard.reset();
    t.join();
    EXPECT_GE(counter_successfull, 5);
    EXPECT_EQ(counter_unsuccessfull,
              1);  // first response from bybit is status response
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}