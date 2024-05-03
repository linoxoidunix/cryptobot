#include "aot/WS.h"

#include <boost/beast/core.hpp>

#include "aot/Logger.h"
#include "aot/impl/WSImpl.h"
#include "aot/root_certificates.hpp"
#include "aot/Types.h"



WS::WS(boost::asio::io_context& ioc, std::string_view request,
       // Args args,
       OnMessage msg_cb
       /*OnError error_cb,
       OnClose close_cb
       */
       )
    : ioc_(ioc) {
    load_root_certificates(ctx_);
    session_ =
        std::shared_ptr<WSSession>(new WSSession(ioc, ctx_, request, msg_cb));
}

WS::~WS() {}

void WS::Run(std::string_view host, std::string_view port,
             std::string_view endpoint) {
    session_->run(host.data(), port.data(), endpoint.data());
}
