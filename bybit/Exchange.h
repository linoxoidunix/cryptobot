#pragma once
#include <bybit/third_party/fmt/core.h>

#include <boost/algorithm/string.hpp>
#include <string>
#include <string_view>

/**
 * @brief Interval = 1day or 1sec
 *
 */
class Interval {
    enum class Unit { SECONDS, MINUTES, HOURS, DAYS, UNKNOWN };
    /**
     * @brief @brief for Interval = 1day unit = DAYS
     *
     */
    Unit unit = Unit::UNKNOWN;
    /**
     * @brief for Interval = 1day val = 1
     *
     */
    uint val  = 0;
};
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

/**
 * @brief for different exchanges ChartInterval has different format 1m or 1s or
 * 1M or 1d
 *
 */
class ChartInterval {
  public:
    virtual std::string ToString() const = 0;
    virtual ~ChartInterval()             = default;
};

/**
 * @brief make stream channel for fetch kline from exchange
 *
 */
class KLineStreamI {
  public:
    /**
     * @brief
     *
     * @return std::string as name channel
     */
    virtual std::string ToString() const = 0;
    virtual ~KLineStreamI()              = default;
};

/**
 * @brief Symbol e.g BTCUSDT. May be on different exchane that symbol may be
 * have different separate symbol
 *
 */
class SymbolI {
  public:
    virtual std::string ToString() const = 0;
    virtual ~SymbolI()                   = default;
};

/**
 * @brief get OHLCVI structure from json response from exchange
 *
 */
class ParserKLineResponseI {
  public:
    virtual OHLCVI Get(std::string_view response_from_exchange) const = 0;
    virtual ~ParserKLineResponseI()                                   = default;
};