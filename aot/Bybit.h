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

namespace bybit {
enum class Type { LIMIT, MARKET };
enum class TimeInForce { GTC, IOC, FOK, POST_ONLY };

namespace testnet {
class HttpsExchange : public https::ExchangeI {
  public:
    /**
     * @brief Construct a new Https Exchange object
     *
     * @param recv_window
     * https://bybit-exchange.github.io/docs/v3/intro
     * We also provide X-BAPI-RECV-WINDOW (unit in millisecond and default value
     * is 5,000) to specify how long an HTTP request is valid. It is also used
     * to prevent replay attacks.
     */
    explicit HttpsExchange(std::uint64_t recv_window = 5000)
        : recv_window_(recv_window) {
        recv_window_ = (recv_window > 5000) ? 5000 : recv_window;
    };

    ~HttpsExchange() override = default;
    std::string_view Host() const override {
        return std::string_view("api-testnet.bybit.com");
    };
    std::string_view Port() const override { return "443"; };
    std::uint64_t RecvWindow() const override { return recv_window_; };

  private:
    std::uint64_t recv_window_;
};
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
    explicit HttpsExchange(std::uint64_t recv_window = 5000) {
        /**
         * @brief
          https://binance-docs.github.io/apidocs/spot/en/#endpoint-security-type
          It is recommended to use a small recvWindow of 5000 or less! The max
         cannot go beyond 60,000!
         *
         */
        recv_window_ = (recv_window > 5000) ? 5000 : recv_window;
    };

    virtual ~HttpsExchange() = default;
    std::string_view Host() const override {
        return std::string_view("api.bybit.com");
    };
    std::string_view Port() const override { return "443"; };
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
    bybit::testnet::HttpsExchange test_net_;
    bybit::mainnet::HttpsExchange main_net_;
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
     * @brief return query string starts with ?
     *
     * @return std::string
     */
    virtual std::string Body();

    virtual std::string FinalQueryString() { return {}; };
    virtual ~ArgsBody() = default;
};

class m1 : public KLineStreamI::ChartInterval {
  public:
    explicit m1() = default;
    std::string ToString() const override { return "1"; }
    uint Seconds() const override { return 60; };
};
class m3 : public KLineStreamI::ChartInterval {
  public:
    explicit m3() = default;
    std::string ToString() const override { return "3"; }
    uint Seconds() const override { return 180; };
};
class m5 : public KLineStreamI::ChartInterval {
  public:
    explicit m5() = default;
    std::string ToString() const override { return "5"; }
    uint Seconds() const override { return 300; };
};
class m15 : public KLineStreamI::ChartInterval {
  public:
    explicit m15() = default;
    std::string ToString() const override { return "15"; }
    uint Seconds() const override { return 900; };
};
class m60 : public KLineStreamI::ChartInterval {
  public:
    explicit m60() = default;
    std::string ToString() const override { return "30"; }
    uint Seconds() const override { return 3600; };
};
class m120 : public KLineStreamI::ChartInterval {
  public:
    explicit m120() = default;
    std::string ToString() const override { return "120"; }
    uint Seconds() const override { return 7200; };
};
class m240 : public KLineStreamI::ChartInterval {
  public:
    explicit m240() = default;
    std::string ToString() const override { return "240"; }
    uint Seconds() const override { return 14400; };
};
class m360 : public KLineStreamI::ChartInterval {
  public:
    explicit m360() = default;
    std::string ToString() const override { return "360"; }
    uint Seconds() const override { return 21600; };
};
class m720 : public KLineStreamI::ChartInterval {
  public:
    explicit m720() = default;
    std::string ToString() const override { return "720"; }
    uint Seconds() const override { return 43200; };
};
class D1 : public KLineStreamI::ChartInterval {
  public:
    explicit D1() = default;
    std::string ToString() const override { return "D"; }
    uint Seconds() const override { return 86400; };
};
class W1 : public KLineStreamI::ChartInterval {
  public:
    explicit W1() = default;
    std::string ToString() const override { return "W"; }
    uint Seconds() const override { return 604800; };
};
class M1 : public KLineStreamI::ChartInterval {
  public:
    explicit M1() = default;
    std::string ToString() const override { return "M"; }
    uint Seconds() const override { return 2.628e6; };
};

class KLineStream : public KLineStreamI {
  public:
    explicit KLineStream(std::string_view trading_pair,
                         const KLineStreamI::ChartInterval* chart_interval)
        : trading_pair_(trading_pair), chart_interval_(chart_interval) {};
    std::string ToString() const override {
        return fmt::format("kline.{0}.{1}", trading_pair_,
                           chart_interval_->ToString());
    };
    ~KLineStream() override = default;

  private:
    std::string_view trading_pair_;
    const KLineStreamI::ChartInterval* chart_interval_;
};

class ParserKLineResponse : public ParserKLineResponseI {
  public:
    explicit ParserKLineResponse() = default;
    OHLCVExt Get(std::string_view response_from_exchange) const override {
        return {};
    };
};

// class OHLCVI : public OHLCVGetter {
//   public:
//     OHLCVI(const Symbol* s, const KLineStreamI::ChartInterval*
//     chart_interval,
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

namespace detail {
class FamilyBookEventGetter {
    static constexpr std::string_view end_point_as_json = "/v5/public/spot";

  public:
    class ParserResponse {
        common::TradingPairHashMap& pairs_;
        common::TradingPairReverseHashMap& pairs_reverse_;

      public:
        using ResponseVariant =
            std::variant<std::monostate, Exchange::BookSnapshot,
                         Exchange::BookDiffSnapshot>;
        explicit ParserResponse(
            common::TradingPairHashMap& pairs,
            common::TradingPairReverseHashMap& pairs_reverse)
            : pairs_reverse_(pairs_reverse), pairs_(pairs) {};

        ResponseVariant Parse(std::string_view response);

      private:
        template <typename T>
        bool ResolveTradingPair(T& t, std::string_view trading_pair) {
            auto status = false;
            if (pairs_reverse_.count(trading_pair)) [[likely]] {
                t.trading_pair = pairs_reverse_.find(trading_pair)->second;
                status           = true;
            } else {
                loge("pairs_reverse not contain {}", trading_pair);
            }
            return status;
        }
    };
    // class ArgsOrder : public ArgsBody {
    //   public:
    //     using SymbolType = std::string_view;
    //     using Limit      = uint16_t;
    //     explicit ArgsOrder(SymbolType ticker1, SymbolType ticker2, Limit
    //     limit)
    //         : ArgsBody() {
    //         SetSymbol(ticker1, ticker2);
    //         SetLimit(limit);
    //     };
    //     explicit ArgsOrder(
    //         const Exchange::RequestSnapshot* request_cancel_order,
    //         common::TradingPairHashMap& pairs)
    //         : ArgsBody() {
    //         SetSymbol(
    //             pairs[request_cancel_order->trading_pair].https_query_request);
    //         SetLimit(request_cancel_order->depth);
    //     };

    //   private:
    //     void SetSymbol(SymbolType symbol) {
    //         storage["symbol"] = symbol.data();
    //     };
    //     void SetSymbol(SymbolType ticker1, SymbolType ticker2) {
    //         SymbolUpperCase formatter(ticker1, ticker2);
    //         storage["symbol"] = formatter.ToString().data();
    //     };
    //     void SetLimit(Limit limit) {
    //         /**
    //          * @brief If limit > 5000, then the response will truncate to
    //          5000.
    //          * https://binance-docs.github.io/apidocs/spot/en/#exchange-information
    //          */
    //         limit            = (limit > 5000) ? 5000 : limit;

    //         storage["limit"] = std::to_string(limit);
    //     };

    //   private:
    //     ArgsOrder& storage = *this;
    // };

    class ArgsBody : public bybit::ArgsBody {
        common::TradingPairHashMap& pairs_;
        /**
         * @brief id_ need add when send json request to exchange
         *
         */
        unsigned int id_;

      public:
        ArgsBody(const Exchange::RequestDiffOrderBook* request,
                 common::TradingPairHashMap& pairs, unsigned int id)
            : bybit::ArgsBody(), pairs_(pairs), id_(id) {
            SetMethod();
            SetParams(request);
            SetId(id_);
        }

      private:
        void SetMethod() { storage["op"] = "\"subscribe\""; };
        void SetParams(const Exchange::RequestDiffOrderBook* request) {
            /**
             * @brief frequency = depth 
             * https://bybit-exchange.github.io/docs/v5/websocket/public/orderbook
             * 
             */
            auto depth = request->frequency;
            if (depth == common::kFrequencyMSInvalid){
                return;
            }
            storage["args"] = fmt::format(
                    "{}.{}.{}","orderbook", depth, pairs_[request->trading_pair].https_json_request);
                    return;
            
        };
        void SetId(unsigned int id) {
            // i don't want process an id now because for binance an id is
            // integer, but for ArgsBody value is a string
        };

      private:
        ArgsBody& storage = *this;
    };
    virtual ~FamilyBookEventGetter() = default;
};
};  // namespace detail

namespace detail {
/**
 * @brief for GET request
 *
 */
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
          need_sign_(need_sign) {};
    boost::beast::http::request<boost::beast::http::string_body> operator()() {
        boost::beast::http::request<boost::beast::http::string_body> req;
        if (need_sign_) {
            auto current_time      = AddSignParams(req);
            /**
             * @brief
             * https://bybit-exchange.github.io/docs/v5/guide#parameters-for-authenticated-endpoints
             * Basic steps:
                1) timestamp + API key + (recv_window) + (queryString |
             jsonBodyString)
             *
             */
            auto request_body_args = fmt::format(
                "{}{}{}{}", current_time, signer_->ApiKey(),
                std::to_string(exchange_->RecvWindow()), args_.Body());
            /**
             * @brief
             * 2) Use the HMAC_SHA256 or RSA_SHA256 algorithm to sign the string
             * in step 1, and convert it to a hex string (HMAC_SHA256) / base64
             * (RSA_SHA256) to obtain the sign parameter.
             */
            auto signature = signer_->Sign(request_body_args);

            AddSignature(signature, req);
        }
        req.version(11);
        req.method(action_);
        req.target(end_point_);
        /**
         * @brief API-keys are passed into the REST API via the X-MBX-APIKEY
         * header.
         *
         */
        req.insert("X-BAPI-API-KEY", signer_->ApiKey().data());
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
        req.set(boost::beast::http::field::content_type, "application/json");
        req.body() = args_.Body();
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
    std::string AddSignParams(
        boost::beast::http::request<boost::beast::http::string_body>& req)
        const {
        CurrentTime time_service;
        auto time = std::to_string(time_service.Time());
        req.insert("X-BAPI-TIMESTAMP", time);
        req.insert("X-BAPI-RECV-WINDOW",
                   std::to_string(exchange_->RecvWindow()));
        return time;
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
    void AddSignature(
        std::string_view signature,
        boost::beast::http::request<boost::beast::http::string_body>& req)
        const {
        req.insert("X-BAPI-SIGN", signature.data());
        req.insert("X-BAPI-SIGN-TYPE", "2");
    };

  private:
    const https::ExchangeI* exchange_;
    std::string end_point_;
    ArgsBody& args_;
    boost::beast::http::verb action_;
    SignerI* signer_;
    bool need_sign_;
};
};  // namespace detail
// class OrderNewLimit : public inner::OrderNewI {
//     static constexpr std::string_view end_point = "/v5/order/create";

//   public:
//     class ArgsOrder : public ArgsBody {
//       public:
//         using SymbolType = std::string_view;
//         explicit ArgsOrder(Exchange::RequestNewOrder* new_order,
//                            common::TradingPairHashMap& pairs)
//             : ArgsBody() {
//             storage["category"] = "spot";
//             SetSymbol(pairs[new_order->trading_pair].https_query_request);
//             SetSide(new_order->side);
//             SetType(Type::LIMIT);
//             auto qty_prec =
//                 std::pow(10, -pairs[new_order->trading_pair].qty_precission);
//             SetQuantity(new_order->qty * qty_prec,
//                         pairs[new_order->trading_pair].qty_precission);
//             auto price_prec =
//                 std::pow(10,
//                 -pairs[new_order->trading_pair].price_precission);
//             SetPrice(new_order->price * price_prec,
//                      pairs[new_order->trading_pair].price_precission);
//             SetTimeInForce(TimeInForce::IOC);
//         };

//       private:
//         void SetSymbol(SymbolType symbol) {
//             assert(false);
//             SymbolUpperCase formatter(symbol.data(), symbol.data());
//             storage["symbol"] = formatter.ToString();
//         };
//         void SetSide(common::Side side) {
//             switch (side) {
//                 case common::Side::BUY:
//                     storage["side"] = "Buy";
//                     break;
//                 case common::Side::SELL:
//                     storage["side"] = "Sell";
//                     break;
//             }
//         };
//         void SetType(Type type) {
//             switch (type) {
//                 case Type::LIMIT:
//                     storage["orderType"] = "Limit";
//                     break;
//                 case Type::MARKET:
//                     storage["orderType"] = "Market";
//                     break;
//                 default:
//                     storage["orderType"] = "Limit";
//             }
//         };
//         void SetQuantity(double quantity, uint8_t qty_prec) {
//             storage["qty"] = fmt::format("{:.{}f}", quantity, qty_prec);
//         };
//         void SetPrice(double price, uint8_t price_prec) {
//             storage["price"] = fmt::format("{:.{}f}", price, price_prec);
//         };
//         void SetTimeInForce(TimeInForce time_in_force) {
//             switch (time_in_force) {
//                 case TimeInForce::FOK:
//                     storage["timeInForce"] = "FOK";
//                     break;
//                 case TimeInForce::GTC:
//                     storage["timeInForce"] = "GTC";
//                     break;
//                 case TimeInForce::IOC:
//                     storage["timeInForce"] = "IOC";
//                     break;
//                 case TimeInForce::POST_ONLY:
//                     storage["timeInForce"] = "PostOnly";
//                     break;
//                 default:
//                     storage["timeInForce"] = "PostOnly";
//             }
//         };

//       private:
//         ArgsOrder& storage = *this;
//     };

//   public:
//     explicit OrderNewLimit(SignerI* signer, TypeExchange type,
//                            common::TradingPairHashMap& pairs)
//         : signer_(signer), pairs_(pairs) {
//         switch (type) {
//             case TypeExchange::MAINNET:
//                 current_exchange_ = &testnet_exchange;
//                 break;
//             case TypeExchange::TESTNET:
//                 current_exchange_ = &testnet_exchange;
//                 break;
//             default:
//                 current_exchange_ = &testnet_exchange;
//                 break;
//         }
//     };
//     void Exec(Exchange::RequestNewOrder* new_order,
//               Exchange::ClientResponseLFQueue* response_lfqueue) override {
//         ArgsOrder args(new_order, pairs_);
//         bool need_sign = true;
//         detail::FactoryRequestJson factory{current_exchange_,
//                                            OrderNewLimit::end_point,
//                                            args,
//                                            boost::beast::http::verb::post,
//                                            signer_,
//                                            need_sign};
//         boost::asio::io_context ioc;
//         OnHttpsResponce cb;
//         cb = [response_lfqueue](
//                  boost::beast::http::response<boost::beast::http::string_body>&
//                      buffer) {
//             const auto& resut = buffer.body();
//             logi("{}", resut);
//         };
//         std::make_shared<Https>(ioc, cb)->Run(
//             factory.Host().data(), factory.Port().data(),
//             factory.EndPoint().data(), factory());
//         ioc.run();
//     };

//   private:
//     testnet::HttpsExchange testnet_exchange;
//     https::ExchangeI* current_exchange_;
//     SignerI* signer_;
//     common::TradingPairHashMap& pairs_;
// };

template <typename Executor>
class BookEventGetter3 : public detail::FamilyBookEventGetter,
                         public inner::BookEventGetterI {
    ::V2::ConnectionPool<WSSesionType3, const std::string_view&>* session_pool_;
    common::TradingPairHashMap& pairs_;
    // Add a callback map to store parsing callbacks for each trading pair
    std::unordered_map<common::TradingPair, const OnWssResponse*,
                       common::TradingPairHash, common::TradingPairEqual>
        callback_map_;
  protected:
    Executor executor_;
    boost::asio::cancellation_signal& signal_;

  public:
    BookEventGetter3(
        Executor&& executor,
        ::V2::ConnectionPool<WSSesionType3, const std::string_view&>*
            session_pool,
        TypeExchange type, common::TradingPairHashMap& pairs,
        boost::asio::cancellation_signal& signal)
        : executor_(std::move(executor)),
          session_pool_(session_pool),
          pairs_(pairs),
          signal_(signal) {
        switch (type) {
            case TypeExchange::MAINNET:
                current_exchange_ = &binance_main_net_;
                break;
            default:
                current_exchange_ = &binance_test_net_;
                break;
        }
    };
    ~BookEventGetter3() override = default;
    boost::asio::awaitable<void> CoExec(
        boost::intrusive_ptr<Exchange::BusEventRequestDiffOrderBook>
            bus_event_request_diff_order_book) override {
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
            [this, bus_event_request_diff_order_book]()
                -> boost::asio::awaitable<void> {
                logd("start book event getter for bybit");
                auto& trading_pair =
                    bus_event_request_diff_order_book->WrappedEvent()
                        ->trading_pair;
                auto callback_it = callback_map_.find(trading_pair);
                if (callback_it == callback_map_.end()) {
                    loge("No callback registered for trading pair: {}",
                         trading_pair.ToString());
                    co_return;
                }
                detail::FamilyBookEventGetter::ArgsBody args(
                    bus_event_request_diff_order_book->WrappedEvent(), pairs_,
                    1);
                logd("start prepare event getter for bybit request");
                auto req = args.Body();
                logd("end prepare event getter for bybit request");

                // bus_event_request_diff_order_book->Release();
                // bus_event_request_diff_order_book->WrappedEvent()->Release();
                auto session = session_pool_->AcquireConnection();
                logd("start send event getter for bybit request");

                auto slot      = signal_.slot();
                auto& callback = callback_it->second;
                if (auto status = co_await session->AsyncRequest(
                        std::move(req), callback, slot);
                    status == false)
                    loge("AsyncRequest finished unsuccessfully");

                logd("end send event getter for bybit request");
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
                          const OnWssResponse* callback) {
        callback_map_[trading_pair] = callback;
    }


  private:
    bybit::testnet::HttpsExchange binance_test_net_;
    bybit::mainnet::HttpsExchange binance_main_net_;

    https::ExchangeI* current_exchange_;
};

template <typename Executor>
class BookEventGetterComponent : public bus::Component,
                                 public BookEventGetter3<Executor> {
    static constexpr std::string_view name_component_ = "bybit::BookEventGetterComponent";  
  public:
    common::MemoryPool<Exchange::BookDiffSnapshot2> book_diff_mem_pool_;
    common::MemoryPool<Exchange::BusEventBookDiffSnapshot>
        bus_event_book_diff_snapshot_mem_pool_;
    Exchange::BookSnapshot2Pool snapshot_mem_pool_;
    Exchange::BusEventResponseNewSnapshotPool
        bus_event_response_snapshot_mem_pool_;
    explicit BookEventGetterComponent(
        Executor&& executor, size_t number_responses, TypeExchange type,
        common::TradingPairHashMap& pairs,
        ::V2::ConnectionPool<WSSesionType3, const std::string_view&>*
            session_pool,
        boost::asio::cancellation_signal& cancel_signal)
        : BookEventGetter3<Executor>(std::move(executor), session_pool, type,
                                     pairs, cancel_signal),
          book_diff_mem_pool_(number_responses),
          bus_event_book_diff_snapshot_mem_pool_(number_responses),
          /**
           * @brief snapshot_mem_pool_ size can be less. it is just snapsot
           * 
           */
          snapshot_mem_pool_(number_responses),
          /**
           * @brief bus_event_response_snapshot_mem_pool_ size can be less. it is just snapsot
           * 
           */
          bus_event_response_snapshot_mem_pool_(number_responses) {}
    ~BookEventGetterComponent() override = default;

    void AsyncHandleEvent(
        boost::intrusive_ptr< Exchange::BusEventRequestDiffOrderBook> event) override {
        boost::asio::co_spawn(BookEventGetter3<Executor>::executor_,
                              BookEventGetter3<Executor>::CoExec(event),
                              boost::asio::detached);
    };
    void AsyncStop() override {
        net::steady_timer delay_timer(BookEventGetter3<Executor>::executor_,
                                      std::chrono::nanoseconds(1));
        delay_timer.async_wait([&](const boost::system::error_code&) {
            BookEventGetter3<Executor>::signal_.emit(
                boost::asio::cancellation_type::all);
        });
    }
        std::string_view GetName() const override{
        return BookEventGetterComponent<Executor>::name_component_;
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

    class ArgsOrder : public ArgsBody {
      public:
        using SymbolType = std::string_view;
        using Limit      = uint64_t;
        explicit ArgsOrder(const Exchange::RequestSnapshot* request_snapshot,
                           common::TradingPairHashMap& pairs)
            : ArgsBody() {
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
    class ArgsBody : public bybit::ArgsBody {
        common::TradingPairHashMap& pairs_;
        /**
         * @brief id_ need add when send json request to exchange
         *
         */
        unsigned int id_;

      public:
        ArgsBody(const Exchange::RequestSnapshot* request_snapshot,
                 common::TradingPairHashMap& pairs)
            : bybit::ArgsBody(), pairs_(pairs) {
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

template <typename Executor>
class BookSnapshot2 : public inner::BookSnapshotI {
    SignerI* signer_;
    ::V2::ConnectionPool<HTTPSesionType3>* session_pool_;
    common::TradingPairHashMap& pairs_;
    // Add a callback map to store parsing callbacks for each trading pair
    std::unordered_map<common::TradingPair, const OnHttpsResponce*,
                       common::TradingPairHash, common::TradingPairEqual>
        callback_map_;

  protected:
    Executor executor_;
    boost::asio::cancellation_signal& signal_;

  public:
    explicit BookSnapshot2(Executor&& executor, SignerI* signer,
                           ::V2::ConnectionPool<HTTPSesionType3>* session_pool,
                           TypeExchange type, common::TradingPairHashMap& pairs,
                           boost::asio::cancellation_signal& signal)
        : executor_(std::move(executor)),
          signer_(signer),
          session_pool_(session_pool),
          pairs_(pairs),
          signal_(signal) {
        switch (type) {
            case TypeExchange::MAINNET:
                current_exchange_ = &bybit_main_net_;
                break;
            default:
                current_exchange_ = &bybit_test_net_;
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
                detail::FactoryRequestJson factory{
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

                auto slot = signal_.slot();
                if (auto status = co_await session->AsyncRequest(
                        std::move(req), callback, slot);
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
    bybit::testnet::HttpsExchange bybit_test_net_;
    bybit::mainnet::HttpsExchange bybit_main_net_;

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
        ::V2::ConnectionPool<HTTPSesionType3>* session_pool,
        boost::asio::cancellation_signal& signal)
        : BookSnapshot2<Executor>(std::move(executor), signer, session_pool,
                                  type, pairs, signal),
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
        net::steady_timer delay_timer(BookSnapshot2<Executor>::executor_,
                                      std::chrono::nanoseconds(1));
        delay_timer.async_wait([&](const boost::system::error_code&) {
            BookSnapshot2<Executor>::signal_.emit(
                boost::asio::cancellation_type::all);
        });
    }
};

template <typename Executor>
class BidAskGeneratorComponent : public bus::Component {
  public:
    using SnapshotCallback = std::function<void(const Exchange::BookSnapshot&)>;
    using DiffCallback =
        std::function<void(const Exchange::BookDiffSnapshot2&)>;

  private:
    static constexpr std::string_view name_component_ = "bybit::BidAskGeneratorComponent";  
    BidAskGeneratorComponent::SnapshotCallback snapshot_callback_ = nullptr;
    BidAskGeneratorComponent::DiffCallback diff_callback_         = nullptr;
    std::unordered_map<common::TradingPair, BidAskState,
                       common::TradingPairHash, common::TradingPairEqual>
        state_map_;
    std::unordered_map<common::TradingPair, BusEventRequestBBOPrice,
                       common::TradingPairHash, common::TradingPairEqual>
        request_bbo_;

    Executor executor_;
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
        Executor&& executor, aot::CoBus& bus,
        const unsigned int number_snapshots, const unsigned int number_diff,
        const unsigned int max_number_event_per_tick)
        : executor_(std::move(executor)),
          bus_(bus),
          request_bus_event_snapshot_mem_pool_(number_snapshots),
          request_snapshot_mem_pool_(number_snapshots),
          request_bus_event_diff_mem_pool_(number_diff),
          request_diff_mem_pool_(number_diff),
          out_diff_mem_pool_(max_number_event_per_tick),
          out_bus_event_diff_mem_pool_(max_number_event_per_tick) {};

    void AsyncHandleEvent(
        Exchange::BusEventResponseNewSnapshot* event) override {
        boost::asio::co_spawn(executor_, HandleNewSnapshotEvent(event),
                              boost::asio::detached);
    };
    void AsyncHandleEvent(Exchange::BusEventBookDiffSnapshot* event) override {
        boost::asio::co_spawn(executor_, HandleBookDiffSnapshotEvent(event),
                              boost::asio::detached);
    };
    void AsyncHandleEvent(
        boost::intrusive_ptr<BusEventRequestBBOPrice> event) override {
        boost::asio::co_spawn(executor_, HandleTrackingNewTradingPair(event),
                              boost::asio::detached);
    };
    void AsyncStop() override {};

    void RegisterSnapshotCallback(
        BidAskGeneratorComponent::SnapshotCallback callback) {
        snapshot_callback_ = std::move(callback);
    }

    // External API to register diff callback
    void RegisterDiffCallback(BidAskGeneratorComponent::DiffCallback callback) {
        diff_callback_ = std::move(callback);
    }
    std::string_view GetName() const override{
        return BidAskGeneratorComponent<Executor>::name_component_;
    };
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
        Exchange::BusEventResponseNewSnapshot* event) {
        // Extract the exchange ID from the event.
        const auto& exchange_id = event->WrappedEvent()->exchange_id;

        // Validate the exchange ID; this component only supports Binance.
        if (exchange_id != common::ExchangeId::kBybit) {
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
        state.last_id_diff_book_event = state.snapshot.lastUpdateId;

        // End the coroutine since the snapshot update is complete.
        co_return;
    }
    boost::asio::awaitable<void> HandleBookDiffSnapshotEvent(
        Exchange::BusEventBookDiffSnapshot* event) {
        //auto diff_intr = event->WrappedEventIntrusive();
        //auto& diff = *diff_intr.get();
        auto& diff = *event->WrappedEvent();

        logi("[DIFF ACCEPTED] {}, Diff Last ID Range: [{}]",
             diff.trading_pair.ToString(), diff.last_id);

        const auto& exchange_id = diff.exchange_id;
        if (exchange_id != common::ExchangeId::kBybit) {
            loge("[UNSUPPORTED EXCHANGE] Exchange ID: {}", exchange_id);
            co_return;
        }

        auto trading_pair = diff.trading_pair;
        auto& state       = state_map_[trading_pair];
        logi("[CURRENT SNAPSHOT] {}, Snapshot LastUpdateId: {}",
             trading_pair.ToString(), state.snapshot.lastUpdateId);

        // Check for packet loss
        state.diff_packet_lost =
            (diff.last_id != state.last_id_diff_book_event + 1);
        state.last_id_diff_book_event = diff.last_id;

        if (state.diff_packet_lost) {
            logw(
                "[PACKET LOSS] {}, Expected last ID: {}, Actual "
                "First ID: {}",
                trading_pair.ToString(), state.last_id_diff_book_event + 1,
                diff.last_id);
                std::exit(1);
        }

        if (state.need_make_snapshot) {
            const bool snapshot_and_diff_now_sync =
                (diff.last_id == state.snapshot.lastUpdateId + 1);
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
        // const bool need_snapshot =
        //     (state.need_make_snapshot || state.diff_packet_lost);
        // if (need_snapshot) {
        //     if (diff.last_id <= state.snapshot.lastUpdateId) {
        //         logw(
        //             "[OUTDATED DIFF] {}, Diff Last ID: {}, "
        //             "Snapshot LastUpdateId: {}. Awaiting newer diff.",
        //             trading_pair.ToString(), diff.last_id,
        //             state.snapshot.lastUpdateId);
        //         co_return;
        //     }

        //     logd(
        //         "[REQUESTING NEW SNAPSHOT] {}, Current Snapshot "
        //         "LastUpdateId: {}",
        //         trading_pair.ToString(), state.snapshot.lastUpdateId);
        //     state.need_make_snapshot = true;
        //     //not sure that i need comment this
        //     //co_await RequestNewSnapshot(trading_pair);
        //     co_return;
        // }

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
                "[PROCESSING DIFF] {}, Diff ID Range: [{}], "
                "Snapshot LastUpdateId: {}",
                trading_pair.ToString(), diff.last_id,
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
    // boost::asio::awaitable<void> RequestNewSnapshot(
    //     common::TradingPair trading_pair) {
    //     if (!request_bbo_.count(trading_pair)) {
    //         loge(
    //             "can't find info to prepare request new snapshot for this "
    //             "pair:{} request_bbo.size():{}",
    //             trading_pair.ToString(), request_bbo_.size());
    //         co_return;
    //     }
    //     auto& info = request_bbo_[trading_pair];

    //     auto* ptr  = request_snapshot_mem_pool_.Allocate(
    //         &request_snapshot_mem_pool_, info.exchange_id, info.trading_pair,
    //         info.snapshot_depth);
    //     auto intr_ptr_request =
    //         boost::intrusive_ptr<Exchange::RequestSnapshot>(ptr);

    //     auto* bus_evt_request = request_bus_event_snapshot_mem_pool_.Allocate(
    //         &request_bus_event_snapshot_mem_pool_, intr_ptr_request);
    //     auto intr_ptr_bus_event_request =
    //         boost::intrusive_ptr<Exchange::BusEventRequestNewSnapshot>(
    //             bus_evt_request);

    //     bus_.AsyncSend(this, intr_ptr_bus_event_request);
    //     co_return;
    // }
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
            info.snapshot_depth);
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
};  // namespace bybit