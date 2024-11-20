#include "boost/asio.hpp"
#include "boost/asio/io_context.hpp"

#include <gtest/gtest.h>

#include "aot/Binance.h"
#include "aot/Exchange.h"
#include "aot/WS.h"
#include "aot/common/types.h"


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
        io_context, WSSesionType3::Timeout{30}, 1, "testnet.binance.vision", "443",
        "/ws"};
    size_t number_responses = 100;
    common::TradingPairHashMap pairs;
    boost::asio::thread_pool thread_pool;

    // The component we are going to test

    void SetUp() override {
        common::TradingPairInfo pair_info{.price_precission     = 2,
                                          .qty_precission       = 5,
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
TEST_F(BookEventGetterComponentTest, TestAsyncHandleEvent) {
    //boost::asio::steady_timer timer(io_context, std::chrono::seconds(20));

    fmtlog::setLogLevel(fmtlog::DBG);
    common::TradingPairReverseHashMap pair_reverse = common::InitTPsJR(pairs);

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard(io_context.get_executor());
    std::thread t([this] { io_context.run(); });
    //timer.async_wait(std::bind(&StopIoContext, std::ref(io_context), std::placeholders::_1));
    boost::asio::cancellation_signal cancel_signal;

    binance::BookEventGetterComponent component(
        boost::asio::make_strand(thread_pool), number_responses,
        TypeExchange::TESTNET, pairs, &session_pool, cancel_signal);

    Exchange::RequestDiffOrderBook request;
    request.exchange_id  = common::ExchangeId::kBinance;
    request.trading_pair = {2, 1};

    //Exchange::BusEventRequestDiffOrderBookPool mem_pool(5);
    Exchange::BusEventRequestDiffOrderBook bus_event_request(nullptr, &request);
    //bus_event_request.AddReference();

    uint64_t counter_successfull   = 0;
    uint64_t counter_unsuccessfull = 0;

    OnWssResponse cb = [&component, &counter_successfull, &counter_unsuccessfull, this,
                        &pair_reverse](boost::beast::flat_buffer& fb) {
        auto data     = fb.data();  // returns a const_buffer
        auto response = std::string_view(static_cast<const char*>(data.data()),
                                         data.size());
        binance::detail::FamilyBookEventGetter::ParserResponse parser(
            pairs, pair_reverse);
        auto result = parser.Parse(response);
        if ((result.exchange_id ==
                common::ExchangeId::kBinance) && 
            (result.trading_pair ==
                common::TradingPair(2, 1)))
            counter_successfull++;
        else
            counter_unsuccessfull++;
        if (counter_successfull == 5) {
            component.AsyncStop();
        }
    };
    auto cancelation_slot = cancel_signal.slot();
    component.RegisterCallback(request.trading_pair, &cb);
    component.AsyncHandleEvent(&bus_event_request);
    thread_pool.join();
    session_pool.CloseAllSessions();
    work_guard.reset();
    t.join();
    EXPECT_GE(counter_successfull, 5);
    EXPECT_EQ(counter_unsuccessfull, 1);//first response from binance is status response
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}