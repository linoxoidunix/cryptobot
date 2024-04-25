#pragma once
#include <bybit/third_party/fmt/core.h>

#include <boost/algorithm/string.hpp>
#include <string>
#include <string_view>

/**
 * @brief Interval = 1day or 1sec
 * 
 */
class Interval
{
  enum class Unit
  {
    SECONDS,
    MINUTES,
    HOURS,
    DAYS,
    UNKNOWN
  };
  /**
   * @brief @brief for Interval = 1day unit = DAYS
   * 
   */
  Unit unit = Unit::UNKNOWN;
  /**
   * @brief for Interval = 1day val = 1
   * 
   */
  uint val = 0;
};

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
    virtual ~KLineStreamI()               = default;
};

/**
 * @brief Symbol e.g BTCUSDT. May be on different exchane that symbol may be
 * have different separate symbol
 *
 */
class SymbolI {
  public:
    // explicit Symbol(std::string_view first, std::string_view second)
    //     : first_(first.data()),
    //       second_(second.data()){

    //       };
    virtual std::string ToString() const = 0; 
    // {
    //     auto out = fmt::format("{0}{1}", first_, second_);
    //     //boost::algorithm::to_lower(out);
    //     return out;
    // }
    virtual ~SymbolI() = default;

  private:
    std::string first_;
    std::string second_;
};