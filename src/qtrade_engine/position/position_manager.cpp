/// @file      position_manager.cpp
/// @brief     持仓管理器实现
/// @details   本地表为 map<instrument, map<direction, Position>>；按官方方向存取。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/position/position_manager.hpp"

#include "qtrade/common/utils/trade_dedup.hpp"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <shared_mutex>

namespace qtrade::engine::position {
namespace trader = qtrade::sdk::trader;
using qtrade::common::utils::GenerateTradeDedupKey;

namespace {

/// @brief 按平今/平昨语义扣减今昨仓
/// @param today 今仓引用
/// @param yesterday 昨仓引用
/// @param effect 开平类型
/// @param volume 扣减数量
void ReduceTodayYesterday(std::int64_t& today,
                          std::int64_t& yesterday,
                          trader::PositionEffectType effect,
                          std::int64_t volume) {
  if (effect == trader::PositionEffectType::kCloseToday) {
    today = std::max<std::int64_t>(0, today - volume);
    return;
  }
  if (effect == trader::PositionEffectType::kCloseYesterday) {
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
  if (total_volume == 0) {
    return 0.0;
  }

  return (current_average * static_cast<double>(current_volume) + trade_price * static_cast<double>(trade_volume)) /
         static_cast<double>(total_volume);
}

/// @brief 对单条 Position 按开平语义扣减
/// @param position 持仓引用
/// @param effect 开平类型
/// @param volume 扣减数量
void ReducePosition(trader::Position& position, trader::PositionEffectType effect, std::int64_t volume) {
  // trader::Position 只存 total_volume / yesterday_volume；今仓 = total - yesterday
  auto today = position.total_volume - position.yesterday_volume;
  auto yesterday = position.yesterday_volume;
  ReduceTodayYesterday(today, yesterday, effect, volume);
  position.yesterday_volume = yesterday;
  position.total_volume = yesterday + std::max<std::int64_t>(0, today);
  if (position.total_volume == 0) {
    position.avg_price = 0.0;
  }
}

/// @brief 开仓：增加今仓并更新均价
/// @param position 持仓引用
/// @param trade_price 成交价
/// @param trade_volume 成交量
void OpenPosition(trader::Position& position, double trade_price, std::int64_t trade_volume) {
  const auto current = position.total_volume;
  position.avg_price = AddAveragePrice(position.avg_price, current, trade_price, trade_volume);
  // 开仓只增今仓，yesterday_volume 不变 → 直接 total_volume += volume
  position.total_volume += trade_volume;
}

}  // namespace

void PositionManager::ApplyPositionSnapshot(const std::vector<Position>& positions) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  // 1. 全量替换本地持仓表
  positions_.clear();
  for (const auto& source : positions) {
    if (source.instrument.empty() || source.total_volume < 0) {
      continue;
    }

    // 2. 按官方方向原样写入（与 OnQueryPosition 一条回调对应一条记录）
    positions_[source.instrument][source.direction] = source;
  }
}

void PositionManager::ApplyTrade(const Trade& trade) {
  if (trade.instrument.empty() || trade.volume <= 0 || !std::isfinite(trade.price)) {
    return;
  }

  // 1. 幂等去重
  const std::string dedup_key = GenerateTradeDedupKey(trade);
  std::unique_lock<std::shared_mutex> lock(mutex_);
  if (!applied_trade_ids_.insert(dedup_key).second) {
    return;
  }

  // 2. 开仓或现货买入：增加对应方向今仓并更新均价
  const bool is_open = (trade.position_effect == trader::PositionEffectType::kOpen);
  const bool is_cash = (trade.position_effect == trader::PositionEffectType::kInit);
  if (is_open || (is_cash && trade.side == trader::SideType::kBuy)) {
    if (trade.side == trader::SideType::kBuy) {
      auto& position = positions_[trade.instrument][trader::PositionDirectionType::kLong];
      OpenPosition(position, trade.price, trade.volume);
    } else if (trade.side == trader::SideType::kSell) {
      auto& position = positions_[trade.instrument][trader::PositionDirectionType::kShort];
      OpenPosition(position, trade.price, trade.volume);
    }
    return;
  }

  // 3. 平仓：扣减对侧方向持仓
  if (trade.side == trader::SideType::kSell) {
    auto it = positions_.find(trade.instrument);
    if (it == positions_.end()) {
      return;
    }
    const auto long_it = it->second.find(trader::PositionDirectionType::kLong);
    if (long_it != it->second.end()) {
      ReducePosition(long_it->second, trade.position_effect, trade.volume);
    }
  } else if (trade.side == trader::SideType::kBuy) {
    auto it = positions_.find(trade.instrument);
    if (it == positions_.end()) {
      return;
    }
    const auto short_it = it->second.find(trader::PositionDirectionType::kShort);
    if (short_it != it->second.end()) {
      ReducePosition(short_it->second, trade.position_effect, trade.volume);
    }
  }
}

std::int64_t PositionManager::GetNetPosition(const std::string& instrument) const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  const auto it = positions_.find(instrument);
  if (it == positions_.end()) {
    return 0;
  }

  // 如果存在净持仓，则直接返回净持仓
  const auto net_it = it->second.find(trader::PositionDirectionType::kNet);
  if (net_it != it->second.end()) {
    return net_it->second.total_volume;
  }

  // 如果不存在净持仓，则按官方语义加权求和
  std::int64_t net = 0;
  for (const auto& [direction, position] : it->second) {
    if (direction == trader::PositionDirectionType::kLong) {
      net += position.total_volume;
    } else if (direction == trader::PositionDirectionType::kShort) {
      net -= position.total_volume;
    }
  }
  return net;
}

std::int64_t PositionManager::GetGrossPosition(const std::string& instrument) const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  const auto it = positions_.find(instrument);
  if (it == positions_.end()) {
    return 0;
  }

  // 总持仓 = 各方向 total_volume 之和（kUnknown 不计）
  std::int64_t gross = 0;
  for (const auto& [direction, position] : it->second) {
    if (direction != trader::PositionDirectionType::kUnknown && direction != trader::PositionDirectionType::kNet) {
      gross += position.total_volume;
    }
  }
  return gross;
}

std::optional<Position> PositionManager::GetPosition(const std::string& instrument,
                                                     PositionDirectionType direction) const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  const auto instrument_it = positions_.find(instrument);
  if (instrument_it == positions_.end()) {
    return std::nullopt;
  }

  const auto direction_it = instrument_it->second.find(direction);
  if (direction_it == instrument_it->second.end()) {
    return std::nullopt;
  }
  return std::optional<Position>(direction_it->second);
}

std::map<PositionDirectionType, Position> PositionManager::GetPositions(const std::string& instrument) const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  const auto it = positions_.find(instrument);
  if (it == positions_.end()) {
    return std::map<PositionDirectionType, Position>{};
  }
  return it->second;
}

}  // namespace qtrade::engine::position
