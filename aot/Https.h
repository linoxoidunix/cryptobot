#pragma once

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "boost/asio.hpp"
#include "boost/asio/as_tuple.hpp"
#include "boost/asio/co_spawn.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ssl/context.hpp"
#include "boost/asio/strand.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/ssl.hpp"
#include "boost/beast/version.hpp"
#include "boost/noncopyable.hpp"

// #include "aot/Exchange.h"
#include "aot/Logger.h"
#include "aot/Types.h"
#include "aot/common/mem_pool.h"
#include "aot/root_certificates.hpp"
#include "aot/session_status.h"
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
template <typename _Timeout, typename... AdditionalArgs>
class HttpsSession {
  public:
    using Timeout = _Timeout;

  private:
    bool is_connected_ = false;
    bool is_used_      = false;
    bool is_expired_   = false;

    /**
     * @brief req variable must manage only via SetRequest() method
     *
     */
    boost::beast::http::request<boost::beast::http::string_body> req_;

    tcp::resolver resolver_;
    beast::ssl_stream<beast::tcp_stream> stream_;
    beast::flat_buffer buffer_;  // (Must persist between reads)
    http::response<http::string_body> res_;
    OnHttpsResponce cb_;
    boost::asio::io_context& ioc_;
    Timeout timeout_;
    boost::asio::steady_timer timer_;  // Timer to track session expiration
  public:
    virtual ~HttpsSession() = default;
    explicit HttpsSession(boost::asio::io_context& ioc, ssl::context& ctx,
                          _Timeout timeout,

                          const std::string_view host,
                          const std::string_view port, AdditionalArgs&&...)
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
        cb_      = handler;
        req_     = std::move(req);
        // Set up the timer for session expiration
        // start_timer();

        // Send the HTTP request asynchronously
        http::async_write(
            stream_, req_,
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                this->on_write(ec, bytes_transferred);
            });

        return true;
    }

    // Close the session gracefully
    void AsyncCloseSessionGracefully() {
        // Perform SSL shutdown asynchronously
        stream_.async_shutdown([this](beast::error_code ec) {
            if (ec) {
                // If shutdown fails, log the error but still close the socket
                logi("SSL shutdown failed: {}", ec.message());
            }

            // Close the underlying TCP connection
            close_session();
        });
    }

    inline bool IsConnected() const { return is_connected_; }
    inline bool IsExpired() const { return is_expired_; }
    inline bool IsUsed() const { return is_used_; }

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
                fail(ec, "session expired. it is closed by async timer");
                close_session();
            }
        });
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) return fail(ec, "resolve");

        beast::get_lowest_layer(stream_).async_connect(
            results, [this](beast::error_code ec,
                            tcp::resolver::results_type::endpoint_type) {
                this->on_connect(ec);
            });
    }

    void on_connect(beast::error_code ec) {
        if (ec) return fail(ec, "connect");

        stream_.async_handshake(
            ssl::stream_base::client,
            [this](beast::error_code ec) { this->on_handshake(ec); });
    }

    void on_handshake(beast::error_code ec) {
        if (ec) return fail(ec, "handshake");

        // Set the connected flag to true after a successful handshake
        is_connected_ = true;
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) return fail(ec, "write");

        http::async_read(
            stream_, buffer_, res_,
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                this->on_read(ec, bytes_transferred);
            });
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) return fail(ec, "read");

        //  Call the response callback
        cb_(res_);
        close_session();
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

template <typename _Timeout, typename... AdditionalArgs>
class HttpsSession2 {
  public:
    using Timeout = _Timeout;

  private:
    bool is_connected_ = false;
    bool is_used_      = false;
    bool is_expired_   = false;

    /**
     * @brief req variable must manage only via SetRequest() method
     *
     */
    boost::beast::http::request<boost::beast::http::string_body> req_;

    tcp::resolver resolver_;
    beast::ssl_stream<beast::tcp_stream> stream_;
    beast::flat_buffer buffer_;  // (Must persist between reads)
    http::response<http::string_body> res_;
    const OnHttpsResponce* cb_;
    boost::asio::io_context& ioc_;
    Timeout timeout_;
    boost::asio::steady_timer timer_;  // Timer to track session expiration
  public:
    virtual ~HttpsSession2() = default;
    explicit HttpsSession2(boost::asio::io_context& ioc, ssl::context& ctx,
                           _Timeout timeout,

                           const std::string_view host,
                           const std::string_view port, AdditionalArgs&&...)
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
                      const CompletionHandler* handler) {
        if (!IsConnected()) return false;
        if (IsUsed()) return false;

        is_used_ = true;
        cb_      = handler;
        req_     = std::move(req);
        // Set up the timer for session expiration
        // start_timer();

        // Send the HTTP request asynchronously
        http::async_write(
            stream_, req_,
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                this->on_write(ec, bytes_transferred);
            });

        return true;
    }

    // Close the session gracefully
    void AsyncCloseSessionGracefully() {
        // Perform SSL shutdown asynchronously
        stream_.async_shutdown([this](beast::error_code ec) {
            if (ec) {
                // If shutdown fails, log the error but still close the socket
                logi("SSL shutdown failed: {}", ec.message());
            }

            // Close the underlying TCP connection
            close_session();
        });
    }

    inline bool IsConnected() const { return is_connected_; }
    inline bool IsExpired() const { return is_expired_; }
    inline bool IsUsed() const { return is_used_; }

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
                fail(ec, "session expired. it is closed by async timer");
                close_session();
            }
        });
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) return fail(ec, "resolve");

        beast::get_lowest_layer(stream_).async_connect(
            results, [this](beast::error_code ec,
                            tcp::resolver::results_type::endpoint_type) {
                this->on_connect(ec);
            });
    }

    void on_connect(beast::error_code ec) {
        if (ec) return fail(ec, "connect");

        stream_.async_handshake(
            ssl::stream_base::client,
            [this](beast::error_code ec) { this->on_handshake(ec); });
    }

    void on_handshake(beast::error_code ec) {
        if (ec) return fail(ec, "handshake");

        // Set the connected flag to true after a successful handshake
        is_connected_ = true;
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) return fail(ec, "write");

        http::async_read(
            stream_, buffer_, res_,
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                this->on_read(ec, bytes_transferred);
            });
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) return fail(ec, "read");

        //  Call the response callback
        const auto& cb = *cb_;
        cb(res_);
        close_session();
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

// template <typename _Timeout, typename... AdditionalArgs>
// class HttpsSession3 {
// public:
//     using Timeout = _Timeout;

// private:
//     bool is_connected_ = false;
//     bool is_used_      = false;
//     bool is_expired_   = false;
//     bool is_shutting_down_ = false;  // Flag to indicate if shutdown is in
//     progress

//     boost::beast::http::request<boost::beast::http::string_body> req_;
//     tcp::resolver resolver_;
//     beast::ssl_stream<beast::tcp_stream> stream_;
//     beast::flat_buffer buffer_;
//     http::response<http::string_body> res_;
//     const OnHttpsResponce* cb_;
//     boost::asio::io_context& ioc_;
//     Timeout timeout_;
//     boost::asio::steady_timer timer_;

// public:
//     virtual ~HttpsSession3() = default;
//     explicit HttpsSession3(boost::asio::io_context& ioc, ssl::context& ctx,
//     _Timeout timeout, const std::string_view host, const std::string_view
//     port, AdditionalArgs&&...)
//         : resolver_(net::make_strand(ioc)),
//           stream_(net::make_strand(ioc), ctx),
//           ioc_(ioc),
//           timeout_(timeout),
//           timer_(ioc) {
//         net::co_spawn(ioc_, Run(host.data(), port.data()),
//         [](std::exception_ptr e) {
//             if (e) std::rethrow_exception(e);
//         });
//     }

//     template <typename CompletionHandler>
//     net::awaitable<bool> AsyncRequest(http::request<http::string_body>&& req,
//     const CompletionHandler* handler) {
//         if (!IsConnected() || IsUsed()) co_return false;
//         req_ = std::move(req);
//         cb_ = handler;
//         is_used_ = true;
//         // slot.assign([this](boost::asio::cancellation_type_t) {
//         //     AsyncCloseSessionGracefully();
//         // });
//         auto [ec_write, bytes_written] = co_await http::async_write(stream_,
//         req_, net::as_tuple(net::use_awaitable)); if (ec_write) {
//             if (ec_write == net::error::operation_aborted) {
//                 logd("Operation cancelled");
//             } else {
//                 fail(ec_write, "write");
//             }
//             co_return false;
//         }

//         auto [ec_read, bytes_read] = co_await http::async_read(stream_,
//         buffer_, res_, net::as_tuple(net::use_awaitable)); if (ec_read) {
//             if (ec_read == net::error::operation_aborted) {
//                 logd("Operation cancelled");
//             } else {
//                 fail(ec_read, "read");
//             }
//             co_return false;
//         }
//         try{
//             if (cb_) (*cb_)(res_);
//         }
//         catch (const boost::system::system_error& e) {
//             logi("Exception in cb: {}", e.what());
//             co_return false;
//         }
//         AsyncCloseSessionGracefully();
//         co_return true;
//     }

//     // Close the session gracefully
//     void AsyncCloseSessionGracefully() {
//        boost::asio::post(ioc_, [this]() {
//         if (is_shutting_down_) {
//             logi("Shutdown is already in progress.");
//             return;
//         }
//         is_shutting_down_ = true;

//         stream_.async_shutdown([this](beast::error_code ec) {
//             if (ec) {
//                 // Если `shutdown` завершился с ошибкой, логируем её, но всё
//                 равно закрываем соединение logi("SSL shutdown failed: {}",
//                 ec.message());
//             }

//             // close tcp connection
//             close_session();
//         });
//     });
//     }

//     inline bool IsConnected() const { return is_connected_; }
//     inline bool IsExpired() const { return is_expired_; }
//     inline bool IsUsed() const { return is_used_; }

// private:
//     net::awaitable<void> Run(const char* host, const char* port) {
//         if (!SSL_set_tlsext_host_name(stream_.native_handle(), host)) {
//             auto ec = beast::error_code(static_cast<int>(::ERR_get_error()),
//             net::error::get_ssl_category()); loge("{}", ec.message());
//             co_return;
//         }
//         boost::beast::error_code ec;
//         start_timer();
//         auto results = co_await resolver_.async_resolve(host, port,
//         net::use_awaitable);

//         auto ep = co_await
//         beast::get_lowest_layer(stream_).async_connect(results,
//         net::use_awaitable);
//         beast::get_lowest_layer(stream_).expires_never();

//         co_await stream_.async_handshake(ssl::stream_base::client,
//                                               boost::asio::redirect_error(boost::asio::use_awaitable,
//                                               ec));

//         if (ec) {
//             loge("fail handshake");
//             co_return;
//         }

//         is_connected_ = true;
//     }

//     void start_timer() {
//         timer_.expires_after(timeout_);
//         timer_.async_wait([this](beast::error_code ec) {
//             if (ec != boost::asio::error::operation_aborted) {
//                 fail(ec, "session expired. it is closed by async timer");
//                 close_session();
//             }
//         });
//     }

//     void fail(beast::error_code ec, const char* what) {
//         logi("{}: {}", what, ec.message());
//         close_session();
//     }

//     void close_session() {
//         beast::get_lowest_layer(stream_).close();
//         is_expired_ = true;
//     }
// };

/**
 * @brief Template class for managing an asynchronous HTTPS session.
 * 
 * This class uses Boost.Beast and Boost.Asio to perform asynchronous HTTPS
 * operations, such as sending HTTP requests and receiving responses, with
 * support for cancellation and timeout management.
 * 
 * @tparam _Timeout Type representing the timeout duration.
 * @tparam AdditionalArgs Additional arguments for customization.
 */
template <typename _Timeout, typename... AdditionalArgs>
class HttpsSession3 {
  public:
    using Timeout = _Timeout; ///< Type alias for the timeout duration.

  private:
    // Static atomic counter for tracking the number of instances.
    static std::atomic<size_t> instance_count;

    boost::beast::http::request<boost::beast::http::string_body> req_; ///< HTTP request object.
    boost::beast::http::response<boost::beast::http::string_body> res_; ///< HTTP response object.

    tcp::resolver resolver_; ///< Resolver for translating hostnames to endpoints.
    beast::ssl_stream<beast::tcp_stream> stream_; ///< SSL/TLS stream for secure communication.
    beast::flat_buffer buffer_; ///< Buffer for reading HTTP responses.

    boost::asio::io_context& ioc_; ///< Reference to the I/O context.
    Timeout timeout_; ///< Configured timeout duration.
    boost::asio::steady_timer timer_; ///< Timer for managing timeouts.

    aot::StatusSession status_ = aot::StatusSession::Resolving; ///< Current session status.
    boost::asio::cancellation_signal cancel_signal_; ///< Signal for handling cancellation requests.

    std::atomic<bool> need_execute_on_closed_system = true; ///< Indicates whether system callbacks should be executed.

    // Callback functions
    std::function<void(boost::beast::http::response<boost::beast::http::string_body>&)> on_response_; ///< Callback for handling HTTP responses.
    std::function<void()> on_ready_; ///< Callback for when the session is ready.
    std::function<void()> on_expired_; ///< Callback for when the session times out.
    std::function<void()> on_closed_system_; ///< Callback for when the session is closed by the system.
    std::function<void()> on_closed_user_; ///< Callback for when the session is closed by the user.

  public:
    /**
     * @brief Destructor for the class.
     */
    virtual ~HttpsSession3() {
        // Decrement the instance count and log it.
        size_t count = --instance_count;
        logi("HttpsSession3 destroyed. Current instance count: {}", count);
    };

    /**
     * @brief Constructor to initialize the session.
     * 
     * @param ioc The I/O context.
     * @param ctx The SSL context.
     * @param timeout The timeout duration.
     * @param host The target host.
     * @param port The target port.
     * @param args Additional arguments for customization.
     */
    explicit HttpsSession3(boost::asio::io_context& ioc, ssl::context& ctx,
                           _Timeout timeout, const std::string_view host,
                           const std::string_view port, AdditionalArgs&&...)
        : resolver_(net::make_strand(ioc)),
          stream_(net::make_strand(ioc), ctx),
          ioc_(ioc),
          timeout_(timeout),
          timer_(ioc) {
        // Increment the instance count and log it.
        size_t count = ++instance_count;
        logi("HttpsSession3 created. Current instance count: {}", count);
        net::co_spawn(ioc_, Run(host.data(), port.data()),
                      [](std::exception_ptr e) {
                          if (e) std::rethrow_exception(e);
                      });
        cancel_signal_.slot().assign(
            [this](boost::asio::cancellation_type type) {
                logd("Cancellation requested");
                HandleCancellation(type);
            });
    }

    /**
     * @brief Sends an asynchronous HTTP request.
     * 
     * @param req The HTTP request object.
     * @return net::awaitable<bool> True if the request was successful, otherwise false.
     */
    net::awaitable<bool> AsyncRequest(http::request<http::string_body>&& req) {
        if (status_ != aot::StatusSession::Ready) co_return false;
        req_ = std::move(req);

        // Write the HTTP request asynchronously.
        auto [write_ec, bytes_written] = co_await http::async_write(
            stream_, req_, net::as_tuple(net::use_awaitable));
        if (write_ec) {
            if (write_ec == net::error::operation_aborted) {
                logd("Write operation cancelled");
                CloseSessionFast();
                co_return false;
            }
            CloseSessionFast();
            TransitionTo(aot::StatusSession::Closing);
            co_return false;
        }

        // Read the HTTP response asynchronously.
        auto [ec_read, bytes_read] = co_await http::async_read(
            stream_, buffer_, res_, net::as_tuple(net::use_awaitable));
        if (ec_read) {
            if (ec_read == net::error::operation_aborted) {
                logd("Read operation cancelled");
                CloseSessionFast();
                co_return false;
            }
            CloseSessionFast();
            TransitionTo(aot::StatusSession::Closing);
            co_return false;
        }
        // Trigger the on_response_ callback if registered.
        if (on_response_) on_response_(res_);
        
        CloseSessionFast();
        TransitionTo(aot::StatusSession::Closing);
        co_return true;
    }

    /**
     * @brief Gracefully closes the session asynchronously.
     */
    void AsyncCloseSessionGracefully() {
        cancel_signal_.emit(boost::asio::cancellation_type::all);
    }

    /**
     * @brief Retrieves the current status of the session.
     * 
     * @return aot::StatusSession Current session status.
     */
    inline aot::StatusSession GetStatus() const { return status_; }

    // Callback registration methods
    /**
     * @brief Registers a callback to handle HTTP responses.
     * 
     * @param callback The response handling callback.
     */
    void RegisterOnResponse(
        std::function<void(http::response<http::string_body>&)> callback) {
        on_response_ = std::move(callback);
    }

    /** @brief Unregisters the response handling callback. */
    void UnregisterOnResponse() { on_response_ = nullptr; }

    /**
     * @brief Registers a callback to be invoked when the session is ready.
     * 
     * @param callback The ready callback.
     */
    void RegisterOnReady(std::function<void()> callback) {
        on_ready_ = std::move(callback);
    }

    /** @brief Unregisters the ready callback. */
    void UnregisterOnReady() { on_ready_ = nullptr; }

    /**
     * @brief Registers a callback to handle session expiration.
     * 
     * @param callback The expiration handling callback.
     */
    void RegisterOnExpired(std::function<void()> callback) {
        on_expired_ = std::move(callback);
    }

    /** @brief Unregisters the expiration callback. */
    void UnregisterOnExpired() { on_expired_ = nullptr; }

    /**
     * @brief Registers a callback to handle system-initiated session closures.
     * 
     * @param callback The system-closure handling callback.
     */
    void RegisterOnSystemClosed(std::function<void()> callback) {
        on_closed_system_ = std::move(callback);
    }

    /** @brief Unregisters the system-closure callback. */
    void UnregisterOnSystemClosed() { on_closed_system_ = nullptr; }

    /**
     * @brief Registers a callback to handle user-initiated session closures.
     * 
     * @param callback The user-closure handling callback.
     */
    void RegisterOnUserClosed(std::function<void()> callback) {
        on_closed_user_ = std::move(callback);
    }

    /** @brief Unregisters the user-closure callback. */
    void UnRegisterOnUserClosed() { on_closed_user_ = nullptr; }

  private:
    /**
     * @brief Core coroutine to establish the session.
     * 
     * Resolves the host and port, establishes an SSL connection, and transitions
     * the session to a ready state.
     * 
     * @param host The target host.
     * @param port The target port.
     * @return net::awaitable<void> Coroutine.
     */
    net::awaitable<void> Run(const char* host, const char* port) {
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host)) {
            loge("SSL set host error");
            CloseSessionFast();
            TransitionTo(aot::StatusSession::Expired);
            co_return;
        }

        boost::beast::error_code ec;
        start_timer();

        // Resolve the hostname to an endpoint.
        status_ = aot::StatusSession::Resolving;
        auto results = co_await resolver_.async_resolve(host, port, net::use_awaitable);

        // Connect to the resolved endpoint.
        status_ = aot::StatusSession::Connecting;
        co_await beast::get_lowest_layer(stream_).async_connect(results, net::use_awaitable);

        // Perform the SSL handshake.
        status_ = aot::StatusSession::Handshaking;
        co_await stream_.async_handshake(
            ssl::stream_base::client,
            boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec) {
            CloseSessionFast();
            TransitionTo(aot::StatusSession::Expired);
            co_return;
        }

        cancel_timer();
        TransitionTo(aot::StatusSession::Ready);
    }

    /**
     * @brief Starts the timeout timer.
     */
    void start_timer() {
        timer_.expires_after(timeout_);
        timer_.async_wait([this](beast::error_code ec) {
            if (ec != boost::asio::error::operation_aborted) {
                logi("timer expired");
                CloseSessionFast();
                TransitionTo(aot::StatusSession::Expired);
            }
        });
    }

    /**
     * @brief Cancels the timeout timer.
     */
    void cancel_timer() {
        try {
            timer_.cancel();
        } catch (const boost::system::system_error& e) {
            logw("Error while canceling timer:{}", e.what());
        }
    }

    /**
     * @brief Closes the session immediately.
     */
    void CloseSessionFast() {
        logi("start close session fast");
        beast::error_code ec;
        stream_.shutdown(ec); // Gracefully shut down the TLS session.
        beast::get_lowest_layer(stream_).close(); // Close the underlying transport.
        if (on_closed_user_) on_closed_user_();
    }

    /**
     * @brief Transitions the session to a new status.
     * 
     * @param new_state The new session status.
     */
    void TransitionTo(aot::StatusSession new_state) {
        if (status_ == new_state) return;

        status_ = new_state;
        switch (status_) {
            case aot::StatusSession::Ready:
                logi("invoke on ready");
                if (on_ready_) on_ready_();
                break;
            case aot::StatusSession::Expired:
                logi("invoke on expired");
                if (need_execute_on_closed_system.exchange(false))
                    if (on_expired_) on_expired_();
                break;
            case aot::StatusSession::Closing:
                logi("invoke on closing");
                if (need_execute_on_closed_system.exchange(false))
                    if (on_closed_system_) on_closed_system_();
                break;
            default:
                break;
        }
    }

    /**
     * @brief Handles cancellation requests.
     * 
     * @param type The type of cancellation.
     */
    void HandleCancellation(boost::asio::cancellation_type type) {
        TransitionTo(aot::StatusSession::Cancelling);
        need_execute_on_closed_system = false;
    }
};


/**
 * @class ConnectionPool
 * @brief Manages a pool of reusable HTTPSessionType connections.
 * 
 * @tparam HTTPSessionType The type of session managed by the pool.
 * @tparam Args Additional arguments required to construct sessions.
 */
template <typename HTTPSessionType, typename... Args>
class ConnectionPool {
    /**
     * @brief Queue of available connections.
     */
    moodycamel::ConcurrentQueue<HTTPSessionType*> awaiable_connections_;

    /**
     * @brief Queue of connections marked as useless or expired.
     */
    moodycamel::ConcurrentQueue<HTTPSessionType*> useless_connections_;

    /**
     * @brief Queue of connections ready for copying.
     */
    moodycamel::ConcurrentQueue<HTTPSessionType*> copy_ready_connections_;

    /**
     * @brief Reference to the IO context for asynchronous operations.
     */
    boost::asio::io_context& ioc_;

    /**
     * @brief SSL context for secure connections.
     */
    ssl::context ssl_ctx_{ssl::context::sslv23};

    /**
     * @brief Host address for the connections.
     */
    std::string host_;

    /**
     * @brief Port for the connections.
     */
    std::string port_;

    /**
     * @brief Maximum number of connections in the pool.
     */
    size_t pool_size_;

    /**
     * @brief Memory pool for allocating session objects.
     */
    common::MemoryPool<HTTPSessionType> session_pool_;

    /**
     * @brief Timeout configuration for the sessions.
     */
    HTTPSessionType::Timeout timeout_;

    /**
     * @brief Tuple storing additional constructor arguments for the sessions.
     */
    std::tuple<std::decay_t<Args>...> ctor_args_;

    std::atomic_flag is_handling_session = ATOMIC_FLAG_INIT;
  public:
    /**
     * @brief Constructor for the connection pool.
     * 
     * @param ioc IO context for asynchronous operations.
     * @param timeout Timeout configuration for sessions.
     * @param pool_size Number of connections in the pool.
     * @param host Host address for the connections.
     * @param port Port for the connections.
     * @param args Additional arguments required to construct sessions.
     */
    ConnectionPool(boost::asio::io_context& ioc,
                   HTTPSessionType::Timeout timeout, size_t pool_size,
                   const std::string_view host, const std::string_view port,
                   Args&&... args)
        : ioc_(ioc),
          host_(host),
          port_(port),
          pool_size_(pool_size),
          session_pool_(pool_size),
          timeout_(timeout),
          ctor_args_(std::forward<Args>(args)...) {
        for (std::size_t i = 0; i < pool_size; ++i) {
           try {
            auto session = CreateSession();
            session->RegisterOnReady([this, session]() {
                if (!awaiable_connections_.try_enqueue(session)) {
                    logi("can't enqueu new session");
                    // Лог или обработка ошибки
                }
            });

            session->RegisterOnSystemClosed([this, session]() {
                if (!is_handling_session.test_and_set()) { // Проверка и установка флага
                    session_pool_.Deallocate(session);
                    TryCreateNewSession();
                    is_handling_session.clear(); // Сброс флага
                }
            });

            session->RegisterOnExpired([this, session]() {
                if (!is_handling_session.test_and_set()) {
                    session_pool_.Deallocate(session);
                    TryCreateNewSession();
                    is_handling_session.clear();
                }
            });

        } catch (const std::exception& e) {
            logi("some exception here");
            // Обработка исключений при создании сессии
            // Например, лог ошибки
        }
        }
    }

    /**
     * @brief Acquire a connection from the pool.
     * 
     * Blocks until a ready connection is available.
     * 
     * @return Pointer to the acquired connection.
     */
    HTTPSessionType* AcquireConnection() {
        HTTPSessionType* session = nullptr;

        // Block until we get a ready session
        while (true){ 
            // Try to dequeue a session from the available connections
            while (!awaiable_connections_.try_dequeue(session)) {
                // Block until a ready session is available
            }
            awaiable_connections_.try_enqueue(session);
            // Check the status of the session
            auto status = session->GetStatus();
            if(status == aot::StatusSession::Ready)
                break;
            // // If the session is expired or closing, release it and try another one
            // if (status == aot::StatusSession::Expired ||
            //     status == aot::StatusSession::Closing) {
            //     useless_connections_.try_enqueue(session);
            //     session = nullptr;  // Try to get a new session
            // } else if (status != aot::StatusSession::Ready) {
            //     // If the session is not ready, enqueue it back to the queue for a retry
            //     awaiable_connections_.try_enqueue(session);
            //     session = nullptr;  // Try to get another session
            // } else {
            //     // Copy the session
            //     copy_ready_connections_.try_enqueue(session);
            // }
        }
        return session;
    }

    /**
     * @brief Destructor for the connection pool.
     */
    ~ConnectionPool() {}

    /**
     * @brief Close all sessions in the pool.
     */
    void CloseAllSessions() {
        HTTPSessionType* session = nullptr;

        // Disable callbacks and close available sessions from the queue
        while (awaiable_connections_.try_dequeue(session)) {
            if (session) {
                session->AsyncCloseSessionGracefully();
                // session_pool_.Deallocate(session);
            }
        }
        while (useless_connections_.try_dequeue(session)) {
            if (session) {
                session->AsyncCloseSessionGracefully();
                // session_pool_.Deallocate(session);
            }
        }
        while (copy_ready_connections_.try_dequeue(session)) {
            if (session) {
                session->AsyncCloseSessionGracefully();
                // session_pool_.Deallocate(session);
            }
        }
        // Release all remaining sessions in the pool
        // session_pool_.Reset();
    }

  private:
    /**
     * @brief Create a new session using the memory pool.
     * 
     * @return Pointer to the newly created session.
     */
    HTTPSessionType* CreateSession() {
        // If args are provided, they will be forwarded to Allocate
        return std::apply(
            [this](auto&&... unpacked_args) {
                return session_pool_.Allocate(
                    ioc_, ssl_ctx_, timeout_, host_, port_,
                    std::forward<decltype(unpacked_args)>(unpacked_args)...);
            },
            ctor_args_);
    }

   void TryCreateNewSession() {
    try {
        // Создаем новую сессию
        auto session = CreateSession();
        auto shared_session = std::shared_ptr<HTTPSessionType>(session);

        // Обработка события "готовности"
        shared_session->RegisterOnReady([this, shared_session]() {
            if (!awaiable_connections_.try_enqueue(shared_session.get())) {
                logi("Can't enqueue session in available_connections");
                // Логирование ошибки или другая обработка
            }
        });

        // Обработка события "системное закрытие"
        shared_session->RegisterOnSystemClosed([this, shared_session]() {
            if (!is_handling_session.test_and_set()) { // Проверка и установка флага
                session_pool_.Deallocate(shared_session.get());
                logi("Session system closed. Recreating session...");
                TryCreateNewSession(); // Рекурсивное создание новой сессии
                is_handling_session.clear(); // Сброс флага
            }
        });

        // Обработка события "истечение срока действия"
        shared_session->RegisterOnExpired([this, shared_session]() {
            if (!is_handling_session.test_and_set()) { // Проверка и установка флага
                session_pool_.Deallocate(shared_session.get());
                logi("Session expired. Recreating session...");
                TryCreateNewSession(); // Рекурсивное создание новой сессии
                is_handling_session.clear(); // Сброс флага
            }
        });

    } catch (const std::exception& e) {
        // Логирование ошибок создания новой сессии
        loge("Error while creating new session: {}", e.what());
    }
    }
};
};  // namespace V2

// Initialize the static atomic counter.
template <typename _Timeout, typename... AdditionalArgs>
std::atomic<size_t> V2::HttpsSession3<_Timeout, AdditionalArgs...>::instance_count{0};