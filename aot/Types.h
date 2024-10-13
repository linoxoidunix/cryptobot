#pragma once    
#include <boost/beast/core.hpp>    
#include <boost/beast/http.hpp>

using OnMessage = std::function<void(boost::beast::flat_buffer& fb)>;
using OnHttpsResponce = std::function<void(boost::beast::http::response<boost::beast::http::string_body>& fb)>;
using OnWssResponse = OnMessage;


