#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "aot/Exchange.h"
#include "aot/Https.h"
#include "aot/Logger.h"
#include "aot/WS.h"
#include "aot/bus/bus.h"
#include "aot/client_request.h"
#include "aot/client_response.h"
#include "aot/common/thread_utils.h"
#include "aot/common/time_utils.h"
#include "aot/common/types.h"
#include "aot/market_data/market_update.h"
#include "aot/prometheus/event.h"
#include "boost/asio.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/http/message.hpp"
#include "boost/beast/version.hpp"
#include "nlohmann/json.hpp"
#include "simdjson.h"

// Spot API URL                               Spot Test Network URL
// https://api.binance.com/api https://testnet.binance.vision/api
// wss://stream.binance.com:9443/ws	         wss://testnet.binance.vision/ws
// wss://stream.binance.com:9443/stream	     wss://testnet.binance.vision/stream

namespace binance {

const auto kMeasureTForGeneratorBidAskService =
    MEASURE_T_FOR_GENERATOR_BID_ASK_SERVICE;  // MEASURE_T_FOR_GENERATOR_BID_ASK_SERVICE
                                              // define in cmakelists.txt
enum class TimeInForce { GTC, IOC, FOK };

namespace testnet {
class HttpsExchange : public https::ExchangeI {
  public:
    /**
     * @brief Construct a new Https Exchange object
     *
     * @param recv_window
     * https://binance-docs.github.io/apidocs/spot/en/#endpoint-security-type
     * An additional parameter, recvWindow, may be sent to specify the number of
     * milliseconds after timestamp the request is valid for. If recvWindow is
     * not sent, it defaults to 5000.
     */
    explicit HttpsExchange(std::uint64_t recv_window = 5000)
        : recv_window_((recv_window > 60000) ? 60000 : recv_window) {
              /**
               * @brief
                https://binance-docs.github.io/apidocs/spot/en/#endpoint-security-type
                It is recommended to use a small recvWindow of 5000 or less! The
               max cannot go beyond 60,000!
               *
               */
          };

    ~HttpsExchange() override = default;
    std::string_view Host() const override {
        return std::string_view("testnet.binance.vision");
    };
    std::string_view Port() const override { return "443"; };
    std::uint64_t RecvWindow() const override { return recv_window_; };

  private:
    std::uint64_t recv_window_;
};

namespace sp {
template <std::uint64_t DefaultRecvWindow = 5000,
          std::uint64_t MaxRecvWindow     = 60000>
class HttpsExchange;  // Forward declaration of HttpsExchange
template <std::uint64_t DefaultRecvWindow, std::uint64_t MaxRecvWindow>
class HttpsExchange : public https::sp::ExchangeB<
                          HttpsExchange<DefaultRecvWindow, MaxRecvWindow>> {
  public:
    /**
     * @brief Construct a new Https Exchange object
     *
     * @param recv_window
     * https://binance-docs.github.io/apidocs/spot/en/#endpoint-security-type
     * An additional parameter, recvWindow, may be sent to specify the number of
     * milliseconds after timestamp the request is valid for. If recvWindow is
     * not sent, it defaults to DefaultRecvWindow.
     */
    explicit HttpsExchange(std::uint64_t recv_window = DefaultRecvWindow)
        : recv_window_((recv_window > MaxRecvWindow) ? MaxRecvWindow
                                                     : recv_window) {
              /**
               * @brief
               * https://binance-docs.github.io/apidocs/spot/en/#endpoint-security-type
               * It is recommended to use a small recvWindow of 5000 or less!
               * The max cannot go beyond 60,000!
               *
               */
          };

    // Using static polymorphism to provide behavior
    constexpr std::string_view HostImpl() const {
        return "testnet.binance.vision";
    };

    constexpr std::string_view PortImpl() const { return "443"; };

    constexpr std::uint64_t RecvWindowImpl() const { return recv_window_; };

  private:
    std::uint64_t recv_window_;
};
};  // namespace sp

};  // namespace testnet
namespace mainnet {
class HttpsExchange : public https::ExchangeI {
  public:
    /**
     * @brief Construct a new Https Exchange object
     *
     * @param recv_window
     * https://binance-docs.github.io/apidocs/spot/en/#endpoint-security-type
     * An additional parameter, recvWindow, may be sent to specify the number of
     * milliseconds after timestamp the request is valid for. If recvWindow is
     * not sent, it defaults to 5000.
     */
    explicit HttpsExchange(std::uint64_t recv_window = 5000)
        : recv_window_((recv_window > 60000) ? 60000 : recv_window) {
              /**
               * @brief
                https://binance-docs.github.io/apidocs/spot/en/#endpoint-security-type
                It is recommended to use a small recvWindow of 5000 or less! The
               max cannot go beyond 60,000!
               *
               */
          };

    virtual ~HttpsExchange() = default;
    std::string_view Host() const override {
        return std::string_view("stream.binance.com");
    };
    std::string_view Port() const override { return "9443"; };
    std::uint64_t RecvWindow() const override { return recv_window_; };

  private:
    std::uint64_t recv_window_;
};
};  // namespace mainnet

class ExchangeChooser {
  public:
    explicit ExchangeChooser() = default;
    https::ExchangeI* Get(TypeExchange type_exchange) {
        https::ExchangeI* current_exchange;
        switch (type_exchange) {
            case TypeExchange::MAINNET:
                current_exchange = &main_net_;
                break;
            case TypeExchange::TESTNET:
            default:
                current_exchange = &test_net_;
                break;
        }
        return current_exchange;
    }

  private:
    binance::testnet::HttpsExchange test_net_;
    binance::mainnet::HttpsExchange main_net_;
};

enum class Type {
    LIMIT,
    MARKET,
    STOP_LOSS,
    STOP_LOSS_LIMIT,
    TAKE_PROFIT,
    TAKE_PROFIT_LIMIT,
    LIMIT_MAKER
};

struct StringEqual {
    using is_transparent = void;

    bool operator()(std::string_view l, std::string_view r) const {
        return l.compare(r) == 0;
    }
};

struct StringHash {
    using transparent_key_equal = StringEqual;  // or std::equal_to<>

    std::size_t operator()(const std::string& s) const { return s.size(); }

    std::size_t operator()(const char* s) const { return std::strlen(s); }
};

class Args : public std::unordered_map<std::string, std::string, StringHash,
                                       StringHash::transparent_key_equal> {};

class ArgsBody : public Args {
  public:
    explicit ArgsBody() : Args() {};
    /**
     * @brief json wrapped around unordered_map
     *
     * @return std::string
     */
    virtual std::string Body();
    virtual std::string QueryString() {
        std::list<std::string> merged;
        std::ranges::for_each(*this, [&merged](const auto& expr) {
            merged.emplace_back(fmt::format("{}={}", expr.first, expr.second));
        });
        auto out = boost::algorithm::join(merged, "&");
        return out;
    };

    virtual ~ArgsBody() = default;
};

class ArgsQuery : public Args {
  public:
    explicit ArgsQuery() : Args() {};
    /**
     * @brief return query string starts with ?
     *
     * @return std::string
     */
    virtual std::string QueryString() {
        std::list<std::string> merged;
        std::ranges::for_each(*this, [&merged](const auto& expr) {
            merged.emplace_back(fmt::format("{}={}", expr.first, expr.second));
        });
        // std::for_each(begin(), end(), [&merged](const auto& expr) {
        //     merged.emplace_back(fmt::format("{}={}", expr.first,
        //     expr.second));
        // });
        auto out = boost::algorithm::join(merged, "&");
        return out;
    };
    virtual std::string FinalQueryString() {
        auto out = QueryString();
        if (!out.empty()) out.insert(out.begin(), '?');
        return out;
    };
    virtual ~ArgsQuery() = default;
};

class s1 : public KLineStreamI::ChartInterval {
  public:
    explicit s1() = default;
    std::string ToString() const override { return "1s"; };
    uint Seconds() const override { return 1; };
};
class m1 : public KLineStreamI::ChartInterval {
  public:
    explicit m1() = default;
    std::string ToString() const override { return "1m"; };
    uint Seconds() const override { return 60; };
};
class m3 : public KLineStreamI::ChartInterval {
  public:
    explicit m3() = default;
    std::string ToString() const override { return "3m"; };
    uint Seconds() const override { return 180; };
};
class m5 : public KLineStreamI::ChartInterval {
  public:
    explicit m5() = default;
    std::string ToString() const override { return "5m"; }
    uint Seconds() const override { return 300; };
};
class m15 : public KLineStreamI::ChartInterval {
  public:
    explicit m15() = default;
    std::string ToString() const override { return "15m"; }
    uint Seconds() const override { return 900; };
};
class m30 : public KLineStreamI::ChartInterval {
  public:
    explicit m30() = default;
    std::string ToString() const override { return "30m"; }
    uint Seconds() const override { return 1800; };
};
class h1 : public KLineStreamI::ChartInterval {
  public:
    explicit h1() = default;
    std::string ToString() const override { return "1h"; }
    uint Seconds() const override { return 3600; };
};
class h2 : public KLineStreamI::ChartInterval {
  public:
    explicit h2() = default;
    std::string ToString() const override { return "2h"; }
    uint Seconds() const override { return 7200; };
};
class h4 : public KLineStreamI::ChartInterval {
  public:
    explicit h4() = default;
    std::string ToString() const override { return "4h"; }
    uint Seconds() const override { return 14400; };
};
class h6 : public KLineStreamI::ChartInterval {
  public:
    explicit h6() = default;
    std::string ToString() const override { return "6h"; }
    uint Seconds() const override { return 21600; };
};
class h8 : public KLineStreamI::ChartInterval {
  public:
    explicit h8() = default;
    std::string ToString() const override { return "8h"; }
    uint Seconds() const override { return 28800; };
};
class h12 : public KLineStreamI::ChartInterval {
  public:
    explicit h12() = default;
    std::string ToString() const override { return "12h"; }
    uint Seconds() const override { return 43200; };
};
class d1 : public KLineStreamI::ChartInterval {
  public:
    explicit d1() = default;
    std::string ToString() const override { return "1d"; }
    uint Seconds() const override { return 86400; };
};
class d3 : public KLineStreamI::ChartInterval {
  public:
    explicit d3() = default;
    std::string ToString() const override { return "3d"; }
    uint Seconds() const override { return 259200; };
};
class w1 : public KLineStreamI::ChartInterval {
  public:
    explicit w1() = default;
    std::string ToString() const override { return "1w"; }
    uint Seconds() const override { return 604800; };
};
class M1 : public KLineStreamI::ChartInterval {
  public:
    explicit M1() = default;
    std::string ToString() const override { return "1M"; }
    uint Seconds() const override { return 2.628e6; };
};
class KLineStream : public KLineStreamI {
  public:
    explicit KLineStream(std::string_view trading_pair,
                         const KLineStreamI::ChartInterval* chart_interval)
        : trading_pair_(trading_pair), chart_interval_(chart_interval) {};
    std::string ToString() const override {
        return fmt::format("{0}@kline_{1}", trading_pair_,
                           chart_interval_->ToString());
    };
    ~KLineStream() override = default;

  private:
    std::string_view trading_pair_;
    const KLineStreamI::ChartInterval* chart_interval_;
};

class DiffDepthStream : public DiffDepthStreamI {
  public:
    class StreamIntervalI {
      public:
        virtual std::string ToString() const = 0;
        virtual ~StreamIntervalI()           = default;
    };
    class ms1000 : public StreamIntervalI {
      public:
        explicit ms1000() = default;
        std::string ToString() const override { return "1000ms"; };
        ~ms1000() override = default;
    };
    class ms100 : public StreamIntervalI {
      public:
        explicit ms100() = default;
        std::string ToString() const override { return "100ms"; };
        ~ms100() override = default;
    };

    explicit DiffDepthStream(const common::TradingPairInfo& s,
                             const StreamIntervalI* interval)
        : symbol_(s), interval_(interval) {};
    std::string ToString() const override {
        return fmt::format("{0}@depth@{1}", symbol_.ws_query_request,
                           interval_->ToString());
    };

  private:
    const common::TradingPairInfo& symbol_;
    const StreamIntervalI* interval_;
};

class OHLCVI : public OHLCVGetter {
    common::TradingPairHashMap& map_;
    common::TradingPairReverseHashMap pairs_reverse_;
    common::TradingPair pair_;
    class ParserResponse {
      public:
        explicit ParserResponse(
            common::TradingPairReverseHashMap& pairs_reverse,
            common::TradingPairHashMap& map, common::TradingPair pair)
            : pairs_reverse_(pairs_reverse), map_(map), pair_(pair) {};
        OHLCVExt Parse(std::string_view response);

      private:
        common::TradingPairReverseHashMap& pairs_reverse_;
        common::TradingPairHashMap& map_;
        common::TradingPair pair_;
    };

  public:
    OHLCVI(common::TradingPairHashMap& map,
           common::TradingPairReverseHashMap& pair_reverse,
           common::TradingPair pair,
           const KLineStreamI::ChartInterval* chart_interval,
           TypeExchange type_exchange)
        : current_exchange_(exchange_.Get(type_exchange)),
          map_(map),
          pairs_reverse_(pair_reverse),
          pair_(pair),
          chart_interval_(chart_interval),
          type_exchange_(type_exchange) {};
    bool LaunchOne() override {
        ioc.run_one();
        return true;
    };
    void Init(OHLCVILFQueue& lf_queue) override {
        std::function<void(boost::beast::flat_buffer & buffer)> OnMessageCB;
        OnMessageCB = [&lf_queue, this](boost::beast::flat_buffer& buffer) {
            auto result = boost::beast::buffers_to_string(buffer.data());
            ParserResponse parser(pairs_reverse_, map_, pair_);
            lf_queue.try_enqueue(parser.Parse(result));
        };
        assert(false);
        using kls = KLineStream;
        kls channel(map_[pair_].ws_query_request, chart_interval_);
        std::string empty_request = "{}";
        std::make_shared<WS>(ioc, empty_request, OnMessageCB)
            ->Run(current_exchange_->Host(), current_exchange_->Port(),
                  fmt::format("/ws/{0}", channel.ToString()));
    };

  private:
    boost::asio::io_context ioc;
    ExchangeChooser exchange_;
    https::ExchangeI* current_exchange_ = nullptr;
    const KLineStreamI::ChartInterval* chart_interval_;
    TypeExchange type_exchange_;
};

namespace detail {
class FamilyBookEventGetter {
    static constexpr std::string_view end_point_as_json = "/ws";

  public:
    class ParserResponse {
      public:
        explicit ParserResponse(
            common::TradingPairHashMap& pairs,
            common::TradingPairReverseHashMap& pairs_reverse)
            : pairs_reverse_(pairs_reverse), pairs_(pairs) {};

        Exchange::BookDiffSnapshot Parse(std::string_view response);
        Exchange::BookDiffSnapshot Parse(simdjson::ondemand::document& doc);

      private:
        common::TradingPairHashMap& pairs_;
        common::TradingPairReverseHashMap& pairs_reverse_;
    };
    class ArgsOrder : public ArgsQuery {
      public:
        using SymbolType = std::string_view;
        using Limit      = uint16_t;
        explicit ArgsOrder(SymbolType ticker1, SymbolType ticker2, Limit limit)
            : ArgsQuery() {
            SetSymbol(ticker1, ticker2);
            SetLimit(limit);
        };
        explicit ArgsOrder(
            const Exchange::RequestSnapshot* request_cancel_order,
            common::TradingPairHashMap& pairs)
            : ArgsQuery() {
            SetSymbol(
                pairs[request_cancel_order->trading_pair].https_query_request);
            SetLimit(request_cancel_order->depth);
        };

      private:
        void SetSymbol(SymbolType symbol) {
            storage["symbol"] = symbol.data();
        };
        void SetSymbol(SymbolType ticker1, SymbolType ticker2) {
            SymbolUpperCase formatter(ticker1, ticker2);
            storage["symbol"] = formatter.ToString().data();
        };
        void SetLimit(Limit limit) {
            /**
             * @brief If limit > 5000, then the response will truncate to 5000.
             * https://binance-docs.github.io/apidocs/spot/en/#exchange-information
             */
            limit            = (limit > 5000) ? 5000 : limit;

            storage["limit"] = std::to_string(limit);
        };

      private:
        ArgsOrder& storage = *this;
    };

    /**
     * @class ArgsBody
     * @brief A class to wrap request parameters into a JSON object,
     * specifically designed for trading pair requests.
     */
    class ArgsBody : public nlohmann::json {
      public:
        /**
         * @brief Constructs the ArgsBody object and initializes JSON
         * parameters.
         * @param request Pointer to the RequestDiffOrderBook object containing
         * request details.
         * @param pairs Reference to the trading pair hash map.
         */
        ArgsBody(const Exchange::RequestDiffOrderBook* request,
                 common::TradingPairHashMap& pairs)
            : nlohmann::json(), pairs_(pairs) {
            InitializeParams(request);
        }

        /**
         * @brief Serializes the JSON object to a string.
         * @return A string representation of the JSON object.
         */
        std::string Body() const { return this->dump(); }

        /**
         * @brief Virtual destructor for ArgsBody.
         */
        virtual ~ArgsBody() = default;

      private:
        common::TradingPairHashMap&
            pairs_;  ///< Reference to the trading pair hash map.

        /**
         * @brief Initializes the parameters of the JSON object based on the
         * request and trading pairs.
         * @param request Pointer to the RequestDiffOrderBook object containing
         * request details.
         */
        void InitializeParams(const Exchange::RequestDiffOrderBook* request) {
            SetParams(request);
            SetMethod(request->subscribe);
            SetId(request->id);
        }

        /**
         * @brief Sets the "params" field in the JSON object.
         * @param request Pointer to the RequestDiffOrderBook object containing
         * request details.
         */
        void SetParams(const Exchange::RequestDiffOrderBook* request) {
            if (request->frequency == common::kFrequencyMSInvalid) {
                nlohmann::json params_array = nlohmann::json::array();
                params_array.push_back(fmt::format(
                    "{}@{}", pairs_.at(request->trading_pair).ws_query_request,
                    "depth"));
                (*this)["params"] = params_array;
            }
        }

        /**
         * @brief Sets the "method" field in the JSON object based on the
         * subscription type.
         * @param subscribe Boolean indicating whether the request is a
         * subscription.
         */
        void SetMethod(bool subscribe) {
            (*this)["method"] = subscribe ? "SUBSCRIBE" : "UNSUBSCRIBE";
        }

        /**
         * @brief Sets the "id" field in the JSON object using a variant type.
         * @param id Variant containing the ID as a string, int, or unsigned
         * int.
         */
        void SetId(
            const std::variant<std::string, long int, long unsigned int>& id) {
            std::visit(
                [this](const auto& value) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(value)>,
                                                 std::string>) {
                        if (value.empty()) {
                            logw("ID request is empty");
                            return;
                        }
                    }
                    (*this)["id"] = value;
                },
                id);
        }
    };
    virtual ~FamilyBookEventGetter() = default;
};
};  // namespace detail

class BookEventGetter : public BookEventGetterI {
    class ParserResponse {
        const common::TradingPairInfo& pair_info_;

      public:
        explicit ParserResponse(const common::TradingPairInfo& pair_info)
            : pair_info_(pair_info) {};
        Exchange::BookDiffSnapshot Parse(std::string_view response);
    };

  public:
    BookEventGetter(const common::TradingPairInfo& pair_info,
                    const DiffDepthStream::StreamIntervalI* interval,
                    TypeExchange type_exchange)
        : current_exchange_(exchange_.Get(type_exchange)),
          pair_info_(pair_info),
          interval_(interval),
          type_exchange_(type_exchange) {};
    void Get() override {};
    void LaunchOne() override { ioc.run_one(); };
    void Init(Exchange::BookDiffLFQueue& queue) override {
        std::function<void(boost::beast::flat_buffer & buffer)> OnMessageCB;
        OnMessageCB = [&queue, this](boost::beast::flat_buffer& buffer) {
            auto resut = boost::beast::buffers_to_string(buffer.data());
            ParserResponse parser(pair_info_);
            auto answer    = parser.Parse(resut);
            bool status_op = queue.try_enqueue(answer);
            if (!status_op) [[unlikely]]
                loge("my queuee is full. need clean my queue");
        };

        using dds = DiffDepthStream;
        dds channel(pair_info_, interval_);
        std::string empty_request = "{}";
        std::make_shared<WS>(ioc, empty_request, OnMessageCB)
            ->Run(current_exchange_->Host(), current_exchange_->Port(),
                  fmt::format("/ws/{0}", channel.ToString()));
    };

    ~BookEventGetter() override = default;

  private:
    boost::asio::io_context ioc;
    ExchangeChooser exchange_;
    https::ExchangeI* current_exchange_;
    const common::TradingPairInfo& pair_info_;
    const DiffDepthStream::StreamIntervalI* interval_;
    bool callback_execute_ = false;
    TypeExchange type_exchange_;
};

namespace detail {
/**
 * @brief for GET request
 *
 */
class FactoryRequest {
  public:
    explicit FactoryRequest(const https::ExchangeI* exchange,
                            std::string_view end_point, const ArgsQuery& args,
                            boost::beast::http::verb action, SignerI* signer,
                            bool need_sign = false)
        : exchange_(exchange),
          args_(args),
          action_(action),
          signer_(signer),
          need_sign_(need_sign) {
        if (need_sign) {
            AddSignParams();
            auto request_args = args_.QueryString();
            auto signature    = signer_->Sign(request_args);
            AddSignature(signature);
        }
        end_point_ = end_point.data() + args_.FinalQueryString();
    };
    boost::beast::http::request<boost::beast::http::string_body> operator()() {
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.version(11);
        req.method(action_);
        req.target(end_point_);
        /**
         * @brief API-keys are passed into the REST API via the X-MBX-APIKEY
         * header.
         *
         */
        if (need_sign_) req.insert("X-MBX-APIKEY", signer_->ApiKey().data());
        req.set(boost::beast::http::field::host, exchange_->Host().data());
        req.set(boost::beast::http::field::user_agent,
                BOOST_BEAST_VERSION_STRING);
        /**
         * @brief For POST, PUT, and DELETE endpoints, the parameters may be
         * sent as a query string or in the request body with content type
         * application/x-www-form-urlencoded. You may mix parameters between
         * both the query string and request body if you wish to do so.
         *
         */
        req.set(boost::beast::http::field::content_type,
                "application/x-www-form-urlencoded");
        return req;
    };
    std::string_view Host() const { return exchange_->Host(); };
    std::string_view Port() const { return exchange_->Port(); };
    std::string_view EndPoint() const { return end_point_; }

  private:
    void AddSignParams() {
        CurrentTime time_service;
        args_["recvWindow"] = std::to_string(exchange_->RecvWindow());
        args_["timestamp"]  = std::to_string(time_service.Time());
    };
    void AddSignature(std::string_view signature) {
        args_["signature"] = signature.data();
    };

  private:
    const https::ExchangeI* exchange_;
    std::string end_point_;
    ArgsQuery args_;
    boost::beast::http::verb action_;
    SignerI* signer_;
    bool need_sign_ = false;
};

class FactoryRequestJson {
  public:
    explicit FactoryRequestJson(const https::ExchangeI* exchange,
                                std::string_view end_point, ArgsBody& args,
                                boost::beast::http::verb action,
                                SignerI* signer, bool need_sign = false)
        : exchange_(exchange),
          end_point_(end_point.data()),
          args_(args),
          action_(action),
          signer_(signer),
          need_sign_(need_sign) {
        /**
         * @brief Construct a new assert object i don't debug this
         *
         */
        if (need_sign) {
            AddSignParams();
            auto request_args = args_.QueryString();
            auto signature    = signer_->Sign(request_args);
            AddSignature(signature);
        }
    };
    boost::beast::http::request<boost::beast::http::string_body> operator()() {
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.version(11);
        req.method(action_);
        req.target(end_point_);
        /**
         * @brief API-keys are passed into the REST API via the X-MBX-APIKEY
         * header.
         *
         */
        if (need_sign_) req.insert("X-MBX-APIKEY", signer_->ApiKey().data());
        req.set(boost::beast::http::field::host, exchange_->Host().data());
        req.set(boost::beast::http::field::user_agent,
                BOOST_BEAST_VERSION_STRING);
        /**
         * @brief For POST, PUT, and DELETE endpoints, the parameters may be
         * sent as a query string or in the request body with content type
         * application/x-www-form-urlencoded. You may mix parameters between
         * both the query string and request body if you wish to do so.
         *
         */
        if (action_ == boost::beast::http::verb::get) assert(false);

        if (action_ == boost::beast::http::verb::post ||
            action_ == boost::beast::http::verb::put ||
            action_ == boost::beast::http::verb::delete_)
            req.set(boost::beast::http::field::content_type,
                    "application/x-www-form-urlencoded");
        req.body() = args_.QueryString();
        req.prepare_payload();
        return req;
    };

    std::string_view Host() const { return exchange_->Host(); };
    std::string_view Port() const { return exchange_->Port(); };
    std::string_view EndPoint() const { return end_point_; }

  private:
    /**
     * @brief
     *
     * @param req
     * @return std::string return time which insert into headers
     */
    void AddSignParams() const {
        CurrentTime time_service;
        auto time           = std::to_string(time_service.Time());
        args_["timestamp"]  = time;
        args_["recvWindow"] = std::to_string(exchange_->RecvWindow());
    };
    /**
     * @brief
     * Append the sign parameter to request header, and
     * send the HTTP request. Note: the plain text for GET and POST requests is
     * different. Please refer to blew examples.
     *
     * @param signature
     * @param req
     */
    void AddSignature(std::string_view signature) {
        args_["signature"] = signature.data();
    };

  private:
    const https::ExchangeI* exchange_;
    std::string end_point_;
    ArgsBody& args_;
    boost::beast::http::verb action_;
    SignerI* signer_;
    bool need_sign_ = false;
};

class FamilyLimitOrder {
  public:
    virtual ~FamilyLimitOrder()                 = default;
    explicit FamilyLimitOrder()                 = default;
    static constexpr std::string_view end_point = "/api/v3/order";
    class ParserResponse {
      public:
        /**
         * @brief Construct a new Parser Response object
         *
         * @param pairs is needed to extract precission qty and price
         * @param pairs_reverse
         */
        explicit ParserResponse(
            common::TradingPairHashMap& pairs,
            common::TradingPairReverseHashMap& pairs_reverse)
            : pairs_(pairs), pairs_reverse_(pairs_reverse) {};
        Exchange::MEClientResponse Parse(std::string_view response);

      private:
        common::TradingPairHashMap& pairs_;
        common::TradingPairReverseHashMap& pairs_reverse_;
    };
    class ArgsOrder : public ArgsQuery {
      public:
        using SymbolType = std::string_view;
        explicit ArgsOrder(const Exchange::RequestNewOrder* new_order,
                           common::TradingPairHashMap& pairs)
            : ArgsQuery() {
            SetSymbol(pairs[new_order->trading_pair].https_query_request);
            SetSide(new_order->side);
            SetType(Type::LIMIT);
            auto qty_prec =
                std::pow(10, -pairs[new_order->trading_pair].qty_precission);
            SetQuantity(new_order->qty * qty_prec,
                        pairs[new_order->trading_pair].qty_precission);
            auto price_prec =
                std::pow(10, -pairs[new_order->trading_pair].price_precission);
            SetPrice(new_order->price * price_prec,
                     pairs[new_order->trading_pair].price_precission);
            SetTimeInForce(TimeInForce::GTC);
            SetOrderId(new_order->order_id);
        };

      private:
        void SetSymbol(SymbolType symbol) {
            storage["symbol"] = symbol.data();
        };
        void SetSide(common::Side side) {
            switch (side) {
                using enum common::Side;
                case BUY:
                    storage["side"] = "BUY";
                    break;
                case SELL:
                    storage["side"] = "SELL";
                    break;
                default:
                    loge("side is not SELL or BUY");
            }
        };
        void SetType(Type type) {
            switch (type) {
                using enum Type;
                case LIMIT:
                    storage["type"] = "LIMIT";
                    break;
                case MARKET:
                    storage["type"] = "MARKET";
                    break;
                case STOP_LOSS:
                    storage["type"] = "STOP_LOSS";
                    break;
                case STOP_LOSS_LIMIT:
                    storage["type"] = "STOP_LOSS_LIMIT";
                    break;
                case TAKE_PROFIT:
                    storage["type"] = "TAKE_PROFIT";
                    break;
                case TAKE_PROFIT_LIMIT:
                    storage["type"] = "TAKE_PROFIT_LIMIT";
                    break;
                case LIMIT_MAKER:
                    storage["type"] = "LIMIT_MAKER";
                    break;
            }
        };
        void SetQuantity(double quantity, uint8_t qty_prec) {
            storage["quantity"] = fmt::format("{:.{}f}", quantity, qty_prec);
        };
        void SetPrice(double price, uint8_t price_prec) {
            storage["price"] = fmt::format("{:.{}f}", price, price_prec);
        };
        void SetTimeInForce(TimeInForce time_in_force) {
            switch (time_in_force) {
                case TimeInForce::FOK:
                    storage["timeInForce"] = "FOK";
                    break;
                case TimeInForce::GTC:
                    storage["timeInForce"] = "GTC";
                    break;
                case TimeInForce::IOC:
                    storage["timeInForce"] = "IOC";
                    break;
                default:
                    storage["timeInForce"] = "FOK";
            }
        };
        void SetOrderId(common::OrderId order_id) {
            if (order_id != common::kOrderIdInvalid) [[likely]]
                storage["newClientOrderId"] = common::orderIdToString(order_id);
        };

      private:
        ArgsOrder& storage = *this;
    };
};
class FamilyCancelOrder {
  public:
    static constexpr std::string_view end_point = "/api/v3/order";
    explicit FamilyCancelOrder()                = default;
    virtual ~FamilyCancelOrder()                = default;

    class ParserResponse {
      public:
        explicit ParserResponse(
            common::TradingPairReverseHashMap& pairs_reverse)
            : pairs_reverse_(pairs_reverse) {};
        Exchange::MEClientResponse Parse(std::string_view response);

      private:
        common::TradingPairReverseHashMap& pairs_reverse_;
    };
    class ArgsOrder : public ArgsQuery {
      public:
        using SymbolType = std::string_view;
        /**
         * @brief Construct a new Args Order object. it is old ctor. don't use
         * it
         *
         * @param symbol
         * @param order_id
         */
        explicit ArgsOrder(SymbolType symbol, common::OrderId order_id)
            : ArgsQuery() {
            SetSymbol(symbol);
            SetOrderId(order_id);
        };
        explicit ArgsOrder(
            const Exchange::RequestCancelOrder* request_cancel_order,
            common::TradingPairHashMap& pairs)
            : ArgsQuery() {
            SetSymbol(
                pairs[request_cancel_order->trading_pair].https_query_request);
            SetOrderId(request_cancel_order->order_id);
        };

      private:
        void SetSymbol(SymbolType symbol) {
            storage["symbol"] = symbol.data();
        };
        void SetOrderId(common::OrderId order_id) {
            if (order_id != common::kOrderIdInvalid) [[likely]]
                storage["origClientOrderId"] =
                    common::orderIdToString(order_id);
        };

      private:
        ArgsOrder& storage = *this;
    };
};
};  // namespace detail

template <typename Executor>
class BookEventGetter2 : public detail::FamilyBookEventGetter,
                         public inner::BookEventGetterI {
    ::V2::ConnectionPool<WSSesionType2, const std::string_view&>* session_pool_;
    common::TradingPairHashMap& pairs_;

  protected:
    Executor executor_;

  public:
    BookEventGetter2(
        Executor&& executor,
        ::V2::ConnectionPool<WSSesionType2, const std::string_view&>*
            session_pool,
        TypeExchange type, common::TradingPairHashMap& pairs)
        : executor_(std::move(executor)),
          session_pool_(session_pool),
          pairs_(pairs) {
        switch (type) {
            case TypeExchange::MAINNET:
                current_exchange_ = &binance_main_net_;
                break;
            default:
                current_exchange_ = &binance_test_net_;
                break;
        }
    };
    ~BookEventGetter2() override = default;
    boost::asio::awaitable<void> CoExec(
        Exchange::BusEventRequestDiffOrderBook*
            bus_event_request_diff_order_book,
        const OnWssResponse* callback) override {
        co_await boost::asio::post(executor_, boost::asio::use_awaitable);
        if (bus_event_request_diff_order_book == nullptr) {
            loge("bus_event_request_diff_order_book == nullptr");
            co_return;
        }
        if (session_pool_ == nullptr) {
            loge("session_pool_ == nullptr");
            co_return;
        }
        boost::asio::co_spawn(
            executor_,
            [this, bus_event_request_diff_order_book,
             &callback]() -> boost::asio::awaitable<void> {
                logd("start book event getter for binance");

                detail::FamilyBookEventGetter::ArgsBody args(
                    bus_event_request_diff_order_book->WrappedEvent(), pairs_);
                logd("start prepare event getter for binance request");
                auto req = args.Body();
                logd("end prepare event getter for binance request");

                // bus_event_request_diff_order_book->Release();

                auto session = session_pool_->AcquireConnection();
                logd("start send event getter for binance request");

                if (auto status = co_await session->AsyncRequest(std::move(req),
                                                                 callback);
                    status == false)
                    loge("AsyncRequest finished unsuccessfully");
                // bus_event_request_diff_order_book->WrappedEvent()->Release();
                logd("end send event getter for binance request");
                co_return;
            },
            boost::asio::detached);
        co_return;
    }

  private:
    binance::testnet::HttpsExchange binance_test_net_;
    binance::mainnet::HttpsExchange binance_main_net_;

    https::ExchangeI* current_exchange_;
};

/**
 * @brief Template class for managing and handling book events asynchronously.
 *
 * @tparam Executor The executor type used for asynchronous operations.
 */
template <typename Executor>
class BookEventGetter3 : public detail::FamilyBookEventGetter,
                         public inner::BookEventGetterI {
    using CallbackMap =
        std::unordered_map<common::TradingPair, const OnWssResponse*,
                           common::TradingPairHash, common::TradingPairEqual>;
    using CloseSessionCallbackMap =
        std::unordered_map<common::TradingPair, const OnCloseSession*,
                           common::TradingPairHash, common::TradingPairEqual>;

    ::V2::ConnectionPool<WSSesionType3, const std::string_view&>* session_pool_;
    common::TradingPairHashMap& pairs_;
    CallbackMap callback_map_;
    CloseSessionCallbackMap callback_on_close_session_map_;
    std::atomic<WSSesionType3*> active_session_{nullptr};
    testnet::HttpsExchange binance_test_net_;
    mainnet::HttpsExchange binance_main_net_;
    https::ExchangeI* current_exchange_;

  protected:
    Executor& executor_;

  public:
    /**
     * @brief Constructor for BookEventGetter3.
     *
     * @param executor The executor for asynchronous operations.
     * @param session_pool Pointer to the WebSocket session pool.
     * @param type The type of exchange (mainnet or testnet).
     * @param pairs Reference to the trading pair hash map.
     */
    BookEventGetter3(
        Executor& executor,
        ::V2::ConnectionPool<WSSesionType3, const std::string_view&>*
            session_pool,
        TypeExchange type, common::TradingPairHashMap& pairs)
        : session_pool_(session_pool),
          pairs_(pairs),
          executor_(executor),
          current_exchange_(GetExchange(type)) {}
    /**
     * @brief Default destructor.
     */
    ~BookEventGetter3() override = default;
    /**
     * @brief Asynchronously handles book events.
     *
     * @param bus_event_request_diff_order_book Pointer to the event request for
     * the order book.
     */
    boost::asio::awaitable<void> CoExec(
        boost::intrusive_ptr<Exchange::BusEventRequestDiffOrderBook>
            bus_event_request_diff_order_book) override {
        co_await boost::asio::post(executor_, boost::asio::use_awaitable);

        if (!bus_event_request_diff_order_book || !session_pool_) {
            loge("Invalid bus_event_request_diff_order_book or session_pool");
            co_return;
        }

        co_await HandleBookEvent(bus_event_request_diff_order_book);
    }
    /**
     * @brief Registers a callback for a specific trading pair's WebSocket
     * response.
     *
     * @param trading_pair The trading pair to register the callback for.
     * @param callback Pointer to the callback function.
     */
    void RegisterCallback(common::TradingPair trading_pair,
                          const OnWssResponse* callback) {
        callback_map_[trading_pair] = callback;
    }
    /**
     * @brief Registers a callback for a specific trading pair when a session is
     * closed.
     *
     * @param trading_pair The trading pair to register the callback for.
     * @param callback Pointer to the close session callback function.
     */
    void RegisterCallbackOnCloseSession(common::TradingPair trading_pair,
                                        const OnCloseSession* callback) {
        callback_on_close_session_map_[trading_pair] = callback;
    }

    /**
     * @brief Asynchronously stops the active WebSocket session gracefully.
     */
    void AsyncStop() {
        if (auto session = active_session_.load()) {
            session->AsyncCloseSessionGracefully();
        } else {
            logw("No active session to stop");
        }
    }

  private:
    /**
     * @brief Returns the appropriate exchange object based on the type of
     * exchange.
     *
     * @param type The type of exchange (mainnet or testnet).
     * @return Pointer to the selected exchange object.
     */
    https::ExchangeI* GetExchange(TypeExchange type) {
        return type == TypeExchange::MAINNET
                   ? static_cast<https::ExchangeI*>(&binance_main_net_)
                   : static_cast<https::ExchangeI*>(&binance_test_net_);
    }

    /**
     * @brief Handles book events by processing the provided event request.
     *
     * @param bus_event_request_diff_order_book Pointer to the event request for
     * the order book.
     */
    boost::asio::awaitable<void> HandleBookEvent(
        boost::intrusive_ptr<Exchange::BusEventRequestDiffOrderBook>
            bus_event_request_diff_order_book) {
        auto* wrapped_event = bus_event_request_diff_order_book->WrappedEvent();
        if (!wrapped_event) {
            loge("Wrapped event is null");
            co_return;
        }

        auto& trading_pair = wrapped_event->trading_pair;
        detail::FamilyBookEventGetter::ArgsBody args(
            bus_event_request_diff_order_book->WrappedEvent(), pairs_);
        auto req = args.Body();

        if (AcquireActiveSession()) {
            if (!RegisterCallbacksForTradingPair(trading_pair)) {
                co_return;
            }
        }

        if (auto result = co_await SendAsyncRequest(req); !result) {
            loge("AsyncRequest finished unsuccessfully");
        }

        logd("Finished sending event getter for Binance request");
    }

    /**
     * @brief Acquires an active session from the session pool.
     *
     * @return True if a session was successfully acquired, otherwise false.
     */
    bool AcquireActiveSession() {
        WSSesionType3* expected = nullptr;
        auto session            = session_pool_->AcquireConnection();
        if (active_session_.compare_exchange_strong(expected, session)) {
            logd("Active session acquired");
            return true;
        }
        return false;
    }
    /**
     * @brief Registers callbacks for a specific trading pair.
     *
     * @param trading_pair The trading pair to register callbacks for.
     * @return True if registration was successful, otherwise false.
     */
    bool RegisterCallbacksForTradingPair(
        const common::TradingPair& trading_pair) {
        if (auto callback = FindCallback(callback_map_, trading_pair)) {
            RegisterCallbackOnSession(callback);
        } else {
            loge("No callback on response registered for trading pair: {}",
                 trading_pair.ToString());
            return false;
        }

        if (auto callback =
                FindCallback(callback_on_close_session_map_, trading_pair)) {
            RegisterCallbackOnSessionClose(callback);
        } else {
            logw("No callback on close session registered for trading pair: {}",
                 trading_pair.ToString());
        }

        RegisterDefaultCallbackOnSessionClose();
        return true;
    }
    /**
     * @brief Finds a callback in the specified map for a given trading pair.
     *
     * @tparam MapType The type of the callback map.
     * @param map The map to search for the callback.
     * @param trading_pair The trading pair to search for.
     * @return Pointer to the callback if found, otherwise nullptr.
     */
    template <typename MapType>
    typename MapType::mapped_type FindCallback(
        const MapType& map, const common::TradingPair& trading_pair) const {
        auto it = map.find(trading_pair);
        return it != map.end() ? it->second : nullptr;
    }
    /**
     * @brief Registers a response callback on the active session.
     *
     * @param callback The callback to register.
     */
    void RegisterCallbackOnSession(const OnWssResponse* callback) {
        if (auto session = active_session_.load()) {
            session->RegisterCallbackOnResponse(*callback);
        }
    }
    /**
     * @brief Registers a close session callback on the active session.
     *
     * @param callback The callback to register.
     */
    void RegisterCallbackOnSessionClose(const OnCloseSession* callback) {
        if (auto session = active_session_.load()) {
            session->RegisterCallbackOnCloseSession(*callback);
        }
    }
    /**
     * @brief Registers the default callback to execute when a session is
     * closed.
     */
    void RegisterDefaultCallbackOnSessionClose() {
        if (auto session = active_session_.load()) {
            session->RegisterCallbackOnCloseSession(
                [this]() { DefaultCBOnCloseSession(); });
        }
    }

    /**
     * @brief Sends an asynchronous request using the active session.
     *
     * @tparam RequestType The type of the request.
     * @param req The request to send.
     * @return True if the request was sent successfully, otherwise false.
     */
    boost::asio::awaitable<bool> SendAsyncRequest(auto&& req) {
        if (auto session = active_session_.load()) {
            co_return co_await session->AsyncRequest(std::move(req));
        }
        co_return false;
    }

    /**
     * @brief Default callback executed when a session is closed.
     */
    void DefaultCBOnCloseSession() {
        active_session_.store(nullptr, std::memory_order_release);
    }
};

template <typename Executor>
class BookEventGetterComponent : public bus::Component,
                                 public BookEventGetter3<Executor> {
    static constexpr std::string_view name_component_ =
        "binance::BookEventGetterComponent";

  public:
    common::MemoryPool<Exchange::BookDiffSnapshot2> book_diff_mem_pool_;
    common::MemoryPool<Exchange::BusEventBookDiffSnapshot>
        bus_event_book_diff_snapshot_mem_pool_;
    explicit BookEventGetterComponent(
        Executor& executor, size_t number_responses, TypeExchange type,
        common::TradingPairHashMap& pairs,
        ::V2::ConnectionPool<WSSesionType3, const std::string_view&>*
            session_pool)
        : BookEventGetter3<Executor>(executor, session_pool, type, pairs),
          book_diff_mem_pool_(number_responses),
          bus_event_book_diff_snapshot_mem_pool_(number_responses) {}
    ~BookEventGetterComponent() override = default;

    void AsyncHandleEvent(
        boost::intrusive_ptr<Exchange::BusEventRequestDiffOrderBook> event)
        override {
        boost::asio::co_spawn(BookEventGetter3<Executor>::executor_,
                              BookEventGetter3<Executor>::CoExec(event),
                              boost::asio::detached);
    };
    void AsyncStop() override { BookEventGetter3<Executor>::AsyncStop(); }
};

class OrderNewLimit : public inner::OrderNewI, public detail::FamilyLimitOrder {
  public:
    explicit OrderNewLimit(SignerI* signer, TypeExchange type,
                           common::TradingPairHashMap& pairs,
                           common::TradingPairReverseHashMap& pairs_reverse)
        : current_exchange_(exchange_.Get(type)),
          signer_(signer),
          pairs_(pairs),
          pairs_reverse_(pairs_reverse) {};
    void Exec(Exchange::RequestNewOrder* new_order,
              Exchange::ClientResponseLFQueue* response_lfqueue) override {
        ArgsOrder args(new_order, pairs_);

        bool need_sign = true;
        detail::FactoryRequest factory{current_exchange_,
                                       detail::FamilyLimitOrder::end_point,
                                       args,
                                       boost::beast::http::verb::post,
                                       signer_,
                                       need_sign};
        boost::asio::io_context ioc;
        OnHttpsResponce cb;
        cb = [response_lfqueue, this](
                 boost::beast::http::response<boost::beast::http::string_body>&
                     buffer) {
            const auto& resut = buffer.body();
            logi("{}", resut);
            ParserResponse parser(pairs_, pairs_reverse_);
            auto answer    = parser.Parse(resut);
            bool status_op = response_lfqueue->try_enqueue(answer);
            if (!status_op) [[unlikely]]
                loge("my queuee is full. need clean my queue");
        };
        logi("init memory start");
        Https http_session(ioc, cb);
        http_session.Run(factory.Host().data(), factory.Port().data(),
                         factory.EndPoint().data(), factory());
        logi("init memory finished");
        ioc.run();
        logi("go out from exec");
    };
    ~OrderNewLimit() override = default;

  private:
    ExchangeChooser exchange_;
    https::ExchangeI* current_exchange_;
    SignerI* signer_;
    common::TradingPairHashMap& pairs_;
    common::TradingPairReverseHashMap& pairs_reverse_;
};

class OrderNewLimit2 : public inner::OrderNewI,
                       public detail::FamilyLimitOrder {
  public:
    explicit OrderNewLimit2(SignerI* signer, TypeExchange type,
                            common::TradingPairHashMap& pairs,
                            common::TradingPairReverseHashMap& pairs_reverse,
                            ::V2::ConnectionPool<HTTPSesionType>* session_pool)
        : current_exchange_(exchange_.Get(type)),
          signer_(signer),
          pairs_(pairs),
          pairs_reverse_(pairs_reverse),
          session_pool_(session_pool) {};
    void Exec(Exchange::RequestNewOrder* new_order,
              Exchange::ClientResponseLFQueue* response_lfqueue) override {
        if (response_lfqueue == nullptr) {
            loge("response_lfqueue == nullptr");
            return;
        }
        if (session_pool_ == nullptr) {
            loge("session_pool_ == nullptr");
            return;
        }
        logd("start exec");
        ArgsOrder args(new_order, pairs_);

        bool need_sign = true;
        detail::FactoryRequest factory{current_exchange_,
                                       detail::FamilyLimitOrder::end_point,
                                       args,
                                       boost::beast::http::verb::post,
                                       signer_,
                                       need_sign};
        logd("start prepare new limit order request");
        auto req = factory();
        logd("end prepare new limit order request");

        auto cb =
            [response_lfqueue, this](
                boost::beast::http::response<boost::beast::http::string_body>&
                    buffer) {
                const auto& resut = buffer.body();
                logi("{}", resut);
                ParserResponse parser(pairs_, pairs_reverse_);
                auto answer    = parser.Parse(resut);
                bool status_op = response_lfqueue->try_enqueue(answer);
                if (!status_op) [[unlikely]]
                    loge("my queuee is full. need clean my queue");
            };
        auto session = session_pool_->AcquireConnection();
        logd("start send new limit order request");

        if (auto status = session->AsyncRequest(std::move(req), cb);
            status == false)
            loge("AsyncRequest wasn't sent in io_context");

        logd("end send new limit order request");
    };
    ~OrderNewLimit2() override = default;

  private:
    ExchangeChooser exchange_;
    // pass pairs_ without const due to i want [] operator
    // pass pairs_reverse_ without const due to i want [] operator
    common::TradingPairReverseHashMap& pairs_reverse_;

  protected:
    SignerI* signer_;
    common::TradingPairHashMap& pairs_;
    https::ExchangeI* current_exchange_;
    ::V2::ConnectionPool<HTTPSesionType>* session_pool_;
};

/**
 * @brief OrderNewLimit3 is wrapper above OrderNewLimit2 with coroutine CoExec
 *
 * @tparam Executor
 */
template <typename Executor>
class OrderNewLimit3 : public OrderNewLimit2 {
  protected:
    Executor executor_;

  public:
    explicit OrderNewLimit3(Executor&& executor, SignerI* signer,
                            TypeExchange type,
                            common::TradingPairHashMap& pairs,
                            common::TradingPairReverseHashMap& pairs_reverse,
                            ::V2::ConnectionPool<HTTPSesionType>* session_pool)
        : executor_(std::move(executor)),
          OrderNewLimit2(signer, type, pairs, pairs_reverse, session_pool) {};

    ~OrderNewLimit3() override = default;
    boost::asio::awaitable<void> CoExec(
        Exchange::BusEventRequestNewLimitOrder* bus_event_request_new_order,
        const OnHttpsResponce& callback) override {
        co_await boost::asio::post(executor_, boost::asio::use_awaitable);
        if (bus_event_request_new_order == nullptr) {
            loge("bus_event_request_new_order = nullptr");
            co_return;
        }
        if (session_pool_ == nullptr) {
            loge("session_pool_ == nullptr");
            co_return;
        }
        boost::asio::co_spawn(
            executor_,
            [this, bus_event_request_new_order,
             &callback]() -> boost::asio::awaitable<void> {
                logd("Start CoExec");

                ArgsOrder args(bus_event_request_new_order->request, pairs_);

                bool need_sign = true;
                detail::FactoryRequest factory{
                    current_exchange_,
                    detail::FamilyLimitOrder::end_point,
                    args,
                    boost::beast::http::verb::post,
                    signer_,
                    need_sign};

                logd("Start preparing new limit order request");
                auto req = factory();
                logd("End preparing new limit order request");
                bus_event_request_new_order->Release();
                auto session = session_pool_->AcquireConnection();
                logd("Start sending new limit order request");

                if (auto status =
                        session->AsyncRequest(std::move(req), callback);
                    status == false) {
                    loge("AsyncRequest wasn't sent in io_context");
                }

                logd("End sending new limit order request");
                co_return;  // Exit the coroutine
            },
            boost::asio::detached);  // Detach to avoid awaiting here
        co_return;
    }
};

template <typename Executor>
class OrderNewLimitComponent : public bus::Component,
                               public OrderNewLimit3<Executor> {
  public:
    common::MemoryPool<Exchange::MEClientResponse> exchange_response_mem_pool_;
    common::MemoryPool<Exchange::BusEventResponse> bus_event_response_mem_pool_;
    explicit OrderNewLimitComponent(
        Executor&& executor, size_t number_responses, SignerI* signer,
        TypeExchange type, common::TradingPairHashMap& pairs,
        common::TradingPairReverseHashMap& pairs_reverse,
        ::V2::ConnectionPool<HTTPSesionType>* session_pool)
        : OrderNewLimit3<Executor>(std::move(executor), signer, type, pairs,
                                   pairs_reverse, session_pool),
          exchange_response_mem_pool_(number_responses),
          bus_event_response_mem_pool_(number_responses) {}
    ~OrderNewLimitComponent() override = default;

    void AsyncHandleEvent(Exchange::BusEventRequestNewLimitOrder* event,
                          const OnHttpsResponce& cb) override {
        boost::asio::co_spawn(OrderNewLimit3<Executor>::executor_,
                              OrderNewLimit3<Executor>::CoExec(event, cb),
                              boost::asio::detached);
    };
};

class CancelOrder : public inner::CancelOrderI,
                    public detail::FamilyCancelOrder {
  public:
    explicit CancelOrder(SignerI* signer, TypeExchange type,
                         common::TradingPairHashMap& pairs)
        : current_exchange_(exchange_.Get(type)),
          signer_(signer),
          pairs_(pairs) {};
    void Exec(Exchange::RequestCancelOrder* request_cancel_order,
              Exchange::ClientResponseLFQueue* response_lfqueue) override {
        ArgsOrder args(request_cancel_order, pairs_);

        bool need_sign = true;
        detail::FactoryRequest factory{current_exchange_,
                                       detail::FamilyCancelOrder::end_point,
                                       args,
                                       boost::beast::http::verb::delete_,
                                       signer_,
                                       need_sign};
        boost::asio::io_context ioc;
        OnHttpsResponce cb;
        cb = [response_lfqueue, this](
                 boost::beast::http::response<boost::beast::http::string_body>&
                     buffer) {
            const auto& resut = buffer.body();
            logi("{}", resut);
            ParserResponse parser(pairs_reverse_);
            auto answer    = parser.Parse(resut);
            bool status_op = response_lfqueue->try_enqueue(answer);
            if (!status_op) [[unlikely]]
                loge("my queue is full. need clean my queue");
        };
        std::make_shared<Https>(ioc, cb)->Run(
            factory.Host().data(), factory.Port().data(),
            factory.EndPoint().data(), factory());
        ioc.run();
    };
    ~CancelOrder() override = default;

  private:
    ExchangeChooser exchange_;
    https::ExchangeI* current_exchange_;
    SignerI* signer_;
    common::TradingPairHashMap& pairs_;
    common::TradingPairReverseHashMap pairs_reverse_;
};

class CancelOrder2 : public inner::CancelOrderI,
                     public detail::FamilyCancelOrder {
  public:
    explicit CancelOrder2(SignerI* signer, TypeExchange type,
                          common::TradingPairHashMap& pairs,
                          common::TradingPairReverseHashMap& pairs_reverse,
                          ::V2::ConnectionPool<HTTPSesionType>* session_pool)
        : current_exchange_(exchange_.Get(type)),
          signer_(signer),
          pairs_(pairs),
          pairs_reverse_(pairs_reverse),
          session_pool_(session_pool) {};
    void Exec(Exchange::RequestCancelOrder* request_cancel_order,
              Exchange::ClientResponseLFQueue* response_lfqueue) override {
        if (response_lfqueue == nullptr) {
            loge("response_lfqueue == nullptr");
            return;
        }
        if (session_pool_ == nullptr) {
            loge("session_pool_ == nullptr");
            return;
        }
        logd("start exec");
        ArgsOrder args(request_cancel_order, pairs_);
        bool need_sign = true;
        detail::FactoryRequest factory{current_exchange_,
                                       detail::FamilyCancelOrder::end_point,
                                       args,
                                       boost::beast::http::verb::delete_,
                                       signer_,
                                       need_sign};
        logd("start prepare cancel request");
        auto req = factory();
        logd("end prepare cancel request");

        auto cb =
            [response_lfqueue, this](
                boost::beast::http::response<boost::beast::http::string_body>&
                    buffer) {
                const auto& resut = buffer.body();
                logi("{}", resut);
                ParserResponse parser(pairs_reverse_);
                auto answer    = parser.Parse(resut);
                bool status_op = response_lfqueue->try_enqueue(answer);
                if (!status_op) [[unlikely]]
                    loge("my queue is full. need clean my queue");
            };

        auto session = session_pool_->AcquireConnection();
        logd("start send cancel request");

        if (auto status = session->AsyncRequest(std::move(req), cb);
            status == false)
            loge("AsyncRequest wasn't sent in io_context");

        logd("end send cancel request");
        // session_pool_->ReleaseConnection(session);
    };
    ~CancelOrder2() override = default;

  private:
    ExchangeChooser exchange_;
    common::TradingPairReverseHashMap& pairs_reverse_;

  protected:
    SignerI* signer_;
    common::TradingPairHashMap& pairs_;
    https::ExchangeI* current_exchange_;
    ::V2::ConnectionPool<HTTPSesionType>* session_pool_;
};

/**
 * @brief CancelOrder3 is wrapper above CancelOrder2 with coroutine CoExec
 *
 * @tparam Executor
 */
template <typename Executor>
class CancelOrder3 : public CancelOrder2 {
  protected:
    Executor executor_;

  public:
    explicit CancelOrder3(Executor&& executor, SignerI* signer,
                          TypeExchange type, common::TradingPairHashMap& pairs,
                          common::TradingPairReverseHashMap& pairs_reverse,
                          ::V2::ConnectionPool<HTTPSesionType>* session_pool)
        : executor_(std::move(executor)),
          CancelOrder2(signer, type, pairs, pairs_reverse, session_pool) {};
    boost::asio::awaitable<void> CoExec(
        Exchange::BusEventRequestCancelOrder* bus_event_request_cancel_order,
        const OnHttpsResponce& callback) override {
        co_await boost::asio::post(executor_, boost::asio::use_awaitable);
        if (bus_event_request_cancel_order == nullptr) {
            loge("bus_event_request_cancel_order == nullptr");
            co_return;
        }
        if (session_pool_ == nullptr) {
            loge("session_pool_ == nullptr");
            co_return;
        }
        boost::asio::co_spawn(
            executor_,
            [this, bus_event_request_cancel_order,
             &callback]() -> boost::asio::awaitable<void> {
                logd("start exec");

                ArgsOrder args(bus_event_request_cancel_order->request, pairs_);
                bool need_sign = true;
                detail::FactoryRequest factory{
                    current_exchange_,
                    detail::FamilyCancelOrder::end_point,
                    args,
                    boost::beast::http::verb::delete_,
                    signer_,
                    need_sign};
                logd("start prepare cancel request");
                auto req = factory();
                logd("end prepare cancel request");

                bus_event_request_cancel_order->Release();

                auto session = session_pool_->AcquireConnection();
                logd("start send cancel request");

                if (auto status =
                        session->AsyncRequest(std::move(req), callback);
                    status == false)
                    loge("AsyncRequest wasn't sent in io_context");

                logd("end send cancel request");
                co_return;
            },
            boost::asio::detached);
        co_return;
    };
    ~CancelOrder3() override = default;
};

template <typename Executor>
class CancelOrderComponent : public bus::Component,
                             public CancelOrder3<Executor> {
  public:
    common::MemoryPool<Exchange::MEClientResponse> exchange_response_mem_pool_;
    common::MemoryPool<Exchange::BusEventResponse> bus_event_response_mem_pool_;
    explicit CancelOrderComponent(
        Executor&& executor, size_t number_responses, SignerI* signer,
        TypeExchange type, common::TradingPairHashMap& pairs,
        common::TradingPairReverseHashMap& pairs_reverse,
        ::V2::ConnectionPool<HTTPSesionType>* session_pool)
        : CancelOrder3<Executor>(std::move(executor), signer, type, pairs,
                                 pairs_reverse, session_pool),
          exchange_response_mem_pool_(number_responses),
          bus_event_response_mem_pool_(number_responses) {}
    ~CancelOrderComponent() override = default;

    void AsyncHandleEvent(Exchange::BusEventRequestCancelOrder* event,
                          const OnHttpsResponce& cb) override {
        boost::asio::co_spawn(CancelOrder3<Executor>::executor_,
                              CancelOrder3<Executor>::CoExec(event, cb),
                              boost::asio::detached);
    };
};

namespace detail {
class FamilyBookSnapshot {
  public:
    static constexpr std::string_view end_point = "/api/v3/depth";
    class ParserResponse {
        const common::TradingPairInfo& pair_info_;

      public:
        explicit ParserResponse(const common::TradingPairInfo& pair_info)
            : pair_info_(pair_info) {};
        Exchange::BookSnapshot Parse(std::string_view response);
    };

    class ArgsOrder : public ArgsQuery {
      public:
        using SymbolType = std::string_view;
        using Limit      = uint64_t;
        explicit ArgsOrder(SymbolType ticker1, SymbolType ticker2, Limit limit)
            : ArgsQuery() {
            SetSymbol(ticker1, ticker2);
            SetLimit(limit);
        };
        explicit ArgsOrder(const Exchange::RequestSnapshot* request_snapshot,
                           common::TradingPairHashMap& pairs)
            : ArgsQuery() {
            SetSymbol(
                pairs[request_snapshot->trading_pair].https_query_request);
            // SetLimit(request_cancel_order->depth);
        };
        void SetSymbol(SymbolType symbol) {
            storage["symbol"] = symbol.data();
        };
        void SetSymbol(SymbolType ticker1, SymbolType ticker2) {
            SymbolUpperCase formatter(ticker1, ticker2);
            storage["symbol"] = formatter.ToString().data();
        };
        void SetLimit(Limit limit) {
            /**
             * @brief If limit > 5000, then the response will truncate to 5000.
             * https://binance-docs.github.io/apidocs/spot/en/#exchange-information
             */
            limit            = (limit > 5000) ? 5000 : limit;

            storage["limit"] = std::to_string(limit);
        };

      private:
        ArgsOrder& storage = *this;
    };
    class ArgsBody : public binance::ArgsBody {
        common::TradingPairHashMap& pairs_;
        /**
         * @brief id_ need add when send json request to exchange
         *
         */
        unsigned int id_;

      public:
        ArgsBody(const Exchange::RequestSnapshot* request_snapshot,
                 common::TradingPairHashMap& pairs)
            : binance::ArgsBody(), pairs_(pairs) {
            SetSymbol(
                pairs[request_snapshot->trading_pair].https_query_request);
            // SetLimit(request_cancel_order->depth);
        }

      private:
        void SetSymbol(std::string_view trading_pair) {
            storage["symbol"] = trading_pair.data();
        };
        void SetLimit(unsigned int limit) {
            /**
             * @brief If limit > 5000, then the response will truncate to 5000.
             * https://binance-docs.github.io/apidocs/spot/en/#exchange-information
             */
            limit            = (limit > 5000) ? 5000 : limit;

            storage["limit"] = std::to_string(limit);
        };

      private:
        ArgsBody& storage = *this;
    };
};
};  // namespace detail

class BookSnapshot : public inner::BookSnapshotI,
                     public detail::FamilyBookSnapshot {
  public:
    explicit BookSnapshot(detail::FamilyBookSnapshot::ArgsOrder&& args,
                          TypeExchange type, Exchange::BookSnapshot* snapshot,
                          const common::TradingPairInfo& pair_info)
        : args_(std::move(args)), snapshot_(snapshot), pair_info_(pair_info) {
        switch (type) {
            case TypeExchange::MAINNET:
                current_exchange_ = &binance_main_net_;
                break;
            default:
                current_exchange_ = &binance_test_net_;
                break;
        }
    };
    void Exec() override {
        bool need_sign = false;
        detail::FactoryRequest factory{current_exchange_,
                                       detail::FamilyBookSnapshot::end_point,
                                       args_,
                                       boost::beast::http::verb::get,
                                       signer_,
                                       need_sign};
        boost::asio::io_context ioc;
        OnHttpsResponce cb;
        cb = [this](
                 boost::beast::http::response<boost::beast::http::string_body>&
                     buffer) {
            const auto& resut = buffer.body();
            detail::FamilyBookSnapshot::ParserResponse parser(pair_info_);
            auto answer = parser.Parse(resut);
            // answer.ticker = args_["symbol"];
            //*snapshot_  = std::move(answer);
            assert(false);
        };
        std::make_shared<Https>(ioc, cb)->Run(
            factory.Host().data(), factory.Port().data(),
            factory.EndPoint().data(), factory());
        ioc.run();
    };

  private:
    detail::FamilyBookSnapshot::ArgsOrder args_;
    binance::testnet::HttpsExchange binance_test_net_;
    binance::mainnet::HttpsExchange binance_main_net_;

    https::ExchangeI* current_exchange_;
    SignerI* signer_ = nullptr;
    Exchange::BookSnapshot* snapshot_;
    const common::TradingPairInfo& pair_info_;
};

template <typename Executor>
class BookSnapshot2 : public inner::BookSnapshotI {
    SignerI* signer_ = nullptr;
    ::V2::ConnectionPool<HTTPSesionType3>* session_pool_;
    common::TradingPairHashMap& pairs_;
    // Add a callback map to store parsing callbacks for each trading pair
    std::unordered_map<common::TradingPair, const OnHttpsResponce*,
                       common::TradingPairHash, common::TradingPairEqual>
        callback_map_;

  protected:
    Executor executor_;

  public:
    explicit BookSnapshot2(Executor&& executor, SignerI* signer,
                           ::V2::ConnectionPool<HTTPSesionType3>* session_pool,
                           TypeExchange type, common::TradingPairHashMap& pairs)
        : executor_(std::move(executor)),
          signer_(signer),
          session_pool_(session_pool),
          pairs_(pairs) {
        switch (type) {
            case TypeExchange::MAINNET:
                current_exchange_ = &binance_main_net_;
                break;
            default:
                current_exchange_ = &binance_test_net_;
                break;
        }
    };

    ~BookSnapshot2() override = default;
    boost::asio::awaitable<void> CoExec(
        Exchange::BusEventRequestNewSnapshot* bus_event_request_new_snapshot)
        override {
        co_await boost::asio::post(executor_, boost::asio::use_awaitable);
        if (bus_event_request_new_snapshot == nullptr) {
            loge("bus_event_request_new_snapshot == nullptr");
            co_return;
        }
        if (session_pool_ == nullptr) {
            loge("session_pool_ == nullptr");
            co_return;
        }
        boost::asio::co_spawn(
            executor_,
            [this,
             bus_event_request_new_snapshot]() -> boost::asio::awaitable<void> {
                if (bus_event_request_new_snapshot == nullptr) {
                    loge("bus_event_request_new_snapshot = nulptr");
                    co_return;
                }
                auto& trading_pair =
                    bus_event_request_new_snapshot->WrappedEvent()
                        ->trading_pair;
                auto callback_it = callback_map_.find(trading_pair);
                if (callback_it == callback_map_.end()) {
                    loge("No callback registered for trading pair: {}",
                         trading_pair.ToString());
                    co_return;
                }
                auto& callback = callback_it->second;
                logd("start book snapshot exec");
                /**
                 * @brief
                 * https://binance-docs.github.io/apidocs/spot/en/#general-api-information:~:text=For%20GET%20endpoints%2C%20parameters%20must%20be%20sent%20as%20a%20query%20string.
                 * For GET endpoints, parameters must be sent as a query string.
                 */
                detail::FamilyBookSnapshot::ArgsOrder args(
                    bus_event_request_new_snapshot->WrappedEvent(), pairs_);
                bool need_sign = false;
                detail::FactoryRequest factory{
                    current_exchange_,
                    detail::FamilyBookSnapshot::end_point,
                    args,
                    boost::beast::http::verb::get,
                    signer_,
                    need_sign};
                logd("start prepare new snapshot request");
                auto req = factory();
                logd("end prepare new snapshot request");

                // bus_event_request_new_snapshot->Release();
                // bus_event_request_new_snapshot->WrappedEvent()->Release();
                auto session = session_pool_->AcquireConnection();
                logd("start send new snapshot request");

                if (auto status = co_await session->AsyncRequest(std::move(req),
                                                                 callback);
                    status == false)
                    loge("AsyncRequest wasn't sent in io_context");

                logd("end send new snapshot request");
                co_return;
            },
            boost::asio::detached);
        co_return;
    }
    /**
     * @brief Add a function to register callbacks for trading pairs
     *
     * @param trading_pair
     * @param callback
     */
    void RegisterCallback(common::TradingPair trading_pair,
                          const OnHttpsResponce* callback) {
        callback_map_[trading_pair] = callback;
    }

  private:
    binance::testnet::HttpsExchange binance_test_net_;
    binance::mainnet::HttpsExchange binance_main_net_;

    https::ExchangeI* current_exchange_;
};

template <typename Executor>
class BookSnapshotComponent : public bus::Component,
                              public BookSnapshot2<Executor> {
  public:
    common::MemoryPool<Exchange::BookSnapshot2> snapshot_mem_pool_;
    common::MemoryPool<Exchange::BusEventResponseNewSnapshot>
        bus_event_response_snapshot_mem_pool_;
    explicit BookSnapshotComponent(
        Executor&& executor, size_t number_responses, SignerI* signer,
        TypeExchange type, common::TradingPairHashMap& pairs,
        common::TradingPairReverseHashMap& pairs_reverse,
        ::V2::ConnectionPool<HTTPSesionType3>* session_pool)
        : BookSnapshot2<Executor>(std::move(executor), signer, session_pool,
                                  type, pairs),
          snapshot_mem_pool_(number_responses),
          bus_event_response_snapshot_mem_pool_(number_responses) {}
    ~BookSnapshotComponent() override = default;

    void AsyncHandleEvent(
        Exchange::BusEventRequestNewSnapshot* event) override {
        boost::asio::co_spawn(BookSnapshot2<Executor>::executor_,
                              BookSnapshot2<Executor>::CoExec(event),
                              boost::asio::detached);
    };
    void AsyncStop() override {
        // TODO: need impl Async Stop for session
        //  net::steady_timer delay_timer(BookSnapshot2<Executor>::executor_,
        //                                std::chrono::nanoseconds(1));
        //  delay_timer.async_wait([&](const boost::system::error_code&) {
        //      BookSnapshot2<Executor>::signal_.emit(
        //          boost::asio::cancellation_type::all);
        //  });
    }
};

/**
 * @brief generator service new bid ask from exchange
 * the main aim of this class is pack new bids and asks to NewBidLFQueue and
 * NewAskLFQueue
 *
 */
class GeneratorBidAskService {
  public:
    explicit GeneratorBidAskService(
        Exchange::EventLFQueue* event_lfqueue,
        prometheus::EventLFQueue* prometheus_event_lfqueue,
        const common::TradingPairInfo& trading_pair_info,
        common::TickerHashMap& ticker_hash_map,
        common::TradingPair trading_pair,
        const DiffDepthStream::StreamIntervalI* interval, TypeExchange type);
    auto Start() {
        run_    = true;
        thread_ = std::unique_ptr<std::thread>(common::createAndStartThread(
            -1, "Trading/GeneratorBidAskService", [this]() { Run(); }));
        ASSERT(thread_ != nullptr, "Failed to start MarketData thread.");
    };
    common::Delta GetDownTimeInS() const { return time_manager_.GetDeltaInS(); }
    ~GeneratorBidAskService() {
        Stop();
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(2s);
        if (thread_ && thread_->joinable()) [[likely]]
            thread_->join();
    }
    auto Stop() -> void { run_ = false; }

    GeneratorBidAskService()                                          = delete;

    GeneratorBidAskService(const GeneratorBidAskService&)             = delete;

    GeneratorBidAskService(const GeneratorBidAskService&&)            = delete;

    GeneratorBidAskService& operator=(const GeneratorBidAskService&)  = delete;

    GeneratorBidAskService& operator=(const GeneratorBidAskService&&) = delete;

  private:
    std::unique_ptr<std::thread> thread_;

    volatile bool run_                                  = false;

    Exchange::EventLFQueue* event_lfqueue_              = nullptr;
    prometheus::EventLFQueue* prometheus_event_lfqueue_ = nullptr;
    common::TimeManager time_manager_;
    size_t next_inc_seq_num_                             = 0;
    std::unique_ptr<BookEventGetterI> book_event_getter_ = nullptr;
    Exchange::BookDiffLFQueue book_diff_lfqueue_;
    Exchange::BookSnapshot snapshot_;
    const common::TradingPairInfo& pair_info_;
    common::TickerHashMap& ticker_hash_map_;
    common::TradingPair trading_pair_;
    const DiffDepthStream::StreamIntervalI* interval_;
    uint64_t last_id_diff_book_event;
    TypeExchange type_exchange_;

    binance::testnet::HttpsExchange binance_test_net_;
    binance::mainnet::HttpsExchange binance_main_net_;
    https::ExchangeI* current_exchange_;

  private:
    auto Run() noexcept -> void;
};

template <typename ThreadPool>
class BidAskGeneratorComponent : public bus::Component {
  public:
    using SnapshotCallback = std::function<void(const Exchange::BookSnapshot&)>;
    using DiffCallback =
        std::function<void(const Exchange::BookDiffSnapshot2&)>;

  private:
    BidAskGeneratorComponent::SnapshotCallback snapshot_callback_ = nullptr;
    BidAskGeneratorComponent::DiffCallback diff_callback_         = nullptr;
    std::unordered_map<common::TradingPair, BidAskState,
                       common::TradingPairHash, common::TradingPairEqual>
        state_map_;
    std::unordered_map<common::TradingPair, BusEventRequestBBOPrice,
                       common::TradingPairHash, common::TradingPairEqual>
        request_bbo_;

    ThreadPool& thread_pool_;
    boost::asio::strand<typename ThreadPool::executor_type> strand_;
    aot::CoBus& bus_;

  public:
    Exchange::BusEventRequestNewSnapshotPool
        request_bus_event_snapshot_mem_pool_;
    Exchange::RequestSnapshotPool request_snapshot_mem_pool_;

    Exchange::BusEventRequestDiffOrderBookPool request_bus_event_diff_mem_pool_;
    Exchange::RequestDiffOrderBookPool request_diff_mem_pool_;

    Exchange::MEMarketUpdate2Pool out_diff_mem_pool_;
    Exchange::BusEventMEMarketUpdate2Pool out_bus_event_diff_mem_pool_;

    /**
     * @brief Construct a new Bid Ask Generator Component object
     *
     * @param executor
     * @param bus
     * @param number_snapshots
     * @param number_diff
     * @param max_number_event_per_tick how much diff events produce from
     * BidAskGeneratorComponent per tick. need this variable for mem pool
     */
    explicit BidAskGeneratorComponent(
        ThreadPool& thread_pool, aot::CoBus& bus,
        const unsigned int number_snapshots, const unsigned int number_diff,
        const unsigned int max_number_event_per_tick)
        : thread_pool_(thread_pool),
          strand_(boost::asio::make_strand(thread_pool)),
          bus_(bus),
          request_bus_event_snapshot_mem_pool_(number_snapshots),
          request_snapshot_mem_pool_(number_snapshots),
          request_bus_event_diff_mem_pool_(number_diff),
          request_diff_mem_pool_(number_diff),
          out_diff_mem_pool_(max_number_event_per_tick),
          out_bus_event_diff_mem_pool_(max_number_event_per_tick) {};

    void AsyncHandleEvent(
        Exchange::BusEventResponseNewSnapshot* event) override {
        // need extend lifetime object
        auto event_ptr =
            boost::intrusive_ptr<Exchange::BusEventResponseNewSnapshot>(event);
        boost::asio::co_spawn(strand_, HandleNewSnapshotEvent(event_ptr),
                              boost::asio::detached);
    };
    void AsyncHandleEvent(Exchange::BusEventBookDiffSnapshot* event) override {
        // need extend lifetime object
        auto event_ptr =
            boost::intrusive_ptr<Exchange::BusEventBookDiffSnapshot>(event);
        boost::asio::co_spawn(strand_, HandleBookDiffSnapshotEvent(event_ptr),
                              boost::asio::detached);
    };
    void AsyncHandleEvent(
        boost::intrusive_ptr<BusEventRequestBBOPrice> event) override {
        boost::asio::co_spawn(strand_, HandleTrackingNewTradingPair(event),
                              boost::asio::detached);
    };
    void AsyncStop() override {

    };

    void RegisterSnapshotCallback(
        BidAskGeneratorComponent::SnapshotCallback callback) {
        snapshot_callback_ = std::move(callback);
    }

    // External API to register diff callback
    void RegisterDiffCallback(BidAskGeneratorComponent::DiffCallback callback) {
        diff_callback_ = std::move(callback);
    }

  private:
    boost::asio::awaitable<void> HandleTrackingNewTradingPair(
        boost::intrusive_ptr<BusEventRequestBBOPrice> event) {
        auto& evt          = *event.get();
        auto& trading_pair = evt.trading_pair;
        if (request_bbo_.count(trading_pair)) {
            loge("request_bbo_ contains trading pair {}",
                 trading_pair.ToString());
            co_return;
        }
        // copy info
        request_bbo_[trading_pair] = *event;
        // start tracking new trading pair

        // need send a signal to launch diff
        co_await RequestSubscribeToDiff(trading_pair);
        co_return;
    }
    boost::asio::awaitable<void> HandleNewSnapshotEvent(
        boost::intrusive_ptr<Exchange::BusEventResponseNewSnapshot> event) {
        // Extract the exchange ID from the event.
        const auto& exchange_id = event->WrappedEvent()->exchange_id;

        // Validate the exchange ID; this component only supports Binance.
        if (exchange_id != common::ExchangeId::kBinance) {
            loge("[UNSUPPORTED EXCHANGE] Exchange ID: {}", exchange_id);
            co_return;  // Exit early for unsupported exchanges.
        }

        // Extract the trading pair from the event.
        auto trading_pair = event->WrappedEvent()->trading_pair;

        // Retrieve the state associated with the trading pair.
        auto& state       = state_map_[trading_pair];

        // Log that a new snapshot is being processed for the trading pair.
        logd("[PROCESSING NEW SNAPSHOT] {}", trading_pair.ToString());

        // Update the state with the new snapshot.
        // The snapshot is moved into the state to avoid unnecessary copies.
        auto wrapped_event = event->WrappedEvent();
        state.snapshot     = std::move(*wrapped_event);

        // End the coroutine since the snapshot update is complete.
        co_return;
    }
    boost::asio::awaitable<void> HandleBookDiffSnapshotEvent(
        boost::intrusive_ptr<Exchange::BusEventBookDiffSnapshot> event) {
        const auto& diff = *event->WrappedEvent();
        logi("[DIFF ACCEPTED] {}, Diff ID Range: [{}-{}]",
             diff.trading_pair.ToString(), diff.first_id, diff.last_id);

        const auto& exchange_id = diff.exchange_id;
        if (exchange_id != common::ExchangeId::kBinance) {
            loge("[UNSUPPORTED EXCHANGE] Exchange ID: {}", exchange_id);
            co_return;
        }

        auto trading_pair = diff.trading_pair;
        auto& state       = state_map_[trading_pair];
        logi("[CURRENT SNAPSHOT] {}, Snapshot LastUpdateId: {}",
             trading_pair.ToString(), state.snapshot.lastUpdateId);

        // Check for packet loss
        state.diff_packet_lost =
            (diff.first_id != state.last_id_diff_book_event + 1);
        state.last_id_diff_book_event = diff.last_id;

        if (state.diff_packet_lost) {
            logw(
                "[PACKET LOSS] {}, Expected First ID: {}, Actual "
                "First ID: {}",
                trading_pair.ToString(), state.last_id_diff_book_event + 1,
                diff.first_id);
        }

        if (state.need_make_snapshot) {
            const bool snapshot_and_diff_now_sync =
                (diff.first_id <= state.snapshot.lastUpdateId + 1) &&
                (diff.last_id >= state.snapshot.lastUpdateId + 1);
            if (snapshot_and_diff_now_sync) {
                state.need_make_snapshot            = false;
                state.need_process_current_snapshot = true;
                logd(
                    "[SYNC ACHIEVED] {}, Snapshot and Diff are "
                    "synchronized",
                    trading_pair.ToString());
            }
        }

        // Determine if a new snapshot is needed
        const bool need_snapshot =
            (state.need_make_snapshot || state.diff_packet_lost);
        if (need_snapshot) {
            if (diff.last_id <= state.snapshot.lastUpdateId) {
                logw(
                    "[OUTDATED DIFF] {}, Diff Last ID: {}, "
                    "Snapshot LastUpdateId: {}. Awaiting newer diff.",
                    trading_pair.ToString(), diff.last_id,
                    state.snapshot.lastUpdateId);
                co_return;
            }

            logd(
                "[REQUESTING NEW SNAPSHOT] {}, Current Snapshot "
                "LastUpdateId: {}",
                trading_pair.ToString(), state.snapshot.lastUpdateId);
            state.need_make_snapshot = true;
            boost::asio::co_spawn(strand_, RequestNewSnapshot(trading_pair),
                                  boost::asio::detached);
            co_return;
        }

        if (state.need_process_current_snapshot) {
            logd(
                "[PROCESSING SNAPSHOT] {}, Snapshot LastUpdateId: "
                "{}",
                trading_pair.ToString(), state.snapshot.lastUpdateId);
            state.need_process_current_snapshot = false;
            state.need_process_current_diff     = true;
            if (!snapshot_callback_) {
                loge("[SNAPSHOT CALLBACK NOT FOUND]");
                co_return;
            }
            try {
                snapshot_callback_(state.snapshot);
            } catch (const std::exception& e) {
                loge("Exception in snapshot callback: {}", e.what());
            } catch (...) {
                loge("Unknown exception in snapshot callback");
            }
        }

        if (!state.diff_packet_lost && state.need_process_current_diff) {
            logd(
                "[PROCESSING DIFF] {}, Diff ID Range: [{}-{}], "
                "Snapshot LastUpdateId: {}",
                trading_pair.ToString(), diff.first_id, diff.last_id,
                state.snapshot.lastUpdateId);
            if (!diff_callback_) {
                loge("[DIFF CALLBACK NOT FOUND]");
                co_return;
            }
            try {
                diff_callback_(diff);
            } catch (const std::exception& e) {
                loge("Exception in snapshot callback: {}", e.what());
            } catch (...) {
                loge("Unknown exception in snapshot callback");
            }
        }
        co_return;
    }
    boost::asio::awaitable<void> RequestNewSnapshot(
        common::TradingPair trading_pair) {
        if (!request_bbo_.count(trading_pair)) {
            loge(
                "can't find info to prepare request new snapshot for this "
                "pair:{} request_bbo.size():{}",
                trading_pair.ToString(), request_bbo_.size());
            co_return;
        }
        auto& info = request_bbo_[trading_pair];

        auto* ptr  = request_snapshot_mem_pool_.Allocate(
            &request_snapshot_mem_pool_, info.exchange_id, info.trading_pair,
            info.snapshot_depth);
        auto intr_ptr_request =
            boost::intrusive_ptr<Exchange::RequestSnapshot>(ptr);

        auto* bus_evt_request = request_bus_event_snapshot_mem_pool_.Allocate(
            &request_bus_event_snapshot_mem_pool_, intr_ptr_request);
        auto intr_ptr_bus_event_request =
            boost::intrusive_ptr<Exchange::BusEventRequestNewSnapshot>(
                bus_evt_request);

        bus_.AsyncSend(this, intr_ptr_bus_event_request);
        co_return;
    }
    boost::asio::awaitable<void> RequestSubscribeToDiff(
        common::TradingPair trading_pair) {
        if (!request_bbo_.count(trading_pair)) {
            loge(
                "can't find info to prepare request to subscribe new trading "
                "pair");
            co_return;
        }
        auto& info = request_bbo_[trading_pair];

        // TODO need process common::kFrequencyMSInvalid, need add
        // common::kFrequencyMSInvalid to info
        auto* ptr  = request_diff_mem_pool_.Allocate(
            &request_diff_mem_pool_, info.exchange_id, info.trading_pair,
            common::kFrequencyMSInvalid, true, info.id);
        auto intr_ptr_request =
            boost::intrusive_ptr<Exchange::RequestDiffOrderBook>(ptr);

        auto* bus_evt_request = request_bus_event_diff_mem_pool_.Allocate(
            &request_bus_event_diff_mem_pool_, intr_ptr_request);
        auto intr_ptr_bus_event_request =
            boost::intrusive_ptr<Exchange::BusEventRequestDiffOrderBook>(
                bus_evt_request);

        bus_.AsyncSend(this, intr_ptr_bus_event_request);
        co_return;
    }
};

class HttpsConnectionPoolFactory : public ::HttpsConnectionPoolFactory {
    common::MemoryPool<::V2::ConnectionPool<::HTTPSesionType>> pool_;

  public:
    explicit HttpsConnectionPoolFactory(size_t default_number_session = 10)
        : pool_(default_number_session) {};
    ~HttpsConnectionPoolFactory() override = default;
    virtual ::V2::ConnectionPool<HTTPSesionType>* Create(
        boost::asio::io_context& io_context, HTTPSesionType::Timeout timeout,
        std::size_t pool_size, https::ExchangeI* exchange) override {
        return pool_.Allocate(io_context, timeout, pool_size, exchange->Host(),
                              exchange->Port());
    };
};
class HttpsConnectionPoolFactory2 : public ::HttpsConnectionPoolFactory2 {
    common::MemoryPool<::V2::ConnectionPool<HTTPSesionType3>> pool_;

  public:
    explicit HttpsConnectionPoolFactory2(size_t default_number_session = 10)
        : pool_(default_number_session) {};
    ~HttpsConnectionPoolFactory2() override = default;
    virtual ::V2::ConnectionPool<HTTPSesionType3>* Create(
        boost::asio::io_context& io_context, HTTPSesionType3::Timeout timeout,
        std::size_t pool_size, https::ExchangeI* exchange) override {
        return pool_.Allocate(io_context, timeout, pool_size, exchange->Host(),
                              exchange->Port());
    };
};

/**
 * @brief Represents parsed data which could be either a BookDiffSnapshot or
 * ApiResponseData.
 */
using ParsedData = std::variant<Exchange::BookDiffSnapshot, ApiResponseData>;

/**
 * @brief Manages JSON parsing and dispatches processing based on response
 * types.
 */
class ParserManager {
    /**
     * @brief A map of handlers for each response type.
     *
     * This map associates a `ResponseType` with a handler function that
     * processes a `simdjson::ondemand::document` and returns `ParsedData`. The
     * appropriate handler function is called during parsing based on the
     * response type.
     */
    std::unordered_map<ResponseType,
                       std::function<ParsedData(simdjson::ondemand::document&)>>
        handlers_;

  public:
    /**
     * @brief Registers a handler for a specific response type.
     *
     * Associates a handler function with a particular `ResponseType`. The
     * handler is invoked to process the JSON document when the corresponding
     * response type is parsed.
     *
     * @param type The `ResponseType` to associate with the handler.
     * @param handler A function that processes a `simdjson::ondemand::document`
     * and returns `ParsedData`.
     */
    void RegisterHandler(
        ResponseType type,
        std::function<ParsedData(simdjson::ondemand::document&)> handler) {
        handlers_[type] = handler;
    }

    /**
     * @brief Parses the JSON response for the appropriate response type based
     * on registered handlers.
     *
     * This method parses the provided JSON response using SIMDJSON and invokes
     * the registered handler for the corresponding response type, as determined
     * by `DetermineType`. The handler processes the parsed document and returns
     * the parsed data as a `ParsedData` variant.
     *
     * @param response A string view of the JSON response to be parsed.
     * @return ParsedData The parsed data wrapped in a `ParsedData` variant.
     * @throws std::runtime_error If no handler is registered for the response
     * type or if parsing fails.
     */
    ParsedData Parse(std::string_view response) {
        simdjson::ondemand::parser parser;
        simdjson::padded_string padded_response(response);
        simdjson::ondemand::document doc = parser.iterate(padded_response);
        auto type                        = DetermineType(doc);

        // Find the handler for the determined response type
        auto it                          = handlers_.find(type);
        if (it == handlers_.end()) {
            loge("No handler registered for this response type");
            return {};  // Return empty if no handler is registered
        }

        // Call the handler function for the determined response type
        return it->second(doc);
    }

  private:
    ResponseType DetermineType(simdjson::ondemand::document& doc) {
        // Check if this is a depth update response
        if (doc["e"].error() == simdjson::SUCCESS && doc["e"].is_string() &&
            doc["e"] == "depthUpdate") {
            return ResponseType::kDepthUpdate;
        }

        // Check if this is an error response (contains "code" and "msg")
        if (doc["code"].error() == simdjson::SUCCESS &&
            doc["code"].type() == simdjson::ondemand::json_type::number &&
            doc["msg"].error() == simdjson::SUCCESS &&
            doc["msg"].type() == simdjson::ondemand::json_type::string) {
            return ResponseType::kErrorResponse;
        }

        // Check if this is a success response (contains "result": null)
        if (doc["result"].error() == simdjson::SUCCESS
            //&& doc["result"].is_null()
        ) {
            return ResponseType::kNonQueryResponse;
        }

        return ResponseType::kUnknown;  // Default case if no match
    }
};

/**
 * @class ApiResponseParser
 * @brief Parses API responses from JSON into structured data.
 *
 * The ApiResponseParser class is responsible for parsing JSON responses
 * from an API into an ApiResponseData structure. It supports success responses,
 * error responses, and responses with an optional ID field.
 */
class ApiResponseParser {
  public:
    /**
     * @brief Parses a JSON API response into an ApiResponseData structure.
     *
     * This method processes a JSON document and extracts relevant fields such
     * as:
     * - `result`: If null, indicates a successful non-query request (e.g.,
     * subscription).
     * - `code` and `msg`: Error code and message, if an error response is
     * present.
     * - `id`: Optional ID field for specific responses.
     *
     * @param doc A reference to a `simdjson::ondemand::document` containing the
     * API response.
     * @return An `ApiResponseData` object with the parsed data.
     */
    ApiResponseData Parse(simdjson::ondemand::document& doc) {
        ApiResponseData data;

        // Check for "result": null
        auto result_field = doc["result"];
        if (!result_field.error()) {
            if (result_field.is_null())
                data.status = ApiResponseStatus::kSuccess;

            auto id_field = doc["id"];
            if (!id_field.error()) {
                if (id_field.type() == simdjson::ondemand::json_type::string) {
                    std::string_view id_value;
                    if (id_field.get_string().get(id_value) ==
                        simdjson::SUCCESS) {
                        data.id = std::string(id_value);  // Store as string
                    }
                } else if (id_field.type() ==
                           simdjson::ondemand::json_type::number) {
                    simdjson::ondemand::number number = id_field.get_number();
                    simdjson::ondemand::number_type t =
                        number.get_number_type();
                    switch (t) {
                        case simdjson::ondemand::number_type::signed_integer:
                            if (number.is_int64()) {
                                data.id = number.get_int64();
                            } else {
                                loge("Unexpected signed integer size.");
                            }
                            break;
                        case simdjson::ondemand::number_type::unsigned_integer:
                            if (number.is_uint64()) {
                                data.id = number.get_uint64();
                            } else {
                                loge("Unexpected unsigned integer size.");
                            }
                            break;
                        case simdjson::ondemand::number_type::
                            floating_point_number:
                            // If it's a floating point, you can get the value
                            // as a double
                            loge("Unexpected double");
                            break;
                        case simdjson::ondemand::number_type::big_integer:
                            // Handle big integers (e.g., large numbers out of
                            // int64_t range)
                            loge("Big integer value detected.");
                            break;
                        default:
                            loge("Unknown number type.");
                            break;
                    }
                }
            }
        }
        return data;  // Return populated or empty data
    };
};

ParserManager InitParserManager(
    common::TradingPairHashMap& pairs,
    common::TradingPairReverseHashMap& pair_reverse,
    ApiResponseParser& api_response_parser,
    detail::FamilyBookEventGetter::ParserResponse& parser_ob_diff);
};  // namespace binance