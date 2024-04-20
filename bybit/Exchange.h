#pragma once
#include <string>
#include <string_view>
#include <bybit/third_party/fmt/core.h>
#include <boost/algorithm/string.hpp>

class ChartInterval
{
    public:
        virtual std::string ToString() const = 0;
        virtual ~ChartInterval() = default;
};

/**
 * @brief make stream channel for fetch kline from exchange
 * 
 */
class KLineStream
{
    public:
        /**
         * @brief 
         * 
         * @return std::string as name channel 
         */
        virtual std::string ToString() const = 0;
        virtual ~KLineStream() = default;

};

/**
 * @brief Symbol e.g BTCUSDT. May be on different exchane that symbol may be have different separate symbol
 * 
 */
class Symbol
{
    public:
        explicit Symbol(std::string_view first, std::string_view second) :
        first_(first.data()),
        second_(second.data())
        {

        };
        virtual std::string ToString() const
        {
            auto out = fmt::format("{0}{1}", first_, second_);
            boost::algorithm::to_lower(out);
            return out;
        }
    private:
        std::string first_;
        std::string second_;
};