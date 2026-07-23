/// @file      order_journal.hpp
/// @brief     订单主日志
/// @details   以追加写 JSON Lines 保存订单状态快照，作为 OMS 崩溃恢复的事实源
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_OMS_ORDER_JOURNAL_HPP_
#define QTRADE_ENGINE_OMS_ORDER_JOURNAL_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::engine::oms {

/// @brief 引擎内订单生命周期状态
enum class OrderLifecycleState : std::uint8_t {
  /// 已完成本地准入并持久化
  kPrepared = 0,
  /// 已进入 EMS 队列
  kEmsQueued = 1,
  /// 正在调用交易通道发送
  kSendPending = 2,
  /// 通道已接受，等待后续回报
  kWorking = 3,
  /// 已部分成交
  kPartiallyFilled = 4,
  /// 已全部成交
  kFilled = 5,
  /// 撤单请求已提交
  kCancelPending = 6,
  /// 已撤单
  kCanceled = 7,
  /// 已拒绝或确定发送失败
  kRejected = 8,
  /// 发送结果未知，恢复时必须查询柜台
  kSendUnknown = 9,
};

/// @brief 订单主日志事件类型
enum class OrderJournalEventType : std::uint8_t {
  /// 订单已准备
  kPrepared = 0,
  /// 订单已进入 EMS
  kEmsQueued = 1,
  /// 订单开始发送
  kSendPending = 2,
  /// 发送调用已返回
  kSendResult = 3,
  /// 收到订单回报
  kOrderReport = 4,
  /// 收到成交回报
  kTradeReport = 5,
  /// 已请求撤单
  kCancelRequested = 6,
  /// 撤单调用已返回
  kCancelResult = 7,
};

/// @brief 单条订单主日志记录
struct OrderJournalRecord {
  /// 日志内单调递增序号
  std::uint64_t sequence = 0;
  /// Unix 时间戳，单位纳秒
  std::uint64_t timestamp_ns = 0;
  /// 引擎 epoch
  std::uint64_t engine_epoch = 0;
  /// 事件类型
  OrderJournalEventType event_type = OrderJournalEventType::kPrepared;
  /// 事件完成后的订单快照
  qtrade_sdk::trader::Order order;
  /// 引擎内生命周期状态
  OrderLifecycleState lifecycle_state = OrderLifecycleState::kPrepared;
  /// 成交事件携带的原始成交回报
  std::optional<qtrade_sdk::trader::Trade> trade;
  /// 错误或补充信息
  std::string message;
};

/// @brief 追加写订单主日志
class OrderJournal {
 public:
  /// @brief 构造未打开的日志
  OrderJournal() = default;

  /// @brief 析构并关闭文件
  ~OrderJournal();

  /// @brief 禁止拷贝构造
  OrderJournal(const OrderJournal&) = delete;

  /// @brief 禁止拷贝赋值
  OrderJournal& operator=(const OrderJournal&) = delete;

  /// @brief 打开日志文件
  /// @param path 日志文件路径
  /// @param sync_on_append 每次追加后是否调用 fsync
  /// @return 成功返回 kSuccess
  ErrorCode Open(const std::string& path, bool sync_on_append);

  /// @brief 关闭日志文件
  void Close();

  /// @brief 追加订单事件
  /// @param record 待写入记录；sequence 与 timestamp_ns 由日志覆盖
  /// @return 成功返回 kSuccess，写入或同步失败返回 kSystemError
  ErrorCode Append(OrderJournalRecord record);

  /// @brief 回放全部有效记录
  /// @return 按 sequence 排序的日志记录；无法读取时返回空列表
  [[nodiscard]] std::vector<OrderJournalRecord> Replay() const;

  /// @brief 日志是否已打开
  /// @return 文件描述符有效时返回 true
  [[nodiscard]] bool IsOpen() const;

 private:
  /// 日志文件路径
  std::string path_;
  /// 文件描述符
  int fd_ = -1;
  /// 是否每次追加后落盘
  bool sync_on_append_ = true;
  /// 下一个日志序号
  std::uint64_t next_sequence_ = 1;
  /// 保护文件描述符和序号
  mutable std::mutex mutex_;
};

}  // namespace qtrade::engine::oms

#endif  // QTRADE_ENGINE_OMS_ORDER_JOURNAL_HPP_
