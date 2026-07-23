/// @file      order_reservation.hpp
/// @brief     order_reservation 表 DAO 声明（§3.2 订单预占表（Reserve/Release/Settle））
/// @details   当前仅提供 Record 与 DDL；DML 待后续实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_ORDER_RESERVATION_HPP_
#define QTRADE_DAO_ORDER_RESERVATION_HPP_

#include <qtrade/dao/ddl.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief order_reservation 表行记录
/// @details 表主键说明：(tenant_id, account_id, order_id)
///
struct OrderReservationRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 交易账户 ID
  std::optional<std::string> account_id;
  /// 全局订单 ID
  std::optional<std::string> order_id;
  /// 预占 ID
  std::optional<std::string> reservation_id;
  /// 发起预占的引擎实例 ID
  std::optional<std::string> engine_id;
  /// 策略 ID；可为空
  std::optional<std::string> strategy_id;
  /// 合约/品种 ID
  std::optional<std::string> instrument_id;
  /// 买卖方向
  std::optional<std::string> side;
  /// 预占名义金额
  std::optional<double> reserved_notional;
  /// 预占保证金
  std::optional<double> reserved_margin;
  /// 预占数量
  std::optional<std::uint64_t> quantity;
  /// 预占状态：reserved / released / settled / expired
  std::optional<std::string> status;
  /// 预占过期时间（Unix 毫秒）
  std::optional<std::int64_t> expires_at_unix_ms;
  /// 创建时间（Unix 毫秒）
  std::optional<std::int64_t> created_at_unix_ms;
  /// 最近更新时间（Unix 毫秒）
  std::optional<std::int64_t> updated_at_unix_ms;
};

/// @brief order_reservation 表 DDL
/// @details 实现 ITableDdl；增删改查尚未实现
class OrderReservation final : public ITableDdl {
 public:
  OrderReservation() = default;
  OrderReservation(OrderReservation&&) = delete;
  OrderReservation(const OrderReservation&) = delete;
  OrderReservation& operator=(OrderReservation&&) = delete;
  OrderReservation& operator=(const OrderReservation&) = delete;
  ~OrderReservation() noexcept override = default;

  /// @brief 获取逻辑数据库名
  /// @return 固定为 "account_risk"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "order_reservation"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_ORDER_RESERVATION_HPP_
