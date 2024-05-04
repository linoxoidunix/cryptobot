#pragma once

#include <boost/beast/core.hpp>
#include <boost/asio/ssl/context.hpp>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aot/Types.h"

namespace bst = boost::beast;

class HttpsSession;

class Https {
  public:
    explicit Https(boost::asio::io_context& ctx, std::string_view request,
                OnMessage msg_cb);
    void Run(std::string_view host, std::string_view port,
             std::string_view endpoint);
    Https(const Https&)                = delete;
    Https& operator=(const Https&)     = delete;
    Https(Https&&) noexcept            = default;
    Https& operator=(Https&&) noexcept = default;
    ~Https();

  private:
    std::shared_ptr<HttpsSession> session_;
    boost::asio::io_context& ioc_;
    boost::asio::ssl::context ctx_{boost::asio::ssl::context::tlsv12_client};
};