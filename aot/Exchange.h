#pragma once
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <boost/algorithm/string.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <string_view>

#include "aot/third_party/fmt/core.h"
namespace https {
class ExchangeI {
  public:
    virtual ~ExchangeI()                  = default;
    virtual std::string_view Host() const = 0;
    virtual std::string_view Port() const = 0;
};
};  // namespace https

namespace hmac_sha256 {
class Signer {
  public:
    explicit Signer(std::string_view api_key) : api_key_(api_key) {}

    /**
     * @brief add timestamp and recvWindow to data then signed this data
     *
     * @param api_key Get grom exchange
     * @param data class sign this data
     * @return std::string
     */
    std::string Sign(std::string_view data) {
        std::uint8_t digest[EVP_MAX_MD_SIZE];
        std::uint32_t dilen{};

        auto p = ::HMAC(::EVP_sha256(), api_key_.c_str(), api_key_.length(),
                        (std::uint8_t *)data.data(), data.size(), digest, &dilen);
        assert(p);

        return b2a_hex(digest, dilen);
    }

  private:
    std::uint64_t get_current_ms_epoch() {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
    }

    std::string b2a_hex(const std::uint8_t *p, std::size_t n) {
        static const char hex[] = "0123456789abcdef";
        std::string res;
        res.reserve(n * 2);

        for (auto end = p + n; p != end; ++p) {
            const std::uint8_t v  = (*p);
            res                  += hex[(v >> 4) & 0x0F];
            res                  += hex[v & 0x0F];
        }

        return res;
    }

    std::string hmac_sha256(const char *key, std::size_t klen, const char *data,
                            std::size_t dlen) {
        std::uint8_t digest[EVP_MAX_MD_SIZE];
        std::uint32_t dilen{};

        auto p = ::HMAC(::EVP_sha256(), key, klen, (std::uint8_t *)data, dlen,
                        digest, &dilen);
        assert(p);

        return b2a_hex(digest, dilen);
    }

  private:
    std::string api_key_;
};
};  // namespace hmac_sha256
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
    virtual void Get(OHLCVIStorage &buffer) = 0;
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

namespace inner {
class OrderNewI {
  public:
    /**
     * @brief send order to exchange
     *
     */
    virtual void Exec() = 0;
};
}  // namespace inner