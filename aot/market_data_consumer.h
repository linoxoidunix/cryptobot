// #pragma once

// #include <functional>
// #include <map>

// #include "aot/common/thread_utils.h"
// #include "aot/common/macros.h"
// #include "moodycamel/concurrentqueue.h"

// #include "exchange/market_data/market_update.h"

// namespace Trading {
//   class MarketDataConsumer {
//   public:
//     MarketDataConsumer(common::ClientId client_id, Exchange::MEMarketUpdateLFQueue *market_updates, const std::string &iface,
//                        const std::string &snapshot_ip, int snapshot_port,
//                        const std::string &incremental_ip, int incremental_port);

//     ~MarketDataConsumer() {
//       stop();

//       using namespace std::literals::chrono_literals;
//       std::this_thread::sleep_for(5s);
//     }

//     auto start() {
//       run_ = true;
//       ASSERT(common::createAndStartThread(-1, "Trading/MarketDataConsumer", [this]() { run(); }) != nullptr, "Failed to start MarketData thread.");
//     }

//     auto stop() -> void {
//       run_ = false;
//     }

//     // Deleted default, copy & move constructors and assignment-operators.
//     MarketDataConsumer() = delete;

//     MarketDataConsumer(const MarketDataConsumer &) = delete;

//     MarketDataConsumer(const MarketDataConsumer &&) = delete;

//     MarketDataConsumer &operator=(const MarketDataConsumer &) = delete;

//     MarketDataConsumer &operator=(const MarketDataConsumer &&) = delete;

//   private:
//     size_t next_exp_inc_seq_num_ = 1;
//     Exchange::MEMarketUpdateLFQueue *incoming_md_updates_ = nullptr;

//     volatile bool run_ = false;

//     std::string time_str_;
//     Logger logger_;
//     common::McastSocket incremental_mcast_socket_, snapshot_mcast_socket_;

//     bool in_recovery_ = false;
//     const std::string iface_, snapshot_ip_;
//     const int snapshot_port_;
//     typedef std::map<size_t, Exchange::MEMarketUpdate> QueuedMarketUpdates;
//     QueuedMarketUpdates snapshot_queued_msgs_, incremental_queued_msgs_;

//   private:
//     auto run() noexcept -> void;

//     auto recvCallback(McastSocket *socket) noexcept -> void;

//     auto queueMessage(bool is_snapshot, const Exchange::MDPMarketUpdate *request);

//     auto startSnapshotSync() -> void;
//     auto checkSnapshotSync() -> void;
//   };
// }