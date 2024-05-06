#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/version.hpp>
#include <string>
#include <string_view>

#include "aot/Exchange.h"
#include "aot/Https.h"
#include "aot/Logger.h"
#include "aot/WS.h"
namespace binance {

namespace testnet {
class HttpsExchange : public https::ExchangeI {
  public:
    explicit HttpsExchange() = default;
    virtual ~HttpsExchange() = default;
    std::string_view Host() const override {
        return std::string_view("testnet.binance.vision");
    };
    std::string_view Port() const override { return "443"; };
};  // namespace https
};  // namespace testnet

enum class Side { BUY, SELL };
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
    }
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
            // auto resut = boost::beast::buffers_to_string(buffer.data());
            // logi("{}", resut);
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

class OrderNew : public inner::OrderNewI {
  public:
    class ArgsI {
      public:
        virtual void SetSymbol(SymbolI* symbol) = 0;
        virtual void SetSide(Side side)         = 0;
        virtual void SetType(Type type)         = 0;
        /**
         * @brief return query string starts with ?
         *
         * @return std::string
         */
        virtual std::string QueryString()       = 0;
    };
    class Args : public std::unordered_map<std::string, std::string>,
                 public ArgsI {
      public:
        explicit Args(SymbolI* symbol, Side side, Type type) {
            SetSymbol(symbol);
            SetSide(side);
            SetType(type);
        };
        void SetSymbol(SymbolI* symbol) override {
            insert({"symbol", symbol->ToString()});
        };
        void SetSide(Side side) override {
            auto& storage = *this;
            switch (side) {
                case Side::BUY:
                    storage["side"] = "BUY";
                    break;
                case Side::SELL:
                    storage["side"] = "SELL";
                    break;
            }
        };
        void SetType(Type type) override {
            auto& storage = *this;
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
        std::string QueryString() override {
            std::list<std::string> merged;
            std::for_each(begin(), end(), [&merged](const auto& expr) {
                merged.emplace_back(
                    fmt::format("{}={}", expr.first, expr.second));
            });
            auto out = boost::algorithm::join(merged, "&");
            if (!out.empty()) out.insert(out.begin(), '?');
            return out;
        }
    };
    class FactoryPostRequest {
      public:
        explicit FactoryPostRequest(const https::ExchangeI* exchange,
                                    std::string_view target, Args&& args)
            : exchange_(exchange), args_(args){target_ = target.data() + args_.QueryString();};
        boost::beast::http::request<boost::beast::http::empty_body>
        operator()() {
            boost::beast::http::request<boost::beast::http::empty_body> req;
            req.version(11);
            req.method(boost::beast::http::verb::get);
            req.target(target_.data());
            req.set(boost::beast::http::field::host, exchange_->Host().data());
            req.set(boost::beast::http::field::user_agent,
                    BOOST_BEAST_VERSION_STRING);
            return req;
        };
        std::string_view Host() const { return exchange_->Host(); };
        std::string_view Port() const { return exchange_->Port(); };
        std::string_view Target() const { return target_; }

      private:
        const https::ExchangeI* exchange_;
        std::string target_;
        Args args_;
    };

  public:
    explicit OrderNew(FactoryPostRequest* factory) : factory_(factory){};
    void Exec() override {
        boost::asio::io_context ioc;
        // fmtlog::setLogFile("log", true);
        fmtlog::setLogLevel(fmtlog::DBG);

        OnHttpsResponce cb;
        cb = [](boost::beast::http::response<boost::beast::http::string_body>&
                    buffer) {
            const auto& resut = buffer.body();
            logi("{}", resut);
            fmtlog::poll();
        };
        auto& test = *factory_;
        std::make_shared<Https>(ioc, cb)->Run(
            factory_->Host().data(), factory_->Port().data(),
            factory_->Target().data(), test());
        ioc.run();
    }

  private:
    FactoryPostRequest* factory_;
};
};  // namespace binance