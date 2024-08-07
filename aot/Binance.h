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
#include "aot/market_data/market_update.h"

// Spot API URL                               Spot Test Network URL
// https://api.binance.com/api https://testnet.binance.vision/api
// wss://stream.binance.com:9443/ws	         wss://testnet.binance.vision/ws
// wss://stream.binance.com:9443/stream	     wss://testnet.binance.vision/stream

namespace binance {
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
class Symbol : public SymbolI {
  public:
    explicit Symbol(std::string_view first, std::string_view second)
        : first_(first.data()), second_(second.data()) {};
    std::string ToString() const override {
        auto out = fmt::format("{0}{1}", first_, second_);
        boost::algorithm::to_lower(out);
        return out;
    };
    ~Symbol() override = default;

  private:
    std::string first_;
    std::string second_;
};
class s1 : public ChartInterval {
  public:
    explicit s1() = default;
    std::string ToString() const override { return "1s"; };
    uint Seconds() const override { return 1; };
};
class m1 : public ChartInterval {
  public:
    explicit m1() = default;
    std::string ToString() const override { return "1m"; };
    uint Seconds() const override { return 60; };
};
class m3 : public ChartInterval {
  public:
    explicit m3() = default;
    std::string ToString() const override { return "3m"; };
    uint Seconds() const override { return 180; };
};
class m5 : public ChartInterval {
  public:
    explicit m5() = default;
    std::string ToString() const override { return "5m"; }
    uint Seconds() const override { return 300; };
};
class m15 : public ChartInterval {
  public:
    explicit m15() = default;
    std::string ToString() const override { return "15m"; }
    uint Seconds() const override { return 900; };
};
class m30 : public ChartInterval {
  public:
    explicit m30() = default;
    std::string ToString() const override { return "30m"; }
    uint Seconds() const override { return 1800; };
};
class h1 : public ChartInterval {
  public:
    explicit h1() = default;
    std::string ToString() const override { return "1h"; }
    uint Seconds() const override { return 3600; };
};
class h2 : public ChartInterval {
  public:
    explicit h2() = default;
    std::string ToString() const override { return "2h"; }
    uint Seconds() const override { return 7200; };
};
class h4 : public ChartInterval {
  public:
    explicit h4() = default;
    std::string ToString() const override { return "4h"; }
    uint Seconds() const override { return 14400; };
};
class h6 : public ChartInterval {
  public:
    explicit h6() = default;
    std::string ToString() const override { return "6h"; }
    uint Seconds() const override { return 21600; };
};
class h8 : public ChartInterval {
  public:
    explicit h8() = default;
    std::string ToString() const override { return "8h"; }
    uint Seconds() const override { return 28800; };
};
class h12 : public ChartInterval {
  public:
    explicit h12() = default;
    std::string ToString() const override { return "12h"; }
    uint Seconds() const override { return 43200; };
};
class d1 : public ChartInterval {
  public:
    explicit d1() = default;
    std::string ToString() const override { return "1d"; }
    uint Seconds() const override { return 86400; };
};
class d3 : public ChartInterval {
  public:
    explicit d3() = default;
    std::string ToString() const override { return "3d"; }
    uint Seconds() const override { return 259200; };
};
class w1 : public ChartInterval {
  public:
    explicit w1() = default;
    std::string ToString() const override { return "1w"; }
    uint Seconds() const override { return 604800; };
};
class M1 : public ChartInterval {
  public:
    explicit M1() = default;
    std::string ToString() const override { return "1M"; }
    uint Seconds() const override { return 2.628e6; };
};
class KLineStream : public KLineStreamI {
  public:
    explicit KLineStream(const SymbolI* s, const ChartInterval* chart_interval)
        : symbol_(s), chart_interval_(chart_interval) {};
    std::string ToString() const override {
        return fmt::format("{0}@kline_{1}", symbol_->ToString(),
                           chart_interval_->ToString());
    };
    ~KLineStream() override = default;

  private:
    const SymbolI* symbol_;
    const ChartInterval* chart_interval_;
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

    explicit DiffDepthStream(const SymbolI* s, const StreamIntervalI* interval)
        : symbol_(s), interval_(interval) {};
    std::string ToString() const override {
        return fmt::format("{0}@depth@{1}", symbol_->ToString(),
                           interval_->ToString());
    };

  private:
    const SymbolI* symbol_;
    const StreamIntervalI* interval_;
};

class OHLCVI : public OHLCVGetter {
    class ParserResponse {
      public:
        explicit ParserResponse() = default;
        OHLCVExt Parse(std::string_view response);
    };

  public:
    OHLCVI(const Symbol* s, const ChartInterval* chart_interval,
           TypeExchange type_exchange)
        : current_exchange_(exchange_.Get(type_exchange)),
          s_(s),
          chart_interval_(chart_interval),
          type_exchange_(type_exchange) {};
    void LaunchOne() override { ioc.run_one(); };

    void Init(OHLCVILFQueue& lf_queue) override {
        std::function<void(boost::beast::flat_buffer & buffer)> OnMessageCB;
        OnMessageCB = [&lf_queue](boost::beast::flat_buffer& buffer) {
            auto result = boost::beast::buffers_to_string(buffer.data());
            ParserResponse parser;
            lf_queue.try_enqueue(parser.Parse(result));
            // logi("{}", result);
            // fmtlog::poll();
        };

        using kls = KLineStream;
        kls channel(s_, chart_interval_);
        std::string empty_request = "{}";
        std::make_shared<WS>(ioc, empty_request, OnMessageCB)
            ->Run(current_exchange_->Host(), current_exchange_->Port(),
                  fmt::format("/ws/{0}", channel.ToString()));
    };

  private:
    boost::asio::io_context ioc;
    ExchangeChooser exchange_;
    https::ExchangeI* current_exchange_ = nullptr;
    const Symbol* s_;
    const ChartInterval* chart_interval_;
    TypeExchange type_exchange_;
};

class BookEventGetter : public BookEventGetterI {
    class ParserResponse {
      public:
        explicit ParserResponse() = default;
        Exchange::BookDiffSnapshot Parse(std::string_view response);
    };

  public:
    BookEventGetter(const SymbolI* s,
                    const DiffDepthStream::StreamIntervalI* interval,
                    TypeExchange type_exchange)
        : current_exchange_(exchange_.Get(type_exchange)),
          s_(s),
          interval_(interval),
          type_exchange_(type_exchange) {};
    void Get() override {};
    void LaunchOne() override { ioc.run_one(); };
    void Init(Exchange::BookDiffLFQueue& queue) override {
        std::function<void(boost::beast::flat_buffer & buffer)> OnMessageCB;
        OnMessageCB = [&queue](boost::beast::flat_buffer& buffer) {
            auto resut = boost::beast::buffers_to_string(buffer.data());
            // logi("{}", resut);
            ParserResponse parser;
            auto answer = parser.Parse(resut);
            queue.enqueue(answer);
        };

        using dds = DiffDepthStream;
        dds channel(s_, interval_);
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
    const SymbolI* s_;
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
        if (signer_) req.insert("X-MBX-APIKEY", signer_->ApiKey());
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
};
/**
 * @brief in future read property from file
 * now - just copy qty from input
 */
class FormatterQty {
  public:
    explicit FormatterQty() = default;
    double Format(std::string_view symbol, double qty) { return qty; };
};

class FormatterPrice {
  public:
    explicit FormatterPrice() = default;
    double Format(std::string_view symbol, double price) { return price; };
};
};  // namespace detail

class OrderNewLimit : public inner::OrderNewI {
    static constexpr std::string_view end_point = "/api/v3/order";

  public:
    class ParserResponse {
      public:
        explicit ParserResponse() = default;
        Exchange::MEClientResponse Parse(std::string_view response);
    };
    class ArgsOrder : public ArgsQuery {
      public:
        using SymbolType = std::string_view;
        explicit ArgsOrder(SymbolType symbol, double quantity, double price,
                           TimeInForce time_in_force, Common::Side side,
                           Type type)
            : ArgsQuery() {  // TODO UNUSED. NEED USE ONLY CTOR WITH
                             // Exchange::RequestNewOrder*
            SetSymbol(symbol);
            SetSide(side);
            SetType(type);
            SetQuantity(quantity);
            SetPrice(price);
            SetTimeInForce(time_in_force);
        };
        explicit ArgsOrder(Exchange::RequestNewOrder* new_order) : ArgsQuery() {
            SetSymbol(new_order->ticker);
            SetSide(new_order->side);
            SetType(Type::LIMIT);
            SetQuantity(new_order->qty);
            SetPrice(new_order->price);
            SetTimeInForce(TimeInForce::GTC);
            SetOrderId(new_order->order_id);
        };

      private:
        void SetSymbol(SymbolType symbol) {
            SymbolUpperCase formatter(symbol.data());
            storage["symbol"] = formatter.ToString();
        };
        void SetSide(Common::Side side) {
            switch (side) {
                using enum Common::Side;
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
        void SetQuantity(double quantity) {
            storage["quantity"] = std::to_string(
                formatter_qty_.Format(storage["symbol"], quantity));
        };
        void SetPrice(double price) {
            storage["price"] = std::to_string(
                formatter_price_.Format(storage["symbol"], price));
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
        void SetOrderId(Common::OrderId order_id) {
            if (order_id != Common::OrderId_INVALID) [[likely]]
                storage["newClientOrderId"] = Common::orderIdToString(order_id);
        };

      private:
        ArgsOrder& storage = *this;

        detail::FormatterQty formatter_qty_;
        detail::FormatterPrice formatter_price_;
    };

  public:
    explicit OrderNewLimit(SignerI* signer, TypeExchange type)
        : current_exchange_(exchange_.Get(type)), signer_(signer) {};
    void Exec(Exchange::RequestNewOrder* new_order,
              Exchange::ClientResponseLFQueue* response_lfqueue) override {
        ArgsOrder args(new_order);

        bool need_sign = true;
        detail::FactoryRequest factory{current_exchange_,
                                       OrderNewLimit::end_point,
                                       args,
                                       boost::beast::http::verb::post,
                                       signer_,
                                       need_sign};
        boost::asio::io_context ioc;
        OnHttpsResponce cb;
        cb = [response_lfqueue](
                 boost::beast::http::response<boost::beast::http::string_body>&
                     buffer) {
            const auto& resut = buffer.body();
            logi("{}", resut);
            fmtlog::poll();
            ParserResponse parser;
            auto answer = parser.Parse(resut);
            response_lfqueue->enqueue(answer);
            fmtlog::poll();
        };
        std::make_shared<Https>(ioc, cb)->Run(
            factory.Host().data(), factory.Port().data(),
            factory.EndPoint().data(), factory());
        ioc.run();
    };
    ~OrderNewLimit() override = default;

  private:
    ExchangeChooser exchange_;
    https::ExchangeI* current_exchange_;
    SignerI* signer_;
};

class CancelOrder : public inner::CancelOrderI {
    static constexpr std::string_view end_point = "/api/v3/order";

  public:
    class ParserResponse {
      public:
        explicit ParserResponse() = default;
        Exchange::MEClientResponse Parse(std::string_view response);
    };
    class ArgsOrder : public ArgsQuery {
      public:
        using SymbolType = std::string_view;
        explicit ArgsOrder(SymbolType symbol, Common::OrderId order_id)
            : ArgsQuery() {
            SetSymbol(symbol);
            SetOrderId(order_id);
        };
        explicit ArgsOrder(
            const Exchange::RequestCancelOrder* request_cancel_order)
            : ArgsQuery() {
            SetSymbol(request_cancel_order->ticker);
            SetOrderId(request_cancel_order->order_id);
        };

      private:
        void SetSymbol(SymbolType symbol) {
            SymbolUpperCase formatter(symbol.data());
            storage["symbol"] = formatter.ToString();
        };
        void SetOrderId(Common::OrderId order_id) {
            if (order_id != Common::OrderId_INVALID) [[likely]]
                storage["origClientOrderId"] =
                    Common::orderIdToString(order_id);
        };

      private:
        ArgsOrder& storage = *this;
    };

  public:
    explicit CancelOrder(SignerI* signer, TypeExchange type)
        : current_exchange_(exchange_.Get(type)), signer_(signer) {};
    void Exec(Exchange::RequestCancelOrder* request_cancel_order,
              Exchange::ClientResponseLFQueue* response_lfqueue) override {
        ArgsOrder args(request_cancel_order);

        bool need_sign = true;
        detail::FactoryRequest factory{current_exchange_,
                                       CancelOrder::end_point,
                                       args,
                                       boost::beast::http::verb::delete_,
                                       signer_,
                                       need_sign};
        boost::asio::io_context ioc;
        OnHttpsResponce cb;
        cb = [response_lfqueue](
                 boost::beast::http::response<boost::beast::http::string_body>&
                     buffer) {
            const auto& resut = buffer.body();
            logi("{}", resut);
            fmtlog::poll();
            ParserResponse parser;
            auto answer = parser.Parse(resut);
            response_lfqueue->enqueue(answer);
            fmtlog::poll();
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
};

class BookSnapshot : public inner::BookSnapshotI {
    static constexpr std::string_view end_point = "/api/v3/depth";
    class ParserResponse {
      public:
        explicit ParserResponse() = default;
        Exchange::BookSnapshot Parse(std::string_view response);
    };

  public:
    class ArgsOrder : public ArgsQuery {
      public:
        using SymbolType = std::string_view;
        using Limit      = uint16_t;
        explicit ArgsOrder(SymbolType symbol, Limit limit) : ArgsQuery() {
            SetSymbol(symbol);
            SetLimit(limit);
        };
        void SetSymbol(SymbolType symbol) {
            SymbolUpperCase formatter(symbol.data());
            storage["symbol"] = formatter.ToString();
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
                          Exchange::BookSnapshot* snapshot)
        : args_(std::move(args)), snapshot_(snapshot) {
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
            // logi("{}", resut);
            ParserResponse parser;
            auto answer   = parser.Parse(resut);
            answer.ticker = args_["symbol"];
            *snapshot_    = answer;
            fmtlog::poll();
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
        Exchange::EventLFQueue* event_lfqueue, const Ticker& ticker,
        const DiffDepthStream::StreamIntervalI* interval, TypeExchange type);
    auto Start() {
        run_    = true;
        thread_ = std::unique_ptr<std::thread>(common::createAndStartThread(
            -1, "Trading/GeneratorBidAskService", [this]() { Run(); }));
        ASSERT(thread_ != nullptr, "Failed to start MarketData thread.");
    };
    common::Delta GetDownTimeInS() const { return time_manager_.GetDeltaInS(); }
    ~GeneratorBidAskService() {
        stop();
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(2s);
        thread_->join();
    }
    auto stop() -> void { run_ = false; }

    GeneratorBidAskService()                                          = delete;

    GeneratorBidAskService(const GeneratorBidAskService&)             = delete;

    GeneratorBidAskService(const GeneratorBidAskService&&)            = delete;

    GeneratorBidAskService& operator=(const GeneratorBidAskService&)  = delete;

    GeneratorBidAskService& operator=(const GeneratorBidAskService&&) = delete;

  private:
    std::unique_ptr<std::thread> thread_;

    volatile bool run_                     = false;

    Exchange::EventLFQueue* event_lfqueue_ = nullptr;
    common::TimeManager time_manager_;
    size_t next_inc_seq_num_                             = 0;
    std::unique_ptr<BookEventGetterI> book_event_getter_ = nullptr;
    Exchange::BookDiffLFQueue book_diff_lfqueue_;
    Exchange::BookSnapshot snapshot_;
    const Ticker& ticker_;
    const DiffDepthStream::StreamIntervalI* interval_;
    uint64_t last_id_diff_book_event;
    TypeExchange type_exchange_;

    binance::testnet::HttpsExchange binance_test_net_;
    binance::mainnet::HttpsExchange binance_main_net_;
    https::ExchangeI* current_exchange_;

  private:
    /// Main loop for this thread - reads and processes messages from the
    /// multicast sockets - the heavy lifting is in the recvCallback() and
    /// checkSnapshotSync() methods.
    auto Run() noexcept -> void;
};
};  // namespace binance