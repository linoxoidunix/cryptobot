#pragma once

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aot/Https.h"
#include "aot/Logger.h"
#include "aot/WS.h"
#include "aot/client_response.h"
#include "aot/common/types.h"
#include "aot/market_data/market_update.h"
#include "aot/third_party/emhash/hash_table7.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/asio/awaitable.hpp"
#include "boost/beast/http.hpp"
#include "concurrentqueue.h"
#include "magic_enum/magic_enum.hpp"

/**
 * Enum representing the network type.
 * @enum Network
 */
enum class Network {
    kMainnet,  ///< Main network
    kTestnet   ///< Test network
};

/**
 * Enum representing the protocol type.
 * @enum Protocol
 */
enum class Protocol {
    kHTTPS,  ///< HTTPS protocol
    kWS      ///< WebSocket protocol
};

/**
 * Class representing the configuration of an endpoint for a particular
 * exchange. Stores information like the host, port, recv_window, and threshold.
 */
class Endpoint {
  public:
    /**
     * Constructor for the Endpoint class.
     * Initializes the endpoint with the provided host, port, recv_window, and
     * threshold. Limits the recv_window to a maximum of 60000.
     * @param host The host URL of the endpoint.
     * @param port The port of the endpoint.
     * @param recv_window The receive window in milliseconds (default: 60000).
     * @param threshold The threshold value (default: 60000).
     */
    Endpoint(std::string_view host, int port, int recv_window, int threashold)
        : host_(host.data()),
          port_(port),
          port_as_string_(std::to_string(port)),
          recv_window_((recv_window > threashold) ? threashold : recv_window),
          threashold_(threashold) {
        /**
         * The recvWindow value is capped at a maximum of 60000 milliseconds, as
         * per exchange specifications. A smaller recvWindow (like 5000 ms or
         * less) is recommended, but the maximum allowed is 60000 ms.
         */
    }
    std::string_view Host() const { return host_; }
    int PortAsInteger() const { return port_; }
    std::string_view PortAsStringView() const { return port_as_string_; }
    int RecvWindow() const { return recv_window_; }

  private:
    std::string host_;            ///< Host URL of the endpoint
    int port_;                    ///< Port number of the endpoint
    std::string port_as_string_;  ///< Port number of the endpoint

    int recv_window_ = 5000;   ///< Maximum recvWindow (capped at 60000)
    int threashold_  = 60000;  ///< Threshold for the endpoint
};

/**
 * Struct used as a key for storing endpoints in an unordered_map.
 * This struct represents the combination of exchange, network, and protocol.
 */
struct EndpointKey {
    common::ExchangeId exchange;  ///< The exchange identifier
    Network network;              ///< The network type (mainnet or testnet)
    Protocol protocol;            ///< The protocol (HTTPS or WebSocket)

    /**
     * Equality operator to compare EndpointKey objects.
     */
    constexpr bool operator==(const EndpointKey &other) const {
        return exchange == other.exchange && network == other.network &&
               protocol == other.protocol;
    }
};

/**
 * Hash function for the EndpointKey struct, used in unordered_map.
 */
struct EndpointKeyHash {
    /**
     * Hashes the EndpointKey object by combining the hashes of its individual
     * fields.
     * @param key The EndpointKey object to hash.
     * @return The computed hash value.
     */
    std::size_t operator()(const EndpointKey &key) const {
        return std::hash<int>()(static_cast<int>(key.exchange)) ^
               (std::hash<int>()(static_cast<int>(key.network)) << 1) ^
               (std::hash<int>()(static_cast<int>(key.protocol)) << 2);
    }
};

/**
 * Class responsible for managing and accessing endpoint configurations.
 */
class EndpointManager {
  public:
    /**
     * Retrieves the endpoint based on the provided exchange, network, and
     * protocol.
     * @param exchange The exchange identifier.
     * @param network The network type (mainnet or testnet).
     * @param protocol The protocol type (HTTPS or WebSocket).
     * @return The corresponding Endpoint object.
     * @throws std::runtime_error If the endpoint is not found.
     */
    std::optional<std::reference_wrapper<const Endpoint>> GetEndpoint(
        common::ExchangeId exchange, Network network, Protocol protocol) const {
        EndpointKey key{exchange, network, protocol};
        auto it = endpoints_.find(key);
        if (it != endpoints_.end()) {
            return it->second;  // Return reference to the found Endpoint
        }
        return std::nullopt;  // If not found, return empty value
    }

  private:
    // The unordered_map that stores the endpoints with keys of type
    // EndpointKey.
    std::unordered_map<EndpointKey, Endpoint, EndpointKeyHash> endpoints_{
        // Initializing the endpoints with exchange configurations
        {{common::ExchangeId::kBinance, Network::kMainnet, Protocol::kHTTPS},
         {"api.binance.com", 443, 5000, 60000}},
        {{common::ExchangeId::kBinance, Network::kMainnet, Protocol::kWS},
         {"stream.binance.com", 9443, 5000, 60000}},
        {{common::ExchangeId::kBinance, Network::kTestnet, Protocol::kHTTPS},
         {"testnet.binance.vision", 443, 5000, 60000}},  // recv_window = 60000
        {{common::ExchangeId::kBinance, Network::kTestnet, Protocol::kWS},
         {"testnet.binance.vision", 9443, 5000, 60000}},
        {{common::ExchangeId::kBybit, Network::kMainnet, Protocol::kHTTPS},
         {"api.bybit.com", 9443, 5000, 100}},
        {{common::ExchangeId::kBybit, Network::kMainnet, Protocol::kWS},
         {"api.bybit.com", 443, 5000, 100}},
        {{common::ExchangeId::kBybit, Network::kTestnet, Protocol::kHTTPS},
         {"api-testnet.bybit.com", 443, 5000, 60000}},  // recv_window = 60000
        {{common::ExchangeId::kBybit, Network::kTestnet, Protocol::kWS},
         {"api-testnet.bybit.com", 443, 5000, 60000}}};
};

enum class TypeExchange { TESTNET, MAINNET };
// enum class Side { BUY, SELL };

/**
 * @brief Enum for response types that the parser can handle.
 */
enum class ResponseType {
    kSnapshot,          ///< Represents a snapshot response.
    kDepthUpdate,       ///< Represents a Depth Update response.
    kApiResponse,       ///< Represents a generic API response.
    kErrorResponse,     ///< Represents an error response.
    kNonQueryResponse,  ///< Represents a success response for non-query
                        ///< requests (e.g., subscribing/unsubscribing).
    kUnknown  ///< Represents an unknown response type.
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
        boost::intrusive_ptr<Exchange::BusEventRequestDiffOrderBook>
            bus_event_request_diff_order_book) {
        co_return;
    }
    virtual void AsyncStop() {}
    virtual ~BookEventGetterI() = default;
};

};  // namespace inner

struct BidAskState {
    bool need_make_snapshot = true;  // Tracks if this is the first run
    bool diff_packet_lost   = true;  // Tracks if diff packet sequence is lost
    bool need_process_current_snapshot = false;  // Tracks synchronization state
    bool need_process_current_diff     = false;  // Tracks synchronization state
    bool snapshot_and_diff_now_sync    = false;
    uint64_t last_id_diff_book_event = 0;  // Last processed diff book event ID
    Exchange::BookSnapshot snapshot;       // Snapshot of the order book
    explicit BidAskState() = default;
    void Reset() {
        need_make_snapshot            = true;
        diff_packet_lost              = true;
        need_process_current_snapshot = false;
        need_process_current_diff     = false;
        snapshot_and_diff_now_sync    = false;
        last_id_diff_book_event       = 0;
    }
};











struct BusEventRequestBBOPrice;
using BusEventRequestBBOPricePool = common::MemoryPool<BusEventRequestBBOPrice>;

/**
 * @brief when you want get actual BBOPrice for current trading pair
 * for given Exchange you need launch this signal
 *
 */
struct BusEventRequestBBOPrice : 
    public bus::Event2<BusEventRequestBBOPricePool> {
    common::ExchangeId exchange_id = common::kExchangeIdInvalid;
    common::TradingPair trading_pair;
    unsigned int snapshot_depth = 1000;
    bool subscribe              = true;
    // id request
    std::variant<std::string, long int, long unsigned int> id;
    explicit BusEventRequestBBOPrice() : bus::Event2<BusEventRequestBBOPricePool>(nullptr) {}
    explicit BusEventRequestBBOPrice(
        BusEventRequestBBOPricePool *mem_pool, common::ExchangeId _exchange_id,
        common::TradingPair _trading_pair, unsigned int _snapshot_depth,
        bool _subscribe,
        std::variant<std::string, long int, long unsigned int> _id)
        : bus::Event2<BusEventRequestBBOPricePool>(mem_pool),
            exchange_id(_exchange_id),
          trading_pair(_trading_pair),
          snapshot_depth(_snapshot_depth),
          subscribe(_subscribe),
          id(_id) {}

    friend void intrusive_ptr_release(BusEventRequestBBOPrice* ptr) {
        if (ptr->ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (ptr->memory_pool_) {
                ptr->memory_pool_->Deallocate(ptr);  // Return to the pool
                logd("free BusEventMEMarketUpdate2 ptr");
            }
        }
    }
    friend void intrusive_ptr_add_ref(BusEventRequestBBOPrice* ptr) {
        ptr->ref_count_.fetch_add(1, std::memory_order_relaxed);
    }
};

// using HTTPSesionType = V2::HttpsSession<std::chrono::seconds>;
using HTTPSesionType2 = V2::HttpsSession2<std::chrono::seconds>;
using HTTPSesionType3 = V2::HttpsSession3<std::chrono::seconds>;
using HTTPSesionType  = HTTPSesionType3;

using WSSesionType    = WssSession<std::chrono::seconds>;
using WSSesionType2   = WssSession2<std::chrono::seconds>;
using WSSesionType3   = WssSession3<std::chrono::seconds>;

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
        std::size_t pool_size, Network network,
        const EndpointManager &endpoint_manager) = 0;
};

/**
 * @enum ApiResponseStatus
 * @brief Represents the possible status values for an API response.
 *
 * This enum defines the status codes that indicate the result of an API
 * request.
 */
enum class ApiResponseStatus {
    kSuccess,  ///< The API request was successful.
    // kFailure, ///< The API request failed.
    // kPending, ///< The API request is pending.
    kError,  ///< There was an error in processing the API request.
};

/**
 * @struct ApiResponseData
 * @brief Represents the response data for API responses.
 *
 * This structure holds the status and ID fields from the API response.
 * It is used to store data returned from the API call, providing an easy way to
 * access the status of the response and the associated identifier.
 */
struct ApiResponseData {
    /**
     * @brief Status code of the API response.
     *
     * This field contains the status code returned by the API, represented by
     * an enum. It can be used to determine the success or failure of the API
     * request.
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
    int error_code;  // For error responses
    // Other fields for different responses can be added here
};

template <>
class fmt::formatter<ApiResponseData> {
  public:
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(const ApiResponseData &foo, Context &ctx) const {
        std::string id_str;
        std::visit(
            [&id_str](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    id_str = arg;  // If it's a string, store it
                } else {
                    id_str = std::to_string(arg);  // If it's an int or unsigned
                                                   // int, convert to string
                }
            },
            foo.id);
        return fmt::format_to(ctx.out(), "ApiResponseData[status:{} id:{}]",
                              magic_enum::enum_name(foo.status), id_str);
    }
};

template <>
class fmt::formatter<Network> {
  public:
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(const Network &foo, Context &ctx) const {
        return fmt::format_to(ctx.out(), "Network[{}]",
                              magic_enum::enum_name(foo));
    }
};

template <>
class fmt::formatter<Protocol> {
  public:
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(const Protocol &foo, Context &ctx) const {
        return fmt::format_to(ctx.out(), "Protocol[{}]",
                              magic_enum::enum_name(foo));
    }
};

template <>
class fmt::formatter<Endpoint> {
  public:
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(const Endpoint &foo, Context &ctx) const {
        return fmt::format_to(
            ctx.out(), "Endpoint[host:{} port:{} recv_window:{} threshold:{}]",
            foo.host_, foo.port_, foo.recv_window_, foo.threashold_);
    }
};