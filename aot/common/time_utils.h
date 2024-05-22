#pragma once

#include <chrono>
#include <ctime>
#include <string>

namespace common {
typedef int64_t Nanos;
typedef int64_t Delta;
constexpr Nanos NANOS_TO_MICROS  = 1000;
constexpr Nanos MICROS_TO_MILLIS = 1000;
constexpr Nanos MILLIS_TO_SECS   = 1000;
constexpr Nanos NANOS_TO_MILLIS  = NANOS_TO_MICROS * MICROS_TO_MILLIS;
constexpr Nanos NANOS_TO_SECS    = NANOS_TO_MILLIS * MILLIS_TO_SECS;

inline auto getCurrentNanoS() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

inline auto& getCurrentTimeStr(std::string* time_str) {
    const auto time =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time_str->assign(ctime(&time));
    if (!time_str->empty()) time_str->at(time_str->length() - 1) = '\0';
    return *time_str;
};

class TimeManager {
  public:
    explicit TimeManager() = default;
    void Update() { last_ = common::getCurrentNanoS(); };
    common::Delta GetDeltaInNanos() const {
        return common::getCurrentNanoS() - last_;
    };
    common::Delta GetDeltaInS() const {
        return (common::getCurrentNanoS() - last_) / NANOS_TO_SECS;
    };
    common::Delta GetDeltaInMilliS() const {
        return (common::getCurrentNanoS() - last_) / NANOS_TO_MILLIS;
    };
    common::Delta GetDeltaInMicroS() const {
        return (common::getCurrentNanoS() - last_) / NANOS_TO_MICROS;
    };

  private:
    common::Nanos start_ = common::getCurrentNanoS();
    common::Nanos last_  = start_;
};

}  // namespace Common