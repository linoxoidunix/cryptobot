#pragma once
#include <string_view>
#include <bybit/ChartInterval.h>

namespace binance {
    class s1 : public ChartInterval
    {
        public:
        explicit s1() = default;
        std::string ToString(){
            return "1s";
        }
    }; 
    class m1 : public ChartInterval
    {
        public:
        explicit m1() = default;
        std::string ToString(){
            return "1m";
        }
    };
    class m3 : public ChartInterval
    {
        public:
        explicit m3() = default;
        std::string ToString(){
            return "3m";
        }
    }; 
    class m5 : public ChartInterval
    {
        public:
        explicit m5() = default;
        std::string ToString(){
            return "5m";
        }
    }; 
    class m15 : public ChartInterval
    {
        public:
        explicit m15() = default;
        std::string ToString(){
            return "15m";
        }
    };
    class m30 : public ChartInterval
    {
        public:
        explicit m30() = default;
        std::string ToString(){
            return "30m";
        }
    };    
    class h1 : public ChartInterval
    {
        public:
        explicit h1() = default;
        std::string ToString(){
            return "1h";
        }
    };
    class h2 : public ChartInterval
    {
        public:
        explicit h2() = default;
        std::string ToString(){
            return "2h";
        }
    };    
    class h4 : public ChartInterval
    {
        public:
        explicit h4() = default;
        std::string ToString(){
            return "4h";
        }
    };    
    class h6 : public ChartInterval
    {
        public:
        explicit h6() = default;
        std::string ToString(){
            return "6h";
        }
    };    
    class h8 : public ChartInterval
    {
        public:
        explicit h8() = default;
        std::string ToString(){
            return "8h";
        }
    };    
    class h12 : public ChartInterval
    {
        public:
        explicit h12() = default;
        std::string ToString(){
            return "12h";
        }
    };    
    class d1 : public ChartInterval
    {
        public:
        explicit d1() = default;
        std::string ToString(){
            return "1d";
        }
    };    
    class d3 : public ChartInterval
    {
        public:
        explicit d3() = default;
        std::string ToString(){
            return "3d";
        }
    };    
    class w1 : public ChartInterval
    {
        public:
        explicit w1() = default;
        std::string ToString(){
            return "1w";
        }
    };
    class M1 : public ChartInterval
    {
        public:
        explicit M1() = default;
        std::string ToString(){
            return "1M";
        }
    };
}