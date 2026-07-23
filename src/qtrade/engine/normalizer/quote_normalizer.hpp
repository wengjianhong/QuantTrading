/// @file      quote_normalizer.hpp
/// @brief     行情标准化模块
/// @details   跨柜台行情语义统一、校验过滤；标准化后投递 Lane-M（队列由 EventBus 承担）
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_NORMALIZER_QUOTE_NORMALIZER_HPP_
#define QTRADE_TRADING_ENGINE_NORMALIZER_QUOTE_NORMALIZER_HPP_

#include "qtrade/engine/event_bus/event_lanes.hpp"

#include <qtrade_sdk/quote/quote_api.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace qtrade::engine::normalizer {

/// @brief 行情健康检查参数
struct QuoteHealthOptions {
  /// 最后一笔有效行情允许的最大静默时间
  std::chrono::milliseconds max_stale_age{3000};
};

/// @brief 行情校验、健康监控与 Lane-M 投递
class QuoteNormalizer {
 public:
  /// @brief 行情健康变化回调
  using HealthHandler = std::function<void(bool healthy)>;

  /// @brief 构造行情标准化器
  /// @param market_event_reactor Lane-M 行情事件反应器
  explicit QuoteNormalizer(event_bus::MarketEventReactor& market_event_reactor);

  /// @brief 析构并停止健康监控线程
  ~QuoteNormalizer();

  /// @brief 启动健康监控线程
  void Start();

  /// @brief 停止健康监控并断开行情源
  void Stop();

  /// @brief 绑定行情通道 API 并注册回调
  /// @param source 行情适配器所有权
  void SetQuoteApi(std::unique_ptr<qtrade_sdk::quote::QuoteApi> source);

  /// @brief 返回当前行情通道 API
  /// @return 行情适配器指针；未设置时返回 nullptr
  qtrade_sdk::quote::QuoteApi* GetQuoteApi();

  /// @brief 订阅合约行情
  /// @param instruments 合约列表
  void Subscribe(const std::vector<std::string>& instruments);

  /// @brief 取消订阅合约行情
  /// @param instruments 合约列表
  void Unsubscribe(const std::vector<std::string>& instruments);

  /// @brief 设置行情健康变化回调
  /// @param handler 首次有效行情或健康丢失时调用
  void SetHealthHandler(HealthHandler handler);

  /// @brief 设置行情健康检查参数
  /// @param options 健康检查参数
  /// @return 参数有效返回 kSuccess
  ErrorCode ConfigureHealth(const QuoteHealthOptions& options);

  /// @brief 查询行情是否健康
  /// @return 已收到有效行情返回 true
  [[nodiscard]] bool IsHealthy() const;

 private:
  /// 保护行情源与健康回调
  std::mutex mutex_;
  /// Lane-M 行情事件反应器
  event_bus::MarketEventReactor& market_event_reactor_;
  /// 行情通道 API
  std::unique_ptr<qtrade_sdk::quote::QuoteApi> market_source_;
  /// 是否已 Start
  std::atomic<bool> running_ = false;
  /// 是否已收到有效行情
  std::atomic<bool> healthy_ = false;
  /// 行情健康变化回调
  HealthHandler health_handler_;
  /// 健康检查参数
  QuoteHealthOptions health_options_;
  /// 最后一笔有效行情的 steady_clock 毫秒
  std::atomic<std::int64_t> last_valid_tick_ms_ = 0;
  /// 健康监控线程
  std::thread health_worker_;
  /// 健康监控唤醒条件
  std::condition_variable health_cv_;

  /// @brief 校验 Tick 后发布至 Lane-M，并更新健康状态
  /// @param tick 原始行情 Tick
  void OnTick(const qtrade_sdk::quote::MarketTick& tick);

  /// @brief 校验 Bar 后发布至 Lane-M
  /// @param bar 原始 K 线
  void OnBar(const qtrade_sdk::quote::Bar& bar);

  /// @brief 行情静默超时监控循环
  void WatchHealth();
};

}  // namespace qtrade::engine::normalizer

#endif  // QTRADE_TRADING_ENGINE_NORMALIZER_QUOTE_NORMALIZER_HPP_
