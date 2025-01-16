#pragma once    
#include <boost/beast/core.hpp>    
#include <boost/beast/http.hpp>

#include "aot/common/types.h"

using OnMessage = std::function<void(boost::beast::flat_buffer& fb)>;
using OnMessageTradingPair = std::function<void(boost::beast::flat_buffer& fb, common::TradingPair)>;
using OnHttpsResponce = std::function<void(boost::beast::http::response<boost::beast::http::string_body>& fb)>;
using OnHttpsResponseExtended = std::function<void(boost::beast::http::response<boost::beast::http::string_body>& fb, common::TradingPair)>;
using OnWssResponse = OnMessage;
using OnWssResponseTradingPair = OnMessageTradingPair;
using OnCloseSession = std::function<void()>;
