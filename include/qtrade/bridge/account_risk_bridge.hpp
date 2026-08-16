/// @file      account_risk_bridge.hpp
/// @brief     账户硬风控桥接接口与相关结构
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_ACCOUNT_RISK_BRIDGE_HPP_
#define QTRADE_BRIDGE_ACCOUNT_RISK_BRIDGE_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_types.hpp>
#include <qtrade/structs/result.hpp>

#include <cstdint>
#include <string>

namespace qtrade::account_risk {

/// @brief 预占生命周期状态
enum class ReservationState {
  /// 未指定
  kUnspecified = 0,
  /// 已预占
  kReserved = 1,
  /// 已拒绝
  kRejected = 2,
  /// 已释放
  kReleased = 3,
  /// 已结算
  kSettled = 4,
  /// 已过期
  kExpired = 5,
};

/// @brief 释放预占原因
enum class ReleaseReason {
  /// 未指定
  kUnspecified = 0,
  /// 发送失败
  kSendFailed = 1,
  /// 被交易所拒绝
  kRejectedByVenue = 2,
  /// 已撤单
  kCanceled = 3,
  /// 已结算
  kSettled = 4,
  /// 已过期
  kExpired = 5,
};

/// @brief 账户硬风控策略
/// @details 以全局唯一 account_id 标识账户。
struct AccountRiskPolicy {
  /// 交易账户号（全局唯一）
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

/// @brief 订单预计产生的风险敞口
/// @details 仅包含账户硬风控裁决需要的订单事实，不包含预占生命周期字段。
struct OrderExposure {
  /// 引擎实例标识
  std::string engine_id;
  /// 策略实例标识；可为空
  std::string strategy_id;
  /// 合约/品种标识
  std::string instrument_id;
  /// 买卖方向
  qtrade::sdk::trader::SideType side = qtrade::sdk::trader::SideType::kUnknown;
  /// 委托价格
  double price = 0.0;
  /// 委托数量
  std::uint64_t quantity = 0;
  /// 预计名义金额；0 时可由服务按 price × quantity 推导
  double notional = 0.0;
  /// 预计保证金占用
  double margin = 0.0;
};

/// @brief 创建订单风险预占的请求
struct ReserveRequest {
  /// 交易账户号（全局唯一）
  std::string account_id;
  /// 全局订单 ID（同一账户内唯一）
  std::string order_id;
  /// 订单风险敞口
  OrderExposure exposure;
  /// 期望服务端采用的策略版本；0 表示不作版本前置校验
  std::uint64_t expected_policy_version = 0;
  /// 预占有效期（毫秒）；0 表示使用服务端默认 TTL
  std::int64_t ttl_ms = 0;
};

/// @brief 释放或结算订单风险预占的请求
struct ReleaseRequest {
  /// 交易账户号（全局唯一）
  std::string account_id;
  /// 全局订单 ID（同一账户内唯一）
  std::string order_id;
  /// 释放原因
  ReleaseReason reason = ReleaseReason::kUnspecified;
  /// 实际结算名义金额
  double notional = 0.0;
  /// 实际结算保证金
  double margin = 0.0;
};

/// @brief 订单风险预占快照
/// @details Reserve、Release 与 QueryReservation 均返回该统一资源表示。
struct Reservation {
  /// 交易账户号（全局唯一）
  std::string account_id;
  /// 全局订单 ID（同一账户内唯一）
  std::string order_id;
  /// 预占 ID
  std::string reservation_id;
  /// 当前生命周期状态
  ReservationState state = ReservationState::kUnspecified;
  /// 拒绝或状态变更原因；无原因时为空
  std::string reason;
  /// 裁决实际采用的策略版本
  std::uint64_t policy_version = 0;
  /// 预占过期时间（Unix 毫秒）
  std::int64_t expires_at_unix_ms = 0;
};

/// @brief 账户硬风控桥接器
/// @details 注入引擎前须已可用；连接等生命周期由实现方 / 持有方管理，本接口不包含 Start/Stop。
class IAccountRiskBridge {
 public:
  /// @brief 销毁账户硬风控桥接接口实例
  virtual ~IAccountRiskBridge() = default;

  /// @brief 读取账户硬风控策略
  /// @param account_id 交易账户号（全局唯一）
  /// @return Result<AccountRiskPolicy> 策略快照
  virtual Result<AccountRiskPolicy> GetAccountRiskPolicy(const std::string& account_id) const = 0;

  /// @brief 创建订单风险预占
  /// @param request 预占标识、风险敞口及一致性约束
  /// @return Result<Reservation> 创建后的预占快照
  virtual Result<Reservation> Reserve(const ReserveRequest& request) = 0;

  /// @brief 释放或结算订单风险预占
  /// @param request 预占标识、释放原因及可选结算数据
  /// @return Result<Reservation> 操作后的预占快照
  virtual Result<Reservation> Release(const ReleaseRequest& request) = 0;

  /// @brief 查询指定订单预占状态
  /// @param account_id 交易账户号（全局唯一）
  /// @param order_id 全局订单 ID（同一账户内唯一）
  /// @return Result<Reservation> 预占状态
  virtual Result<Reservation> QueryReservation(const std::string& account_id, const std::string& order_id) const = 0;
};

}  // namespace qtrade::account_risk

#endif  // QTRADE_BRIDGE_ACCOUNT_RISK_BRIDGE_HPP_
