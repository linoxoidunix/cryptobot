#pragma once
#include <boost/beast/core.hpp>

#include "Exchange.h"

struct OHLCV {
    uint data;
    double open;
    double high;
    double low;
    double close;
    double volume;
};

struct OHLCVI {
    OHLCV ohlcv;
    Interval interval;
};

using OHLCVIStorage = std::list<OHLCVI>;

/**
 * @brief return OHLCVI struct fron raw data from web socket
 *
 */
class OHLCVGetter {
  public:
    /**
     * @brief write all OHLCVI to buffer in real time from exchange
     *
     * @param buffer
     */
    virtual void Get(OHLCVIStorage& buffer) = 0;
    virtual ~OHLCVGetter()                  = default;
};