/// @file      account_manager.cpp
/// @brief     账户管理器实现
/// @details   实现柜台资产快照、订单冻结与成交现金流的本地视图更新
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/account/account_manager.hpp"

#include "qtrade/common/utils/trade_dedup.hpp"

#include <algorithm>
#include <cmath>

namespace qtrade::engine::account {
using qtrade::common::utils::GenerateTradeDedupKey;

void AccountManager::ApplyAssetSnapshot(const qtrade_sdk::trader::AccountAsset& asset) {
  std::lock_guard lock(mutex_);
  asset_ = asset;
  has_asset_snapshot_ = true;
  // 柜台快照已含最新可用资金，重置本地成交现金流避免双重计入
  net_cash_flow_ = 0.0;
}

void AccountManager::ApplyOrder(const Order& order) {
  if (order.order_id.empty()) {
    return;
  }

  // 1. 计算本订单当前应冻结名义金额（终态或非占用资金方向为 0）
  const bool terminal = order.status == qtrade_sdk::trader::OrderStatusType::kFilled ||
                        order.status == qtrade_sdk::trader::OrderStatusType::kCanceled ||
                        order.status == qtrade_sdk::trader::OrderStatusType::kRejected;
  const bool consumes_cash = order.side == qtrade_sdk::trader::SideType::kBuy ||
                             order.side == qtrade_sdk::trader::SideType::kMarginTrade ||
                             order.side == qtrade_sdk::trader::SideType::kPurchase;
  double frozen = 0.0;
  if (!terminal && consumes_cash && std::isfinite(order.price) && order.price > 0.0) {
    frozen = order.price * static_cast<double>(std::max<std::int64_t>(0, order.left_volume));
  }

  // 2. 按差额更新冻结合计，终态订单从映射中移除
  std::lock_guard<std::mutex> lock(mutex_);
  double& previous = order_frozen_amounts_[order.order_id];
  frozen_amount_ = std::max(0.0, frozen_amount_ + frozen - previous);
  previous = frozen;
  if (terminal) {
    order_frozen_amounts_.erase(order.order_id);
  }
}

void AccountManager::ApplyTrade(const Trade& trade) {
  if (trade.volume <= 0 || !std::isfinite(trade.price)) {
    return;
  }

  // 1. 构造幂等键与成交金额
  const std::string dedup_key = GenerateTradeDedupKey(trade);
  const double amount = trade.trade_amount > 0.0 ? trade.trade_amount : trade.price * static_cast<double>(trade.volume);

  // 2. 幂等写入后按方向更新净现金流
  std::lock_guard<std::mutex> lock(mutex_);
  if (!applied_trade_ids_.insert(dedup_key).second) {
    return;
  }
  filled_amount_ += amount;
  if (trade.side == qtrade_sdk::trader::SideType::kBuy || trade.side == qtrade_sdk::trader::SideType::kMarginTrade ||
      trade.side == qtrade_sdk::trader::SideType::kPurchase) {
    net_cash_flow_ -= amount;
  } else if (trade.side == qtrade_sdk::trader::SideType::kSell ||
             trade.side == qtrade_sdk::trader::SideType::kShortSell ||
             trade.side == qtrade_sdk::trader::SideType::kRedemption) {
    net_cash_flow_ += amount;
  }
}

double AccountManager::GetFilledAmount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return filled_amount_;
}

double AccountManager::GetFrozenAmount() const {
  std::lock_guard lock(mutex_);
  return frozen_amount_;
}

double AccountManager::GetNetCashFlow() const {
  std::lock_guard lock(mutex_);
  return net_cash_flow_;
}

double AccountManager::GetAvailableFunds() const {
  std::lock_guard lock(mutex_);
  // 可用资金 = 柜台购买力 + 本地成交净现金流 - 未成交买单冻结
  const double buying_power = has_asset_snapshot_ ? asset_.buying_power : 0.0;
  return buying_power + net_cash_flow_ - frozen_amount_;
}

}  // namespace qtrade::engine::account
