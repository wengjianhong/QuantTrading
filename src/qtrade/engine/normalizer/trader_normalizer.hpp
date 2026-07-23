/// @file      trader_normalizer.hpp
/// @brief     交易标准化模块
/// @details   跨柜台订单/成交回报语义统一、校验过滤；标准化后交 OMS，再投递 Lane-R
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_NORMALIZER_TRADER_NORMALIZER_HPP_
#define QTRADE_TRADING_ENGINE_NORMALIZER_TRADER_NORMALIZER_HPP_

#include "qtrade/engine/event_bus/event_lanes.hpp"

#include <qtrade_sdk/trader/trader_api.hpp>

#include <atomic>
#include <memory>
#include <mutex>

namespace qtrade::engine::normalizer {

/// @brief 交易回报校验过滤与 Lane-R 投递
class TraderNormalizer {
 public:
  /// @brief 构造交易标准化器
  /// @param return_event_reactor Lane-R 回报事件反应器
  explicit TraderNormalizer(event_bus::ReturnEventReactor& return_event_reactor);

  /// @brief 析构并停止标准化器
  ~TraderNormalizer();

  /// @brief 启动回报接收
  void Start();

  /// @brief 停止并断开交易通道
  void Stop();

  /// @brief 绑定交易通道 API 并注册回报回调
  /// @param trader_api 交易适配器所有权；仅未运行时可设置
  void SetTraderApi(std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api);

  /// @brief 返回当前交易通道 API
  /// @return 交易适配器指针；未设置时返回 nullptr
  qtrade_sdk::trader::TraderApi* GetTraderApi();

  /// @brief 查询交易回报通道是否健康
  /// @return 已启动且 TraderApi 已连接返回 true
  [[nodiscard]] bool IsHealthy() const;

 private:
  /// @brief 校验订单回报后发布至 Lane-R
  /// @param order 原始订单回报
  void OnOrder(const qtrade_sdk::trader::Order& order);

  /// @brief 校验成交回报后发布至 Lane-R
  /// @param trade 原始成交回报
  void OnTrade(const qtrade_sdk::trader::Trade& trade);

  /// 保护交易通道与运行状态
  mutable std::mutex mutex_;
  /// 是否已 Start
  std::atomic_bool running_ = false;
  /// Lane-R 回报事件反应器
  event_bus::ReturnEventReactor& return_event_reactor_;
  /// 交易通道 API
  std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api_;
};

}  // namespace qtrade::engine::normalizer

#endif  // QTRADE_TRADING_ENGINE_NORMALIZER_TRADER_NORMALIZER_HPP_
