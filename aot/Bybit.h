#pragma once
#include <openssl/hmac.h>

#include <boost/beast/http.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include "aot/Exchange.h"
#include "aot/Https.h"
#include "aot/Logger.h"
#include "aot/WS.h"

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
        : recv_window_(recv_window){};

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

class Symbol : public SymbolI {
  public:
    explicit Symbol(std::string_view first, std::string_view second)
        : first_(first.data()), second_(second.data()){};
    std::string ToString() const override {
        auto out = fmt::format("{0}{1}", first_, second_);
        boost::algorithm::to_upper(out);
        return out;
    }

  private:
    std::string first_;
    std::string second_;
};
class m1 : public ChartInterval {
  public:
    explicit m1() = default;
    std::string ToString() const override { return "1"; }
};
class m3 : public ChartInterval {
  public:
    explicit m3() = default;
    std::string ToString() const override { return "3"; }
};
class m5 : public ChartInterval {
  public:
    explicit m5() = default;
    std::string ToString() const override { return "5"; }
};
class m15 : public ChartInterval {
  public:
    explicit m15() = default;
    std::string ToString() const override { return "15"; }
};
class m60 : public ChartInterval {
  public:
    explicit m60() = default;
    std::string ToString() const override { return "30"; }
};
class m120 : public ChartInterval {
  public:
    explicit m120() = default;
    std::string ToString() const override { return "120"; }
};
class m240 : public ChartInterval {
  public:
    explicit m240() = default;
    std::string ToString() const override { return "240"; }
};
class m360 : public ChartInterval {
  public:
    explicit m360() = default;
    std::string ToString() const override { return "360"; }
};
class m720 : public ChartInterval {
  public:
    explicit m720() = default;
    std::string ToString() const override { return "720"; }
};
class D1 : public ChartInterval {
  public:
    explicit D1() = default;
    std::string ToString() const override { return "D"; }
};
class W1 : public ChartInterval {
  public:
    explicit W1() = default;
    std::string ToString() const override { return "W"; }
};
class M1 : public ChartInterval {
  public:
    explicit M1() = default;
    std::string ToString() const override { return "M"; }
};

class KLineStream : public KLineStreamI {
  public:
    explicit KLineStream(const Symbol* s, const ChartInterval* chart_interval)
        : symbol_(s), chart_interval_(chart_interval){};
    std::string ToString() const override {
        return fmt::format("kline.{0}.{1}", chart_interval_->ToString(),
                           symbol_->ToString());
    };

  private:
    const Symbol* symbol_;
    const ChartInterval* chart_interval_;
};

class ParserKLineResponse : public ParserKLineResponseI {
  public:
    explicit ParserKLineResponse() = default;
    OHLCVI Get(std::string_view response_from_exchange) const override {
        return {};
    };
};

class OHLCVI : public OHLCVGetter {
  public:
    OHLCVI(const Symbol* s, const ChartInterval* chart_interval)
        : s_(s), chart_interval_(chart_interval){};
    void Get(OHLCVIStorage& buffer) override {
        boost::asio::io_context ioc;
        // fmtlog::setLogFile("log", true);
        std::function<void(boost::beast::flat_buffer & buffer)> OnMessageCB;
        OnMessageCB = [](boost::beast::flat_buffer& buffer) {
            auto resut = boost::beast::buffers_to_string(buffer.data());
            logi("{}", resut);
            fmtlog::poll();
        };

        using kls = KLineStream;
        kls channel(s_, chart_interval_);
        auto request_without_bracket = fmt::format(
            "\"req_id\": \"test\",\"op\": \"subscribe\", \"args\": [\"{}\"]",
            channel.ToString());
        std::string request = "{" + request_without_bracket + "}";
        std::make_shared<WS>(ioc, request, OnMessageCB)
            ->Run("stream-testnet.bybit.com", "443", "/v5/public/spot");
        ioc.run();
    };

  private:
    const Symbol* s_;
    const ChartInterval* chart_interval_;
};

class Args : public std::unordered_map<std::string, std::string> {};
class ArgsBody : public Args {
  public:
    explicit ArgsBody() : Args(){};
    /**
     * @brief return query string starts with ?
     *
     * @return std::string
     */
    virtual std::string Body();

    virtual std::string FinalQueryString() { return {}; };
    virtual ~ArgsBody() = default;
};

namespace detail {
class FactoryRequest {
  public:
    explicit FactoryRequest(const https::ExchangeI* exchange,
                            std::string_view end_point, ArgsBody args,
                            boost::beast::http::verb action, SignerI* signer,
                            bool need_sign = false)
        : exchange_(exchange),
          end_point_(end_point.data()),
          args_(args),
          action_(action),
          signer_(signer),
          need_sign_(need_sign){};
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
        req.insert("X-BAPI-API-KEY", signer_->ApiKey());
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
        std::cout << req << "\n";
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
    std::string AddSignParams (
        boost::beast::http::request<boost::beast::http::string_body>& req) const{
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
    void AddSignature (
        std::string_view signature,
        boost::beast::http::request<boost::beast::http::string_body>& req) const{
        req.insert("X-BAPI-SIGN", signature.data());
        req.insert("X-BAPI-SIGN-TYPE", "2");
    };

  private:
    const https::ExchangeI* exchange_;
    std::string end_point_;
    ArgsBody args_;
    boost::beast::http::verb action_;
    SignerI* signer_;
    bool need_sign_;
};
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
    static constexpr std::string_view end_point = "/v5/order/create";

  public:
    class ArgsOrder : public ArgsBody {
      public:
        using SymbolType = std::string_view;
        explicit ArgsOrder(SymbolType symbol, double quantity, double price,
                           TimeInForce time_in_force, Side side, Type type)
            : ArgsBody() {
            storage["category"] = "spot";
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
                    storage["side"] = "Buy";
                    break;
                case Side::SELL:
                    storage["side"] = "Sell";
                    break;
            }
        };
        void SetType(Type type) {
            switch (type) {
                case Type::LIMIT:
                    storage["orderType"] = "Limit";
                    break;
                case Type::MARKET:
                    storage["orderType"] = "Market";
                    break;
                default:
                    storage["orderType"] = "Limit";
            }
        };
        void SetQuantity(double quantity) {
            storage["qty"] = fmt::format(
                "{:.{}f}", formatter_qty_.Format(storage["symbol"], quantity),
                3);
        };
        void SetPrice(double price) {
            storage["price"] = fmt::format(
                "{:.{}f}", formatter_price_.Format(storage["symbol"], price),
                0);
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
                case TimeInForce::POST_ONLY:
                    storage["timeInForce"] = "PostOnly";
                    break;
                default:
                    storage["timeInForce"] = "PostOnly";
            }
        };

      private:
        ArgsOrder& storage = *this;

        [[no_unique_address]] detail::FormatterQty formatter_qty_;
        [[no_unique_address]] detail::FormatterPrice formatter_price_;
    };

  public:
    explicit OrderNewLimit(ArgsOrder&& args, SignerI* signer, TypeExchange type)
        : args_(std::move(args)), signer_(signer) {
        switch (type) {
            case TypeExchange::MAINNET:
                current_exchange_ = &testnet_exchange;
                break;
            case TypeExchange::TESTNET:
                current_exchange_ = &testnet_exchange;
                break;
            default:
                current_exchange_ = &testnet_exchange;
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
    testnet::HttpsExchange testnet_exchange;
    https::ExchangeI* current_exchange_;
    SignerI* signer_;
};
};  // namespace bybit