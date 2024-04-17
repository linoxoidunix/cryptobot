#include "bybit/Logger.h"

#define FMT_HEADER_ONLY
#include <bybit/third_party/fmt/core.h> 
#define FMTLOG_HEADER_ONLY
#include <bybit/third_party/fmtlog.h>


Logger::Logger(std::string_view file_name)
{
    file_out_ = fopen (file_name.data() , "w");
    fmtlog::setLogFile(file_out_, true);
    fmtlog::setLogLevel(fmtlog::DBG);
}

void Logger::Log(std::string_view message)
{
    logd("{}",message.data());
}

void LoggerEmpty::Log(std::string_view message)
{
}