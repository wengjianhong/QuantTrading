#include "qtrade/engine/account/account_manager.hpp"
#include "qtrade/engine/account_risk/account_risk_manager.hpp"
#include "qtrade/engine/core/lane_event_handler.hpp"
#include "qtrade/engine/orders/order_manager.hpp"
#include "qtrade/engine/positions/position_manager.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace {

class RecordingAccountRiskBridge final : public qtrade::account_risk::IAccountRiskBridge {
 public:
  qtrade::Result<qtrade::account_risk::AccountRiskPolicy> GetAccountRiskPolicy(const std::string&) const override {
    qtrade::Result<qtrade::account_risk::AccountRiskPolicy> result;
    result.error_code = qtrade::ErrorCode::kNotSupported;
    return result;
  }

  qtrade::Result<qtrade::account_risk::Reservation> Reserve(const qtrade::account_risk::ReserveRequest&) override {
    qtrade::Result<qtrade::account_risk::Reservation> result;
    result.error_code = qtrade::ErrorCode::kNotSupported;
    return result;
  }

  qtrade::Result<qtrade::account_risk::Reservation> Release(const qtrade::account_risk::ReleaseRequest& request) override {
    {
      std::lock_guard lock(mutex_);
      releases_.push_back(request);
    }
    cv_.notify_all();
    qtrade::Result<qtrade::account_risk::Reservation> result;
    qtrade::account_risk::Reservation reservation;
    reservation.order_id = request.order_id;
    reservation.state = qtrade::account_risk::ReservationState::kReleased;
    result.data = reservation;
    return result;
  }

  qtrade::Result<qtrade::account_risk::Reservation> QueryReservation(const std::string&, const std::string&) const override {
    qtrade::Result<qtrade::account_risk::Reservation> result;
    result.error_code = qtrade::ErrorCode::kNotFound;
    return result;
  }

  bool WaitForReleases(std::size_t count, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000)) {
    std::unique_lock lock(mutex_);
    return cv_.wait_for(lock, timeout, [&] { return releases_.size() >= count; });
  }

  std::vector<qtrade::account_risk::ReleaseRequest> Releases() const {
    std::lock_guard lock(mutex_);
    return releases_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::vector<qtrade::account_risk::ReleaseRequest> releases_;
};

qtrade::engine::orders::OrderManagerOptions MakeOptions() {
  qtrade::engine::orders::OrderManagerOptions options;
  options.account_id = "acct";
  options.engine_id = "engine";
  options.engine_epoch = 7;
  return options;
}

}  // namespace

TEST(LaneEventHandler, RejectedOrderReleasesViaAccountRiskApi) {
  qtrade::engine::orders::OrderManager orders;
  ASSERT_EQ(orders.Initialize(MakeOptions()), qtrade::ErrorCode::kSuccess);
  qtrade::engine::account::AccountManager account;
  qtrade::engine::positions::PositionManager position;
  RecordingAccountRiskBridge bridge;
  qtrade::engine::account_risk::AccountRiskManager account_risk;
  account_risk.SetBridge(&bridge);
  account_risk.SetIdentity("acct", "engine");
  account_risk.Start();

  qtrade::engine::LaneEventHandler handler(orders, account, position, account_risk);
  qtrade::sdk::trader::OrderRequest request;
  request.client_order_id = 1;
  request.instrument = "IF2506";
  request.price = 10.0;
  request.volume = 2;
  const auto created = orders.CreateOrder(request);
  ASSERT_TRUE(created.has_value());

  qtrade::sdk::trader::Order report = *created;
  report.status = qtrade::sdk::trader::OrderStatusType::kRejected;
  handler.OnOrder(report);

  ASSERT_TRUE(bridge.WaitForReleases(1));
  account_risk.Stop();
  ASSERT_EQ(bridge.Releases().size(), 1U);
  EXPECT_EQ(bridge.Releases()[0].reason, qtrade::account_risk::ReleaseReason::kRejectedByVenue);
}

TEST(LaneEventHandler, NoBridgeStillAppliesOrder) {
  qtrade::engine::orders::OrderManager orders;
  ASSERT_EQ(orders.Initialize(MakeOptions()), qtrade::ErrorCode::kSuccess);
  qtrade::engine::account::AccountManager account;
  qtrade::engine::positions::PositionManager position;
  qtrade::engine::account_risk::AccountRiskManager account_risk;
  qtrade::engine::LaneEventHandler handler(orders, account, position, account_risk);

  qtrade::sdk::trader::OrderRequest request;
  request.client_order_id = 3;
  request.instrument = "IF2506";
  request.price = 10.0;
  request.volume = 1;
  request.side = qtrade::sdk::trader::SideType::kBuy;
  const auto created = orders.CreateOrder(request);
  ASSERT_TRUE(created.has_value());

  qtrade::sdk::trader::Order report = *created;
  report.status = qtrade::sdk::trader::OrderStatusType::kRejected;
  handler.OnOrder(report);
  EXPECT_TRUE(orders.GetOrder(created->order_id).has_value());
}
