#pragma once

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/noncopyable.hpp>
#include <functional>
#include <memory>
#include <future>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aot/Exchange.h"
#include "aot/Types.h"
#include "aot/common/mem_pool.h"
#include "aot/root_certificates.hpp"
#include "concurrentqueue.h"

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
template <typename _Timeout>
class HttpsSession {
  public:
    using Timeout = _Timeout;

  private:
    bool is_connected_ = false;
    bool is_used_       = false;
    bool is_expired_   = false;

    tcp::resolver resolver_;
    beast::ssl_stream<beast::tcp_stream> stream_;
    beast::flat_buffer buffer_;  // (Must persist between reads)
    http::response<http::string_body> res_;
    OnHttpsResponce cb_;
    boost::asio::io_context& ioc_;
    Timeout timeout_;
    boost::asio::steady_timer timer_;  // Timer to track session expiration

  public:
    explicit HttpsSession(boost::asio::io_context& ioc, ssl::context& ctx,
                          const std::string_view host,
                          const std::string_view port, _Timeout timeout)
        : resolver_(net::make_strand(ioc)),
          stream_(net::make_strand(ioc), ctx),
          ioc_(ioc),
          timeout_(timeout),
          timer_(ioc)  // Initialize the timer
    {
        Run(host.data(), port.data(), nullptr, {});
    }

    template <typename CompletionHandler>
    bool AsyncRequest(http::request<http::string_body>&& req,
                      CompletionHandler handler) {
        if (!IsConnected()) return false;
        if (IsUsed()) return false;

        is_used_ = true;
        cb_     = handler;

        // Set up the timer for session expiration
        start_timer();

        // Send the HTTP request asynchronously
        http::async_write(
            stream_, req,
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                this->on_write(ec, bytes_transferred);
            });

        return true;
    }
    inline bool IsConnected() const{
        return is_connected_;
    }
    inline bool IsExpired() const{
        return is_expired_;
    }
        inline bool IsUsed() const{
        return is_used_;
    }
  private:
    // Start the asynchronous operation
    void Run(char const* host, char const* port, char const* target,
             http::request<http::string_body>&& req) {
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host)) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                 net::error::get_ssl_category()};
            std::cerr << ec.message() << "\n";
            return;
        }

        // Start the timer for expiration
        start_timer();

        // Start the resolver
        resolver_.async_resolve(
            host, port,
            [this](beast::error_code ec, tcp::resolver::results_type results) {
                this->on_resolve(ec, results);
            });
    }

    // Start the session expiration timer
    void start_timer() {
        // Set the timer to expire after the specified timeout
        timer_.expires_after(timeout_);

        // Set up the handler to catch when the timer expires
        timer_.async_wait([this](beast::error_code ec) {
            if (ec !=
                boost::asio::error::operation_aborted) {  // Ignore if the timer
                                                          // was canceled
                is_expired_ = true;
                fail(ec, "session expired");
                // Handle session expiration (close, notify user, etc.)
                if (cb_) {
                    beast::error_code timeout_error{
                        boost::asio::error::timed_out};
                    cb_(res_);  // Notify the callback about the timeout
                }
                close_session();
            }
        });
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) return fail(ec, "resolve");

        // Reset the timer for the next operation
        start_timer();

        beast::get_lowest_layer(stream_).async_connect(
            results, [this](beast::error_code ec,
                            tcp::resolver::results_type::endpoint_type) {
                this->on_connect(ec);
            });
    }

    void on_connect(beast::error_code ec) {
        if (ec) return fail(ec, "connect");

        // Reset the timer for the next operation
        start_timer();

        stream_.async_handshake(
            ssl::stream_base::client,
            [this](beast::error_code ec) { this->on_handshake(ec); });
    }

    void on_handshake(beast::error_code ec) {
        if (ec) return fail(ec, "handshake");

        // Reset the timer for the next operation
        start_timer();

        // Set the connected flag to true after a successful handshake
        is_connected_ = true;
        // Notify that the handshake was successful (you could add a callback
        // here as well) For example, you might want to notify that the session
        // is now active.

        // Optionally, you can now send the HTTP request if needed
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) return fail(ec, "write");

        // Reset the timer for the next operation
        start_timer();

        http::async_read(
            stream_, buffer_, res_,
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                this->on_read(ec, bytes_transferred);
            });
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) return fail(ec, "read");

        // Cancel the timer as the operation is completed successfully
        timer_.cancel();

        // Call the response callback
        cb_(res_);
        close_session();
    }

    // Handle session expiration or failure
    void fail(beast::error_code ec, const char* what) {
        std::cerr << what << ": " << ec.message() << "\n";
        close_session();
    }

    // Close the session gracefully
    void close_session() { beast::get_lowest_layer(stream_).close(); }
};

template <typename HTTPSessionType>
class ConnectionPool {
    moodycamel::ConcurrentQueue<HTTPSessionType*> available_connections_;
    moodycamel::ConcurrentQueue<HTTPSessionType*> used_connections_;
    moodycamel::ConcurrentQueue<HTTPSessionType*> expired_connections_;

    std::unique_ptr<std::jthread> timeout_thread_;
    std::promise<void> helper_thread_finished;
    std::future<void> block_until_helper_thread_finished;

    boost::asio::io_context& ioc_;
    ssl::context& ssl_ctx_;
    std::string host_;
    std::string port_;
    size_t pool_size_;
    common::MemoryPool<HTTPSessionType> session_pool_;
    HTTPSessionType::Timeout timeout_;

  public:
    ConnectionPool(boost::asio::io_context& ioc, ssl::context& ssl_ctx,
                   const std::string_view host, const std::string_view port,
                   std::size_t pool_size, HTTPSessionType::Timeout timeout)
        : block_until_helper_thread_finished(helper_thread_finished.get_future()),
          ioc_(ioc),
          ssl_ctx_(ssl_ctx),
          host_(host),
          port_(port),
          pool_size_(pool_size),
          session_pool_(pool_size),
          timeout_(timeout) {
        for (std::size_t i = 0; i < pool_size; ++i) {
            auto session =
                session_pool_.Allocate(ioc_, ssl_ctx_, host_, port_, timeout_);
            available_connections_.enqueue(session);
        }
        timeout_thread_ = std::make_unique<std::jthread>([this](std::stop_token stoken){MonitorConnections(stoken);});
    }

    ~ConnectionPool() {
        timeout_thread_->request_stop();
        block_until_helper_thread_finished.wait();
        // Deallocate all HttpsSession objects
        HTTPSessionType* session;
        while (available_connections_.try_dequeue(session)) {
            session_pool_.Deallocate(session);
        }
        while (used_connections_.try_dequeue(session)) {
            session_pool_.Deallocate(session);
        }
        while (expired_connections_.try_dequeue(session)) {
            session_pool_.Deallocate(session);
        }
    }

    // Acquire a connection from the pool
    HTTPSessionType* AcquireConnection() {
        HTTPSessionType* session = nullptr;
        while (!available_connections_.try_dequeue(session)) {
            // auto new_session = session_pool_.Allocate(
            //     ioc_, ssl_ctx_, host_, port_,
            //     timeout_);  // Spin-wait until a connection becomes available
            // available_connections_.enqueue(new_session);
        }
        return session;
    }

    // Release a connection back to the pool
    void ReleaseConnection(HTTPSessionType* session) {
        if(!session)
            return;
        used_connections_.enqueue(session);
    }

  private:
    void MonitorConnections(std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            HTTPSessionType* session[100];
            size_t count = available_connections_.try_dequeue_bulk(session, 100);
            for(int i = 0; i < count; i++){
                if (session[i]->IsExpired()) {
                    expired_connections_.enqueue(session[i]);  // Enqueue expired or already used connections for
                } else {
                    available_connections_.enqueue(session[i]);  // Return active connection back to the available_connections_
                }
            }

            // Process expired connections
            // if expired due to long waiting
            HTTPSessionType* expired_session = nullptr;
            while (expired_connections_.try_dequeue(expired_session)) {
                    ReplaceTimedOutConnection(expired_session);
            }

            HTTPSessionType* used_session = nullptr;
            while (used_connections_.try_dequeue(used_session)) {
                //if task complete
                if(used_session->IsExpired())
                    ReplaceTimedOutConnection(used_session);
            }

            while(available_connections_.size_approx() < pool_size_)
            {
                auto new_session = session_pool_.Allocate(ioc_, ssl_ctx_, host_, port_, timeout_);
                available_connections_.enqueue(new_session);
            }
            // Sleep briefly to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        helper_thread_finished.set_value();
    }

    // Replace a timed-out connection by deleting it and adding a new one
    void ReplaceTimedOutConnection(HTTPSessionType* session) {
        session_pool_.Deallocate(session);  // Deallocate the old session

        // Create a new session and add it back to the pool
        auto new_session = session_pool_.Allocate(ioc_, ssl_ctx_, host_, port_, timeout_);
        available_connections_.enqueue(new_session);
    }
};
};  // namespace V2