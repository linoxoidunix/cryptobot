#pragma once
#include <string>
#include <unordered_map>

#include "boost/asio/thread_pool.hpp"
#include "boost/asio/strand.hpp"

#include "aot/Logger.h"
#include "aot/bus/bus_event.h"
#include "aot/bus/bus_component.h"
#include "aot/client_response.h"
#include "aot/common/macros.h"
#include "aot/common/thread_utils.h"
#include "aot/common/types.h"
#include "aot/strategy/cross_arbitrage/signals.h"
#include "aot/strategy/market_order.h"

namespace Trading {

/// PositionInfo tracks the position, pnl (realized and unrealized) and volume
/// for a single trading instrument.
struct PositionInfo {
    int position      = 0;
    double real_pnl   = 0;
    double unreal_pnl = 0;
    double total_pnl  = 0;
    std::array<double, common::sideToIndex(common::Side::kMax) + 1> open_vwap;
    double volume           = 0;
    const Trading::BBO *bbo = nullptr;

    auto ToString() const {
        std::stringstream ss;
        ss << "Position{" << "pos:" << position << " u-pnl:" << unreal_pnl
           << " r-pnl:" << real_pnl << " t-pnl:" << total_pnl
           << " vol:" << volume << " ovwaps:["
           << (position ? open_vwap.at(common::sideToIndex(common::Side::kAsk)) /
                              std::abs(position)
                        : 0)
           << "X"
           << (position
                   ? open_vwap.at(common::sideToIndex(common::Side::kBid)) /
                         std::abs(position)
                   : 0)
           << "] " << (bbo ? bbo->ToString() : "") << "}";

        return ss.str();
    }

    /**
     * @brief add only filled order. it is regulate from trade_engine.cpp
     * Process an execution and update the position, pnl and volume.
     * @param client_response
     * @return auto
     */
    auto addFill(const Exchange::IResponse *client_response) noexcept {
        auto side                 = client_response->GetSide();
        auto exec_qty             = client_response->GetExecQty();
        auto price                = client_response->GetPrice();

        const auto old_position   = position;
        const auto side_index     = common::sideToIndex(side);
        const auto opp_side_index = common::sideToIndex(
            side == common::Side::kAsk ? common::Side::kBid : common::Side::kAsk);
        const auto side_value = common::sideToValue(side);
        assert(exec_qty >= 0);
        assert(price >= 0);
        position += exec_qty * side_value;
        volume   += exec_qty;

        if (old_position * sideToValue(side) >=
            0) {  // opened / increased position.
            open_vwap[side_index] += (price * exec_qty);
        } else {  // decreased position.
            const auto opp_side_vwap =
                open_vwap[opp_side_index] / std::abs(old_position);
            open_vwap[opp_side_index] = opp_side_vwap * std::abs(position);
            real_pnl +=
                std::min(static_cast<int>(exec_qty), std::abs(old_position)) *
                (opp_side_vwap - price) * sideToValue(side);
            if (position * old_position <
                0) {  // flipped position to opposite sign.
                open_vwap[side_index]     = (price * std::abs(position));
                open_vwap[opp_side_index] = 0;
            }
        }

        if (!position) {  // flat
            open_vwap[common::sideToIndex(common::Side::kAsk)] =
                open_vwap[sideToIndex(common::Side::kBid)] = 0;
            unreal_pnl                                     = 0;
        } else {
            if (position > 0)
                unreal_pnl =
                    (price - open_vwap[sideToIndex(common::Side::kAsk)] /
                                 std::abs(position)) *
                    std::abs(position);
            else
                unreal_pnl =
                    (open_vwap[common::sideToIndex(common::Side::kBid)] /
                         std::abs(position) -
                     price) *
                    std::abs(position);
        }

        total_pnl = unreal_pnl + real_pnl;

        logi("{} {}", ToString(), client_response->ToString());
    }

    /// Process a change in top-of-book prices (BBO), and update unrealized pnl
    /// if there is an open position.
    auto updateBBO(const Trading::BBO *_bbo) noexcept {
        bbo = _bbo;

        if (position && bbo->bid_price != common::kPriceInvalid &&
            bbo->ask_price != common::kPriceInvalid) {
            const auto mid_price = (bbo->bid_price + bbo->ask_price) * 0.5;
            if (position > 0)
                unreal_pnl =
                    (mid_price -
                     open_vwap[common::sideToIndex(common::Side::kAsk)] /
                         std::abs(position)) *
                    std::abs(position);
            else
                unreal_pnl =
                    (open_vwap[common::sideToIndex(common::Side::kBid)] /
                         std::abs(position) -
                     mid_price) *
                    std::abs(position);

            const auto old_total_pnl = total_pnl;
            total_pnl                = unreal_pnl + real_pnl;

            if (total_pnl != old_total_pnl)
                logi("{} {}", ToString(), bbo->ToString());
        }
    }
};

/// Top level position keeper class to compute position, pnl and volume for all
/// trading instruments.
class PositionKeeper {
  public:
    explicit PositionKeeper()                          = default;

    /// Deleted default, copy & move constructors and assignment-operators.

    PositionKeeper(const PositionKeeper &)             = delete;

    PositionKeeper(const PositionKeeper &&)            = delete;

    PositionKeeper &operator=(const PositionKeeper &)  = delete;

    PositionKeeper &operator=(const PositionKeeper &&) = delete;

  private:
    /// Hash map container from TickerId -> PositionInfo.
    // std::array<PositionInfo, ME_MAX_TICKERS> ticker_position_;
    std::unordered_map<common::TradingPair, PositionInfo,
                       common::TradingPairHash, common::TradingPairEqual>
        ticker_position;

  public:
    virtual void AddFill(const Exchange::IResponse *client_response) noexcept {
        ticker_position[client_response->GetTradingPair()].addFill(
            client_response);
    };
    virtual void UpdateBBO(const common::TradingPair trading_pair,
                           const Trading::BBO *bbo) noexcept {
        ticker_position[trading_pair].updateBBO(bbo);
    };

    virtual Trading::PositionInfo *GetPositionInfo(
        const common::TradingPair trading_pair) noexcept {
        return &(ticker_position[trading_pair]);
    };

    virtual std::string ToString() const {
        double total_pnl = 0;
        double total_vol = 0;

        std::stringstream ss;
        for (auto &it : ticker_position) {
            ss << "TickerId:" << it.first.ToString() << " "
               << it.second.ToString() << "\n";
            total_pnl += it.second.total_pnl;
            total_vol += it.second.volume;
        }
        ss << "Total PnL:" << total_pnl << " Vol:" << total_vol << "\n";
        return ss.str();
    };
};
};  // namespace Trading

namespace position_keeper {
enum class EventType { kAddFill, kUpdateBBO, kNoEvent };
class Event {
  public:
    virtual ~Event()            = default;
    virtual EventType GetType() = 0;
};
using EventLFQueue = moodycamel::ConcurrentQueue<Event *>;
struct AddFill;
using AddFillPool = common::MemoryPool<AddFill>;

// struct AddFill : public Event {
//     const Exchange::IResponse *client_response = nullptr;
//     AddFillPool *mem_pool                      = nullptr;
//     AddFill() = default;
//     AddFill(const Exchange::IResponse *_client_response,
//               AddFillPool *_mem_pool):client_response(_client_response), mem_pool(_mem_pool) {};
//     ~AddFill() override                        = default;
//     EventType GetType() override { return EventType::kAddFill; };
//     void Deallocate() {
//         if (!mem_pool) {
//             loge("mem_pull = nullptr. Can't free BBidUpdated event");
//             return;
//         }
//         mem_pool->Deallocate(this);
//     }
// };

// struct BusEventAddFill : public AddFill, public bus::Event{
//     ~BusEventAddFill() override = default;
//     void Accept(bus::Component* comp) override{
//         comp->AsyncHandleEvent(this);
//     }
// };


struct UpdateBBO;
using UpdateBBOPool = common::MemPool<UpdateBBO>;
struct UpdateBBO : public Event {
    common::ExchangeId exchange_id;
    common::TradingPair trading_pair;
    const Trading::BBO *bbo;
    UpdateBBOPool *mem_pool = nullptr;
    UpdateBBO() = default;
    UpdateBBO(common::ExchangeId _exchange_id,
     common::TradingPair _trading_pair,
      const Trading::BBO* _bbo,
      UpdateBBOPool *_mem_pool):exchange_id(_exchange_id),trading_pair(_trading_pair), bbo(_bbo), mem_pool(_mem_pool) {};
    ~UpdateBBO() override = default;
    EventType GetType() override { return EventType::kUpdateBBO; };
    void Deallocate() {
        if (!mem_pool) {
            loge("mem_pull = nullptr. can't free BBidUpdated event");
            return;
        }
        mem_pool->deallocate(this);
    }
};

struct BusEventUpdateBBO : public UpdateBBO, public bus::Event{
    ~BusEventUpdateBBO() override = default;
    void Accept(bus::Component* comp) override{
        comp->AsyncHandleEvent(this);
    }
};

};  // namespace position_keeper
namespace exchange {

class PositionKeeper : public ::Trading::PositionKeeper {
  public:
    using ExchangePositionKeeper =
        std::unordered_map<common::ExchangeId, Trading::PositionKeeper *>;
    explicit PositionKeeper(ExchangePositionKeeper &position)
        : Trading::PositionKeeper(), position_(position) {};
    void AddFill(const Exchange::IResponse *client_response) noexcept override {
        if (!client_response) {
            loge("client_response = nullptr");
            return;
        }
        auto exchange_id = client_response->GetExchangeId();
        if (exchange_id == common::kExchangeIdInvalid) {
            loge("Invalid exchange id");
            return;
        }
        if (!position_.count(exchange_id)) {
            loge("don't found position keeper");
            return;
        }
        position_[exchange_id]->AddFill(client_response);
    };
    void HandleResponse(Exchange::BusEventResponse *client_response) noexcept  {
        if (!client_response) {
            loge("client_response = nullptr");
            return;
        }
        auto ptr_response = client_response->response;
        AddFill(ptr_response);
        client_response->Release();
    };
    void UpdateBBO(common::ExchangeId exchange_id,
                   const common::TradingPair trading_pair,
                   const Trading::BBO *bbo) noexcept {
        if (!bbo) {
            loge("bbo = nullptr");
            return;
        }
        if (exchange_id == common::kExchangeIdInvalid) {
            loge("Invalid exchange id");
            return;
        }
        if (!position_.count(exchange_id)) {
            loge("don't found position keeper");
            return;
        }
        position_[exchange_id]->UpdateBBO(trading_pair, bbo);
    };
    Trading::PositionInfo *GetPositionInfo(
        common::ExchangeId exchange_id,
        const common::TradingPair trading_pair) noexcept {
        if (exchange_id == common::kExchangeIdInvalid) {
            loge("Invalid exchange id");
            return nullptr;
        }
        return position_[exchange_id]->GetPositionInfo(trading_pair);
    };
    // void OnNewSignal(Exchange::BusEventResponse *signal) {
    //     if (type == position_keeper::EventType::kAddFill) {
    //         const auto event = static_cast<position_keeper::AddFill *>(signal);
    //         AddFill(event->client_response);
    //         event->Deallocate();
    //         return;
    //     }
    
    //     if (type == position_keeper::EventType::kUpdateBBO) {
    //         auto event = static_cast<position_keeper::UpdateBBO *>(signal);
    //         UpdateBBO(event->exchange_id, event->trading_pair, event->bbo);
    //         event->Deallocate();
    //         return;
    //     }
    //     loge("Unknown signal type");
    //     return;
    // }

  private:
    ExchangePositionKeeper &position_;
};
}  // namespace exchange

namespace Trading {
// class PositionKeeperService : public common::ServiceI {
//   public:
//     explicit PositionKeeperService(exchange::PositionKeeper *pk,
//                           position_keeper::EventLFQueue *incoming_queue)
//         : pk_(pk), queue_(incoming_queue) {}
//     void Start() override {
//         run_    = true;
//         thread_ = std::unique_ptr<std::jthread>(common::createAndStartJThread(
//             -1, "Trading/OrderBookService", [this] { Run(); }));
//         ASSERT(thread_ != nullptr, "Failed to start OrderBookService thread.");
//     }

//     void StopImmediately() override { run_ = false; }

//     void StopWaitAllQueue() override {
//         logi("stop Trading/PositionKeeperService");
//         while (queue_->size_approx()) {
//             logi("Sleeping till all updates are consumed md-size:{}",
//                  queue_->size_approx());
//             using namespace std::literals::chrono_literals;
//             std::this_thread::sleep_for(10ms);
//         }
//         StopImmediately();
//     }

//     ~PositionKeeperService() override {
//         StopImmediately();  // Ensure cleanup
//         using namespace std::literals::chrono_literals;
//         std::this_thread::sleep_for(1s);
//     }
//     void Run() {
//         if (!pk_) {
//             logw("Can't run PositionKeeperService. pk_ = nullptr");
//             return;
//         }
//         if (!queue_) [[unlikely]] {
//             logw("Can't run PositionKeeperService. queue_ = nullptr");
//             return;
//         }
//         logi("Trading::PositionKeeperService start");
//         std::array<position_keeper::Event *, 50> new_events{};
//         while (run_) {
//             auto events_number =
//                 queue_->try_dequeue_bulk(new_events.begin(), 50);
//             for (size_t i = 0; i < events_number; i++) {
//                 auto signal = new_events[i];
//                 pk_->OnNewSignal(signal);
//             }
//         }
//         logi("Trading/PositionKeeperService stopped");
//     };

//   private:
//     volatile bool run_ = false;
//     std::unique_ptr<std::jthread> thread_;

//     exchange::PositionKeeper *pk_         = nullptr;
//     position_keeper::EventLFQueue *queue_ = nullptr;
// };

template<typename Executor>
class PositionKeeperComponent : public bus::Component{
    Executor executor_;
    exchange::PositionKeeper *pk_         = nullptr;
  public:
    explicit PositionKeeperComponent(Executor&& executor, exchange::PositionKeeper *pk)
        : executor_(std::move(executor)),pk_(pk) {}

    ~PositionKeeperComponent() override = default;
    
    void AsyncHandleEvent(Exchange::BusEventResponse* event) override{
         logd("position keeper accept new event response {}", event->response->ToString());
         boost::asio::post(executor_, [this, event]() {
            pk_->HandleResponse(event);
        });
    }
    void AsyncHandleEvent(position_keeper::BusEventUpdateBBO* event) override{
        assert(false);
        //  boost::asio::post(executor_, [this, event]() {
        //     pk_->OnNewSignal(event);
        //     event->Release();
        // });
    }
};

};  // namespace Trading