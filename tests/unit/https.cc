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
#include "aot/Https.h"

using namespace testing;
using namespace boost;
using namespace std::literals::chrono_literals;

class HttpsSessionTest : public ::testing::Test {
  protected:
    boost::asio::io_context io_context;
    boost::asio::ssl::context ssl_context{
        boost::asio::ssl::context::tlsv12_client};

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
    V2::HttpsSession session(io_context, ssl_context, host, port, std::chrono::seconds(30));
    bool connected = false;
    // session->Resolve(host.data(), port.data());
    // Expect the response callback to be called once

    // Start the connection process

    // Run the io_context to process async operations
    std::thread t([this] {
        auto work_guard = boost::asio::make_work_guard(io_context);
        
        io_context.run();
    });

    std::thread t1([this, &session, &host, &connected] {
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
        http::request<http::string_body> req(http::verb::get, "/", 11);

        req.set(boost::beast::http::field::host, host.data());
        req.set(boost::beast::http::field::user_agent,
                BOOST_BEAST_VERSION_STRING);
        /**
         * @brief For POST, PUT, and DELETE endpoints, the parameters may be
         * sent as a query string or in the request body with content type
         * application/x-www-form-urlencoded. You may mix parameters between
         * both the query string and request body if you wish to do so.
         *
         */
        req.set(boost::beast::http::field::content_type,
                "application/x-www-form-urlencoded");
        auto status = session.AsyncRequest(std::move(req),
                             [&connected](boost::beast::http::response<boost::beast::http::string_body>& buffer) {
            //const auto& resut = buffer.body();
            //logi("{}", resut);
            connected = true;
        });
        EXPECT_EQ(status, true);
        std::this_thread::sleep_for(5s);
        io_context.stop();
    });

    t.join();
    t1.join();
    // You can check for connected state here, or any other state you want
    // For example:
    fmtlog::poll();
    EXPECT_EQ(connected, true);
}

class ConnectionPoolTest : public ::testing::Test {
protected:
    boost::asio::io_context ioc;
    ssl::context ssl_ctx{ssl::context::sslv23};
    std::string_view host = "testnet.binance.vision";
    std::string_view port = "443";
    std::size_t pool_size = 5;
    using HTTPSes =  V2::HttpsSession<std::chrono::seconds>;
    V2::ConnectionPool<HTTPSes> pool;
    ConnectionPoolTest() 
        : pool(ioc, ssl_ctx, host, port, pool_size, HTTPSes::Timeout{30}) {}

    // Helper function to simulate connection usage
};

TEST_F(ConnectionPoolTest, ConnectToBinanceWithConnectionPool) {
    fmtlog::setLogLevel(fmtlog::DBG);
    std::string_view host = "testnet.binance.vision";
    std::string_view port = "443";
    //V2::ConnectionPool pool(io_context, ssl_context, host, port, std::chrono::seconds(30));
    //V2::HttpsSession session(io_context, ssl_context, host, port, std::chrono::seconds(30));
    auto session_ = pool.AcquireConnection();
    auto &session = *session_;
    bool connected = false;
    // session->Resolve(host.data(), port.data());
    // Expect the response callback to be called once

    // Start the connection process

    // Run the io_context to process async operations
    std::thread t([this] {
        auto work_guard = boost::asio::make_work_guard(ioc);
        
        ioc.run();
    });

    std::thread t1([this, &session, &host, &connected] {
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
        http::request<http::string_body> req(http::verb::get, "/", 11);

        req.set(boost::beast::http::field::host, host.data());
        req.set(boost::beast::http::field::user_agent,
                BOOST_BEAST_VERSION_STRING);
        /**
         * @brief For POST, PUT, and DELETE endpoints, the parameters may be
         * sent as a query string or in the request body with content type
         * application/x-www-form-urlencoded. You may mix parameters between
         * both the query string and request body if you wish to do so.
         *
         */
        req.set(boost::beast::http::field::content_type,
                "application/x-www-form-urlencoded");
        auto status = session.AsyncRequest(std::move(req),
                             [&connected](boost::beast::http::response<boost::beast::http::string_body>& buffer) {
            //const auto& resut = buffer.body();
            //logi("{}", resut);
            connected = true;
        });
        EXPECT_NE(status, false);
        pool.ReleaseConnection(&session);
        std::this_thread::sleep_for(5s);
        ioc.stop();
    });

    t.join();
    t1.join();
    // You can check for connected state here, or any other state you want
    // For example:
    fmtlog::poll();
    EXPECT_EQ(connected, true);
}

TEST_F(ConnectionPoolTest, CheckTimeoutConnection) {
    fmtlog::setLogLevel(fmtlog::DBG);
    std::string_view host = "testnet.binance.vision";
    std::string_view port = "443";
    //V2::ConnectionPool pool(io_context, ssl_context, host, port, std::chrono::seconds(30));
    //V2::HttpsSession session(io_context, ssl_context, host, port, std::chrono::seconds(30));
    auto session_ = pool.AcquireConnection();
    auto &session = *session_;
    bool connected = false;
    // session->Resolve(host.data(), port.data());
    // Expect the response callback to be called once

    // Start the connection process

    // Run the io_context to process async operations
    std::thread t([this] {
        auto work_guard = boost::asio::make_work_guard(ioc);
        
        ioc.run();
    });
    
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(31s);
    pool.ReleaseConnection(session_);

    std::thread t1([this, &session, &host, &connected] {
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
        http::request<http::string_body> req(http::verb::get, "/", 11);

        req.set(boost::beast::http::field::host, host.data());
        req.set(boost::beast::http::field::user_agent,
                BOOST_BEAST_VERSION_STRING);
        /**
         * @brief For POST, PUT, and DELETE endpoints, the parameters may be
         * sent as a query string or in the request body with content type
         * application/x-www-form-urlencoded. You may mix parameters between
         * both the query string and request body if you wish to do so.
         *
         */
        req.set(boost::beast::http::field::content_type,
                "application/x-www-form-urlencoded");
        auto status = session.AsyncRequest(std::move(req),
                             [&connected](boost::beast::http::response<boost::beast::http::string_body>& buffer) {
            //const auto& resut = buffer.body();
            //logi("{}", resut);
            connected = true;
        });
        EXPECT_NE(status, false);

        std::this_thread::sleep_for(5s);
        ioc.stop();
    });

    t.join();
    t1.join();
    // You can check for connected state here, or any other state you want
    // For example:
    fmtlog::poll();
    EXPECT_EQ(connected, true);
}

// Test that connections can be acquired from the pool
TEST_F(ConnectionPoolTest, AcquireConnection) {
    auto session = pool.AcquireConnection();
    EXPECT_NE(session, nullptr);
    pool.ReleaseConnection(session);
}

// Test that connections can be released back to the pool
TEST_F(ConnectionPoolTest, ReleaseConnection) {
    auto session = pool.AcquireConnection();
    EXPECT_NE(session, nullptr);
    pool.ReleaseConnection(session); // Release it back
}

//Test acquiring multiple connections from the pool
TEST_F(ConnectionPoolTest, MultipleConnections) {
    std::vector<HTTPSes*> sessions;

    for (std::size_t i = 0; i < pool_size; ++i) {
        auto session = pool.AcquireConnection();
        EXPECT_NE(session, nullptr);
        sessions.push_back(session);
    }

    // Ensure that pool can AcquireConnection()
    EXPECT_NE(pool.AcquireConnection(), nullptr); 

    // Release all sessions back to the pool
    for (auto session : sessions) {
        pool.ReleaseConnection(session);
    }
}

// Test if the pool can handle releasing a connection that was never acquired
TEST_F(ConnectionPoolTest, ReleaseUnacquiredConnection) {
    HTTPSes* session = nullptr; // Simulate an unacquired session
    EXPECT_NO_THROW(pool.ReleaseConnection(session)); // Should not throw
}

// Test if the pool can handle exceeding the limit of connections
TEST_F(ConnectionPoolTest, ExceedConnectionLimit) {
    std::vector<HTTPSes*> sessions;

    // Acquire connections up to the limit
    for (std::size_t i = 0; i < pool_size; ++i) {
        auto session = pool.AcquireConnection();
        EXPECT_NE(session, nullptr);
        sessions.push_back(session);
    }

    // Attempt to acquire one more connection
    EXPECT_NE(pool.AcquireConnection(), nullptr); // Should return nullptr

    // Release all sessions back to the pool
    for (auto session : sessions) {
        pool.ReleaseConnection(session);
    }
}

// Main function to run the tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}