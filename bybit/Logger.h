#pragma once
#include <memory>
#include <string_view>
#include <stdio.h>


class LoggerI
{
    public:
        virtual void Log(std::string_view message) = 0;
};

class Logger : public LoggerI
{
    public:
        explicit Logger(std::string_view file_name);
        void Log(std::string_view message);
    private:
        FILE* file_out_;
};

class LoggerEmpty : public LoggerI
{
    public:
        explicit LoggerEmpty() = default;
        void Log(std::string_view message);
    private:
};