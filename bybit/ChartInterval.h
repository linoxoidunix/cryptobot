#pragma once

class ChartInterval
{
    public:
        virtual std::string ToString() const = 0;
        virtual ~ChartInterval() = default;
};

/**
 * @brief 
 * 
 */
class KLineStream
{
    public:
        /**
         * @brief 
         * 
         * @return std::string 
         */
        virtual std::string ToString() const = 0;
        virtual ~KLineStream() = default;

}