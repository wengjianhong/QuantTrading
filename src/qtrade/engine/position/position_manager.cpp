/// @file      position_manager.cpp
/// @brief     持仓管理器实现
/// @details   实现多账户、多策略持仓的实时跟踪与管理
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/position/position_manager.hpp"

#include <algorithm>
#include <cmath>
#include <mutex>

namespace qtrade::engine::position {
namespace {

/// @brief 计算多头总量
/// @param position 持仓快照
/// @return 今仓加昨仓
std::int64_t LongTotal(const PositionSnapshot& position) {
  return position.long_today + position.long_yesterday;
}

/// @brief 计算空头总量
/// @param position 持仓快照
/// @return 今仓加昨仓
std::int64_t ShortTotal(const PositionSnapshot& position) {
  return position.short_today + position.short_yesterday;
}

/// @brief 按平今/平昨语义扣减持仓
/// @param today 今仓引用
/// @param yesterday 昨仓引用
/// @param effect 开平类型
/// @param volume 扣减数量
void ReducePosition(std::int64_t& today,
                    std::int64_t& yesterday,
                    qtrade_sdk::trader::PositionEffectType effect,
                    std::int64_t volume) {
  if (effect == qtrade_sdk::trader::PositionEffectType::kCloseToday) {
    today = std::max<std::int64_t>(0, today - volume);
    return;
  }
  if (effect == qtrade_sdk::trader::PositionEffectType::kCloseYesterday) {
    yesterday = std::max<std::int64_t>(0, yesterday - volume);
    return;
  }
  // 未指定平今/平昨时优先平昨
  const auto from_yesterday = std::min(yesterday, volume);
  yesterday -= from_yesterday;
  today = std::max<std::int64_t>(0, today - (volume - from_yesterday));
}

/// @brief 按成交量加权更新开仓均价
/// @param current_average 当前均价
/// @param current_volume 当前持仓量
/// @param trade_price 本次成交价
/// @param trade_volume 本次成交量
/// @return 更新后的均价；总量为 0 时返回 0
double AddAveragePrice(double current_average,
                       std::int64_t current_volume,
                       double trade_price,
                       std::int64_t trade_volume) {
  const auto total_volume = current_volume + trade_volume;
  return total_volume > 0
           ? (current_average * static_cast<double>(current_volume) +
              trade_price * static_cast<double>(trade_volume)) /
               static_cast<double>(total_volume)
           : 0.0;
}

}  // namespace

void PositionManager::ApplyPositionSnapshot(
  const std::vector<qtrade_sdk::trader::Position>& positions) {
  std::unique_lock lock(mutex_);
  // 1. 全量替换本地持仓表
  positions_.clear();
  for (const auto& source : positions) {
    if (source.instrument.empty() || source.total_volume < 0) {
      continue;
    }
    // 2. 按方向拆分今昨仓并写入均价
    auto& target = positions_[source.instrument];
    const auto yesterday =
      std::clamp(source.yesterday_volume, std::int64_t{0}, source.total_volume);
    const auto today = source.total_volume - yesterday;
    if (source.direction == qtrade_sdk::trader::PositionDirectionType::kShort) {
      target.short_today += today;
      target.short_yesterday += yesterday;
      target.short_avg_price = source.avg_price;
    } else {
      target.long_today += today;
      target.long_yesterday += yesterday;
      target.long_avg_price = source.avg_price;
    }
  }
}

void PositionManager::ApplyTrade(const Trade& trade) {
  if (trade.instrument.empty() || trade.volume <= 0 || !std::isfinite(trade.price)) {
    return;
  }

  // 1. 幂等去重
  const std::string dedup_key =
    !trade.trade_id.empty()
      ? trade.trade_id
      : trade.order_id + ":" + std::to_string(trade.report_index) + ":" +
          std::to_string(trade.client_order_id) + ":" + trade.instrument + ":" +
          std::to_string(trade.trade_time) + ":" + std::to_string(trade.price) + ":" +
          std::to_string(trade.volume);
  std::unique_lock lock(mutex_);
  if (!applied_trade_ids_.insert(dedup_key).second) {
    return;
  }

  // 2. 开仓或现货买入：增加今仓并更新均价
  auto& position = positions_[trade.instrument];
  const bool is_open =
    trade.position_effect == qtrade_sdk::trader::PositionEffectType::kOpen;
  const bool is_cash = trade.position_effect == qtrade_sdk::trader::PositionEffectType::kInit;
  if (is_open || (is_cash && trade.side == qtrade_sdk::trader::SideType::kBuy)) {
    if (trade.side == qtrade_sdk::trader::SideType::kBuy) {
      const auto current = LongTotal(position);
      position.long_avg_price =
        AddAveragePrice(position.long_avg_price, current, trade.price, trade.volume);
      position.long_today += trade.volume;
    } else if (trade.side == qtrade_sdk::trader::SideType::kSell) {
      const auto current = ShortTotal(position);
      position.short_avg_price =
        AddAveragePrice(position.short_avg_price, current, trade.price, trade.volume);
      position.short_today += trade.volume;
    }
    return;
  }

  // 3. 平仓：按方向扣减对侧持仓，仓位清零时重置均价
  if (trade.side == qtrade_sdk::trader::SideType::kSell) {
    ReducePosition(
      position.long_today, position.long_yesterday, trade.position_effect, trade.volume);
    if (LongTotal(position) == 0) {
      position.long_avg_price = 0.0;
    }
  } else if (trade.side == qtrade_sdk::trader::SideType::kBuy) {
    ReducePosition(
      position.short_today, position.short_yesterday, trade.position_effect, trade.volume);
    if (ShortTotal(position) == 0) {
      position.short_avg_price = 0.0;
    }
  }
}

std::int64_t PositionManager::GetNetPosition(const std::string& instrument) const {
  const auto position = GetPosition(instrument);
  return LongTotal(position) - ShortTotal(position);
}

std::int64_t PositionManager::GetGrossPosition(const std::string& instrument) const {
  const auto position = GetPosition(instrument);
  return LongTotal(position) + ShortTotal(position);
}

PositionSnapshot PositionManager::GetPosition(const std::string& instrument) const {
  std::shared_lock lock(mutex_);
  const auto it = positions_.find(instrument);
  return it == positions_.end() ? PositionSnapshot{} : it->second;
}

}  // namespace qtrade::engine::position
