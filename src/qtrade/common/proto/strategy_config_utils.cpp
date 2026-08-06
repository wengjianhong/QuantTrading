/// @file      strategy_config_utils.cpp
/// @brief     config.v1.StrategyConfig 解析工具
/// @author    wengjianhong
/// @date      2026-07-31
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/proto/strategy_config_utils.hpp"

namespace qtrade::common::proto {

qtrade::strategy::StrategyConfig ParseStrategyConfigProto(const StrategyConfigProto& config) {
  qtrade::strategy::StrategyConfig out;
  out.enabled = config.enabled();
  out.strategy_id = config.strategy_id();
  out.order_volume = config.order_volume();
  out.strategy_name = config.strategy_name();
  out.order_cooldown_ms = config.order_cooldown_ms();
  out.max_position_volume = config.max_position_volume();
  out.instruments.assign(config.instruments().begin(), config.instruments().end());
  if (config.has_window_size()) {
    out.window_size = config.window_size();
  }
  if (config.has_order_threshold()) {
    out.order_threshold = config.order_threshold();
  }
  if (config.has_stop_loss_percent()) {
    out.stop_loss_percent = config.stop_loss_percent();
  }
  if (config.has_take_profit_percent()) {
    out.take_profit_percent = config.take_profit_percent();
  }
  return out;
}

}  // namespace qtrade::common::proto
