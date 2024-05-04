#include "aot/Https.h"
#include "aot/impl/HttpsImpl.h"
#include "aot/root_certificates.hpp"
#include "Https.h"

Https::Https(boost::asio::io_context & ctx, std::string_view request, OnMessage msg_cb)
{
    load_root_certificates(ctx_);
    ctx_.set_verify_mode(ssl::verify_peer);
    session_ =
        std::shared_ptr<HttpsSession>(new HttpSession(ioc, ctx_, request, msg_cb));
}
Https::~Https() {}