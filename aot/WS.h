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