#include "aot/Https.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <string>

#include "aot/Logger.h"
#include "aot/session_status.h"

using namespace testing;
using namespace boost;
using namespace std::literals::chrono_literals;

class HttpsSessionTest : public ::testing::Test {
  protected:
    boost::asio::io_context io_context;
    boost::asio::ssl::context ssl_context{
        boost::asio::ssl::context::tlsv12_client};
    boost::asio::thread_pool pool_;
    void SetUp() override {
        load_root_certificates(ssl_context);
        ssl_context.set_verify_mode(ssl::verify_peer);
        // Create an instance of HttpsSession
    }
};

// Test to check connection to Binance
TEST_F(HttpsSessionTest, ConnectToBinance) {
    fmtlog::setLogLevel(fmtlog::DBG);
    std::string_view host = "testnet.binance.vision";
    std::string_view port = "443";
    V2::HttpsSession3 session(io_context, ssl_context, std::chrono::seconds(30),
                              host, port);
    bool connected = false;
    // session->Resolve(host.data(), port.data());
    // Expect the response callback to be called once

    // Start the connection process

    // Run the io_context to process async operations
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard(io_context.get_executor());
    std::thread io_thread([this] { io_context.run(); });

    // Register callback to verify the response
    session.RegisterOnResponse(
        [&connected](
            boost::beast::http::response<boost::beast::http::string_body>&
                res) {
            connected = true;
            logi("Received response: {}", res.body());
        });

    // Register callback to close the io_context when session closes
    session.RegisterOnUserClosed([&]() { io_context.stop(); });
    // Create an HTTP GET request
    boost::beast::http::request<boost::beast::http::string_body> req(
        boost::beast::http::verb::get, "/", 11);
    req.set(boost::beast::http::field::host, host.data());
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // wait until session ready
    while (session.GetStatus() != aot::StatusSession::Ready) {
    }
    // Launch the async request
    boost::asio::co_spawn(
        pool_,
        [&session, req]() mutable -> boost::asio::awaitable<void> {
            bool status = co_await session.AsyncRequest(std::move(req));
            EXPECT_TRUE(status);
            co_return;
        },
        boost::asio::detached);

    io_thread.join();
    pool_.join();
    fmtlog::poll();
    EXPECT_TRUE(connected);
}

class ConnectionPoolTest : public ::testing::Test {
  protected:
    boost::asio::io_context ioc;
    std::string_view host = "testnet.binance.vision";
    std::string_view port = "443";
    std::size_t pool_size = 1;
    using HTTPSes         = V2::HttpsSession3<std::chrono::seconds>;
    V2::ConnectionPool<HTTPSes> connection_pool_;
    boost::asio::thread_pool pool_;

    ConnectionPoolTest()
        : connection_pool_(ioc, HTTPSes::Timeout{30}, pool_size, host, port) {}

    void SetUp() override {

    }

    // Helper function to simulate connection usage
    boost::asio::awaitable<void> UseConnection(
        V2::HttpsSession3<std::chrono::seconds>& session) {
        boost::beast::http::request<boost::beast::http::string_body> req(
            boost::beast::http::verb::get, "/", 11);
        req.set(boost::beast::http::field::host, host.data());
        req.set(boost::beast::http::field::user_agent,
                BOOST_BEAST_VERSION_STRING);

        bool success = co_await session.AsyncRequest(std::move(req));
        EXPECT_TRUE(success);
        co_return;
    }
};

// Test to check connection pool initialization and session reuse
TEST_F(ConnectionPoolTest, PoolInitializationAndReuse) {
    fmtlog::setLogLevel(fmtlog::DBG);
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard(ioc.get_executor());
    std::thread io_thread([&]() { ioc.run(); });
    // Test acquiring a connection from the pool
    boost::asio::co_spawn(
        pool_,
        [this, &work_guard]() -> boost::asio::awaitable<void> {
            auto session = connection_pool_.AcquireConnection();
            EXPECT_NE(session, nullptr);
            session->RegisterOnUserClosed([this, &work_guard]() {
                connection_pool_.CloseAllSessions();
                work_guard.reset();
            });
            // Use the connection for an HTTP request
            co_await UseConnection(*session);

            // Release the connection back to the pool
            co_return;
        },
        boost::asio::detached);

    // boost::asio::co_spawn(
    //     pool_,
    //     [&]() -> boost::asio::awaitable<void> {
    //         auto session = pool.AcquireConnection();
    //         EXPECT_NE(session, nullptr);
    //         session->RegisterOnClosed([&]() { ioc.stop(); });
    //         // Use the connection for an HTTP request
    //         co_await UseConnection(*session);

    //         // Release the connection back to the pool
    //         co_return;
    //     },
    //     boost::asio::detached);
    // Run io_context to process asynchronous operations


    io_thread.join();

    // Ensure all connections are returned to the pool
}

// Test to check connection expiration or failure handling
// TEST_F(ConnectionPoolTest, ConnectionExpirationHandling) {
//     fmtlog::setLogLevel(fmtlog::DBG);

//     // Acquire all connections from the pool
//     boost::asio::co_spawn(
//         ioc,
//         [&]() -> boost::asio::awaitable<void> {
//             for (std::size_t i = 0; i < pool_size; ++i) {
//                 auto session = co_await pool.AcquireConnection();
//                 EXPECT_NE(session, nullptr);

//                 // Simulate a failure or expiration
//                 session->AsyncCloseSessionGracefully();
//             }
//             co_return;
//         },
//         boost::asio::detached);

//     // Run io_context to process asynchronous operations
//     boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
//         work_guard(ioc.get_executor());
//     std::thread io_thread([&]() { ioc.run(); });

//     io_thread.join();

//     // Ensure all failed connections are removed from the pool
// }

// Main function to run the tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}