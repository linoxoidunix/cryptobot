#pragma once
#include <algorithm>
#include <boost/beast/http.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/version.hpp>
#include <string>
#include <string_view>
#include <utility>

#include "aot/Exchange.h"
#include "aot/Https.h"
#include "aot/Logger.h"
#include "aot/WS.h"
#include "aot/client_request.h"
#include "aot/client_response.h"
#include "aot/common/thread_utils.h"
#include "aot/common/time_utils.h"
#include "aot/common/types.h"
#include "aot/market_data/market_update.h"
#include "aot/prometheus/event.h"

namespace mexc {

const auto kMeasureTForGeneratorBidAskService =
    MEASURE_T_FOR_GENERATOR_BID_ASK_SERVICE;  // MEASURE_T_FOR_GENERATOR_BID_ASK_SERVICE
                                              // define in cmakelists.txt
enum class TimeInForce { GTC, IOC, FOK };

namespace mainnet {
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

     */
    explicit HttpsExchange(std::uint64_t recv_window = DefaultRecvWindow)
        : recv_window_((recv_window > MaxRecvWindow) ? MaxRecvWindow
                                                     : recv_window) {
              /**
               * @brief
               * It is recommended to use a small recvWindow of 5000 or less!
               * The max cannot go beyond 60,000!
               *
               */
          };

    // Using static polymorphism to provide behavior
    constexpr std::string_view HostImpl() const {
        return "api.mexc.com";
    };

    constexpr std::string_view PortImpl() const { return "443"; };

    constexpr std::uint64_t RecvWindowImpl() const { return recv_window_; };

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
            default:
                current_exchange = &main_net_;
                break;
        }
        return current_exchange;
    }

  private:
    mainnet::HttpsExchange main_net_;
};

/**
 * @brief https://mexcdevelop.github.io/apidocs/spot_v3_en/#enum-definitions
 * Order type
 * LIMIT (Limit order)
 * MARKET (Market order)
 * LIMIT_MAKER (Limit maker order)
 * IMMEDIATE_OR_CANCEL (Immediate or cancel order)
 * FILL_OR_KILL (Fill or kill order)
 * 
 */
enum class Type {
    LIMIT,
    MARKET,
    LIMIT_MAKER,
    IMMEDIATE_OR_CANCEL,
    FILL_OR_KILL
};

class m1 : public KLineStreamI::ChartInterval {
  public:
    explicit m1() = default;
    std::string ToString() const override { return "Min1"; };
    uint Seconds() const override { return 60; };
};
class m5 : public KLineStreamI::ChartInterval {
  public:
    explicit m5() = default;
    std::string ToString() const override { return "Min5"; }
    uint Seconds() const override { return 300; };
};
class m15 : public KLineStreamI::ChartInterval {
  public:
    explicit m15() = default;
    std::string ToString() const override { return "Min15"; }
    uint Seconds() const override { return 900; };
};
class m30 : public KLineStreamI::ChartInterval {
  public:
    explicit m30() = default;
    std::string ToString() const override { return "Min30"; }
    uint Seconds() const override { return 1800; };
};
class h1 : public KLineStreamI::ChartInterval {
  public:
    explicit m60() = default;
    std::string ToString() const override { return "Min60"; }
    uint Seconds() const override { return 3600; };
};
class h4 : public KLineStreamI::ChartInterval {
  public:
    explicit h4() = default;
    std::string ToString() const override { return "Hour4"; }
    uint Seconds() const override { return 14400; };
};
class h8 : public KLineStreamI::ChartInterval {
  public:
    explicit h4() = default;
    std::string ToString() const override { return "Hour8"; }
    uint Seconds() const override { return 28800; };
};
class d1 : public KLineStreamI::ChartInterval {
  public:
    explicit d1() = default;
    std::string ToString() const override { return "Day1"; }
    uint Seconds() const override { return 86400; };
};
class w1 : public KLineStreamI::ChartInterval {
  public:
    explicit w1() = default;
    std::string ToString() const override { return "Week1"; }
    uint Seconds() const override { return 604800; };
};
class M1 : public KLineStreamI::ChartInterval {
  public:
    explicit M1() = default;
    std::string ToString() const override { return "Month1"; }
    uint Seconds() const override { return 2.628e6; };
};
class KLineStream : public KLineStreamI {
  public:
    explicit KLineStream(std::string_view trading_pair,
                         const KLineStreamI::ChartInterval* chart_interval)
        : trading_pair_(trading_pair), chart_interval_(chart_interval) {};
    std::string ToString() const override {
        return fmt::format("spot@public.kline.v3.api@{}@{}", trading_pair_,
                           chart_interval_->ToString());
    };
    ~KLineStream() override = default;

  private:
    std::string_view trading_pair_;
    const KLineStreamI::ChartInterval* chart_interval_;
};

class DiffDepthStream : public DiffDepthStreamI {
  public:
   explicit DiffDepthStream(const common::TradingPairInfo& s)
        : symbol_(s), interval_(interval) {};
    std::string ToString() const override {
        return fmt::format("spot@public.increase.depth.v3.api@{}", symbol_.https_json_request,
                           interval_->ToString());
    };

  private:
    const common::TradingPairInfo& symbol_;
    const StreamIntervalI* interval_;
};

// class OHLCVI : public OHLCVGetter {
//   public:
//     OHLCVI(const Symbol* s, const KLineStreamI::ChartInterval* chart_interval,
//            TypeExchange type_exchange)
//         : s_(s),
//           chart_interval_(chart_interval),
//           type_exchange_(type_exchange) {
//         current_exchange_ = exchange_.Get(type_exchange);
//     };
//     bool LaunchOne() override { ioc.run_one();return true; };

//     void Init(OHLCVILFQueue& lf_queue) override {
//         std::function<void(boost::beast::flat_buffer & buffer)> OnMessageCB;
//         OnMessageCB = [](boost::beast::flat_buffer& buffer) {
//             auto resut = boost::beast::buffers_to_string(buffer.data());
//             logi("{}", resut);
//         };

//         using kls = KLineStream;
//         kls channel(s_, chart_interval_);
//         auto request_without_bracket = fmt::format(
//             "\"req_id\": \"test\",\"op\": \"subscribe\", \"args\": [\"{}\"]",
//             channel.ToString());
//         std::string request = "{" + request_without_bracket + "}";
//         std::make_shared<WS>(ioc, request, OnMessageCB)
//             ->Run("stream-testnet.bybit.com", "443", "/v5/public/spot");
//         ioc.run();
//     };

//   private:
//     boost::asio::io_context ioc;
//     ExchangeChooser exchange_;
//     https::ExchangeI* current_exchange_ = nullptr;
//     const Symbol* s_;
//     const KLineStreamI::ChartInterval* chart_interval_;
//     TypeExchange type_exchange_;
// };

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
namespace detail {
class FactoryRequest {
  public:
    explicit FactoryRequest(const https::ExchangeI* exchange,
                            std::string_view end_point, const ArgsQuery& args,
                            boost::beast::http::verb action, SignerI* signer,
                            bool need_sign = false)
        : exchange_(exchange), args_(args), action_(action), signer_(signer) {
        if (need_sign) {
            AddSignParams();
            auto request_args = args_.QueryString();
            /**
             * @brief https://mexcdevelop.github.io/apidocs/spot_v3_en/#header:~:text=The%20signature%20is%20support%20lowercase%20only. 
             * The signature is support lowercase only.
             */
            auto signature    = signer_->SignByLowerCase(request_args);
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
        if (signer_) req.insert("X-MEXC-APIKEY", signer_->ApiKey().data());
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
                "application/json");
        return req;
    };
    std::string_view Host() const { return exchange_->Host(); };
    std::string_view Port() const { return exchange_->Port(); };
    std::string_view EndPoint() const { return end_point_; }

  private:
    inlinevoid AddSignParams() {
        CurrentTime time_service;
        args_["recvWindow"] = std::to_string(exchange_->RecvWindow());
        args_["timestamp"]  = std::to_string(time_service.Time());
    };
    inline void AddSignature(std::string_view signature) {
        args_["signature"] = signature.data();
    };

  private:
    const https::ExchangeI* exchange_;
    std::string end_point_;
    ArgsQuery args_;
    boost::beast::http::verb action_;
    SignerI* signer_;
};

class FamilyLimitOrder {
  public:
    virtual ~FamilyLimitOrder()                 = default;
    explicit FamilyLimitOrder()                 = default;
    static constexpr std::string_view end_point = "/api/v3/order";
    class ParserResponse {
      public:
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
                case LIMIT_MAKER:
                    storage["type"] = "LIMIT_MAKER";
                    break;
                case IMMEDIATE_OR_CANCEL:
                    storage["type"] = "IMMEDIATE_OR_CANCEL";
                    break;
                case FILL_OR_KILL:
                    storage["type"] = "FILL_OR_KILL";
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
    explicit FamilyCancelOrder() = default;
    virtual ~FamilyCancelOrder() = default;

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
        // session_pool_->ReleaseConnection(session);
        using namespace std::literals::chrono_literals;
    };
    ~OrderNewLimit2() override = default;

  private:
    ExchangeChooser exchange_;
    https::ExchangeI* current_exchange_;
    SignerI* signer_;
    // pass pairs_ without const due to i want [] operator
    common::TradingPairHashMap& pairs_;
    // pass pairs_reverse_ without const due to i want [] operator
    common::TradingPairReverseHashMap& pairs_reverse_;
    ::V2::ConnectionPool<HTTPSesionType>* session_pool_;
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
    https::ExchangeI* current_exchange_;
    SignerI* signer_;
    common::TradingPairHashMap& pairs_;
    common::TradingPairReverseHashMap& pairs_reverse_;
    ::V2::ConnectionPool<HTTPSesionType>* session_pool_;
};

class BookSnapshot : public inner::BookSnapshotI {
    static constexpr std::string_view end_point = "/api/v3/depth";
    class ParserResponse {
        const common::TradingPairInfo& pair_info_;

      public:
        explicit ParserResponse(const common::TradingPairInfo& pair_info)
            : pair_info_(pair_info) {};
        Exchange::BookSnapshot Parse(std::string_view response);
    };

  public:
    class ArgsOrder : public ArgsQuery {
      public:
        using SymbolType = std::string_view;
        using Limit      = uint16_t;
        explicit ArgsOrder(SymbolType ticker1, SymbolType ticker2, Limit limit)
            : ArgsQuery() {
            SetSymbol(ticker1, ticker2);
            SetLimit(limit);
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
    explicit BookSnapshot(ArgsOrder&& args, TypeExchange type,
                          Exchange::BookSnapshot* snapshot,
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
                                       BookSnapshot::end_point,
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
            ParserResponse parser(pair_info_);
            auto answer = parser.Parse(resut);
            // answer.ticker = args_["symbol"];
            *snapshot_  = answer;
        };
        std::make_shared<Https>(ioc, cb)->Run(
            factory.Host().data(), factory.Port().data(),
            factory.EndPoint().data(), factory());
        ioc.run();
    };

  private:
    ArgsOrder args_;
    binance::testnet::HttpsExchange binance_test_net_;
    binance::mainnet::HttpsExchange binance_main_net_;

    https::ExchangeI* current_exchange_;
    SignerI* signer_ = nullptr;
    Exchange::BookSnapshot* snapshot_;
    const common::TradingPairInfo& pair_info_;
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

    //binance::testnet::HttpsExchange binance_test_net_;
    binance::mainnet::HttpsExchange mecx_main_net_;
    https::ExchangeI* current_exchange_;

  private:
    auto Run() noexcept -> void;
};

class ConnectionPoolFactory : public ::ConnectionPoolFactory {
  public:
    ~ConnectionPoolFactory() override = default;
    virtual ::V2::ConnectionPool<HTTPSesionType>* Create(
        boost::asio::io_context& io_context, https::ExchangeI* exchange,
        std::size_t pool_size, HTTPSesionType::Timeout timeout) override {
        return new V2::ConnectionPool<HTTPSesionType>(
            io_context, exchange->Host(), exchange->Port(), pool_size, timeout);
    };
};
};  // namespace binance