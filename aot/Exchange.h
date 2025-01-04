#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "boost/algorithm/string.hpp"
#include "boost/asio/awaitable.hpp"
#include "boost/beast/http.hpp"

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "magic_enum/magic_enum.hpp"

#include "concurrentqueue.h"

#include "aot/Https.h"
#include "aot/Logger.h"
#include "aot/WS.h"
#include "aot/client_response.h"
#include "aot/common/types.h"
#include "aot/market_data/market_update.h"
#include "aot/third_party/emhash/hash_table7.hpp"


enum class TypeExchange { TESTNET, MAINNET };
// enum class Side { BUY, SELL };

/**
 * @brief Enum for response types that the parser can handle.
 */
enum class ResponseType {
    kSnapshot, ///< Represents a snapshot response.
    kDepthUpdate,      ///< Represents a Depth Update response.
    kApiResponse,      ///< Represents a generic API response.
    kErrorResponse,    ///< Represents an error response.
    kNonQueryResponse,  ///< Represents a success response for non-query requests
                       ///< (e.g., subscribing/unsubscribing).
    kUnknown           ///< Represents an unknown response type.
};
namespace https {
class ExchangeI {
  public:
    virtual ~ExchangeI()                     = default;
    virtual std::string_view Host() const    = 0;
    virtual std::string_view Port() const    = 0;
    virtual std::uint64_t RecvWindow() const = 0;
};

namespace sp {
template <typename Derived>
class ExchangeB {
  public:
    std::string_view Host() const {
        return static_cast<const Derived *>(this)->HostImpl();
    }
    std::string_view Port() const {
        return static_cast<const Derived *>(this)->PortImpl();
    }
    std::uint64_t RecvWindow() const {
        return static_cast<const Derived *>(this)->RecvWindowImpl();
    }
};
};  // namespace sp
};  // namespace https

class CurrentTime {
  public:
    explicit CurrentTime() = default;
    std::uint64_t Time() const {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
    };
};
class SignerI {
  public:
    virtual std::string Sign(std::string_view data)            = 0;
    virtual std::string SignByLowerCase(std::string_view data) = 0;

    virtual std::string_view ApiKey()                          = 0;
    virtual ~SignerI()                                         = default;
};
namespace hmac_sha256 {
struct Keys {
    std::string_view api_key;
    std::string_view secret_key;
    Keys(std::string_view _api_key, std::string_view _secret_key)
        : api_key(_api_key), secret_key(_secret_key) {};
};
class Signer : public SignerI {
  public:
    explicit Signer(std::string_view secret_key) : secret_key_(secret_key) {};
    explicit Signer(Keys keys)
        : secret_key_(keys.secret_key), api_key_(keys.api_key) {};

    /**
     * @brief add timestamp and recvWindow to data then signed this data
     *
     * @param api_key Get grom exchange
     * @param data class sign this data
     * @return std::string
     */
    std::string Sign(std::string_view data) override {
        std::uint8_t digest[EVP_MAX_MD_SIZE];
        std::uint32_t dilen{};

        auto p =
            ::HMAC(::EVP_sha256(), secret_key_.data(), secret_key_.length(),
                   (std::uint8_t *)data.data(), data.size(), digest, &dilen);
        assert(p);

        return B2aHex(digest, dilen);
    };
    std::string SignByLowerCase(std::string_view data) override {
        auto buffer = Sign(data);
        boost::algorithm::to_lower(buffer);
        return buffer;
    }
    std::string_view ApiKey() override { return api_key_; }
    ~Signer() override = default;

  private:
    std::string B2aHex(const std::uint8_t *p, std::size_t n) {
        static const char hex[] = "0123456789abcdef";
        std::string res;
        res.reserve(n * 2);

        for (auto end = p + n; p != end; ++p) {
            const std::uint8_t v  = (*p);
            res                  += hex[(v >> 4) & 0x0F];
            res                  += hex[v & 0x0F];
        }

        return res;
    };

  private:
    std::string_view secret_key_;
    std::string_view api_key_;
};
};  // namespace hmac_sha256
struct TickerInfo {
    uint8_t price_precission;
    uint8_t qty_precission;
};

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
    common::Price open;
    common::Price high;
    common::Price low;
    common::Price close;
    common::Qty volume;
    std::string ToString() const {
        return fmt::format("o:{} h:{} l:{} c:{} v:{}", open, high, low, close,
                           volume);
    }
};

/**
 * @brief OHLCVExt = OHLCV extended
 *
 */
struct OHLCVExt {
    OHLCV ohlcv;
    Interval interval;
    common::TradingPair trading_pair;
    std::string ToString() const {
        return fmt::format("{} o:{} h:{} l:{} c:{} v:{}",
                           trading_pair.ToString(), ohlcv.open, ohlcv.high,
                           ohlcv.low, ohlcv.close, ohlcv.volume);
    }
};
using OHLCVILFQueue = moodycamel::ConcurrentQueue<OHLCVExt>;

using OHLCVIStorage = std::list<OHLCVExt>;

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
    virtual void Init(OHLCVILFQueue &lf_queue) = 0;
    /**
     * @brief
     *
     * @return true if OHLCVGetter is not empty
     * @return false if OHLCVGetter is empty
     */
    virtual bool LaunchOne()                   = 0;
    virtual ~OHLCVGetter()                     = default;
};

/**
 * @brief capture price and quantity updates from exchange
 *
 */
class BookEventGetterI {
  public:
    virtual void Init(Exchange::BookDiffLFQueue &queue) = 0;
    virtual void Get()                                  = 0;
    virtual void LaunchOne()                            = 0;
    virtual ~BookEventGetterI()                         = default;
};

/**
 * @brief make stream channel for fetch kline from exchange
 *
 */
class KLineStreamI {
  public:
    /**
     * @brief for different exchanges ChartInterval has different format 1m or
     * 1s or 1M or 1d
     *
     */
    class ChartInterval {
      public:
        virtual std::string ToString() const = 0;
        virtual uint Seconds() const         = 0;
        virtual ~ChartInterval()             = default;
    };
    /**
     * @brief
     *
     * @return std::string as name channel
     */
    virtual std::string ToString() const = 0;
    virtual ~KLineStreamI()              = default;
};

class DiffDepthStreamI {
  public:
    /**
     * @brief
     *
     * @return std::string as name channel
     */
    virtual std::string ToString() const = 0;
    virtual ~DiffDepthStreamI()          = default;
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

class SymbolUpperCase : public SymbolI {
  public:
    explicit SymbolUpperCase(std::string_view first, std::string_view second)
        : first_(first.data()), second_(second.data()) {
        ticker_ = fmt::format("{0}{1}", first_, second_);
        boost::algorithm::to_upper(ticker_);
    };
    std::string ToString() const override { return ticker_; };
    ~SymbolUpperCase() override = default;

  private:
    std::string first_;
    std::string second_;
    std::string ticker_;
};

class SymbolLowerCase : public SymbolI {
  public:
    explicit SymbolLowerCase(std::string_view first, std::string_view second)
        : first_(first.data()), second_(second.data()) {
        ticker_ = fmt::format("{0}{1}", first_, second_);
        boost::algorithm::to_lower(ticker_);
    };
    explicit SymbolLowerCase(std::string_view first) : first_(first.data()) {};
    std::string ToString() const override { return ticker_; };
    ~SymbolLowerCase() override = default;

  private:
    std::string first_;
    std::string second_;
    std::string ticker_;
};

struct Ticker {
    const SymbolI *symbol;
    TickerInfo info;
    Ticker(const SymbolI *_symbol, const TickerInfo &_info)
        : symbol(_symbol), info(_info) {};
};

/**
 * @brief get OHLCVI structure from json response from exchange
 *
 */
class ParserKLineResponseI {
  public:
    virtual OHLCVExt Get(std::string_view response_from_exchange) const = 0;
    virtual ~ParserKLineResponseI() = default;
};
namespace Exchange {
class RequestNewOrder;
class BusEventRequestNewLimitOrder;
class RequestCancelOrder;
class BusEventRequestCancelOrder;
class BusEventRequestDiffOrderBook;
};  // namespace Exchange

namespace inner {
class OrderNewI {
  public:
    /**
     * @brief send RequestNewOrder order to exchange and get response from
     * exchange and transfer responce* to ClientResponseLFQueue
     *
     */
    virtual void Exec(Exchange::RequestNewOrder *,
                      Exchange::ClientResponseLFQueue *) = 0;

    virtual boost::asio::awaitable<void> CoExec(
        Exchange::BusEventRequestNewLimitOrder *order,
        const OnHttpsResponce &cb) {
        co_return;
    }
    virtual ~OrderNewI() = default;
};

class CancelOrderI {
  public:
    /**
     * @brief send RequestNewOrder order to exchange and get response from
     * exchange and transfer responce* to ClientResponseLFQueue
     *
     */
    virtual void Exec(Exchange::RequestCancelOrder *,
                      Exchange::ClientResponseLFQueue *) = 0;
    virtual boost::asio::awaitable<void> CoExec(
        Exchange::BusEventRequestCancelOrder *order,
        const OnHttpsResponce &cb) {
        co_return;
    }
    virtual ~CancelOrderI() = default;
};

class BookSnapshotI {
  public:
    virtual void Exec() {};
    virtual boost::asio::awaitable<void> CoExec(
        Exchange::BusEventRequestNewSnapshot *bus_event_request_new_snapshot) {
        co_return;
    }
    virtual ~BookSnapshotI() = default;
};

class BookEventGetterI {
  public:
    virtual boost::asio::awaitable<void> CoExec(
        boost::intrusive_ptr<Exchange::BusEventRequestDiffOrderBook> bus_event_request_diff_order_book) {
        co_return;
    }
    virtual void AsyncStop() {}
    virtual ~BookEventGetterI() = default;
};

};  // namespace inner

struct BidAskState {
    bool need_make_snapshot = true;                // Tracks if this is the first run
    bool diff_packet_lost = true;            // Tracks if diff packet sequence is lost
    bool need_process_current_snapshot = false; // Tracks synchronization state
    bool need_process_current_diff = false; // Tracks synchronization state
    bool snapshot_and_diff_now_sync = false; 
    uint64_t last_id_diff_book_event = 0;    // Last processed diff book event ID
    Exchange::BookSnapshot snapshot;         // Snapshot of the order book
    explicit BidAskState() = default;
    void Reset() {
        need_make_snapshot = true;
        diff_packet_lost = true;
        need_process_current_snapshot = false;
        need_process_current_diff = false;
        snapshot_and_diff_now_sync = false;
        last_id_diff_book_event = 0;
    }
};

/**
 * @brief when you want get actual BBOPrice for current trading pair 
 * for given Exchange you need launch this signal
 * 
 */
struct BusEventRequestBBOPrice{
  common::ExchangeId exchange_id = common::kExchangeIdInvalid;
  common::TradingPair trading_pair;
  unsigned int snapshot_depth = 1000;
  //id request
  std::variant<std::string, long int, long unsigned int> id;
  explicit BusEventRequestBBOPrice() = default;
  friend void intrusive_ptr_release(BusEventRequestBBOPrice* ptr){
  }
  friend void intrusive_ptr_add_ref(BusEventRequestBBOPrice* ptr) {
  }
};

// using HTTPSesionType = V2::HttpsSession<std::chrono::seconds>;
using HTTPSesionType2 = V2::HttpsSession2<std::chrono::seconds>;
using HTTPSesionType3 = V2::HttpsSession3<std::chrono::seconds>;
using HTTPSesionType = HTTPSesionType3;

using WSSesionType   = WssSession<std::chrono::seconds>;
using WSSesionType2  = WssSession2<std::chrono::seconds>;
using WSSesionType3  = WssSession3<std::chrono::seconds>;

using HTTPSSessionPool =
    std::unordered_map<common::ExchangeId,
                       V2::ConnectionPool<HTTPSesionType3> *>;

using WSSessionPool =
    std::unordered_map<common::ExchangeId, V2::ConnectionPool<WSSesionType> *>;
using NewLimitOrderExecutors =
    std::unordered_map<common::ExchangeId, inner::OrderNewI *>;
using CancelOrderExecutors =
    std::unordered_map<common::ExchangeId, inner::CancelOrderI *>;

// class HttpsConnectionPoolFactory {
//   public:
//     virtual ~HttpsConnectionPoolFactory() = default;
//     virtual V2::ConnectionPool<HTTPSesionType> *Create(
//         boost::asio::io_context &io_context, HTTPSesionType::Timeout timeout,
//         std::size_t pool_size, https::ExchangeI *exchange) = 0;
// };

class HttpsConnectionPoolFactory2 {
  public:
    virtual ~HttpsConnectionPoolFactory2() = default;
    virtual V2::ConnectionPool<HTTPSesionType3> *Create(
        boost::asio::io_context &io_context, HTTPSesionType3::Timeout timeout,
        std::size_t pool_size, https::ExchangeI *exchange) = 0;
};

/**
 * @enum ApiResponseStatus
 * @brief Represents the possible status values for an API response.
 *
 * This enum defines the status codes that indicate the result of an API request.
 */
enum class ApiResponseStatus {
    kSuccess, ///< The API request was successful.
    // kFailure, ///< The API request failed.
    // kPending, ///< The API request is pending.
    kError,   ///< There was an error in processing the API request.
};

/**
 * @struct ApiResponseData
 * @brief Represents the response data for API responses.
 *
 * This structure holds the status and ID fields from the API response.
 * It is used to store data returned from the API call, providing an easy way to access
 * the status of the response and the associated identifier.
 */
struct ApiResponseData {
    /**
     * @brief Status code of the API response.
     * 
     * This field contains the status code returned by the API, represented by an enum.
     * It can be used to determine the success or failure of the API request.
     */
    ApiResponseStatus status = ApiResponseStatus::kError;

    /**
     * @brief ID associated with the API request.
     * 
     * This field holds the unique identifier for the API request.
     * It allows tracking of the request across systems.
     */
    std::variant<std::string, long int, long unsigned int> id = 0;
};

// A type to represent any parsed data.
struct ParsedData {
    int error_code;         // For error responses
    // Other fields for different responses can be added here
};

template <>
class fmt::formatter<ApiResponseData> {
  public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(const ApiResponseData& foo,
                          Context& ctx) const {
        std::string id_str;
        std::visit([&id_str](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
                id_str = arg;  // If it's a string, store it
            } else {
                id_str = std::to_string(arg);  // If it's an int or unsigned int, convert to string
            }
        }, foo.id);
        return fmt::format_to(ctx.out(),
                              "ApiResponseData[status:{} id:{}]",
                              magic_enum::enum_name(foo.status), id_str);
    }
};

