#pragma once
#include <bybit/Exchange.h>
#include <bybit/OHLCV.h>
#include <bybit/third_party/fmt/core.h>
#include <string_view>

namespace bybit {
  class Symbol : public SymbolI {
  public:
    explicit Symbol(std::string_view first, std::string_view second)
        : first_(first.data()), second_(second.data()) {};
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
    explicit KLineStream(const Symbol* s,
                              const ChartInterval* chart_interval)
        : symbol_(s), chart_interval_(chart_interval){};
    std::string ToString() const override {
        return fmt::format("kline.{0}.{1}", chart_interval_->ToString(),
                           symbol_->ToString());
    };

  private:
    const Symbol* symbol_;
    const ChartInterval* chart_interval_;
};

class OHLCVI : public OHLCVGetter {
  public:
    OHLCVI(const Symbol* s, const ChartInterval* chart_interval)
        : s_(s), chart_interval_(chart_interval){};
    void Get(OHLCVIStorage& buffer) override {
        net::io_context ioc;
        //fmtlog::setLogFile("log", true);
        fmtlog::setLogLevel(fmtlog::DBG);

        std::function<void(boost::beast::flat_buffer & buffer)> OnMessageCB;
        OnMessageCB = [](boost::beast::flat_buffer& buffer) {
            auto resut = boost::beast::buffers_to_string(buffer.data());
            logi("{}", resut);
            fmtlog::poll();
        };

        using kls = KLineStream;
        Symbol btcusdt("BTC", "USDT");
        auto chart_interval = m1();
        kls channel(&btcusdt, &chart_interval);
        auto request_without_bracket = fmt::format("\"req_id\": \"test\",\"op\": \"subscribe\", \"args\": [\"{}\"]", channel.ToString());
        std::string request = "{"+request_without_bracket + "}";
        std::make_shared<WS>(ioc, request, OnMessageCB)->Run("stream-testnet.bybit.com", "443","/v5/public/spot");
        ioc.run();
    };

  private:
    const Symbol* s_;
    const ChartInterval* chart_interval_;
};
};  // namespace bybit