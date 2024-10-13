#pragma once

// #include <iostream>
// #include <fstream>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/system/error_code.hpp>
#include "aot/Types.h"
#include "aot/Logger.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class WSSession : public std::enable_shared_from_this<WSSession>
{
    tcp::resolver resolver_;
    websocket::stream<
        beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    //std::string text_;
    std::string end_point_;
    //std::string strbuf;
    std::string request_json_;
    std::string_view empty_json = "{}";
    OnMessage on_msg_cb_;
    //std::ofstream ofstream_;
public:
    /**
     * @brief Construct a new WSSession object
     * 
     * @param ioc 
     * @param ctx 
     * @param logger 
     * @param request_json if no need request than request_json={}
     */
    explicit
    WSSession(net::io_context& ioc, ssl::context& ctx, std::string_view request_json, OnMessage om_msg_cb)
        : resolver_(net::make_strand(ioc))
        , ws_(net::make_strand(ioc), ctx),
        on_msg_cb_(om_msg_cb),
        request_json_(request_json.data())
    {
    };

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port,
        char const* end_point)
    {
        // Save these for later
        host_ = host;
        end_point_ = end_point;
        // if(! SSL_set_tlsext_host_name(
        //         ws_.next_layer().native_handle(),
        //         host_.c_str()))
        // {
        //     auto ec = beast::error_code(static_cast<int>(::ERR_get_error()),
        //         net::error::get_ssl_category());
        //     loge("{}", ec.message());
        //     return;
        // }
        // Look up the domain name
        resolver_.async_resolve(
            host_,
            port,
            beast::bind_front_handler(
                &WSSession::on_resolve,
                shared_from_this()));
    };

    void
    on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
    {
        if(ec)
        {
            loge("{}", ec.message());
            return;
        }
        // std::cout << results.size()<< std::endl;
        // for(auto it : results)
        // {
        //     std::cout << it.host_name() << ":" << it.service_name() << " -> " << it.endpoint() << std::endl;
        // }
        // Set a timeout on the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(10));
        if(! SSL_set_tlsext_host_name(
                ws_.next_layer().native_handle(),
                host_.c_str()))
        {
            auto ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category());
            loge("{}", ec.message());
            return;
        }
        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                &WSSession::on_connect,
                shared_from_this()));
    };

    void
    on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
    {
        if(ec)
        {
            loge("{}", ec.message());
            return;
        }
        //std::cout << ep.hostname();
        // Set a timeout on the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host_ += ':' + std::to_string(ep.port());
        
        //Perform the SSL handshake
        ws_.next_layer().async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(
                &WSSession::on_ssl_handshake,
                shared_from_this()));
    };

    void
    on_ssl_handshake(beast::error_code ec)
    {
        if(ec)
        {
            loge("{}", ec.message());
            return;
        }

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();

        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async-ssl");
            }));

        // Perform the websocket handshake
        ws_.async_handshake(host_, end_point_,
            beast::bind_front_handler(
                &WSSession::on_handshake,
                shared_from_this()));
    };

    
    void on_handshake(beast::error_code ec)
    {
        if(ec)
        {
            loge("{}", ec.message());
            return;
        }
        if(request_json_ == empty_json)
            //start listen wright now
            ws_.async_read(
                buffer_,
                beast::bind_front_handler(
                    &WSSession::on_read,
                    shared_from_this()));
        else
            //send subscribe
            ws_.async_write(
                net::buffer(request_json_),
                beast::bind_front_handler(
                    &WSSession::on_write,
                    shared_from_this()));
    };

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
        {
            loge("{}", ec.message());
            return;
        }
        
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &WSSession::on_read,
                shared_from_this()));
    };
    

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        auto size = buffer_.size();
        assert(size == bytes_transferred);
        on_msg_cb_(buffer_);
        buffer_.consume(buffer_.size());
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &WSSession::on_read,
                shared_from_this()));
    };
private:
    void LogError(beast::error_code& ec)
    {
        loge("{}", ec.message());
        return;
    };


};
