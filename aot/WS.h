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
#include "boost/asio.hpp"
#include "boost/asio/awaitable.hpp"
#include "boost/asio/this_coro.hpp"

#include "concurrentqueue.h"

#include "aot/Logger.h"
#include "aot/Types.h"
#include "aot/cb_manager.h"
#include "aot/session_status.h"


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
//-----------------------------------------------------------------
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
enum class StatusSession {
    Resolving,
    Connecting,
    Handshaking,
    Ready,
    Expired,
    Closing
};
/**
 * @brief difference btw WssSession2 that WssSession3 support cancel
 *
 * @tparam _Timeout
 */
template <typename _Timeout>
class WssSession3 {
  public:
    using Timeout = _Timeout;

  private:
    std::atomic<bool> need_read_ = true;
    std::atomic<bool> read_started_ =
        false;  // Флаг для отслеживания состояния чтения
    bool is_connected_ = false;
    bool is_used_      = false;
    bool is_expired_   = false;

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
    /**
     * @brief manage all callbacks when need response
     *
     */
    aot::LockFreeCallbackManager<aot::CallbackNodeTradingPair<boost::beast::flat_buffer&, common::TradingPair>> cb_on_response_manager_;
    /**
     * @brief manage all callbacks when session is closed
     *
     */
    aot::LockFreeCallbackManager<aot::CallbackNode<void>> cb_on_close_session_manager_;
    std::function<void()> on_ready_;
    std::function<void()> on_expired_;
    std::function<void()> on_closed_;


    boost::asio::io_context& ioc_;
    Timeout timeout_;
    std::string_view host_;
    std::string_view port_;
    std::string_view default_endpoint_;
    boost::asio::steady_timer timer_;  // Timer to track session expiration
    moodycamel::ConcurrentQueue<std::string> queue_requests_;
    // signal to cancel all coroutines
    boost::asio::cancellation_signal cancel_signal_;
    aot::StatusSession status_ = aot::StatusSession::Resolving;

  public:
    virtual ~WssSession3() = default;
    explicit WssSession3(boost::asio::io_context& ioc, ssl::context& ctx,
                         _Timeout timeout, const std::string_view host,
                         const std::string_view port,
                         const std::string_view default_endpoint)
        : resolver_(net::make_strand(ioc)),
          stream_(net::make_strand(ioc), ctx),
          ioc_(ioc),
          timeout_(timeout),
          host_(host),
          port_(port),
          default_endpoint_(default_endpoint),
          timer_(ioc)  // Initialize the timer
    {
        // Register cancellation handler for the entire session
        cancel_signal_.slot().assign(
            [this](boost::asio::cancellation_type type) {
                logd("Cancellation requested");
                HandleCancellation(type);
            });

        net::co_spawn(ioc_,
                      Run(host.data(), port.data(), default_endpoint.data()),
                      [](std::exception_ptr e) {
                          if (e) std::rethrow_exception(e);
                      });
    }
    inline aot::StatusSession GetStatus() const { return status_; }

    aot::CallbackID
    RegisterCallbackOnResponse(
        const OnWssResponseTradingPair cb, common::TradingPair trading_pair) {
        return cb_on_response_manager_.RegisterCallback(cb, trading_pair);
    }
    bool UnRegisterCallbackOnResponse(
        aot::CallbackID id) {
        return cb_on_response_manager_.UnregisterCallback(id);
    }
    aot::CallbackID RegisterCallbackOnCloseSession(
        const OnCloseSession cb) {
        return cb_on_close_session_manager_.RegisterCallback(cb);
    }
    bool UnRegisterCallbackOnCloseSession(
        aot::CallbackID id) {
        return cb_on_close_session_manager_.UnregisterCallback(id);
    }

    void RegisterOnReady(std::function<void()> callback) {
        on_ready_ = std::move(callback);
    }

    void UnregisterOnReady() { on_ready_ = nullptr; }

    void RegisterOnExpired(std::function<void()> callback) {
        on_expired_ = std::move(callback);
    }

    void UnregisterOnExpired() { on_expired_ = nullptr; }

    void RegisterOnSystemClosed(std::function<void()> callback) {
        on_closed_ = std::move(callback);
    }

    void UnregisterOnSystemClosed() { on_closed_ = nullptr; }
    net::awaitable<bool> AsyncRequest(std::string&& req) {
        if (!IsConnected()) co_return false;

        auto [write_ec, bytes_written] = co_await stream_.async_write(
            net::buffer(req),
            boost::asio::as_tuple(boost::asio::use_awaitable));

        if (write_ec) {
            if (write_ec == net::error::operation_aborted) {
                logd("Write operation cancelled");
                CloseSessionFast();
                co_return false;
            } else {
            }
            CloseSessionFast();
            TransitionTo(aot::StatusSession::Closing);
            co_return false;
        }
        if (!read_started_.exchange(true)) {
            co_spawn(ioc_, ReadLoop(), net::detached);
        }
        co_return true;
     //    Wrap everything to ensure all operations are executed on the strand associated with stream_
    // co_await stream_.strand().wrap([this, req = std::move(req)]() -> net::awaitable<bool> {
    //     // Check if connected
    //     if (!IsConnected()) co_return false;

    //     // Perform the asynchronous write operation on the strand
    //     auto [write_ec, bytes_written] = co_await stream_.async_write(
    //         net::buffer(req),
    //         boost::asio::as_tuple(boost::asio::use_awaitable));

    //     if (write_ec) {
    //         if (write_ec == net::error::operation_aborted) {
    //             logd("Write operation cancelled");
    //             CloseSessionFast();
    //             co_return false;
    //         } else {
    //             // Handle other write errors if necessary
    //         }
    //         CloseSessionFast();
    //         TransitionTo(aot::StatusSession::Closing);
    //         co_return false;
    //     }

    //     // Ensure that the read operation starts only once
    //     if (!read_started_) {
    //         read_started_ = true;  // Set the flag to prevent restarting the read loop
    //         co_spawn(ioc_, ReadLoop(), net::detached);  // This is also on the strand
    //     }

    //     co_return true;
    // });
    }

    inline bool IsConnected() const { return is_connected_; }
    inline bool IsExpired() const { return is_expired_; }
    inline bool IsUsed() const { return is_used_; }
    void AsyncCloseSessionGracefully() {
        cancel_signal_.emit(boost::asio::cancellation_type::all);
    }

  private:
    // Handles cancellation by closing the session
    void HandleCancellation(boost::asio::cancellation_type_t type) {
        logi("close all async operation for ws session");
        need_read_ = false;
    }
    // Start the asynchronous operation
    net::awaitable<void> Run(const char* host, const char* port,
                             const char* default_end_point) {
        beast::error_code ec;

        auto results = co_await resolver_.async_resolve(
            host, port, net::redirect_error(net::use_awaitable, ec));
        if (ec) {
            loge("Failed to resolve host '{}:{}': {}", host, port,
                 ec.message());
            co_return;
        }
        if (!SSL_set_tlsext_host_name(stream_.next_layer().native_handle(),
                                      host)) {
            auto ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                                        net::error::get_ssl_category());
            loge("{}", ec.message());
            co_return;
        }
        start_timer();
        auto ep = co_await beast::get_lowest_layer(stream_).async_connect(
            results, net::redirect_error(net::use_awaitable, ec));
        if (ec) {
            loge("Failed to connect to endpoint: {}", ec.message());
            co_return;
        }
        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(stream_).expires_never();

        co_await stream_.next_layer().async_handshake(
            ssl::stream_base::client,
            net::redirect_error(net::use_awaitable, ec));
        if (ec) {
            loge("SSL handshake failed: {}", ec.message());
            co_return;
        }
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
        co_await stream_.async_handshake(
            host, default_end_point,
            net::redirect_error(net::use_awaitable, ec));
        if (ec) {
            loge("WebSocket handshake failed: {}", ec.message());
            co_return;
        }

        is_connected_ = true;
        timer_.cancel();
        TransitionTo(aot::StatusSession::Ready);
        co_return;
    }

    void start_timer() {
        // Set the timer to expire after the specified timeout
        timer_.expires_after(timeout_);

        // Set up the handler to catch when the timer expires
        timer_.async_wait([this](beast::error_code ec) {
            if (ec != boost::asio::error::operation_aborted) {  // Ignore if the
                                                                // timer was
                                                                // canceled
                fail(ec, "session expired. it is closed by async timer");
                TransitionTo(aot::StatusSession::Expired);
                CloseSessionFast();
            }
        });
    }

    // Handle session expiration or failure
    void fail(beast::error_code ec, const char* what) {
        logi("{}: {}", what, ec.message());
    }

    // Close the session gracefully
    void CloseSessionFast() {
        beast::get_lowest_layer(stream_).close();
        is_expired_ = true;
        // Invoke all callbacks for session close
        cb_on_close_session_manager_.InvokeAll();
    }
    net::awaitable<void> CloseSessionSlow() {
        try {
            // Асинхронное закрытие WebSocket-соединения
            co_await stream_.async_close(websocket::close_code::normal,
                                         net::use_awaitable);

            // После завершения закрытия можно безопасно установить флаг
            is_expired_ = true;
            status_ = aot::StatusSession::Closing;
            cb_on_close_session_manager_.InvokeAll();
        } catch (const std::exception& e) {
            // Логирование или обработка ошибок, если они возникнут при закрытии
            logd("Error closing session: {}", e.what());
        }
    }

    net::awaitable<void> ReadLoop() {
      
        while (need_read_) {
            logi("start async read");

            boost::system::error_code read_ec;
            std::size_t n = 0;
            try {
                // Чтение данных с проверкой на отмену
                auto result = co_await stream_.async_read(
                    buffer_, boost::asio::as_tuple(boost::asio::use_awaitable));
                read_ec = std::get<0>(result);
                n       = std::get<1>(result);
            } catch (const boost::system::system_error& e) {
                read_ec = e.code();
                if(read_ec == boost::asio::error::operation_aborted){
                    //coroutine was cancelled
                    CloseSessionFast();
                    co_return;
                }else{
                    //other error
                    CloseSessionFast();
                    co_return;
                }
            }
        
            logi("invoke callback");

            // execute all callbacks
            if (n > 0) {
                cb_on_response_manager_.InvokeAll(buffer_);
            } else {
                logd("No data was read");
            }
            buffer_.consume(n);  // Освобождаем потребленные данные из буфера
        }
        logi("finished read");
        // Close the session after the read loop ends
        CloseSessionFast();
        logd("start execute cb when close session");
        //TransitionTo(aot::StatusSession::Closing);
    }

    void TransitionTo(aot::StatusSession new_state) {
        if (status_ == new_state) return;

        status_ = new_state;
        switch (status_) {
            case aot::StatusSession::Ready:
                if (on_ready_) on_ready_();
                break;
            case aot::StatusSession::Expired:
                if (on_expired_) on_expired_();
                break;
            case aot::StatusSession::Closing:
                if (on_closed_) on_closed_();
                break;
            default:
                break;
        }
    }
};

// template <typename _Timeout>
// class WssSession3 {
//   public:
//     using Timeout = _Timeout;

//   private:
//     aot::StatusSession state_ = aot::StatusSession::Resolving;

//     tcp::resolver resolver_;
//     websocket::stream<beast::ssl_stream<beast::tcp_stream>> stream_;
//     beast::flat_buffer buffer_; // (Must persist between reads)
//     std::string end_point_;
//     std::string request_json_;

//     LockFreeCallbackManager<boost::beast::flat_buffer> cb_on_response_manager_;
//     LockFreeCallbackManager<void> cb_on_close_session_manager_;

//     std::function<void()> on_ready_;
//     std::function<void()> on_expired_;
//     std::function<void()> on_closed_;

//     boost::asio::io_context& ioc_;
//     Timeout timeout_;
//     std::string_view host_;
//     std::string_view port_;
//     std::string_view default_endpoint_;
//     boost::asio::steady_timer timer_;

//     moodycamel::ConcurrentQueue<std::string> queue_requests_;

//     boost::asio::cancellation_signal cancel_signal_;
//     std::atomic<bool> read_loop_started_{false}; // Флаг для контроля запуска ReadLoop
//   public:
//     virtual ~WssSession3() = default;

//     explicit WssSession3(boost::asio::io_context& ioc, ssl::context& ctx,
//                          _Timeout timeout, const std::string_view host,
//                          const std::string_view port,
//                          const std::string_view default_endpoint)
//         : resolver_(net::make_strand(ioc)),
//           stream_(net::make_strand(ioc), ctx),
//           ioc_(ioc),
//           timeout_(timeout),
//           host_(host),
//           port_(port),
//           default_endpoint_(default_endpoint),
//           timer_(ioc) {
//         cancel_signal_.slot().assign(
//             [this](boost::asio::cancellation_type type) {
//                 logd("Cancellation requested");
//                 HandleCancellation(type);
//             });

//         net::co_spawn(ioc_,
//                       Run(host.data(), port.data(), default_endpoint.data()),
//                       [](std::exception_ptr e) {
//                           if (e) std::rethrow_exception(e);
//                       });
//     }
//     inline aot::StatusSession GetStatus() const { return state_; }

//     LockFreeCallbackManager<boost::beast::flat_buffer>::CallbackID
//     RegisterCallbackOnResponse(
//         const LockFreeCallbackManager<boost::beast::flat_buffer>::Callback cb) {
//         return cb_on_response_manager_.RegisterCallback(cb);
//     }

//     bool UnRegisterCallbackOnResponse(
//         LockFreeCallbackManager<boost::beast::flat_buffer>::CallbackID id) {
//         return cb_on_response_manager_.UnregisterCallback(id);
//     }

//     LockFreeCallbackManager<void>::CallbackID RegisterCallbackOnCloseSession(
//         const LockFreeCallbackManager<void>::Callback cb) {
//         return cb_on_close_session_manager_.RegisterCallback(cb);
//     }

//     bool UnRegisterCallbackOnCloseSession(
//         LockFreeCallbackManager<void>::CallbackID id) {
//         return cb_on_close_session_manager_.UnregisterCallback(id);
//     }

//     void RegisterOnReady(std::function<void()> callback) {
//         on_ready_ = std::move(callback);
//     }

//     void UnregisterOnReady() { on_ready_ = nullptr; }

//     void RegisterOnExpired(std::function<void()> callback) {
//         on_expired_ = std::move(callback);
//     }

//     void UnregisterOnExpired() { on_expired_ = nullptr; }

//     void RegisterOnClosed(std::function<void()> callback) {
//         on_closed_ = std::move(callback);
//     }

//     void UnregisterOnClosed() { on_closed_ = nullptr; }

//     net::awaitable<bool> AsyncRequest(std::string&& req) {
//         queue_requests_.enqueue(std::move(req));

//         if (state_ != aot::StatusSession::Ready) {
//             co_return false;
//         }

//         std::string req_new;
//         if (!queue_requests_.try_dequeue(req_new)) {
//             loge("can't dequeue request");
//         }

//         auto [write_ec, bytes_written] = co_await stream_.async_write(
//             net::buffer(req_new),
//             boost::asio::as_tuple(boost::asio::use_awaitable));

//         if (write_ec) {
//             if (write_ec == net::error::operation_aborted) {
//                 logd("Write operation cancelled");
//             } else {
//                 TransitionTo(aot::StatusSession::Closing);
//             }
//             co_return false;
//         }
//         if (!read_loop_started_.exchange(true)) {
//             co_spawn(ioc_, ReadLoop(), net::detached);
//         }
//         co_return true;
//     }

//     void AsyncCloseSessionGracefully() {
//         cancel_signal_.emit(boost::asio::cancellation_type::all);
//     }

//   private:
//     void TransitionTo(aot::StatusSession new_state) {
//         if (state_ == new_state) return;

//         state_ = new_state;
//         switch (state_) {
//             case aot::StatusSession::Connecting:
//                 if (on_ready_) on_ready_();
//                 break;
//             case aot::StatusSession::Expired:
//                 if (on_expired_) on_expired_();
//                 CloseSessionFast();
//                 break;
//             case aot::StatusSession::Closing:
//                 if (on_closed_) on_closed_();
//                 CloseSessionFast();
//                 break;
//             case aot::StatusSession::Cancelling:
//                 CloseSessionFast();
//                 break;
//             default:
//                 break;
//         }
//     }

//     void HandleCancellation(boost::asio::cancellation_type type) {
//         boost::asio::post(ioc_, [this](){
//             TransitionTo(aot::StatusSession::Cancelling);
//         });
//     }

//     net::awaitable<void> Run(const char* host, const char* port,
//                              const char* default_end_point) {
//         beast::error_code ec;

//         auto results = co_await resolver_.async_resolve(
//             host, port, net::redirect_error(net::use_awaitable, ec));
//         if (ec) {
//             loge("Failed to resolve host '{}:{}': {}", host, port,
//                  ec.message());
//             TransitionTo(aot::StatusSession::Closing);
//             co_return;
//         }
//         if (!SSL_set_tlsext_host_name(stream_.next_layer().native_handle(),
//                                               host)) {
//             auto ec = beast::error_code(static_cast<int>(::ERR_get_error()),
//                                         net::error::get_ssl_category());
//             loge("{}", ec.message());
//             co_return;
//         }
//         start_timer();

//         auto ep = co_await beast::get_lowest_layer(stream_).async_connect(
//             results, net::redirect_error(net::use_awaitable, ec));
//         if (ec) {
//             loge("Failed to connect to endpoint: {}", ec.message());
//             TransitionTo(aot::StatusSession::Closing);
//             co_return;
//         }
//         TransitionTo(aot::StatusSession::Connecting);

//         beast::get_lowest_layer(stream_).expires_never();

//         co_await stream_.next_layer().async_handshake(
//             ssl::stream_base::client,
//             net::redirect_error(net::use_awaitable, ec));
//         if (ec) {
//             loge("SSL handshake failed: {}", ec.message());
//             TransitionTo(aot::StatusSession::Closing);
//             co_return;
//         }

//         co_await stream_.async_handshake(
//             host, default_end_point,
//             net::redirect_error(net::use_awaitable, ec));
//         if (ec) {
//             loge("WebSocket handshake failed: {}", ec.message());
//             TransitionTo(aot::StatusSession::Closing);
//             co_return;
//         }

//         timer_.cancel();
//         TransitionTo(aot::StatusSession::Ready);
//         co_return;
//     }

//     void start_timer() {
//         timer_.expires_after(timeout_);

//         timer_.async_wait([this](beast::error_code ec) {
//             if (ec != boost::asio::error::operation_aborted) {
//                 logi("Session expired: closing.");
//                 TransitionTo(aot::StatusSession::Expired);
//             }
//         });
//     }

//     net::awaitable<void> ReadLoop() {
//         while (state_ == aot::StatusSession::Ready) {
//             logi("Start async read");

//             boost::system::error_code read_ec;
//             std::size_t n = 0;
//             try {
//                 auto result = co_await stream_.async_read(
//                     buffer_, boost::asio::as_tuple(boost::asio::use_awaitable));
//                 read_ec = std::get<0>(result);
//                 n = std::get<1>(result);
//             } catch (const boost::system::system_error& e) {
//                 read_ec = e.code();
//             }

//             if (read_ec) {
//                 logi("Read error: {}", read_ec.message());
//                 if(state_ == aot::StatusSession::Cancelling)
//                     co_return;
//                 else{
//                     TransitionTo(aot::StatusSession::Closing);
//                     co_return;
//                 }

//             }

//             if (n > 0) {
//                 cb_on_response_manager_.InvokeAll(buffer_);
//                 buffer_.consume(n);
//             } else {
//                 logd("No data was read");
//             }
//         }
//         read_loop_started_.store(false);
//         TransitionTo(aot::StatusSession::Cancelling);
//         co_return;
//     }
//     void CloseSessionFast() {
//          beast::get_lowest_layer(stream_).close();
//          cb_on_close_session_manager_.InvokeAll();
//          // Invoke all callbacks for session close
//      }
// };