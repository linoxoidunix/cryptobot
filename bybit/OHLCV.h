#pragma once
#include "Exchange.h"
#include <boost/beast/core.hpp>

struct OHLCV
{
    uint data;
    double open;
    double high;
    double low;
    double close;
    double volume;
};

struct OHLCVI
{
    OHLCV ohlcv;
    Interval interval;
};

/**
 * @brief return OHLCVI struct fron raw data from web socket
 * 
 */
class OHLCVGetter
{
    public:
        virtual OHLCVI Get(boost::beast::flat_buffer& buffer) = 0;
        virtual ~OHLCVGetter() = default;
};