#pragma once
#include "aot/Exchange.h"
#include "aot/Types.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ssl/context.hpp>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>


namespace bst = boost::beast;

class HttpsSession;

class Https {
  public:
    explicit Https(boost::asio::io_context& ctx,
                OnHttpsResponce msg_cb);
    void Run(std::string_view host, std::string_view port,
             std::string_view endpoint, boost::beast::http::request<boost::beast::http::string_body>&& req);
    Https(const Https&)                = delete;
    Https& operator=(const Https&)     = delete;
    Https(Https&&) noexcept            = default;
    Https& operator=(Https&&) noexcept = default;
    ~Https() = default;

  private:
    std::shared_ptr<HttpsSession> session_;
    //boost::asio::io_context& ioc_;
    boost::asio::ssl::context ctx_{boost::asio::ssl::context::tlsv12_client};
};