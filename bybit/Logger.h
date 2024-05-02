#pragma once
#include <cstdio>

#include <memory>
#include <string_view>
#define FMT_HEADER_ONLY
#include <bybit/third_party/fmt/core.h>
#define FMTLOG_HEADER_ONLY
#include <bybit/third_party/fmtlog.h>


// class LoggerI {
//   public:
//     virtual void Log(std::string_view message) = 0;
// };

// class Logger : public LoggerI {
//   public:
//     explicit Logger(std::string_view file_name);
//     void Log(std::string_view message) override;

//   private:
//     FILE *file_out_;
// };

// class LoggerEmpty : public LoggerI {
//   public:
//     explicit LoggerEmpty() = default;
//     void Log(std::string_view message) override;

//   private:
// };