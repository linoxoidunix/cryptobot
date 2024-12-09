#pragma once

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
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

// #include "aot/Exchange.h"
#include "aot/Logger.h"
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
template <typename _Timeout, typename ...AdditionalArgs>
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
    explicit HttpsSession(boost::asio::io_context& ioc, 
                            ssl::context& ctx,
                             _Timeout timeout, 
                                
                            const std::string_view host,
                          const std::string_view port,
                          AdditionalArgs&& ...)
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
        //start_timer();

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

template <typename _Timeout, typename ...AdditionalArgs>
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
    explicit HttpsSession2(boost::asio::io_context& ioc, 
                            ssl::context& ctx,
                             _Timeout timeout, 
                                
                            const std::string_view host,
                          const std::string_view port,
                          AdditionalArgs&& ...)
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
        //start_timer();

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

template <typename _Timeout, typename... AdditionalArgs>
class HttpsSession3 {
public:
    using Timeout = _Timeout;

private:
    bool is_connected_ = false;
    bool is_used_      = false;
    bool is_expired_   = false;
    bool is_shutting_down_ = false;  // Flag to indicate if shutdown is in progress


    boost::beast::http::request<boost::beast::http::string_body> req_;
    tcp::resolver resolver_;
    beast::ssl_stream<beast::tcp_stream> stream_;
    beast::flat_buffer buffer_;
    http::response<http::string_body> res_;
    const OnHttpsResponce* cb_;
    boost::asio::io_context& ioc_;
    Timeout timeout_;
    boost::asio::steady_timer timer_;

public:
    virtual ~HttpsSession3() = default;
    explicit HttpsSession3(boost::asio::io_context& ioc, ssl::context& ctx, _Timeout timeout, const std::string_view host, const std::string_view port, AdditionalArgs&&...)
        : resolver_(net::make_strand(ioc)),
          stream_(net::make_strand(ioc), ctx),
          ioc_(ioc),
          timeout_(timeout),
          timer_(ioc) {
        net::co_spawn(ioc_, Run(host.data(), port.data()), [](std::exception_ptr e) {
            if (e) std::rethrow_exception(e);
        });
    }

    template <typename CompletionHandler>
    net::awaitable<bool> AsyncRequest(http::request<http::string_body>&& req, const CompletionHandler* handler, net::cancellation_slot& slot) {
        if (!IsConnected() || IsUsed()) co_return false;
        req_ = std::move(req);
        cb_ = handler;
        is_used_ = true;
        slot.assign([this](boost::asio::cancellation_type_t) {
            AsyncCloseSessionGracefully();
        });
        auto [ec_write, bytes_written] = co_await http::async_write(stream_, req_, net::as_tuple(net::use_awaitable));
        if (ec_write) {
            if (ec_write == net::error::operation_aborted) {
                logd("Operation cancelled");
            } else {
                fail(ec_write, "write");
            }
            co_return false;
        }

        auto [ec_read, bytes_read] = co_await http::async_read(stream_, buffer_, res_, net::as_tuple(net::use_awaitable));
        if (ec_read) {
            if (ec_read == net::error::operation_aborted) {
                logd("Operation cancelled");
            } else {
                fail(ec_read, "read");
            }
            co_return false;
        }
        try{
            if (cb_) (*cb_)(res_);
        }
        catch (const boost::system::system_error& e) {
            logi("Exception in cb: {}", e.what());
            co_return false;
        }
        AsyncCloseSessionGracefully();
        slot.clear();
        co_return true;
    }

    // Close the session gracefully
    void AsyncCloseSessionGracefully() {
       boost::asio::post(ioc_, [this]() {
        if (is_shutting_down_) {
            logi("Shutdown is already in progress.");
            return;
        }
        is_shutting_down_ = true;

        stream_.async_shutdown([this](beast::error_code ec) {
            if (ec) {
                // Если `shutdown` завершился с ошибкой, логируем её, но всё равно закрываем соединение
                logi("SSL shutdown failed: {}", ec.message());
            }

            // close tcp connection
            close_session();
        });
    });
    }

    inline bool IsConnected() const { return is_connected_; }
    inline bool IsExpired() const { return is_expired_; }
    inline bool IsUsed() const { return is_used_; }

private:
    net::awaitable<void> Run(const char* host, const char* port) {
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host)) {
            auto ec = beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
            loge("{}", ec.message());
            co_return;
        }
        boost::beast::error_code ec;
        start_timer();
        auto results = co_await resolver_.async_resolve(host, port, net::use_awaitable);

        auto ep = co_await beast::get_lowest_layer(stream_).async_connect(results, net::use_awaitable);
        beast::get_lowest_layer(stream_).expires_never();


        co_await stream_.async_handshake(ssl::stream_base::client,
                                              boost::asio::redirect_error(boost::asio::use_awaitable, ec));

        if (ec) {
            loge("fail handshake");
            co_return;
        }

        is_connected_ = true;
    }

    void start_timer() {
        timer_.expires_after(timeout_);
        timer_.async_wait([this](beast::error_code ec) {
            if (ec != boost::asio::error::operation_aborted) {
                fail(ec, "session expired. it is closed by async timer");
                close_session();
            }
        });
    }

    void fail(beast::error_code ec, const char* what) {
        logi("{}: {}", what, ec.message());
        close_session();
    }

    void close_session() {
        beast::get_lowest_layer(stream_).close();
        is_expired_ = true;
    }
};

template <typename HTTPSessionType, typename... Args>
class ConnectionPool {
    moodycamel::ConcurrentQueue<HTTPSessionType*> until_handshake_connections_;
    moodycamel::ConcurrentQueue<HTTPSessionType*> after_handshake_connections_;
    moodycamel::ConcurrentQueue<HTTPSessionType*> used_connections_;
    moodycamel::ConcurrentQueue<HTTPSessionType*> expired_connections_;

    std::unique_ptr<std::jthread> timeout_thread_;
    std::promise<void> helper_thread_finished;
    std::future<void> block_until_helper_thread_finished;

    boost::asio::io_context& ioc_;
    ssl::context ssl_ctx_{ssl::context::sslv23};
    std::string host_;
    std::string port_;
    size_t pool_size_;
    common::MemoryPool<HTTPSessionType> session_pool_;
    HTTPSessionType::Timeout timeout_;
    std::tuple<std::decay_t<Args>...> ctor_args_;
  public:
    ConnectionPool(boost::asio::io_context& ioc, HTTPSessionType::Timeout timeout, size_t pool_size, const std::string_view host, const std::string_view port, Args&&... args)
        : block_until_helper_thread_finished(
              helper_thread_finished.get_future()),
          ioc_(ioc),
          host_(host),
          port_(port),
          pool_size_(pool_size),
          session_pool_(pool_size),
          timeout_(timeout),
          ctor_args_(std::forward<Args>(args)...) {
        for (std::size_t i = 0; i < pool_size; ++i) {
            auto session = CreateSession();
            until_handshake_connections_.enqueue(session);
        }
        timeout_thread_ =
            std::make_unique<std::jthread>([this](std::stop_token stoken) {
                this->MonitorConnections(stoken);
            });
    }

    ~ConnectionPool() {
        timeout_thread_->request_stop();
        block_until_helper_thread_finished.wait();
        // Deallocate all HttpsSession objects
        FreeQueue(until_handshake_connections_);
        FreeQueue(after_handshake_connections_);
        FreeQueue(used_connections_);
        FreeQueue(expired_connections_);
    }

    // Acquire a connection from the pool
    HTTPSessionType* AcquireConnection() {
        HTTPSessionType* session = nullptr;
        while (!after_handshake_connections_.try_dequeue(session)) {
            //block until get new handshaked connection;
        }
        ReleaseConnection(session);
        return session;
    }

    // Release a connection back to the pool
    void CloseAllSessions() {
    // Stop the helper thread and wait for it to finish
        logd("[START CLOSE ALL SESSION]");
        timeout_thread_->request_stop();
        block_until_helper_thread_finished.wait();
        // Now proceed to close all sessions
        CloseQueueSessions(until_handshake_connections_);
        CloseQueueSessions(after_handshake_connections_);
        CloseQueueSessions(used_connections_);
        CloseQueueSessions(expired_connections_);
        logd("[END CLOSE ALL SESSION]");

    }

  private:
      void ReleaseConnection(HTTPSessionType* session) {
        if (!session) return;
        if(auto status = used_connections_.try_enqueue(session); !status)
            loge("can't enque session in used_connections_");
    }
    void MonitorConnections(std::stop_token stoken) {
        HTTPSessionType* until_handshake_session[100];
        HTTPSessionType* after_handshake_session[100];
        HTTPSessionType* used_session[100];

        while (!stoken.stop_requested()) {
            size_t count_until_handshake = until_handshake_connections_.try_dequeue_bulk(until_handshake_session, 100);
            for (size_t i = 0; i < count_until_handshake; i++)
                ProcessSessionUntilHandshake(until_handshake_session[i]);

            size_t count_after_handshake = after_handshake_connections_.try_dequeue_bulk(after_handshake_session, 100);
            for (size_t i = 0; i < count_after_handshake; i++)
                ProcessSessionAfterHandshake(after_handshake_session[i]);

            // Process expired connections
            // if expired due to long waiting
            HTTPSessionType* expired_session = nullptr;
            while (expired_connections_.try_dequeue(expired_session)) {
                ReplaceTimedOutConnection(expired_session);
            }

            size_t count_used =
                used_connections_.try_dequeue_bulk(used_session, 100);
            for (size_t i = 0; i < count_used; i++)
                ProcessUsedSession(used_session[i]);

            auto ready_connection = after_handshake_connections_.size_approx();
            auto almost_ready_connection = until_handshake_connections_.size_approx();
            
            if(int needed_new_connection = pool_size_ - ready_connection - almost_ready_connection; needed_new_connection > 0){
                logd("add {} new session where ready:{} almost_ready:{}", needed_new_connection, ready_connection, almost_ready_connection);
                for(auto k = 0; k < needed_new_connection; k++) {
                    auto new_session = CreateSession();//session_pool_.Allocate(ioc_, ssl_ctx_, timeout_, host_, port_, ctor_args_);
                    if(auto status = until_handshake_connections_.try_enqueue(new_session); !status)
                        loge("can't enque connection");
                }
            }
            // Sleep briefly to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        helper_thread_finished.set_value();
    }

    // Replace a timed-out connection by deleting it and adding a new one
    void ReplaceTimedOutConnection(HTTPSessionType* session) {
        session_pool_.Deallocate(session);  // Deallocate the old session
        //  Create a new session and add it back to the pool
        auto new_session = CreateSession();
        until_handshake_connections_.enqueue(new_session);
    }

    template <typename T>
    void FreeQueue(T& t) {
        HTTPSessionType* session;
        logi("dealocate queue size:{}", t.size_approx());
        while (t.try_dequeue(session)) {
            session_pool_.Deallocate(session);
        }
    };

    inline void ProcessSessionUntilHandshake(HTTPSessionType* session) {
        if (session->IsExpired()) {
            if (!expired_connections_.try_enqueue(session)) {
                loge("loss session");
            }
            return;
        }

        if (session->IsConnected()) {
            if (!after_handshake_connections_.try_enqueue(session)) {
                loge("loss session");
            }
            return;
        }

        if (!until_handshake_connections_.try_enqueue(session)) {
            loge("loss session");
        }  // Return active connection back to the
           // available_connections_
    }

    inline void ProcessSessionAfterHandshake(HTTPSessionType* session) {
        if (session->IsExpired()) {
            expired_connections_.enqueue(
                session);  // Enqueue expired or already used connections
        } else {
            // Return active connection back to the
            // available_connections_
            if (auto status = after_handshake_connections_.try_enqueue(session);
                !status) {
                loge("loss connection");
            }
        }
    }

    inline void ProcessUsedSession(HTTPSessionType* session) {
        if(!session->IsUsed())
        {
            if (auto status = after_handshake_connections_.try_enqueue(session); !status) {
                loge("can't enque connection to after_handshake_connections_");
            }
            return;
        }
        if (session->IsExpired()) {
            ReplaceTimedOutConnection(session);  // Handle expired connections
            return;
        }
        
            // Return active connection back to the
            // available_connections_
        if (auto status = used_connections_.try_enqueue(session); !status) {
            loge("loss connection");
        }
    }

    template <typename QueueType>
    void CloseQueueSessions(QueueType& queue) {
        HTTPSessionType* session = nullptr;
        while (queue.try_dequeue(session)) {
            if (session) {
                session->AsyncCloseSessionGracefully();
            }
        }
    }

    HTTPSessionType* CreateSession() {
        // If args are provided, they will be forwarded to Allocate
        return std::apply([this](auto&&... unpacked_args) {
            return session_pool_.Allocate(ioc_, ssl_ctx_, timeout_, host_, port_, std::forward<decltype(unpacked_args)>(unpacked_args)...);
        }, ctor_args_);
    }
};
};  // namespace V2