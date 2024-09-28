#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/noncopyable.hpp>

#include "concurrentqueue.h"

#include "aot/Exchange.h"
#include "aot/Types.h"
#include "aot/root_certificates.hpp"
#include "aot/common/mem_pool.h"

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http  = beast::http;           // from <boost/beast/http.hpp>
namespace net   = boost::asio;           // from <boost/asio.hpp>
namespace ssl   = boost::asio::ssl;      // from <boost/asio/ssl.hpp>
using tcp       = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class HttpsSession;

class Https {
  public:
    explicit Https(boost::asio::io_context& ctx, OnHttpsResponce msg_cb);
    void Run(
        std::string_view host, std::string_view port, std::string_view endpoint,
        boost::beast::http::request<boost::beast::http::string_body>&& req);
    Https(const Https&)                = delete;
    Https& operator=(const Https&)     = delete;
    Https(Https&&) noexcept            = default;
    Https& operator=(Https&&) noexcept = default;
    ~Https()                           = default;

  private:
    std::shared_ptr<HttpsSession> session_;
    // boost::asio::io_context& ioc_;
    boost::asio::ssl::context ctx_{boost::asio::ssl::context::tlsv12_client};
};

namespace V2 {
template<typename _Timeout>
class HttpsSession {
  public:
    using Timeout = _Timeout;
private:
    bool connected_ = false;
    tcp::resolver resolver_;
    beast::ssl_stream<beast::tcp_stream> stream_;
    //http::request<http::string_body> req_;
    beast::flat_buffer buffer_;  // (Must persist between reads)
    http::response<http::string_body> res_;
    OnHttpsResponce cb_;
    boost::asio::io_context& ioc_;
    Timeout timeout_;
public:
    explicit HttpsSession(boost::asio::io_context& ioc, ssl::context& ctx, const std::string_view host, const std::string_view port, _Timeout timeout)
        : resolver_(net::make_strand(ioc)),
          stream_(net::make_strand(ioc), ctx),
          ioc_(ioc), timeout_(timeout){
            Run(host.data(), port.data(), nullptr, {});
          }

    template <typename CompletionHandler>
    bool AsyncRequest(http::request<http::string_body>&& req, CompletionHandler handler) {
        if(!connected_)
          return false;
        // Store the completion handler
        // handler_ = handler;
        cb_ = handler;
        // // Set up the HTTP GET request
        // http::request<http::string_body> req{http::verb::get, target, 11};
        // req.set(http::field::host, host_);
        // req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request asynchronously
        http::async_write(stream_, req,
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                this->on_write(ec, bytes_transferred);
            });
        return true;
    }

    // Start the asynchronous operation
    void Run(char const* host, char const* port, char const* target,
             http::request<http::string_body>&& req) {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        logi("start sni host name");
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host)) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                 net::error::get_ssl_category()};
            std::cerr << ec.message() << "\n";
            return;
        }
        logi("end sni host name");

        // Set up the HTTP request
        //req_ = std::move(req);
        logi("start to resolve");
        // Look up the domain name
        resolver_.async_resolve(
            host, port,
            [this, host, port](beast::error_code ec, tcp::resolver::results_type results) {
                this->on_resolve(ec, results);
            });
    }

private:
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        logi("completed to resolve");
        if (ec) return fail(ec, "resolve");

        // Set a timeout on the operation
        beast::get_lowest_layer(stream_).expires_after(timeout_);
        logi("start connect to exchange");

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream_).async_connect(
            results, [this](beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
                this->on_connect(ec);
            });
    }

    void on_connect(beast::error_code ec) {
        logi("completed connect to exchange");
        if (ec) return fail(ec, "connect");

        // Perform the SSL handshake
        logi("start handshake");
        stream_.async_handshake(ssl::stream_base::client,
            [this](beast::error_code ec) {
                this->on_handshake(ec);
            });
    }

    void on_handshake(beast::error_code ec) {
        logi("completed handshake");
        if (ec) return fail(ec, "handshake");

        // Set a timeout on the operation
        beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
        logi("start write operation");
        connected_ = true;
        // Send the HTTP request to the remote host
        // http::async_write(stream_, req_,
        //     [this](beast::error_code ec, std::size_t bytes_transferred) {
        //         this->on_write(ec, bytes_transferred);
        //     });
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        logi("finished write to exchange");
        boost::ignore_unused(bytes_transferred);

        if (ec) return fail(ec, "write");

        // Receive the HTTP response
        http::async_read(stream_, buffer_, res_,
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                this->on_read(ec, bytes_transferred);
            });
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) return fail(ec, "read");

        // Write the response to the callback
        cb_(res_);

        // Set a timeout on the operation
        beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        // Gracefully close the stream
        beast::get_lowest_layer(stream_).socket().cancel(ec);
        if (ec == net::error::eof) {
            ec = {};  // Clear EOF error
        }
        if (ec) return fail(ec, "cancel");

        // You can optionally close the stream or perform any further actions here
    }

    void fail(beast::error_code ec, char const* what) {
        std::cerr << what << ": " << ec.message() << "\n";
        // Handle failure case, e.g., cleanup, logging, etc.
    }
};

template<typename HTTPSessionType>
class ConnectionPool {
    moodycamel::ConcurrentQueue<HTTPSessionType*> available_connections_;
    boost::asio::io_context& ioc_;
    ssl::context& ssl_ctx_;
    std::string host_;
    std::string port_;
    common::MemoryPool<HTTPSessionType> session_pool_;
    HTTPSessionType::Timeout timeout_;
public:
    ConnectionPool(boost::asio::io_context& ioc, ssl::context& ssl_ctx, const std::string_view host, const std::string_view port, std::size_t pool_size, HTTPSessionType::Timeout timeout)
        : ioc_(ioc), ssl_ctx_(ssl_ctx), host_(host), port_(port), session_pool_(pool_size),timeout_(timeout) {
        for (std::size_t i = 0; i < pool_size; ++i) {
            auto session = session_pool_.Allocate(ioc_, ssl_ctx_, host_, port_, timeout_);
            available_connections_.enqueue(session);
        }
    }

    ~ConnectionPool() {
        // Deallocate all HttpsSession objects
        HTTPSessionType* session;
        while (available_connections_.try_dequeue(session)) {
            session_pool_.Deallocate(session);
        }
    }

    // Acquire a connection from the pool
    HTTPSessionType* AcquireConnection() {
        HTTPSessionType* session = nullptr;
        while (!available_connections_.try_dequeue(session)) {
            auto new_session = session_pool_.Allocate(ioc_, ssl_ctx_, host_, port_, timeout_);// Spin-wait until a connection becomes available
            available_connections_.enqueue(new_session);
        }
        return session;
    }

    // Release a connection back to the pool
    void ReleaseConnection(HTTPSessionType* session) {
        available_connections_.enqueue(session);
    }
};
};  // namespace V2