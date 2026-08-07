/// @file      mock_trader_spi.hpp
/// @brief     Mock 交易 SPI（模拟厂商回调，转发至 Target）
/// @details   封装 Mock 交易事件并转发给 qtrade SDK 交易回调接口。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright Copyright (c) 2026 qtrade.
#ifndef QTRADE_ADAPTER_MOCK_TRADER_SPI_HPP_
#define QTRADE_ADAPTER_MOCK_TRADER_SPI_HPP_

#include <qtrade/sdk/trader/trader_spi.hpp>

#include <cstdint>

namespace qtrade::adapter::mock::trader {

/// @brief Mock 交易 SPI 回调适配器。
/// @details 将合成交易回报转发至 qtrade SDK 交易回调接口。
class MockTraderSpi {
 public:
  /// @brief 设置接收模拟交易回调的目标对象。
  /// @param target qtrade SDK 交易回调目标；可为空。
  void SetTarget(qtrade_sdk::trader::TraderSpi* target) {
    target_ = target;
  }

  /// @brief 获取当前交易回调目标对象。
  /// @return 当前目标对象；未设置时为 nullptr。
  qtrade_sdk::trader::TraderSpi* Target() const {
    return target_;
  }

  /// @brief 发布已连接事件。
  void PublishConnected();

  /// @brief 发布订单事件。
  /// @param order_info 订单信息。
  /// @param error_info 订单处理错误信息；无错误时可为空。
  /// @param session_id 会话标识。
  void PublishOrderEvent(const qtrade_sdk::trader::Order& order_info,
                         const qtrade_sdk::trader::RspInfo* error_info,
                         std::uint64_t session_id);

  /// @brief 发布成交事件。
  /// @param trade_info 成交信息。
  /// @param session_id 会话标识。
  void PublishTradeEvent(const qtrade_sdk::trader::Trade& trade_info, std::uint64_t session_id);

 private:
  /// @brief qtrade SDK 交易回调目标对象。
  qtrade_sdk::trader::TraderSpi* target_ = nullptr;
};

}  // namespace qtrade::adapter::mock::trader

#endif  // QTRADE_ADAPTER_MOCK_TRADER_SPI_HPP_
