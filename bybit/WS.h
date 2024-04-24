#include <bybit/WSImpl/WSImpl.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <boost/beast/core.hpp>
#include "Types.h"
class Args : public std::unordered_map<std::string, std::string> {
  public:
    explicit Args(std::string stream);
    std::string_view Host();

  private:
    std::string stream_;
};

namespace bst = boost::beast;

class WS {
  public:
    //using OnMessage = std::function<void(bst::flat_buffer& fb)>;
    using OnError   = std::function<void(std::string)>;
    using OnClose   = std::function<void(std::string)>;
    using Args      = std::unordered_map<std::string, std::string>;
    explicit WS(boost::asio::io_context& ctx,
                // Args args,
                OnMessage msg_cb
                // OnError error_cb,
                // OnClose close_cb
    );
    void Run(std::string_view host, std::string_view port,
             std::string_view endpoint);
    WS(const WS&)                = delete;
    WS& operator=(const WS&)     = delete;
    WS(WS&&) noexcept            = default;
    WS& operator=(WS&&) noexcept = default;

  private:
    std::shared_ptr<WSSession> session_;
    boost::asio::io_context& ioc_;
    ssl::context ctx_{ssl::context::tlsv12_client};
    //OnMessage on_msg_cb_;
};