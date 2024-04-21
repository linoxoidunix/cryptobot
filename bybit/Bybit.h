#pragma once
#include <bybit/Exchange.h>
#include <bybit/OHLCV.h>

#include <string_view>

namespace bybit {
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

class KLineStreamBybit : public KLineStream {
  public:
    explicit KLineStreamBybit(const Symbol& s,
                              const ChartInterval* chart_interval)
        : symbol_(s), chart_interval_(chart_interval){};
    std::string ToString() const override {
        return fmt::format("kline.{0}.{1}", chart_interval_->ToString(),
                           symbol_.ToString());
    };

  private:
    const Symbol& symbol_;
    const ChartInterval* chart_interval_;
};

class OHLCVIBybit : public OHLCVGetter {
  public:
    OHLCVIBybit(const Symbol& s, const ChartInterval* chart_interval):
    s_(s), chart_interval_(chart_interval_){};
    OHLCVI Get(boost::beast::flat_buffer& buffer) override{
      return {};
    };
  private:
    const Symbol& s_;
    const ChartInterval* chart_interval_;
};
};  // namespace bybit