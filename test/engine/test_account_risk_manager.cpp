#include "qtrade/engine/account_risk/account_risk_manager.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <vector>

namespace {

class RecordingAccountRiskBridge final : public qtrade::account_risk::IAccountRiskBridge {
 public:
  /// @brief 设置 Reserve 返回结果
  /// @param error_code Reserve 调用错误码
  /// @param state Reservation 状态；空表示不返回 Reservation
  void SetReserveResult(qtrade::ErrorCode error_code,
                        std::optional<qtrade::account_risk::ReservationState> state) {
    reserve_error_code_ = error_code;
    reserve_state_ = state;
  }

  /// @brief 设置 QueryReservation 返回结果
  /// @param error_code QueryReservation 调用错误码
  /// @param state Reservation 状态；空表示不返回 Reservation
  void SetQueryResult(qtrade::ErrorCode error_code,
                      std::optional<qtrade::account_risk::ReservationState> state) {
    query_error_code_ = error_code;
    query_state_ = state;
  }

  qtrade::Result<qtrade::account_risk::AccountRiskPolicy> GetAccountRiskPolicy(const std::string&) const override {
    qtrade::Result<qtrade::account_risk::AccountRiskPolicy> result;
    result.error_code = qtrade::ErrorCode::kNotSupported;
    return result;
  }

  qtrade::Result<qtrade::account_risk::Reservation> Reserve(const qtrade::account_risk::ReserveRequest& request) override {
    std::lock_guard lock(mutex_);
    last_reserve_ = request;
    qtrade::Result<qtrade::account_risk::Reservation> result;
    result.error_code = reserve_error_code_;
    if (!reserve_state_.has_value()) {
      return result;
    }
    qtrade::account_risk::Reservation reservation;
    reservation.account_id = request.account_id;
    reservation.order_id = request.order_id;
    reservation.state = *reserve_state_;
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

  qtrade::Result<qtrade::account_risk::Reservation> QueryReservation(const std::string& account_id,
                                                                      const std::string& order_id) const override {
    qtrade::Result<qtrade::account_risk::Reservation> result;
    result.error_code = query_error_code_;
    if (!query_state_.has_value()) {
      return result;
    }
    qtrade::account_risk::Reservation reservation;
    reservation.account_id = account_id;
    reservation.order_id = order_id;
    reservation.state = *query_state_;
    result.data = reservation;
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
  qtrade::ErrorCode reserve_error_code_ = qtrade::ErrorCode::kSuccess;
  std::optional<qtrade::account_risk::ReservationState> reserve_state_ =
    qtrade::account_risk::ReservationState::kReserved;
  qtrade::ErrorCode query_error_code_ = qtrade::ErrorCode::kNotFound;
  std::optional<qtrade::account_risk::ReservationState> query_state_;
};

}  // namespace

TEST(AccountRiskManager, NoBridgeReserveSucceedsAndReleaseIsDropped) {
  qtrade::engine::account_risk::AccountRiskManager manager;
  qtrade::sdk::trader::OrderRequest request;
  request.instrument = "IF2506";
  request.volume = 1;
  EXPECT_EQ(manager.Reserve(request, "order-1"), qtrade::ErrorCode::kSuccess);
  manager.Release("order-1", qtrade::account_risk::ReleaseReason::kCanceled);
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

TEST(AccountRiskManager, ReserveConfirmsTimeoutThroughQuery) {
  RecordingAccountRiskBridge bridge;
  bridge.SetReserveResult(qtrade::ErrorCode::kTimeout, std::nullopt);
  bridge.SetQueryResult(qtrade::ErrorCode::kSuccess, qtrade::account_risk::ReservationState::kReserved);
  qtrade::engine::account_risk::AccountRiskManager manager;
  manager.SetBridge(&bridge);
  manager.SetIdentity("acct", "eng");

  qtrade::sdk::trader::OrderRequest request;
  request.instrument = "IF2506";
  request.volume = 1;
  EXPECT_EQ(manager.Reserve(request, "order-unknown"), qtrade::ErrorCode::kSuccess);
}

TEST(AccountRiskManager, ReserveRejectsNonReservedResponse) {
  RecordingAccountRiskBridge bridge;
  bridge.SetReserveResult(qtrade::ErrorCode::kSuccess, qtrade::account_risk::ReservationState::kRejected);
  qtrade::engine::account_risk::AccountRiskManager manager;
  manager.SetBridge(&bridge);
  manager.SetIdentity("acct", "eng");

  qtrade::sdk::trader::OrderRequest request;
  request.instrument = "IF2506";
  request.volume = 1;
  EXPECT_EQ(manager.Reserve(request, "order-rejected"), qtrade::ErrorCode::kInternalError);
}
