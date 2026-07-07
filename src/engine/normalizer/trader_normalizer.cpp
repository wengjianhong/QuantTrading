/// @file      trader_normalizer.cpp
/// @brief     交易标准化模块实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0

#include "engine/normalizer/trader_normalizer.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::normalizer {

TraderNormalizer::TraderNormalizer(event_bus::ReturnEventReactor& return_event_reactor)
  : return_event_reactor_(return_event_reactor), running_(false) {
  (void)return_event_reactor_;
}

TraderNormalizer::~TraderNormalizer() { Stop(); }

void TraderNormalizer::Start() {
  if (running_) {
    return;
  }
  running_ = true;
  spdlog::info("[TraderNormalizer] started successfully");
}

void TraderNormalizer::Stop() {
  if (!running_) {
    return;
  }
  running_ = false;
  spdlog::info("[TraderNormalizer] stopped cleanly");
}

}  // namespace qtrade::engine::normalizer
