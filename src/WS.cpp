#include <bybit/WS.h>
#include <bybit/WSImpl/WSImpl.h>
#include <boost/beast/core.hpp>
#include <bybit/Logger.h>
#include <bybit/root_certificates.hpp>

WS::WS(
    boost::asio::io_context& ioc
    /*Args args,
    OnMessage msg_cb,
    OnError error_cb,
    OnClose close_cb
    */
    ) :
    ioc_(ioc)
    {
        logger_ = std::unique_ptr<Logger>(new Logger("sadasd"));
        load_root_certificates(ctx_);
        session_ = std::shared_ptr<WSSession>(new WSSession(ioc, ctx_,logger_.get()));
    }

    void WS::Run(std::string_view host,
    std::string_view port,
    std::string_view endpoint
    )
    {
        //session_->run("stream.binance.com", "9443", "/ws/bnbusdt@depth@100ms");
        // "stream.binance.com", "9443", "/ws/bnbusdt@depth@100ms"
        session_->run(host.data(), port.data(), endpoint.data());
    }
