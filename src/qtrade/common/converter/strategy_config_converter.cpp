/// @file      strategy_config_converter.cpp
/// @brief     config.v1.StrategyConfig ↔ qtrade::strategy::StrategyConfig
/// @author    wengjianhong
/// @date      2026-07-31
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/converter/strategy_config_converter.hpp"

namespace qtrade::common::converter {

qtrade::strategy::StrategyConfig ParseStrategyConfigProto(const StrategyConfigProto& config) {
  qtrade::strategy::StrategyConfig out;
  out.strategy_id = config.strategy_id();
  out.strategy_name = config.strategy_name();
  out.enabled = config.enabled();
  out.instruments.assign(config.instruments().begin(), config.instruments().end());
  out.order_volume = config.order_volume();
  out.max_position_volume = config.max_position_volume();
  out.order_cooldown_ms = config.order_cooldown_ms();
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

}  // namespace qtrade::common::converter
