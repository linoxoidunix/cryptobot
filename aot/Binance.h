#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast/http/message.hpp>
#include <string_view>

#include "aot/Exchange.h"
#include "aot/Https.h"
#include "aot/WS.h"
#include "aot/third_party/fmtlog.h"
namespace binance {

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

class OrderNew : public inner::OrderNewI {
  public:
    struct Data {};
    explicit OrderNew(Data data) : data_(data){};
    void Exec() override {
        boost::asio::io_context ioc;
        // fmtlog::setLogFile("log", true);
        fmtlog::setLogLevel(fmtlog::DBG);

        OnHttpsResponce cb;
        cb = [](boost::beast::http::response<boost::beast::http::string_body>&
                    buffer) {
                      auto asdasd = buffer.body().data();
            auto resut = buffer.body();//boost::beast::buffers_to_string(buffer.body().data()); 
            logi("{}",resut); 
            fmtlog::poll();
        };

        boost::beast::http::request<boost::beast::http::empty_body> request;

        std::make_shared<Https>(ioc, cb)->Run("testnet.binance.vision", "443",
                                              "/api/v3/time",
                                              std::move(request));
        ioc.run();
    }

  private:
    Data data_;
};
};  // namespace binance