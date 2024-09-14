#include "aot/Https.h"

#include "aot/Https.h"
#include "aot/impl/HttpsImpl.h"
#include "aot/root_certificates.hpp"

Https::Https(boost::asio::io_context& ioc,
             OnHttpsResponce msg_cb) {
    load_root_certificates(ctx_);
    ctx_.set_verify_mode(ssl::verify_peer);
    session_ = std::make_shared<HttpsSession>(ioc, ctx_, msg_cb);
}

void Https::Run(std::string_view host, std::string_view port,
                std::string_view endpoint,
                http::request<http::string_body>&& req) {
    session_->Run(host.data(), port.data(), endpoint.data(), std::move(req));
}