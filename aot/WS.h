#pragma once

#include <boost/asio/detached.hpp>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
// #include <boost/asio/experimental/as_single.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/system/error_code.hpp>

#include "aot/Logger.h"
#include "aot/Types.h"
#include "boost/asio/awaitable.hpp"

namespace beast     = boost::beast;          // from <boost/beast.hpp>
namespace http      = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;      // from <boost/beast/websocket.hpp>
namespace net       = boost::asio;           // from <boost/asio.hpp>
namespace ssl       = boost::asio::ssl;      // from <boost/asio/ssl.hpp>
using tcp           = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class WSSession;

class WS {
  public:
    explicit WS(boost::asio::io_context& ctx, std::string_view request,
                OnMessage msg_cb);
    void Run(std::string_view host, std::string_view port,
             std::string_view endpoint);
    WS(const WS&)                = delete;
    WS& operator=(const WS&)     = delete;
    WS(WS&&) noexcept            = default;
    WS& operator=(WS&&) noexcept = default;
    ~WS();

  private:
    std::shared_ptr<WSSession> session_;
    boost::asio::io_context& ioc_;
    boost::asio::ssl::context ctx_{boost::asio::ssl::context::tlsv12_client};
};

template <typename _Timeout>
class WssSession {
  public:
    using Timeout = _Timeout;

  private:
    std::atomic<bool> need_read_ = true;
    bool is_connected_           = false;
    bool is_used_                = false;
    bool is_expired_             = false;

    /**
     * @brief req variable must manage only via SetRequest() method
     *
     */
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> stream_;
    beast::flat_buffer buffer_;  // (Must persist between reads)
    std::string end_point_;
    std::string request_json_;
    OnWssResponse cb_;
    boost::asio::io_context& ioc_;
    Timeout timeout_;
    boost::asio::steady_timer timer_;  // Timer to track session expiration
  public:
    virtual ~WssSession() = default;
    explicit WssSession(boost::asio::io_context& ioc, ssl::context& ctx,
                        _Timeout timeout, const std::string_view host,
                        const std::string_view port,
                        const std::string_view default_endpoint)
        : resolver_(net::make_strand(ioc)),
          stream_(net::make_strand(ioc), ctx),
          ioc_(ioc),
          timeout_(timeout),
          timer_(ioc)  // Initialize the timer
    {
        net::co_spawn(ioc_,
                      Run(host.data(), port.data(), default_endpoint.data()),
                      [](std::exception_ptr e) {
                          if (e) std::rethrow_exception(e);
                      });
    }

    template <typename CompletionHandler>
    net::awaitable<bool> AsyncRequest(std::string&& req,
                                      CompletionHandler handler) {
        if (!IsConnected()) co_return false;
        if (IsUsed()) co_return false;

        is_used_      = true;
        cb_           = handler;
        request_json_ = std::move(req);
        co_await stream_.async_write(net::buffer(request_json_),
                                     boost::asio::use_awaitable);
        while (need_read_) {
            auto [ec, n] = co_await stream_.async_read(
                buffer_, boost::asio::as_tuple(boost::asio::use_awaitable));
            handler(buffer_);
            buffer_.consume(n);
        }
        co_await stream_.async_close(websocket::close_code::normal,
                                     net::use_awaitable);

        co_return true;
    }

    // Close the session gracefully
    void AsyncCloseSessionGracefully() { need_read_ = false; }

    inline bool IsConnected() const { return is_connected_; }
    inline bool IsExpired() const { return is_expired_; }
    inline bool IsUsed() const { return is_used_; }

  private:
    // Start the asynchronous operation
    net::awaitable<void> Run(const char* host, const char* port,
                             const char* default_end_point) {
        auto results =
            co_await resolver_.async_resolve(host, port, net::use_awaitable);
        if (!SSL_set_tlsext_host_name(stream_.next_layer().native_handle(),
                                      host)) {
            auto ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                                        net::error::get_ssl_category());
            loge("{}", ec.message());
            co_return;
        }
        start_timer();
        auto ep = co_await beast::get_lowest_layer(stream_).async_connect(
            results, net::use_awaitable);
        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(stream_).expires_never();

        co_await stream_.next_layer().async_handshake(ssl::stream_base::client,
                                                      net::use_awaitable);
        // Set suggested timeout settings for the websocket
        stream_.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client));
        // Set a decorator to change the User-Agent of the handshake
        stream_.set_option(
            websocket::stream_base::decorator([](websocket::request_type& req) {
                req.set(http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-client-async-ssl");
            }));
        co_await stream_.async_handshake(host, default_end_point,
                                         net::use_awaitable);

        is_connected_ = true;
        timer_.cancel();
        co_return;
    }

    void start_timer() {
        // Set the timer to expire after the specified timeout
        timer_.expires_after(timeout_);

        // Set up the handler to catch when the timer expires
        timer_.async_wait([this](beast::error_code ec) {
            if (ec !=
                boost::asio::error::operation_aborted) {  // Ignore if the timer
                                                          // was canceled
                fail(ec, "session expired. it is closed by async timer");
                close_session();
            }
        });
    }

    // Handle session expiration or failure
    void fail(beast::error_code ec, const char* what) {
        logi("{}: {}", what, ec.message());
        close_session();
    }

    // Close the session gracefully
    void close_session() {
        beast::get_lowest_layer(stream_).close();
        is_expired_ = true;
    }
};

template <typename _Timeout>
class WssSession2 {
  public:
    using Timeout = _Timeout;

  private:
    std::atomic<bool> need_read_ = true;
    bool is_connected_           = false;
    bool is_used_                = false;
    bool is_expired_             = false;

    /**
     * @brief req variable must manage only via SetRequest() method
     *
     */
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> stream_;
    beast::flat_buffer buffer_;  // (Must persist between reads)
    std::string end_point_;
    std::string request_json_;
    std::list<const OnWssResponse*> cbs_;
    boost::asio::io_context& ioc_;
    Timeout timeout_;
    boost::asio::steady_timer timer_;  // Timer to track session expiration
  public:
    virtual ~WssSession2() = default;
    explicit WssSession2(boost::asio::io_context& ioc, ssl::context& ctx,
                         _Timeout timeout, const std::string_view host,
                         const std::string_view port,
                         const std::string_view default_endpoint)
        : resolver_(net::make_strand(ioc)),
          stream_(net::make_strand(ioc), ctx),
          ioc_(ioc),
          timeout_(timeout),
          timer_(ioc)  // Initialize the timer
    {
        net::co_spawn(ioc_,
                      Run(host.data(), port.data(), default_endpoint.data()),
                      [](std::exception_ptr e) {
                          if (e) std::rethrow_exception(e);
                      });
    }

    template <typename CompletionHandler>
    net::awaitable<bool> AsyncRequest(std::string&& req,
                                      const CompletionHandler* handler) {
        request_json_ = std::move(req);
        if (!IsConnected()) co_return false;
        if (IsUsed()) co_return false;

        is_used_                 = true;
        auto [ec, bytes_written] = co_await stream_.async_write(
            net::buffer(request_json_),
            boost::asio::as_tuple(boost::asio::use_awaitable));
        if (ec) {
            close_session();
            co_return false;
        }
        cbs_.emplace_back(handler);
        while (need_read_) {
            auto [ec, n] = co_await stream_.async_read(
                buffer_, boost::asio::as_tuple(boost::asio::use_awaitable));
            if (ec) {
                close_session();
                co_return false;
            }
            for (auto& cb_ptr : cbs_) {
                if (cb_ptr) {
                    try {
                        auto& cb = *cb_ptr;
                        cb(buffer_);  // Call the handler with the buffer
                                      // content
                    } catch (const std::exception& e) {
                        logi("Handler exception: {}", e.what());
                    }
                }
            }
            buffer_.consume(n);
        }
        co_await stream_.async_close(websocket::close_code::normal,
                                     net::use_awaitable);

        co_return true;
    }

    // Close the session gracefully
    void AsyncCloseSessionGracefully() { need_read_ = false; }

    inline bool IsConnected() const { return is_connected_; }
    inline bool IsExpired() const { return is_expired_; }
    inline bool IsUsed() const { return is_used_; }

  private:
    // Start the asynchronous operation
    net::awaitable<void> Run(const char* host, const char* port,
                             const char* default_end_point) {
        auto results =
            co_await resolver_.async_resolve(host, port, net::use_awaitable);
        if (!SSL_set_tlsext_host_name(stream_.next_layer().native_handle(),
                                      host)) {
            auto ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                                        net::error::get_ssl_category());
            loge("{}", ec.message());
            co_return;
        }
        start_timer();
        auto ep = co_await beast::get_lowest_layer(stream_).async_connect(
            results, net::use_awaitable);
        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(stream_).expires_never();

        co_await stream_.next_layer().async_handshake(ssl::stream_base::client,
                                                      net::use_awaitable);
        // Set suggested timeout settings for the websocket
        stream_.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client));
        // Set a decorator to change the User-Agent of the handshake
        stream_.set_option(
            websocket::stream_base::decorator([](websocket::request_type& req) {
                req.set(http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-client-async-ssl");
            }));
        co_await stream_.async_handshake(host, default_end_point,
                                         net::use_awaitable);

        is_connected_ = true;
        timer_.cancel();
        co_return;
    }

    void start_timer() {
        // Set the timer to expire after the specified timeout
        timer_.expires_after(timeout_);

        // Set up the handler to catch when the timer expires
        timer_.async_wait([this](beast::error_code ec) {
            if (ec !=
                boost::asio::error::operation_aborted) {  // Ignore if the timer
                                                          // was canceled
                fail(ec, "session expired. it is closed by async timer");
                close_session();
            }
        });
    }

    // Handle session expiration or failure
    void fail(beast::error_code ec, const char* what) {
        logi("{}: {}", what, ec.message());
        close_session();
    }

    // Close the session gracefully
    void close_session() {
        beast::get_lowest_layer(stream_).close();
        is_expired_ = true;
    }
};