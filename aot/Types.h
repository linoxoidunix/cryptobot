#pragma once    
#include <boost/beast/core.hpp>    
using OnMessage = std::function<void(boost::beast::flat_buffer& fb)>;
