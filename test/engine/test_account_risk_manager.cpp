#include "qtrade/engine/account_risk/account_risk_manager.hpp"

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

  qtrade::Result<qtrade::account_risk::Reservation> Reserve(const qtrade::account_risk::ReserveRequest& request) override {
    std::lock_guard lock(mutex_);
    last_reserve_ = request;
    qtrade::Result<qtrade::account_risk::Reservation> result;
    qtrade::account_risk::Reservation reservation;
    reservation.account_id = request.account_id;
    reservation.order_id = request.order_id;
    reservation.state = qtrade::account_risk::ReservationState::kReserved;
    result.data = reservation;
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
    reservation.account_id = request.account_id;
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

  qtrade::account_risk::ReserveRequest LastReserve() const {
    std::lock_guard lock(mutex_);
    return last_reserve_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::vector<qtrade::account_risk::ReleaseRequest> releases_;
  qtrade::account_risk::ReserveRequest last_reserve_;
};

}  // namespace

TEST(AccountRiskManager, NoBridgeReserveSucceedsAndReleaseIsDropped) {
  qtrade::engine::account_risk::AccountRiskManager manager;
  qtrade::sdk::trader::OrderRequest request;
  request.instrument = "IF2506";
  request.volume = 1;
  EXPECT_EQ(manager.Reserve(request, "order-1"), qtrade::ErrorCode::kSuccess);
  manager.Release("order-1", qtrade::account_risk::ReleaseReason::kCanceled);
  EXPECT_EQ(manager.PendingCount(), 0U);
}

TEST(AccountRiskManager, ReleaseInvokesBridgeOnWorker) {
  RecordingAccountRiskBridge bridge;
  qtrade::engine::account_risk::AccountRiskManager manager;
  manager.SetBridge(&bridge);
  manager.SetIdentity("acct", "eng");
  manager.Start();
  manager.Release("order-9", qtrade::account_risk::ReleaseReason::kCanceled);
  ASSERT_TRUE(bridge.WaitForReleases(1));
  manager.Stop();
  const auto releases = bridge.Releases();
  ASSERT_EQ(releases.size(), 1U);
  EXPECT_EQ(releases[0].account_id, "acct");
  EXPECT_EQ(releases[0].order_id, "order-9");
  EXPECT_EQ(releases[0].reason, qtrade::account_risk::ReleaseReason::kCanceled);
}

TEST(AccountRiskManager, ReserveCopiesEngineIdentity) {
  RecordingAccountRiskBridge bridge;
  qtrade::engine::account_risk::AccountRiskManager manager;
  manager.SetBridge(&bridge);
  manager.SetIdentity("acct", "eng-1");
  qtrade::sdk::trader::OrderRequest request;
  request.strategy_id = "s1";
  request.instrument = "IF2506";
  request.price = 10.0;
  request.volume = 2;
  EXPECT_EQ(manager.Reserve(request, "oid"), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(bridge.LastReserve().exposure.engine_id, "eng-1");
  EXPECT_EQ(bridge.LastReserve().exposure.strategy_id, "s1");
}
