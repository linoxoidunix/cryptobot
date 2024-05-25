#pragma once
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
    explicit HttpsExchange(std::uint64_t recv_window = 5000) {
        /**
         * @brief
          https://binance-docs.github.io/apidocs/spot/en/#endpoint-security-type
          It is recommended to use a small recvWindow of 5000 or less! The max
         cannot go beyond 60,000!
         *
         */
        recv_window_ = (recv_window > 60000) ? 60000 : recv_window;
    };

    virtual ~HttpsExchange() = default;
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
    explicit HttpsExchange(std::uint64_t recv_window = 5000) {
        /**
         * @brief
          https://binance-docs.github.io/apidocs/spot/en/#endpoint-security-type
          It is recommended to use a small recvWindow of 5000 or less! The max
         cannot go beyond 60,000!
         *
         */
        recv_window_ = (recv_window > 60000) ? 60000 : recv_window;
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
        : first_(first.data()), second_(second.data()){};
    std::string ToString() const override {
        auto out = fmt::format("{0}{1}", first_, second_);
        boost::algorithm::to_lower(out);
        return out;
    };
    ~Symbol() = default;

  private:
    std::string first_;
    std::string second_;
};
class s1 : public ChartInterval {
  public:
    explicit s1() = default;
    std::string ToString() const override { return "1s"; }
};
class m1 : public ChartInterval {
  public:
    explicit m1() = default;
    std::string ToString() const override { return "1m"; }
};
class m3 : public ChartInterval {
  public:
    explicit m3() = default;
    std::string ToString() const override { return "3m"; }
};
class m5 : public ChartInterval {
  public:
    explicit m5() = default;
    std::string ToString() const override { return "5m"; }
};
class m15 : public ChartInterval {
  public:
    explicit m15() = default;
    std::string ToString() const override { return "15m"; }
};
class m30 : public ChartInterval {
  public:
    explicit m30() = default;
    std::string ToString() const override { return "30m"; }
};
class h1 : public ChartInterval {
  public:
    explicit h1() = default;
    std::string ToString() const override { return "1h"; }
};
class h2 : public ChartInterval {
  public:
    explicit h2() = default;
    std::string ToString() const override { return "2h"; }
};
class h4 : public ChartInterval {
  public:
    explicit h4() = default;
    std::string ToString() const override { return "4h"; }
};
class h6 : public ChartInterval {
  public:
    explicit h6() = default;
    std::string ToString() const override { return "6h"; }
};
class h8 : public ChartInterval {
  public:
    explicit h8() = default;
    std::string ToString() const override { return "8h"; }
};
class h12 : public ChartInterval {
  public:
    explicit h12() = default;
    std::string ToString() const override { return "12h"; }
};
class d1 : public ChartInterval {
  public:
    explicit d1() = default;
    std::string ToString() const override { return "1d"; }
};
class d3 : public ChartInterval {
  public:
    explicit d3() = default;
    std::string ToString() const override { return "3d"; }
};
class w1 : public ChartInterval {
  public:
    explicit w1() = default;
    std::string ToString() const override { return "1w"; }
};
class M1 : public ChartInterval {
  public:
    explicit M1() = default;
    std::string ToString() const override { return "1M"; }
};
class KLineStream : public KLineStreamI {
  public:
    explicit KLineStream(const Symbol* s, const ChartInterval* chart_interval)
        : symbol_(s), chart_interval_(chart_interval){};
    std::string ToString() const override {
        return fmt::format("{0}@kline_{1}", symbol_->ToString(),
                           chart_interval_->ToString());
    };
    ~KLineStream() = default;

  private:
    const Symbol* symbol_;
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

    explicit DiffDepthStream(const Symbol* s, const StreamIntervalI* interval)
        : symbol_(s), interval_(interval){};
    std::string ToString() const override {
        return fmt::format("{0}@depth@{1}", symbol_->ToString(),
                           interval_->ToString());
    };

  private:
    const Symbol* symbol_;
    const StreamIntervalI* interval_;
};

class OHLCVI : public OHLCVGetter {
  public:
    OHLCVI(const Symbol* s, const ChartInterval* chart_interval)
        : s_(s), chart_interval_(chart_interval){};
    void Get(OHLCVIStorage& buffer) override {
        boost::asio::io_context ioc;
        // fmtlog::setLogFile("log", true);
        fmtlog::setLogLevel(fmtlog::DBG);

        std::function<void(boost::beast::flat_buffer & buffer)> OnMessageCB;
        OnMessageCB = [](boost::beast::flat_buffer& buffer) {
            auto resut = boost::beast::buffers_to_string(buffer.data());
            logi("{}", resut);
            fmtlog::poll();
        };

        using kls = KLineStream;
        kls channel(s_, chart_interval_);
        std::string empty_request = "{}";
        std::make_shared<WS>(ioc, empty_request, OnMessageCB)
            ->Run("stream.binance.com", "9443",
                  fmt::format("/ws/{0}", channel.ToString()));
        ioc.run();
    };

  private:
    const Symbol* s_;
    const ChartInterval* chart_interval_;
};

class BookEventGetter : public BookEventGetterI {
    class ParserResponse {
      public:
        explicit ParserResponse() = default;
        Exchange::BookDiffSnapshot Parse(std::string_view response);
    };

  public:
    BookEventGetter(const Symbol* s,
                    const DiffDepthStream::StreamIntervalI* interval,
                    TypeExchange type_exchange)
        : s_(s), interval_(interval), type_exchange_(type_exchange) {
        switch (type_exchange) {
            case TypeExchange::MAINNET:
                current_exchange_ = &binance_main_net_;
                break;
            default:
                current_exchange_ = &binance_test_net_;
                break;
        }
    };
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
        // std::make_shared<WS>(ioc, empty_request, OnMessageCB)
        //     ->Run("stream.binance.com", "9443",
        //           fmt::format("/ws/{0}", channel.ToString()));
        std::make_shared<WS>(ioc, empty_request, OnMessageCB)
            ->Run(current_exchange_->Host(), current_exchange_->Port(),
                  fmt::format("/ws/{0}", channel.ToString()));
    };

    ~BookEventGetter() override = default;

  private:
    boost::asio::io_context ioc;
    const Symbol* s_;
    const DiffDepthStream::StreamIntervalI* interval_;
    bool callback_execute_ = false;
    binance::testnet::HttpsExchange binance_test_net_;
    binance::mainnet::HttpsExchange binance_main_net_;
    https::ExchangeI* current_exchange_;
    TypeExchange type_exchange_;
};

class Args : public std::unordered_map<std::string, std::string> {};

class ArgsQuery : public Args {
  public:
    explicit ArgsQuery() : Args(){};
    /**
     * @brief return query string starts with ?
     *
     * @return std::string
     */
    virtual std::string QueryString() {
        std::list<std::string> merged;
        std::for_each(begin(), end(), [&merged](const auto& expr) {
            merged.emplace_back(fmt::format("{}={}", expr.first, expr.second));
        });
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
                            std::string_view end_point, ArgsQuery args,
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
    class ArgsOrder : public ArgsQuery {
      public:
        using SymbolType = std::string_view;
        explicit ArgsOrder(SymbolType symbol, double quantity, double price,
                           TimeInForce time_in_force, Side side, Type type)
            : ArgsQuery() {
            SetSymbol(symbol);
            SetSide(side);
            SetType(type);
            SetQuantity(quantity);
            SetPrice(price);
            SetTimeInForce(time_in_force);
        };
        void SetSymbol(SymbolType symbol) {
            SymbolUpperCase formatter(symbol.data());
            storage["symbol"] = formatter.ToString();
        };
        void SetSide(Side side) {
            switch (side) {
                case Side::BUY:
                    storage["side"] = "BUY";
                    break;
                case Side::SELL:
                    storage["side"] = "SELL";
                    break;
            }
        };
        void SetType(Type type) {
            switch (type) {
                case Type::LIMIT:
                    storage["type"] = "LIMIT";
                    break;
                case Type::MARKET:
                    storage["type"] = "MARKET";
                    break;
                case Type::STOP_LOSS:
                    storage["type"] = "STOP_LOSS";
                    break;
                case Type::STOP_LOSS_LIMIT:
                    storage["type"] = "STOP_LOSS_LIMIT";
                    break;
                case Type::TAKE_PROFIT:
                    storage["type"] = "TAKE_PROFIT";
                    break;
                case Type::TAKE_PROFIT_LIMIT:
                    storage["type"] = "TAKE_PROFIT_LIMIT";
                    break;
                case Type::LIMIT_MAKER:
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

      private:
        ArgsOrder& storage = *this;

        detail::FormatterQty formatter_qty_;
        detail::FormatterPrice formatter_price_;
    };

  public:
    explicit OrderNewLimit(ArgsOrder&& args, SignerI* signer, TypeExchange type)
        : args_(args), signer_(signer) {
        switch (type) {
            case TypeExchange::MAINNET:
                current_exchange_ = &binance_test_net_;
                break;
            default:
                current_exchange_ = &binance_test_net_;
                break;
        }
    };
    void Exec() override {
        bool need_sign = true;
        detail::FactoryRequest factory{current_exchange_,
                                       OrderNewLimit::end_point,
                                       args_,
                                       boost::beast::http::verb::post,
                                       signer_,
                                       need_sign};
        boost::asio::io_context ioc;
        fmtlog::setLogLevel(fmtlog::DBG);

        OnHttpsResponce cb;
        cb = [](boost::beast::http::response<boost::beast::http::string_body>&
                    buffer) {
            const auto& resut = buffer.body();
            logi("{}", resut);
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
        : args_(args), snapshot_(snapshot) {
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
        fmtlog::setLogLevel(fmtlog::DBG);

        OnHttpsResponce cb;
        cb = [this](
                 boost::beast::http::response<boost::beast::http::string_body>&
                     buffer) {
            const auto& resut = buffer.body();
            // logi("{}", resut);
            ParserResponse parser;
            auto answer = parser.Parse(resut);
            *snapshot_  = answer;
            // logd("lastUpdateId:{}", answer.lastUpdateId);

            // logd("asks:");

            // for (auto it : answer.asks) {
            //     logd("{}", it.ToString());
            // }

            // logd("bids:");

            // for (auto it : answer.bids) {
            //     logd("{}", it.ToString());
            // }
            // s
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
    // MEMarketUpdateLFQueue& queue_;
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
        Exchange::EventLFQueue* event_lfqueue, const Symbol* s,
        const DiffDepthStream::StreamIntervalI* interval, TypeExchange type);
    auto Start() {
        run_ = true;
        ASSERT(
            common::createAndStartThread(-1, "Trading/GeneratorBidAskService",
                                         [this]() { Run(); }) != nullptr,
            "Failed to start MarketData thread.");
    };
    common::Delta GetDownTimeInS() { return time_manager_.GetDeltaInS(); }
    ~GeneratorBidAskService() {
        stop();
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(2s);
    }
    auto stop() -> void { run_ = false; }

    GeneratorBidAskService()                                          = delete;

    GeneratorBidAskService(const GeneratorBidAskService&)             = delete;

    GeneratorBidAskService(const GeneratorBidAskService&&)            = delete;

    GeneratorBidAskService& operator=(const GeneratorBidAskService&)  = delete;

    GeneratorBidAskService& operator=(const GeneratorBidAskService&&) = delete;

  private:
    volatile bool run_                     = false;

    Exchange::EventLFQueue* event_lfqueue_ = nullptr;
    common::TimeManager time_manager_;
    size_t next_inc_seq_num_                             = 0;
    std::unique_ptr<BookEventGetterI> book_event_getter_ = nullptr;
    Exchange::BookDiffLFQueue book_diff_lfqueue_;
    Exchange::BookSnapshot snapshot_;
    const Symbol* symbol_;
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