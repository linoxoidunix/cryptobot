

#include <prometheus/exposer.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>

#include <magic_enum.hpp>
#include <memory>

#include "aot/common/thread_utils.h"
#include "aot/common/time_utils.h"
#include "aot/prometheus/event.h"

namespace prometheus {
class Service {
  public:
    explicit Service(std::string_view host, unsigned int port,
                     EventLFQueue* event_lfqueue)
        : host_(host),
          port_(port),
          event_lfqueue_(event_lfqueue),
          exposer_(fmt::format("{}:{}", host, port)) {
        exposer_.RegisterCollectable(registry_);
    };
    ~Service() {
        run_ = false;
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
        if (thread_) [[likely]]
            thread_->join();
    };
    auto Start() -> void {
        run_    = true;
        thread_ = std::unique_ptr<std::thread>(common::createAndStartThread(
            -1, "prometheus::Service", [this] { Run(); }));
        ASSERT(thread_ != nullptr, "Failed to start prometheus::Service.");
    };
    auto Stop() -> void { run_ = false; }
    common::Delta GetDownTimeInS() const { return time_manager_.GetDeltaInS(); }

  private:
    common::TimeManager time_manager_;
    volatile bool run_ = false;
    std::unique_ptr<std::thread> thread_;

    std::string_view host_;
    unsigned int port_;
    EventLFQueue* event_lfqueue_;

    prometheus::Exposer exposer_;
    std::shared_ptr<prometheus::Registry> registry_ =
        std::make_shared<prometheus::Registry>();
    auto Run() noexcept -> void {
        prometheus::Event events[50];
        while (run_) {
            size_t count = event_lfqueue_->try_dequeue_bulk(events, 50);
            for (int i = 0; i < count; i++) {
                auto& gauge = BuildGauge()
                                  .Name("gauge")
                                  .Help("Additional description.")
                                  //.Labels({{"key", "value"}})
                                  .Register(*registry_);
                gauge
                    .Add({{"type", std::string(magic_enum::enum_name(
                                                   events[i].event_type)
                                                   .data())}})
                    .Set(events[i].time);
                time_manager_.Update();
            }
        }
    }
};
}  // namespace prometheus