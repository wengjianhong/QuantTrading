/// @file      account_risk_bridge.hpp
/// @brief     账户硬风控桥接接口与相关结构
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_ACCOUNT_RISK_BRIDGE_HPP_
#define QTRADE_BRIDGE_ACCOUNT_RISK_BRIDGE_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/structs/result.hpp>

#include <cstdint>
#include <string>

namespace qtrade::account_risk {

/// @brief 账户硬风控策略
struct AccountRiskPolicy {
  /// 租户标识
  std::string tenant_id;
  /// 交易账户号
  std::string account_id;
  /// 策略版本
  std::uint64_t version = 0;
  /// 策略失效时间（Unix 毫秒）；0 表示由服务端默认 TTL 决定
  std::int64_t valid_until_unix_ms = 0;
  /// 最大名义敞口；0 表示不限制
  double max_notional = 0.0;
  /// 最大保证金占用；0 表示不限制
  double max_margin = 0.0;
  /// 最大总敞口；0 表示不限制
  double max_gross_exposure = 0.0;
  /// 最大未完成订单数；0 表示不限制
  std::uint64_t max_open_orders = 0;
  /// 安全缓冲
  double safety_buffer = 0.0;
  /// 是否启用该账户硬风控
  bool enabled = false;
};

/// @brief 订单意图（预占时由引擎提交）
struct OrderIntent {
  /// 全局订单 ID
  std::string order_id;
  /// 引擎实例标识
  std::string engine_id;
  /// 策略实例标识；可为空
  std::string strategy_id;
  /// 合约/品种标识
  std::string instrument_id;
  /// 买卖方向字符串表示
  std::string side;
  /// 委托价格
  double price = 0.0;
  /// 委托数量
  std::uint64_t quantity = 0;
  /// 估算名义金额；0 时可由实现按 price × quantity 推导
  double estimated_notional = 0.0;
  /// 估算保证金占用
  double estimated_margin = 0.0;
};

/// @brief 预占裁决结果
enum class ReserveDecision {
  kUnspecified = 0,
  kApproved = 1,
  kRejected = 2,
  kUnknown = 3,
};

/// @brief 预占订单结果
struct ReserveOrderResult {
  /// 裁决结果
  ReserveDecision decision = ReserveDecision::kUnspecified;
  /// 拒绝原因码；批准时可为空
  std::string reject_reason;
  /// 实际生效的策略版本
  std::uint64_t policy_version = 0;
  /// 预占 ID
  std::string reservation_id;
  /// 预占过期时间（Unix 毫秒）
  std::int64_t expires_at_unix_ms = 0;
};

/// @brief 释放预占原因
enum class ReleaseReason {
  kUnspecified = 0,
  kEmsEnqueueFailed = 1,
  kRejectedByVenue = 2,
  kCanceled = 3,
  kSettled = 4,
  kExpired = 5,
};

/// @brief 释放订单预占结果
struct ReleaseOrderResult {
  /// 是否已释放（幂等：不存在也视为已释放）
  bool released = false;
  /// 失败原因；成功时可为空
  std::string reject_reason;
};

/// @brief 单笔预占状态
struct Reservation {
  /// 全局订单 ID
  std::string order_id;
  /// 预占 ID
  std::string reservation_id;
  /// 状态：reserved / released / settled / expired 等
  std::string status;
  /// 预占过期时间（Unix 毫秒）
  std::int64_t expires_at_unix_ms = 0;
};

/// @brief 账户硬风控桥接器
/// @details 注入引擎前须已可用；连接等生命周期由实现方 / 持有方管理，本接口不包含 Start/Stop。
class IAccountRiskBridge {
 public:
  virtual ~IAccountRiskBridge() = default;

  /// @brief 读取账户硬风控策略
  /// @param tenant_id 租户标识
  /// @param account_id 交易账户号
  /// @return Result<AccountRiskPolicy> 策略快照
  virtual Result<AccountRiskPolicy> GetAccountRiskPolicy(const std::string& tenant_id,
                                                         const std::string& account_id) const = 0;

  /// @brief 写入账户硬风控策略
  /// @param policy 策略快照
  /// @return ErrorCode::kSuccess 表示接受；其他表示拒绝
  virtual ErrorCode ApplyAccountRiskPolicy(const AccountRiskPolicy& policy) = 0;

  /// @brief 预占账户额度
  /// @param tenant_id 租户标识
  /// @param account_id 交易账户号
  /// @param intent 订单意图
  /// @param risk_config_version 引擎侧策略版本；0 表示不校验
  /// @param reservation_ttl_ms 预占有效期（毫秒）；0 表示使用默认 TTL
  /// @return Result<ReserveOrderResult> 预占裁决
  virtual Result<ReserveOrderResult> ReserveOrder(const std::string& tenant_id,
                                                  const std::string& account_id,
                                                  const OrderIntent& intent,
                                                  std::uint64_t risk_config_version = 0,
                                                  std::int64_t reservation_ttl_ms = 0) = 0;

  /// @brief 释放订单预占
  /// @param tenant_id 租户标识
  /// @param account_id 交易账户号
  /// @param order_id 全局订单 ID
  /// @param reason 释放原因
  /// @param settled_notional 已结算名义金额；SETTLED 时可填写
  /// @param settled_margin 已结算保证金；SETTLED 时可填写
  /// @return Result<ReleaseOrderResult> 释放结果
  virtual Result<ReleaseOrderResult> ReleaseOrder(const std::string& tenant_id,
                                                  const std::string& account_id,
                                                  const std::string& order_id,
                                                  ReleaseReason reason,
                                                  double settled_notional = 0.0,
                                                  double settled_margin = 0.0) = 0;

  /// @brief 查询指定订单预占状态
  /// @param tenant_id 租户标识
  /// @param account_id 交易账户号
  /// @param order_id 全局订单 ID
  /// @return Result<Reservation> 预占状态
  virtual Result<Reservation> GetReservation(const std::string& tenant_id,
                                             const std::string& account_id,
                                             const std::string& order_id) const = 0;
};

}  // namespace qtrade::account_risk

#endif  // QTRADE_BRIDGE_ACCOUNT_RISK_BRIDGE_HPP_
